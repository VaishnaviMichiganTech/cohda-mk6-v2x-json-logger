/**
 * @addtogroup mod_spat_tx SPAT Tx Module
 * @{
 *
 * @internal
 * 
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "spat-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "asn1defs.h"
#include "debug-levels.h"
#include "dot3-wsmp.h"
#include "j2735-common.h"
#include "j2735-spat.h"
#include "j2735asn.h"
#include "p1609-tx-msg.h"
#include "util.h"
#include "Example1609_debug.h"

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_SPATTx_DebugLevel

#define dbg_printf(f, a...)                                       \
    do {                                                          \
        fprintf(stdout, "%s(%d): " f, __func__, __LINE__, ## a);  \
    } while (0)                                                   \

/// Maximum size of the WSM
// TODO: is this sufficient?
#define SPATTX_WSM_BUF_SIZE      (1386)

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int SPATTx_LoadConfig (struct SPATTx *pSPAT,
                              char * pConfigFileName);

static void SPATTx_ThreadProc (void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

extern pthread_mutex_t P1609Tx_SendWSM_Mutex;

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate SPATTx object, Setup thread and associated data structures
 * @param ppSPAT pointer to SPATTx handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int SPATTx_Open (struct SPATTx **ppSPAT,
                pthread_attr_t *pAttr,
                char *pConfigFileName)
{
  int Res;
  struct SPATTx *pSPAT;

  // Create and check the local structure
  pSPAT = (struct SPATTx *) malloc(sizeof(struct SPATTx));
  if (pSPAT == NULL)
  {
    dbg_printf("malloc() = NULL\n");
    Res = -ENOMEM;
    goto Error;
  }
  memset(pSPAT, 0, sizeof(struct SPATTx));

  // Load config settings
  Res = SPATTx_LoadConfig(pSPAT, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("SPATTx_LoadConfig() != 0\n");
    goto Error;
  }
  // Init state
  ;

  // Assign thread attributes from input param
  pSPAT->ThreadAttr = *pAttr;

  // Create thread
  pSPAT->ThreadState |= SPATTX_THREAD_STATE_INIT;
  Res = pthread_create(&pSPAT->ThreadID,
                       &pSPAT->ThreadAttr,
                       (void *) SPATTx_ThreadProc,
                       pSPAT);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pSPAT->ThreadState = SPATTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppSPAT = pSPAT;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppSPAT = NULL;
  SPATTx_Close(pSPAT);
Success:
  dbg_printf("Successfully started the SPATTx\n");
  return Res;
}

/**
 * @brief Stop execution of SPATTx, free the thread and its associated resources
 * @param pSPAT SPATTx handle
 */
void SPATTx_Close (struct SPATTx *pSPAT)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pSPAT == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pSPAT->ThreadState |= SPATTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pSPAT->ThreadState & SPATTX_THREAD_STATE_INIT)
    pthread_join(pSPAT->ThreadID, NULL);

  // Free up allocated config parameters
  pSPAT->Params.Enabled = false;
  if (pSPAT->Params.pMovements != NULL)
  {
    int ix;
    for (ix = 0; ix < pSPAT->Params.NumMovements; ix++)
    {
      if (pSPAT->Params.pMovements[ix].pSpeedAdvisories != NULL)
      {
        free(pSPAT->Params.pMovements[ix].pSpeedAdvisories);
      }
    }
    free(pSPAT->Params.pMovements);
  }

  // Success!
  Res = 0;

Error:
  free(pSPAT);

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the SPATTx object
 * @param pSPAT SPATTx handle
 */
void SPATTx_PrintStats (struct SPATTx *pSPAT)
{
  fprintf(stdout, "SPATTx: Tx (Okay %8u Fail %8u)\n",
	  pSPAT->Stats.Tx.Okay, pSPAT->Stats.Tx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load 'SPATTx' parameters from the config file
 * @param pSPAT SPAT transmitter handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int SPATTx_LoadConfig (struct SPATTx *pSPAT,
                             char *pConfigFileName)
{
  int Res = -ENOSYS;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for SPAT in config file
  int ConfigVal = 0, NumberOfMovements = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pSPAT->Params.Enabled = false;
  pSPAT->Params.DynamicPhases = SPATTX_CONFIG_VALUE_DEFAULT_SPATDYNAMICPHASES;
  pSPAT->Params.ApplInterval = SPATTX_CONFIG_VALUE_DEFAULT_SPATAPPLINTERVAL;
  pSPAT->Params.IntersectionId = SPATTX_CONFIG_VALUE_DEFAULT_SPATINTERSECTIONID;
  pSPAT->Params.RedPhasePeriod = SPATTX_CONFIG_VALUE_DEFAULT_SPATREDTIME;
  pSPAT->Params.GreenPhasePeriod = SPATTX_CONFIG_VALUE_DEFAULT_SPATGREENTIME;
  pSPAT->Params.YellowPhasePeriod = SPATTX_CONFIG_VALUE_DEFAULT_SPATYELLOWTIME;
  pSPAT->Params.NumMovements = 0;
  pSPAT->Params.pMovements = NULL;

  // WSMP defaults
  {
    memset(pSPAT->Params.WSMP.Tx.DA, 0xFF, 6);
    pSPAT->Params.WSMP.Tx.Priority   = SPATTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY;
    pSPAT->Params.WSMP.Tx.ExpiryTime = SPATTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME;
    pSPAT->Params.WSMP.Version       = DOT3_WSMP_VERSION_3;
    pSPAT->Params.WSMP.ChannelNumber = SPATTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER;
    const tdefault_stack *pIni = default_stack_Get();
    pSPAT->Params.WSMP.ChannelNumber = pIni->ContinuousChanNum;
    default_stack_Release(&pIni);
    pSPAT->Params.WSMP.DataRate      = SPATTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE;
    pSPAT->Params.WSMP.TxPower       = SPATTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER;
    pSPAT->Params.WSMP.PSID          = htonl(SPATTX_CONFIG_VALUE_DEFAULT_WSMP_PSID);
    pSPAT->Params.WSMP.Length        = htons(0);
    pSPAT->Params.WSMP.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
  }

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'SPATTx' Setting
  pSetting = config_lookup(pConfig, SPATTX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", SPATTX_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // WSMP
  {
    config_setting_t *pWSMP;
    pWSMP = config_setting_get_member(pSetting,
                                      SPATTX_CONFIG_VALUE_NAME_WSMP);
    if (pWSMP != NULL)
    {
      // "pWSMP %p\n", pWSMP);

      // Priority
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_PRIORITY,
                                    &ConfigVal))
        pSPAT->Params.WSMP.Tx.Priority = (uint8_t) ConfigVal;
      // ExpiryTime
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_EXPIRY_TIME,
                                    &ConfigVal))
        pSPAT->Params.WSMP.Tx.ExpiryTime = (uint8_t) ConfigVal;
      // ChannelNumber
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER,
                                    &ConfigVal))
        pSPAT->Params.WSMP.ChannelNumber = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_DATA_RATE,
                                    &ConfigVal))
        pSPAT->Params.WSMP.DataRate = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_TX_POWER,
                                    &ConfigVal))
        pSPAT->Params.WSMP.TxPower = (uint8_t) ConfigVal;
      // PSID
      if (config_setting_lookup_int(pWSMP,
                                    SPATTX_CONFIG_VALUE_NAME_PSID,
                                    &ConfigVal))
        pSPAT->Params.WSMP.PSID = htonl((uint32_t) ConfigVal);
    }
  }

  // SPAT Enabled
  if (config_setting_lookup_bool(pSetting,
                                 SPATTX_CONFIG_VALUE_NAME_SPATENABLED,
                                 &ConfigVal))
    pSPAT->Params.Enabled = (bool) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.Enabled = %d\n", pSPAT->Params.Enabled);


  // Dynamic/Fixed SPAT contents
  if (config_setting_lookup_bool(pSetting,
                                 SPATTX_CONFIG_VALUE_NAME_SPATDYNAMICPHASES,
                                 &ConfigVal))
    pSPAT->Params.DynamicPhases = (bool) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.DynamicPhases = %d\n", pSPAT->Params.DynamicPhases);

  // Duration
  if (config_setting_lookup_int(pSetting,
                                SPATTX_CONFIG_VALUE_NAME_SPATAPPLINTERVAL,
                                &ConfigVal))
    pSPAT->Params.ApplInterval = (uint16_t) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.ApplInterval = %d\n", pSPAT->Params.ApplInterval);

  // Intersection ID
  if (config_setting_lookup_int(pSetting,
                                SPATTX_CONFIG_VALUE_NAME_SPATINTERSECTIONID,
                                &ConfigVal))
    pSPAT->Params.IntersectionId = (uint32_t) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.IntersectionId = 0x%x\n", pSPAT->Params.IntersectionId);

  // Red phase reload
  if (config_setting_lookup_int(pSetting,
                                SPATTX_CONFIG_VALUE_NAME_SPATREDTIME,
                                &ConfigVal))
    pSPAT->Params.RedPhasePeriod = (uint16_t) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.RedPhasePeriod = %d\n", pSPAT->Params.RedPhasePeriod);

  // Green phase reload
  if (config_setting_lookup_int(pSetting,
                                SPATTX_CONFIG_VALUE_NAME_SPATGREENTIME,
                                &ConfigVal))
    pSPAT->Params.GreenPhasePeriod = (uint16_t) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.GreenPhasePeriod = %d\n", pSPAT->Params.GreenPhasePeriod);

  // Yellow phase reload
  if (config_setting_lookup_int(pSetting,
                                SPATTX_CONFIG_VALUE_NAME_SPATYELLOWTIME,
                                &ConfigVal))
    pSPAT->Params.YellowPhasePeriod = (uint16_t) ConfigVal;
  d_printf(D_DEBUG, 0, "SPAT.YellowPhasePeriod = %d\n", pSPAT->Params.YellowPhasePeriod);

  // Construct movement states
  config_setting_t *pMovements;
  pMovements = config_setting_get_member(pSetting, SPATTX_CONFIG_VALUE_NAME_SPATMOVEMENETS);
  if (!pMovements)
  {
    d_fnend(D_INTERN, NULL, "(%p): no movements\n", pSetting);
    Res = -EINVAL;
    goto Error;
  }
  NumberOfMovements = pSPAT->Params.NumMovements = config_setting_length(pMovements);

  d_printf(D_DEBUG, 0, "SPAT.NumMovements = %d\n", pSPAT->Params.NumMovements);

  if (NumberOfMovements <= 0)
  {
    pSPAT->Params.NumMovements = 0;
    d_fnend(D_INTERN, NULL, "(%p): 0 movements\n", pSetting);
    Res = -EINVAL;
    goto Error;
  }
  pSPAT->Params.pMovements = (tSPATMovementParams *)
    calloc(sizeof(tSPATMovementParams), NumberOfMovements);
  if (!pSPAT->Params.pMovements)
  {
    pSPAT->Params.NumMovements = 0;
    d_fnend(D_CRIT, NULL, "(%p): Unable to allocate %d Movements\n",
            pSetting, NumberOfMovements);
    Res = -EINVAL;
    goto Error;
  }
  int i;
  for (i = 0; i < NumberOfMovements; ++i)
  {
    tSPATMovementParams *pMovementParams = &(pSPAT->Params.pMovements[i]);
    config_setting_t *pMovement   = config_setting_get_elem(pMovements, i);
    // initialise some defaults
    pMovementParams->SignalGroup  = SPATTX_CONFIG_VALUE_DEFAULT_SPATSIGNALGROUP;
    pMovementParams->TimeToChange = SPATTX_CONFIG_VALUE_DEFAULT_SPATTIMETOCHANGE;
    pMovementParams->CurrentState = SPATTX_CONFIG_VALUE_DEFAULT_SPATCURRENTSTATE;

    if (!pMovement)
    {
      // we couldn't get libconfig to provide the movement info it told us was there.
      d_printf(D_WARN, NULL, "Unable to get movement %d\n", i);
      continue;
    }

    // override the defaults (if they're in the config file)
    if (config_setting_lookup_int(pMovement,
                                  SPATTX_CONFIG_VALUE_NAME_SPATSIGNALGROUP,
                                  &ConfigVal))
      pMovementParams->SignalGroup =  (uint8_t) ConfigVal;
    d_printf(D_DEBUG, 0, "SPAT.pMovements[%d].SignalGroup = %d\n",
             i, pMovementParams->SignalGroup);

    if (config_setting_lookup_int(pMovement,
                                  SPATTX_CONFIG_VALUE_NAME_SPATTIMETOCHANGE,
                                  &ConfigVal))
      pMovementParams->TimeToChange =  (uint16_t) ConfigVal;
    d_printf(D_DEBUG, 0, "SPAT.pMovements[%d].TimeToChange = %d\n",
             i, pMovementParams->TimeToChange);

    if (config_setting_lookup_int(pMovement,
                                  SPATTX_CONFIG_VALUE_NAME_SPATCURRENTSTATE,
                                  &ConfigVal))
      pMovementParams->CurrentState =  (uint8_t) ConfigVal;
    d_printf(D_DEBUG, 0, "SPAT.pMovements[%d].CurrentState = %d\n",
             i, pMovementParams->CurrentState);

    config_setting_t *pSpeedAdvisories;
    pSpeedAdvisories = config_setting_get_member(pMovement,
                                                 SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICE);
    if (!pSpeedAdvisories)
    {
      // no speed advisories
      continue;
    }
    int NumSpeedAdvisories = pMovementParams->NumSpeedAdvisories =
      config_setting_length(pSpeedAdvisories);

    d_printf(D_DEBUG, 0,
             "SPAT.pMovements[%d].NumSpeedAdvice = %d\n",
             i, pMovementParams->NumSpeedAdvisories);

    if (NumSpeedAdvisories <= 0)
    {
      pMovementParams->NumSpeedAdvisories = 0;
      continue;
    }

    pMovementParams->pSpeedAdvisories =
      (tSPATSpeedAdvisory *)calloc(sizeof(tSPATSpeedAdvisory), NumSpeedAdvisories);

    if (!pMovementParams->pSpeedAdvisories)
    {
      d_printf(D_CRIT, NULL,
               "(%p): Unable to allocate %d SpeedAdvisories for movement %d\n",
               pSetting, NumSpeedAdvisories, i);
      pMovementParams->NumSpeedAdvisories = 0;
      continue;
    }
    int j;
    for (j = 0; j < NumSpeedAdvisories; ++j)
    {
      tSPATSpeedAdvisory *pSpeedAdvisory = &(pMovementParams->pSpeedAdvisories[j]);
      // set some defaults ...
      pSpeedAdvisory->Speed_tenthmps = 0;
      pSpeedAdvisory->Distance_m     = 0;
      pSpeedAdvisory->Type           = 0;

      // ... and try to override them
      config_setting_t *pSpeedAdvice = config_setting_get_elem(pSpeedAdvisories, j);
      if (!pSpeedAdvice)
      {
        // config data missing, leave the defaults.
        d_printf(D_WARN, NULL, "Unable to get speedadvice %d for movement %d\n", j, i);
        continue;
      }

      if (config_setting_lookup_int(pSpeedAdvice,
                                    SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICESPEED,
                                    &ConfigVal))
        pSpeedAdvisory->Speed_tenthmps =  (uint16_t)ConfigVal;

      d_printf(D_DEBUG, 0,
               "SPAT.pMovements[%d].SpeedAdvisories[%d].Speed_tenthmps = %d\n",
               i, j, pSpeedAdvisory->Speed_tenthmps);

      if (config_setting_lookup_int(pSpeedAdvice,
                                    SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICEDISTANCE,
                                    &ConfigVal))
        pSpeedAdvisory->Distance_m =  (uint16_t)ConfigVal;

      d_printf(D_DEBUG, 0,
               "SPAT.pMovements[%d].SpeedAdvisories[%d].Distance_m = %d\n",
               i, j, pSpeedAdvisory->Distance_m);

      if (config_setting_lookup_int(pSpeedAdvice,
                                    SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICETYPE,
                                    &ConfigVal))
        pSpeedAdvisory->Type =  (uint8_t)ConfigVal;

      d_printf(D_DEBUG, 0,
               "SPAT.pMovements[%d].SpeedAdvisories[%d].Type = %d\n",
               i, j, pSpeedAdvisory->Type);
    } // end "for (j = 0; j < NumSpeedAdvisories; ++j)"
  } // end "for (i = 0; i < NumberOfMovements; ++i)"

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  return Res;
}

/**
 * @brief Create SPAT message
 * @param pSPAT SPAT Tx module
 * @param[out] ppMsg Created SPAT Message
 * @param pCfg SPAT configuration
 * @return 0 for success, negative for errors
 */
static int SPATTx_CreateSPAT (struct SPATTx *pSPAT,
                              struct SAESPAT **ppMsg,
                              struct SPATCfg *pCfg)
{
  int Res;

  if ((pSPAT == NULL) || (ppMsg == NULL) || (pCfg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735SPAT_Create(ppMsg, pCfg, J2735_CREATE_ALL);

Error:
  return Res;
}

/**
 * @brief Encode SPAT message
 * @param pSPAT
 * @param pMsg Message to be encoded
 * @param[out] pBuf Buffer that contains encoded message
 * @param BufSize Buffer size
 * @return Size of encoded message, negative for errors
 */
static int SPATTx_EncodeSPAT (struct SPATTx *pSPAT,
                              struct SAESPAT *pMsg,
                              uint8_t *pBuf,
                              int BufSize)
{
  int Res;
  int RVal;

  (void)pSPAT;

  // Encode the SPAT to a buffer
  RVal = J2735SPAT_Encode(pMsg, pBuf, BufSize);
  if (RVal == -1)
  {
    // "Failed to encode SPAT at %s.",
    // RVal.failed_type ? RVal.failed_type->name : "unknown");
    Res = -EAGAIN;
  }
  else
  {
    // Encoding finished successfully
    // "Encoded SPAT has %d bytes.", RVal);
    Res = RVal;
  }

  return Res;
}

/**
 * @brief Populate SPAT message
 * @param pSPAT SPAT Tx module
 * @param[out] ppMsg SPAT Message to be populated
 * @param pCfg SPAT configuration
 * @return 0 for success, negative for errors
 */
static int SPATTx_PopulateSPAT (struct SPATTx *pSPAT,
                                struct SAESPAT *pSpat,
                                struct SPATCfg *pCfg)
{
  int Res;

  (void)pCfg;

  if (pSPAT && pSpat)
  {
    int i;
    SAEIntersectionState *pIntersection;
    SAEMovementState *pState;

    // Yes, count is hardcoded as we assume SPAT contains only one intersection
    pSpat->intersections.count = 1;
    pSpat->intersections.tab = (SAEIntersectionState *)
      asn1_mallocz(pSpat->intersections.count * sizeof(SAEIntersectionState));
    d_assert(pSpat->intersections.tab);

    d_printf(D_INTERN, NULL, "Intersections %zd\n", pSpat->intersections.count);

    pIntersection = &pSpat->intersections.tab[0];

    pIntersection->id.id = pSPAT->Params.IntersectionId;
    pIntersection->revision = 1;  // Not currently used in spat framework
    J2735_SetBitString(&pIntersection->status, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    pIntersection->states.count = pSPAT->Params.NumMovements;
    pIntersection->states.tab = (SAEMovementState *)
      asn1_mallocz(pIntersection->states.count * sizeof(SAEMovementState));
    d_assert(pIntersection->states.tab);

    // Calculate current time in TimeMark format. TimeMark is defined as offset
    // to the UTC full hour with a resolution of 36000 in units of 1/10th second
    int NowTimeMark = (Util_Now() / ((int64_t)100)) % 36000;

    for(i = 0; i < pSPAT->Params.NumMovements; i++)
    {
      pState = &pIntersection->states.tab[i];
      const tSPATMovementParams * pSPATMovementParams = &pSPAT->Params.pMovements[i];
      pState->signalGroup = pSPATMovementParams->SignalGroup;
      // For simplicity, each movement state has only one state_time_speed object
      // (update SAETx_UpdateSpatTiming is this is ever changed!)
      pState->state_time_speed.count = 1;
      pState->state_time_speed.tab = (SAEMovementEvent *)
        asn1_mallocz(pState->state_time_speed.count * sizeof(SAEMovementEvent));
      d_assert(pState->state_time_speed.tab);
      pState->state_time_speed.tab->eventState = pSPATMovementParams->CurrentState;
      if (pState->state_time_speed.tab->eventState != SAEMovementPhaseState_unavailable)
      {
        pState->state_time_speed.tab->timing_option = true;
        pState->state_time_speed.tab->timing.minEndTime =
          (pSPATMovementParams->TimeToChange + NowTimeMark)%36000;
        pState->state_time_speed.tab->speeds_option =
          (pSPATMovementParams->NumSpeedAdvisories > 0);

        if (pState->state_time_speed.tab->speeds_option)
        {
          pState->state_time_speed.tab->speeds.count = pSPATMovementParams->NumSpeedAdvisories;
          pState->state_time_speed.tab->speeds.tab = (SAEAdvisorySpeed *)
            asn1_mallocz(pState->state_time_speed.tab->speeds.count * sizeof(SAEAdvisorySpeed));
          d_assert(pState->state_time_speed.tab->speeds.tab);
          int j = 0;
          for (; j < pSPATMovementParams->NumSpeedAdvisories; j++)
          {
            SAEAdvisorySpeed *pAdvisorySpeed = &pState->state_time_speed.tab->speeds.tab[j];
            const tSPATSpeedAdvisory * pSPATSpeedAdvisory = &pSPATMovementParams->pSpeedAdvisories[j];
            pAdvisorySpeed->type            = pSPATSpeedAdvisory->Type;
            pAdvisorySpeed->speed_option    = true;
            pAdvisorySpeed->speed           = pSPATSpeedAdvisory->Speed_tenthmps;
            pAdvisorySpeed->distance_option = true;
            pAdvisorySpeed->distance        = pSPATSpeedAdvisory->Distance_m;
          }
        }
      }
    }
    Res = 0;
  }
  else
  {
    Res = -EINVAL;
    goto Error;
  }

Error:
  return Res;
}


/**
 * @brief Update state time to change to pre-defined SPAT message
 * @param pSPATTx pointer to user parameters
 * @param pSpat pointer to ASN.1 SPAT message that we need to loop and update change time
 */
static void SPATTx_UpdateSPATTiming (struct SPATTx *pSPAT,
                                     struct SAESPAT *pSpat)
{
  d_fnstart(D_INTERN, NULL, "(%p %p)\n", pSPAT, pSpat);

  // For any incorrect parameter, return without updating SPAT message
  if ((!pSPAT) || (!pSpat))
  {
    d_fnend(D_WARN, NULL, "(%p %p): NULL\n", pSPAT, pSpat);
    return;
  }

  // prepate to iterate through all the intersections
  SAEIntersectionStateList * pSAEIntersectionStateList = &pSpat->intersections;

  // unless there aren't any :-)
  if(pSAEIntersectionStateList->count < 1)
  {
    d_fnend(D_INFO, NULL, "(%p %p): No intersections\n", pSPAT, pSpat);
    return;
  }

  // note that ITSTx_PrepareSpat currrently hard-codes exactly one intersection,
  // i.e. this SPAT contains data for one MAP
  size_t i;
  for(i = 0; i < pSAEIntersectionStateList->count; ++i)
  {
    SAEIntersectionState * pIntersection = &(pSAEIntersectionStateList->tab[i]);
    SAEMovementList      * pSAEMovementList = &pIntersection->states;

    // iterate through all the movements of each intersection (6 in the example)
    size_t j;
    for (j = 0; j < pSAEMovementList->count; ++j)
    {
      const tSPATMovementParams * pSPATMovementParams = &pSPAT->Params.pMovements[j];
      SAEMovementState *pSAEMovementState = &pSAEMovementList->tab[j];
      SAEMovementEventList * pSAEMovementEventList = &pSAEMovementState->state_time_speed;

      // iterate through each element of each movement
      size_t k;
      for (k = 0; k < pSAEMovementEventList->count; ++k)
      {
        SAEMovementEvent *pSAEMovementEvent = &(pSAEMovementEventList->tab[k]);
        SAETimeMark      *pEndTime          = &(pSAEMovementEvent->timing.minEndTime);
        SAEMovementPhaseState *pPhase       = &(pSAEMovementEvent->eventState);

        // Calculate current time in TimeMark format. TimeMark is defined as offset
        // to the UTC full hour with a resolution of 36000 in units of 1/10th second
        int NowTimeMark = (Util_Now() / ((int64_t)100)) % 36000;
        // how far into the future is the next change?
        int Diff = *pEndTime - NowTimeMark;

        if (0) // Verbose Debug output @ 10Hz
        {
          char Phase;
          switch (*pPhase)
          {
            case SAEMovementPhaseState_stop_And_Remain:
              Phase = 'R';
              break;
            case SAEMovementPhaseState_protected_clearance:
              Phase = 'Y';
              break;
            case SAEMovementPhaseState_protected_Movement_Allowed:
              Phase = 'G';
              break;
            default:
              Phase = '?';
              break;
          }
          d_printf(D_CRIT, NULL, "DBG [%zd][%zd][%zd] %c %.1fs\n",
                   i, j ,k, Phase, (Diff * 0.1));
        }

        // if we're not doing dynamic phase then simply slip the change time
        // (this gives the effect of an infinite phase)
        if(!pSPAT->Params.DynamicPhases)
        {
          (*pEndTime) = (pSPATMovementParams->TimeToChange + NowTimeMark) % 36000;
          d_printf(D_INFO, NULL, "Infinite phase time at %d, [%zd][%zd][%zd]: {%d,%d} (slip)\n",
                   NowTimeMark,
                   i,j,k, *pPhase, *pEndTime);
          continue;
        }

        // if TempDiff is negative then we're over-due for a phase change. However,
        // if we seem to have missed by more than half an hour assume we've had a wrap-around
        bool PhaseChangeDue = (Diff < 0) && (Diff > -18000);
        if (!PhaseChangeDue)
        {
          d_printf(D_INFO, NULL, "Adapt phase time at %d, [%zd][%zd][%zd]: still {%d,%d} (next change in %d)\n",
                   NowTimeMark,
                   i,j,k, *pPhase, *pEndTime, Diff);
          continue;
        }
        // if we're currently "red" then the next change is to "green"
        bool ChangeToGreen = (((*pPhase) == SAEMovementPhaseState_stop_And_Remain) ||
                              ((*pPhase) == SAEMovementPhaseState_stop_Then_Proceed) ||
                              ((*pPhase) == SAEMovementPhaseState_pre_Movement));
        // if we're not changing to green we might already be "green" and changing to "yellow"
        bool ChangeToYellow = !ChangeToGreen &&
          (((*pPhase) == SAEMovementPhaseState_protected_Movement_Allowed) ||
           ((*pPhase) == SAEMovementPhaseState_permissive_Movement_Allowed));

        // if we're changing to green ...
        if(ChangeToGreen)
        {
          // then advance the phase and update the end time
          (*pPhase)    = SAEMovementPhaseState_protected_Movement_Allowed;
          (*pEndTime) += pSPAT->Params.GreenPhasePeriod;
        }
        // SSDD, see above
        else if(ChangeToYellow)
        {
          (*pPhase)    = SAEMovementPhaseState_protected_clearance;
          (*pEndTime) += pSPAT->Params.YellowPhasePeriod;
        }
        // not changing to green or yellow, must be potentially changing to red
        else
        {
          // Change from (hopefully) YLW to RED state:
          (*pPhase)    = SAEMovementPhaseState_stop_And_Remain;
          (*pEndTime) += pSPAT->Params.RedPhasePeriod;
        }
        // wrap the end time
        (*pEndTime) %= 36000;
        d_printf(D_INFO, NULL, "Change phase at %d, [%zd][%zd][%zd]: %d: -> {%d,%d}\n",
                 NowTimeMark,
                 i,j,k,Diff, *pPhase, *pEndTime);

      }
    }
  }
  d_fnend(D_INTERN, NULL, "(%p %p)\n", pSPAT, pSpat);
}

/**
 * @brief Populate WSM header
 * @param pSPAT
 * @param[out] pHdr WSMP header to be popualted
 * @param Len Size of WSM paylaod
 */
void SPATTx_PopulateWSMHeader (struct SPATTx *pSPAT,
                               struct Dot3WSMPHdr *pHdr,
                               int Len)
{
  memcpy(pHdr, &(pSPAT->Params.WSMP), sizeof(struct Dot3WSMPHdr));
  pHdr->Length = htons(Len);

  // "WSM Header Tx to MAC %02x:%02x:%02x:%02x:%02x:%02x: "
  // "Priority %d ChanNbr %d DataRate %d "
  // "TxPwr %d PSID %08x Load %08x)\n",
  // pHdr->Tx.DA[0],
  // pHdr->Tx.DA[1],
  // pHdr->Tx.DA[2],
  // pHdr->Tx.DA[3],
  // pHdr->Tx.DA[4],
  // pHdr->Tx.DA[5],
  // pHdr->Tx.Priority,
  // pHdr->ChannelNumber,
  // pHdr->DataRate,
  // pHdr->TxPower,
  // ntohl(pHdr->PSID),
  // ntohl(pHdr->ChannelLoad));

}

/**
 * @brief Destroy a SPAT Message
 * @param pSPAT
 * @param pMsg SPAT message
 * @return 0 for success. Negative for errors.
 */
static int SPATTx_DestroySPAT (struct SPATTx *pSPAT,
                               struct SAESPAT *pMsg)
{
  int Res;

  if ((pSPAT == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735SPAT_Free(pMsg, J2735_FREE_ALL);

Error:
  return Res;
}

/**
 * @brief Transmit SPAT messages
 * @param pSPAT SPAT Tx module
 * @return 0 for success. Negative for errors.
 */
static int SPATTx_TransmitSPAT (struct SPATTx *pSPAT)
{
  int Res;
  uint8_t *pWSM = NULL;
  int WSMSize;

  if (pSPAT->pASN == NULL)
  {
    // Create the ASN.1 structure
    Res = SPATTx_CreateSPAT(pSPAT, &(pSPAT->pASN), &(pSPAT->Params.Cfg));
    if (Res != 0)
    {
      pSPAT->pASN = NULL;
      goto Error;
    }
    d_assert(pSPAT->pASN != NULL);

    // Fill in the ASN.1 structure
    Res = SPATTx_PopulateSPAT(pSPAT, pSPAT->pASN, &(pSPAT->Params.Cfg));
    if (Res != 0)
    {
      goto Error;
    }
  }

  // Update the phase timing in the ASN.1 structure
  SPATTx_UpdateSPATTiming(pSPAT, pSPAT->pASN);

  // Allocate the WSM buffer
  WSMSize = sizeof(struct Dot3WSMPHdr) + SPATTX_WSM_BUF_SIZE; // TODO: is the BUF_SIZE sufficient?
  pWSM = calloc(1, WSMSize);
  if (pWSM == NULL)
  {
    // "Failed to allocate Tx WSM buffer for encoding.");
    Res = -ENOMEM;
    goto Error;
  }

  // Encode the ASN.1 structure into the WSM buffer
  {
    uint8_t *pUPER = pWSM + sizeof(struct Dot3WSMPHdr);
    int UPERSize = WSMSize - sizeof(struct Dot3WSMPHdr);

    Res = SPATTx_EncodeSPAT(pSPAT, pSPAT->pASN, pUPER, UPERSize);
    if (Res < 0)
    {
      goto Error;
    }
    // A positive return value means that the encoding worked
    WSMSize = Res;
  }

  // Add the WSM Header to the UPER encoded buffer
  SPATTx_PopulateWSMHeader(pSPAT, (struct Dot3WSMPHdr *) pWSM, WSMSize);

  // Transmit the WSM.
  // Enable signing (might be fake sig if security is disabled)
  // Also disable Tx logging.
  pthread_mutex_lock(&P1609Tx_SendWSM_Mutex);
  Res = P1609Tx_SendWSM((struct Dot3WSMPHdr *) pWSM, true, false, -1);
  pthread_mutex_unlock(&P1609Tx_SendWSM_Mutex);
  if (Res != 0)
  {
    // "P1609Tx_SendWSM failed");
    goto Error;
  }
  else
  {
    // "Sent SPAT (%d bytes)", WSMSize);
  }

Error:
  if (pWSM)
  {
    free(pWSM);
  }
  return Res;
}

/**
 * @brief SPATTx Periodic thread processing - sends a SPAT
 * @param pArg Pointer to SPATTx object passed as generic input parameter
 */
static void SPATTx_ThreadProc (void *pArg)
{
  struct SPATTx *pSPAT;
  struct timespec Time;

  pSPAT = (struct SPATTx *) pArg;

  // "Starting SPATTx Periodic Thread");
  pSPAT->ThreadState |= SPATTX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while ((pSPAT->ThreadState & SPATTX_THREAD_STATE_STOP) == 0)
  {
    // polling delay
    Util_Nap(pSPAT->Params.ApplInterval, &Time);

    if (pSPAT->Params.Enabled)
    {
      int Res = SPATTx_TransmitSPAT(pSPAT); // Transmit a SPAT
      if (Res == 0)
        pSPAT->Stats.Tx.Okay++;
      else
        pSPAT->Stats.Tx.Fail++;
    }
  }

  if (pSPAT->pASN)
    (void) SPATTx_DestroySPAT(pSPAT, pSPAT->pASN);

  // "Leaving Periodic SPATTx Thread");

  // exit thread
  (void) pthread_exit(NULL);

}

// Close the doxygen group
/**
 * @}
 */

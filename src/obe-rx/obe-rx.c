/**
 * @addtogroup mod_obe_rx OBE Rx Module
 * @{
 *
 * @file obe-rx.c Module code for OBE receiver
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2014 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "obe-rx.h"

// v2x-lib includes
#include "debug-levels.h"
#include "ext.h"
#include "id-global.h"
#include "j2735-message.h"
#include "j2735asn.h"
#include "lph.h"
#include "p1609-ctrl-mib.h"
#include "p1609-rx.h"
#include "p1609-tx.h"
#include "qs-lib.h"
#include "spat-rx.h"
#include "util.h"
#include "TA_Err.h"
#include "TC.h"
#include "TM.h"
#include "TM_Err.h"
#include "Vector.h"
#include "Example1609_debug.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_OBERx_DebugLevel

#define dbg_printf(f, a...)                                   \
  do {                                                        \
     fprintf(stderr, "%s(%d): " f, __func__, __LINE__, ## a); \
  } while (0)

#define HMI_VRU_SETTING_PATHNAME            "Example.HMI.VRU"
#define HMI_FCW_SETTING_PATHNAME            "Example.HMI.FCW"
#define HMI_EEBL_SETTING_PATHNAME           "Example.HMI.EEBL"
#define HMI_ICW_SETTING_PATHNAME            "Example.HMI.ICW"
#define HMI_LCW_SETTING_PATHNAME            "Example.HMI.LCW"
#define HMI_LTA_SETTING_PATHNAME            "Example.HMI.LTA"
#define HMI_HLW_SETTING_PATHNAME            "Example.HMI.HLW"
#define HMI_EVW_SETTING_PATHNAME            "Example.HMI.EVW"
#define HMI_SVW_SETTING_PATHNAME            "Example.HMI.SVW"
#define HMI_GREEN_PHASE_SETTING_PATHNAME    "Example.HMI.GREEN_PHASE"
#define HMI_YELLOW_PHASE_SETTING_PATHNAME   "Example.HMI.YELLOW_PHASE"
#define HMI_RED_PHASE_SETTING_PATHNAME      "Example.HMI.RED_PHASE"
#define HMI_CSW_SETTING_PATHNAME            "Example.HMI.TIMCSW"
#define HMI_WZA_SETTING_PATHNAME            "Example.HMI.TIMWZA"
#define HMI_SPD_SETTING_PATHNAME            "Example.HMI.TIMSPD"
#define HMI_ICE_SETTING_PATHNAME            "Example.HMI.ICE"

#define QSQUEUE_OBE_RX   QSQUEUE_APP1
#define QSMSG_OBE_RX_WSM QS_BASE_MSG_APP1

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int OBERx_LoadConfig(struct OBERx *pOBE,
                            const char * pConfigFileName);

static void OBERx_ThreadProc(void *pArg);

static void TACallback(const tTAAlert *pAlert,
                       void *pParam);

static void OBERxExtCallback(tExtEventId Event,
                             tExtMessage *pMsg,
                             void *pPriv);
//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------

/**
 * @brief Get heartbeat info for HMI
 * @param pInfo Info to fill in
 * @param pPriv Private data
 * @return A tHMIErrCode to indicate status
 *
 */
static tHMIErrCode OBERx_GetHeartBeatInfo(tHMIInfoHeartBeat *pInfo,
                                          void *pPriv)
{
  tHMIErrCode Ret = HMI_ERR_NONE;
  void *pLPH;

  // stop compiler warning
  (void)pPriv;

  pInfo->Id = P1609TX_GetTempID();
  strncpy(pInfo->Version, "example obe-rx", HMI_VERSION_MAX_LEN);
  pInfo->Version[HMI_VERSION_MAX_LEN-1] = '\0';
  pInfo->TxPowerdBm = P1609Ctrl_MIBGetTxPowerLCH();
  pInfo->NumTx = P1609TX_GetNumTxWSM();
  pInfo->NumRx = P1609RX_GetNumRxWSM();
  pInfo->NumRemotes = TC_GetNumRemotes();
  pInfo->NumIntersections = SPATMAP_GetNumIntersections(5);
  pLPH = LPH_Open();
  if (pLPH != NULL)
  {
    int Res;
    tLPHPos Pos;

    Res = LPH_Position(pLPH, &Pos);
    if (Res == 0)
      pInfo->Fix = Pos.Mode;
    LPH_Close(pLPH);
  }
  pInfo->Sync = system("timeout -s 9 3 chronyc waitsync 1 0 0 1>/dev/null 2>/dev/null") == 0 ? 1 : 0;

  return Ret;
}


/**
 * @brief Reads a single HMI alert configuration block
 * @param pConfig Configuration structure
 * @param pHMI HMI structure
 * @param pPath The HMI configuration path
 * @param Type The alert type
 */
static void OBERx_ReadHMIAlert(const config_t *pConfig,
                                 tHMI *pHMI,
                                 const char *pPath,
                                 tHMIAlertType Type)
{
  config_setting_t * pSetting;
  const char *pIcon = NULL;
  const char *pLeftIcon = NULL;
  const char *pRightIcon = NULL;
  const char *pAudio = NULL;
  const char *pLeftAudio = NULL;
  const char *pRightAudio = NULL;
  const char *pText = NULL;
  int SeverityLow = 0;
  int SeverityHigh = 255;
  config_setting_t * pSeverityRange;


  config_setting_t * pConfigurations = config_lookup(pConfig, pPath);
  if (pConfigurations != NULL)
  {
    int NumberOfConfigs = config_setting_length(pConfigurations);
    int i = 0;
    for (; i < NumberOfConfigs; i++)
    {
      pSetting = config_setting_get_elem(pConfigurations, i);
      if (pSetting == NULL)
      {
        dbg_printf("Error getting the next hmi configuration block\n");
        goto Exit;
      }
      pSeverityRange = config_setting_get_member(pSetting, "SeverityRange");
      // severity range is a 2 element list
      // element[0] severity low
      // element[1] severity high
      if ((pSeverityRange == NULL) ||
          (config_setting_length(pSeverityRange) != 2))
      {
        dbg_printf("HMI SeverityRange error for %s", pPath);
        goto Exit;
      }
      SeverityLow  = config_setting_get_int_elem(pSeverityRange, 0);
      SeverityHigh = config_setting_get_int_elem(pSeverityRange, 1);

      // must specify either Icon and possibly LeftIcon and RightIcon
      config_setting_lookup_string(pSetting, "Icon", &pIcon);
      config_setting_lookup_string(pSetting, "LeftIcon", &pLeftIcon);
      config_setting_lookup_string(pSetting, "RightIcon", &pRightIcon);
      // must specify either Audio and possibly LeftAudio and RightAudio
      config_setting_lookup_string(pSetting, "Audio", &pAudio);
      config_setting_lookup_string(pSetting, "LeftAudio", &pLeftAudio);
      config_setting_lookup_string(pSetting, "RightAudio", &pRightAudio);
      config_setting_lookup_string(pSetting, "Text", &pText);
      if ((pIcon || (pLeftIcon && pRightIcon)) &&
          (pAudio || (pLeftAudio && pRightAudio)) &&
          pText)
      {
        dbg_printf("Custom %s alert:\n"
                   "\t SeverityLow: %d\n"
                   "\t SeverityHigh: %d\n"
                   "\t LeftIcon: %s\n"
                   "\tRightIcon: %s\n"
                   "\t     Icon: %s\n"
                   "\t LeftAudio: %s\n"
                   "\tRightAudio: %s\n"
                   "\t     Audio: %s\n"
                   "\t     Text: %s\n",
                   (Type == HMI_ALERT_TYPE_VRU ? "VRU" :
                    (Type == HMI_ALERT_TYPE_FCW ? "FCW" :
                     (Type == HMI_ALERT_TYPE_EEBL ? "EEBL" :
                      (Type == HMI_ALERT_TYPE_ICW ? "ICW" :
                       (Type == HMI_ALERT_TYPE_SVA ? "SVA" :
                        (Type == HMI_ALERT_TYPE_BSW_LCW ? "LCW" :
                         (Type == HMI_ALERT_TYPE_DNPW ? "DNPW" :
                          (Type == HMI_ALERT_TYPE_LTA ? "LTA" :
                           (Type == HMI_ALERT_TYPE_ICE ? "HLW" :
                            (Type == HMI_ALERT_TYPE_EVW ? "EVW" :
                             (Type == HMI_ALERT_TYPE_RLW ? "SVW" :
                              (Type == HMI_ALERT_TYPE_RED_PHASE ? "RED_PHASE" :
                               (Type == HMI_ALERT_TYPE_YELLOW_PHASE ? "YELLOW_PHASE" :
                                (Type == HMI_ALERT_TYPE_GREEN_PHASE ? "GREEN_PHASE" :
                                 (Type == HMI_ALERT_TYPE_ROADWORK ? "RWW" :
                                  "UNKNOWN"))))))))))))))),
                   SeverityLow,
                   SeverityHigh,
                   pLeftIcon ? pLeftIcon : "(unspecified)",
                   pRightIcon ? pRightIcon : "(unspecified)",
                   pIcon ? pIcon : "(unspecified)",
                   pLeftAudio ? pLeftAudio : "(unspecified)",
                   pRightAudio ? pRightAudio : "(unspecified)",
                   pAudio ? pAudio : "(unspecified)",
                   pText);
        if (pIcon && pAudio && pText)
        {
          HMI_AddAlertConfig(pHMI,
                             Type,
                             HMI_ALERT_DIRECTION_UNKNOWN,
                             SeverityLow,
                             SeverityHigh,
                             pIcon,
                             pAudio,
                             pText);
          if (pLeftIcon || pLeftAudio)
          {
            HMI_AddAlertConfig(pHMI,
                               Type,
                               HMI_ALERT_DIRECTION_LEFT,
                               SeverityLow,
                               SeverityHigh,
                               pLeftIcon ? pLeftIcon : pIcon,
                               pLeftAudio ? pLeftAudio : pAudio,
                               pText);
          }
          if (pRightIcon || pRightAudio)
          {
            HMI_AddAlertConfig(pHMI,
                               Type,
                               HMI_ALERT_DIRECTION_RIGHT,
                               SeverityLow,
                               SeverityHigh,
                               pRightIcon ? pRightIcon : pIcon,
                               pRightAudio ? pRightAudio : pAudio,
                               pText);
          }
        }
      }
    }
    goto Exit;
  }
  dbg_printf("Missing custom alert HMI configuration for %s alert\n",
             (Type == HMI_ALERT_TYPE_VRU ? "VRU" :
              (Type == HMI_ALERT_TYPE_FCW ? "FCW" :
               (Type == HMI_ALERT_TYPE_EEBL ? "EEBL" :
                (Type == HMI_ALERT_TYPE_ICW ? "ICW" :
                 (Type == HMI_ALERT_TYPE_SVA ? "SVA" :
                  (Type == HMI_ALERT_TYPE_BSW_LCW ? "LCW" :
                   (Type == HMI_ALERT_TYPE_DNPW ? "DNPW" :
                    (Type == HMI_ALERT_TYPE_LTA ? "LTA" :
                     (Type == HMI_ALERT_TYPE_ICE ? "HLW" :
                      (Type == HMI_ALERT_TYPE_EVW ? "EVW" :
                       (Type == HMI_ALERT_TYPE_RLW ? "SVW" :
                        (Type == HMI_ALERT_TYPE_RED_PHASE ? "RED_PHASE" :
                         (Type == HMI_ALERT_TYPE_YELLOW_PHASE ? "YELLOW_PHASE" :
                          (Type == HMI_ALERT_TYPE_GREEN_PHASE ? "GREEN_PHASE" :
                           (Type == HMI_ALERT_TYPE_ROADWORK ? "RWW" :
                            "UNKNOWN"))))))))))))))));
  Exit:
  return;
}


/**
 * @brief Reads the HMI configuration from file
 * @param pConfig The configuration structure
 * @param pHMI The HMI structure
 */
static void OBERx_ReadHMIAlerts(const config_t * pConfig,
                                tHMI * pHMI)
{
  if (pConfig == NULL || pHMI == NULL)
  {
    dbg_printf("pConfig or pHMI is NULL - can't proceed (%p, %p)\n",
               pConfig, pHMI);
    goto Error;
  }

  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_FCW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_FCW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_VRU_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_VRU);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_EEBL_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_EEBL);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_ICW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_ICW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_LCW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_SVA);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_LCW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_BSW_LCW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_LCW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_DNPW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_LTA_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_LTA);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_HLW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_ICE);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_EVW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_EVW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_SVW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_RLW);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_GREEN_PHASE_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_GREEN_PHASE);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_YELLOW_PHASE_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_YELLOW_PHASE);
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_RED_PHASE_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_RED_PHASE);
  // libTIMWorkZone.so
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_WZA_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_ROADWORK);
  // libTIMSpeed.so
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_SPD_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_SPEEDLIMIT);
  // libTIMCSW.so
  OBERx_ReadHMIAlert(pConfig,
                     pHMI,
                     HMI_CSW_SETTING_PATHNAME,
                     HMI_ALERT_TYPE_CSW);
  // libTIMGeneric.so
  OBERx_ReadCustomHMIAlert(pConfig,
                           pHMI,
                           HMI_ICE_SETTING_PATHNAME);

Error:
  return;
}

/**
 * @brief Allocate OBERx object, Setup thread and associated data structures
 * @param ppOBE pointer to OBE handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int OBERx_Open(struct OBERx **ppOBE,
               pthread_attr_t *pAttr,
               char *pConfigFileName)
{
  int Res;
  struct OBERx * pOBE = NULL;
  tdefault_stack_Cohda_TA * pTAParams = NULL;

  // Create and check the local structure
  pOBE = (struct OBERx *) malloc(sizeof(struct OBERx));
  if (pOBE == NULL)
  {
    Res = -ENOMEM;
    goto Error;
  }
  memset(pOBE, 0, sizeof(struct OBERx));

  // Load config settings
  Res = OBERx_LoadConfig(pOBE, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("OBERx_LoadConfig() != 0\n");
    goto Error;
  }

  int Ret;

  // HMI
  Ret = HMI_Open(&pOBE->pHMI, OBERx_GetHeartBeatInfo, pOBE);
  if (Ret != HMI_ERR_NONE)
  {
    dbg_printf("HMI Open Error [%d]", Ret);
    goto Error;
  }

  config_t Config; // configuration object read from config file
  config_init(&Config); // initialise the config object

  // Try to read the specified config file into the config object
  if (config_read_file(&Config, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the HMI Settings
  OBERx_ReadHMIAlerts(&Config, pOBE->pHMI);

  // Update complete - close the configuration
  config_destroy(&Config);

  // Threat Management
  Ret = TM_Init(NULL, TACallback, pOBE->pHMI);
  if (Ret != TM_ERR_NONE)
  {
    dbg_printf("TM Init Error [%d] %s",
            Ret,
            TM_ErrToStr(Ret));
    goto Error;
  }

  // Threat assessment
  Ret = TA_Init(pTAParams,
                TM_AlertCallback, NULL,
                TM_CurrentAlert,  NULL);
  if (Ret != TA_ERR_NONE)
  {
    dbg_printf("TA Init Error [%d] %s",
            Ret,
            TA_ErrToStr(Ret));
    goto Error;
  }

  // Init state

  // Assign thread attributes from input param
  pOBE->ThreadAttr = *pAttr;

  // Create OBERx thread
  pOBE->ThreadState |= OBERX_THREAD_STATE_INIT;
  Res = pthread_create(&pOBE->ThreadID,
                       &pOBE->ThreadAttr,
                       (void *)OBERx_ThreadProc,
                       pOBE);
  if (Res != 0)
  {
    dbg_printf("pthread_create() failed\n");
    // Thread creation error
    pOBE->ThreadState = OBERX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppOBE = pOBE;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppOBE = NULL;
  OBERx_Close(pOBE);
Success:
  dbg_printf("Successfully started the obe-rx\n");
  return Res;
}

/**
 * @brief Stop execution of OBE, free the thread and its associated resources
 * @param pOBE OBE handle
 */
void OBERx_Close (struct OBERx *pOBE)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pOBE == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  TA_Exit();
  HMI_Close(pOBE->pHMI);

  // Signal thread to terminate
  pOBE->ThreadState |= OBERX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pOBE->ThreadState & OBERX_THREAD_STATE_INIT)
    pthread_join(pOBE->ThreadID, NULL);

  // Success!
  Res = 0;

Error:
  free(pOBE);

  (void)Res;    // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the TPSE object
 * @param pOBE OBE handle
 */
void OBERx_PrintStats(struct OBERx *pOBE)
{
  fprintf(stdout, "OBERx: Rx (Okay %8u Fail %8u)\n",
          pOBE->Stats.Rx.Okay, pOBE->Stats.Rx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load OBERX parameters from the config file
 * @param pOBE OBE receiver handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int OBERx_LoadConfig(struct OBERx *pOBE,
                            const char *pConfigFileName)
{
  int Res;
  config_t Config; // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting; // setting for OBERX in config file
  int ConfigVal = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pOBE->Params.RxInterval_ms = OBERX_CONFIG_VALUE_DEFAULT_RXINTERVAL;

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the OBERx Setting
  pSetting = config_lookup(pConfig, OBERX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", OBERX_CONFIG_PATH_NAME);
  }
  else if (config_setting_lookup_int(pSetting,
                                     OBERX_CONFIG_VALUE_NAME_RXINTERVAL,
                                     &ConfigVal))
  {
    // Rx thread Interval
    pOBE->Params.RxInterval_ms = (uint32_t)ConfigVal;
  }

  // config is open, now look for configuration entries


  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  return Res;
}

/**
 * @brief EXT module Callback function
 * @param Event EXT event ID
 * @param pMsg EXT data associated with Event ID
 * @param pPriv Private data (tOBERx)
 *
 */
static void OBERxExtCallback(tExtEventId Event,
                             tExtMessage *pMsg,
                             void *pPriv)
{
  (void)pPriv;

  if (Event != QSMSG_EXT_RX_WSM)
  {
    return;
  }

  const SAEBasicSafetyMessage * pBSM = J2735_MsgExtractBSM(pMsg);
  if (!pBSM)
  {
    return;
  }

  // Return as soon as possible and perform the heavy processing in worker threads.
  // Ext_ForwardMsgTo() is non-blocking unless the Priority is QSPRIORITY_MAX.
  // Note that Ext_ForwardMsgTo() calls Ext_Retain() internally to increment the reference count
  // of tExtMessage.
  Ext_ForwardMsgTo(QSMSG_OBE_RX_WSM, QSQUEUE_OBE_RX, QSPRIORITY_DEFAULT, pMsg);
}

/**
 * @brief TA callback handler
 * @param pAlert Pointer to Alert information
 * @param pParam Additional TA information (HMI handle)
 */
static void TACallback(const tTAAlert *pAlert,
                       void *pParam)
{
  struct HMI *pHMI = (struct HMI *)pParam;
  char AlertStr[40] = {0,};

  dbg_printf("(%p, %p)-> {%08"PRIx32"}\n",
             pAlert, pParam, (pAlert ? pAlert->Type : TA_THREATTYPE_NONE));

  if (pAlert == NULL)
  {
    dbg_printf("Alert pointer NULL!\n");
    goto Exit;
  }

  // A cancel is signigified by a level of 0
  if (pAlert->Level == 0)
  {
    (void)HMI_CancelAlerts(pHMI);
    goto Exit;
  }

  switch (pAlert->Type)
  {
    case TA_THREATTYPE_EEBL:
    {
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_EEBL,
                      HMI_ALERT_DIRECTION_UNKNOWN,
                      pAlert->Level,
                      NULL,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_VRU:
    {
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_VRU,
                      HMI_ALERT_DIRECTION_UNKNOWN,
                      pAlert->Level,
                      NULL,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_FCW:
    {
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_FCW,
                      HMI_ALERT_DIRECTION_UNKNOWN,
                      pAlert->Level,
                      NULL,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_ICW:
    {
      tHDU ThreatBearing;
      tSafetyZone SafetyZone;
      tHMIAlertDirection Direction;
      tHMIAlertSeverity Severity = pAlert->Level;
      tHMIAlertConfig *pConfig;
      tHMIErrCode Err;
      char Warning[1024];
      char *pText = NULL;

      SafetyZone = pAlert->ICW.SafetyZone;
      ThreatBearing = pAlert->ICW.ThreatBearing;

      // figure out which direction flag to use
      if (ThreatBearing > SafetyZone.SouthEast)
        // Rear (tracking only, no icon nor audio)
        Direction = HMI_ALERT_DIRECTION_UNKNOWN;
      else
        if (ThreatBearing > SafetyZone.NorthEast)
          // Right
          Direction = HMI_ALERT_DIRECTION_RIGHT;
        else
          if (ThreatBearing > SafetyZone.NorthWest)
            // Front (should not get here)
            Direction = HMI_ALERT_DIRECTION_UNKNOWN;
          else
            if (ThreatBearing > SafetyZone.SouthWest)
              // Left
              Direction = HMI_ALERT_DIRECTION_LEFT;
            else
              // Rear (tracking only, no icon nor audio)
              Direction = HMI_ALERT_DIRECTION_UNKNOWN;

      // add direction to warning text if has a %s or remove %s from warning text
      // if no direction flag is present
      Err = HMI_GetAlertConfig(pHMI, HMI_ALERT_TYPE_ICW, Direction, Severity, &pConfig);
      if (Err == HMI_ERR_NONE)
      {
        unsigned int i, NumPC = 0;

        // replace %s placeholder in pText if it is provided with direction
        for (i = 0; pConfig->pText != NULL && i < strlen(pConfig->pText); i++)
        {
          if (pConfig->pText[i] == '%')
            NumPC++;
        }

        // allow one format directives - must be %s
        if (NumPC == 1 && strstr(pConfig->pText, "%s") != NULL)
        {
          snprintf(Warning, sizeof(Warning), pConfig->pText,
                   (Direction == HMI_ALERT_DIRECTION_LEFT ? "Left" :
                    (Direction == HMI_ALERT_DIRECTION_RIGHT ? "Right" :
                     "")));

          pText = Warning;
        }

        HMI_FreeAlertConfig(pConfig);
      }
      else
      {
        dbg_printf("Error getting ICW alert config: %d\n", Err);
      }

      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_ICW,
                      Direction,
                      Severity,
                      pText,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_BSW_LCW:
    {
      tHMIAlertDirection Direction = HMI_ALERT_DIRECTION_UNKNOWN;
      if (pAlert->BSW_LCW.RVPosition == TC_CLASSIF_IS_LEFT)
      {
        Direction = HMI_ALERT_DIRECTION_LEFT;
      }
      else if (pAlert->BSW_LCW.RVPosition == TC_CLASSIF_IS_RIGHT)
      {
        Direction = HMI_ALERT_DIRECTION_RIGHT;
      }

      if (pAlert->BSW_LCW.Scenario == TA_BSW_LCW_SCENARIO_BSW)
      {
        (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_BSW_LCW,
                        Direction,
                        pAlert->Level,
                        "Blind Spot Warning",
                        pAlert->DistanceToEvent_m);

      }
      else
      {
        (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_BSW_LCW,
                        Direction,
                        pAlert->Level,
                        NULL,
                        pAlert->DistanceToEvent_m);
      }
    }
    break;

    case TA_THREATTYPE_LTA:
    {
      sprintf(AlertStr,"LTA in %dm", (int)pAlert->DistanceToEvent_m);
      d_printf(D_WARN, NULL,"TA_THREATTYPE_LTA: %x\n", pAlert->Type);
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_LTA,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, AlertStr,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_HLW_FOG:
    case TA_THREATTYPE_HLW_RAIN:
    case TA_THREATTYPE_HLW_TRC:
    {
      sprintf(AlertStr,"HLW in %dm", (int)pAlert->DistanceToEvent_m);
      d_printf(D_WARN, NULL,"TA_THREATTYPE_HLW: %x\n", pAlert->Type);
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_ICE,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, AlertStr,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_EVW:
    {
      // ↑ ↗ → ↘ ↓ ↙ ← ↖
      uint32_t Slice = TC_HeadingToSlice(pAlert->Azimuth);
      const char *pDirection = "!";
      if (Slice & 0xc0000003)
        pDirection = "↑";
      if (Slice & 0x0000003c)
        pDirection = "↗";
      if (Slice & 0x000003c0)
        pDirection = "→";
      if (Slice & 0x00003c00)
        pDirection = "↘";
      if (Slice & 0x0003c000)
        pDirection = "↓";
      if (Slice & 0x003c0000)
        pDirection = "↙";
      if (Slice & 0x03c00000)
        pDirection = "←";
      if (Slice & 0x3c000000)
        pDirection = "↖";

      sprintf(AlertStr,"EVW %s %dm", pDirection, (int)pAlert->DistanceToEvent_m);
      d_printf(D_WARN, NULL,"TA_THREATTYPE_EVW\n");
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_EVW,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, AlertStr,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_SVW:
    {
      d_printf(D_WARN, NULL,"TA_THREATTYPE_SVW\n");
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_RLW,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, NULL,
                      pAlert->DistanceToEvent_m);
    }
    break;

    case TA_THREATTYPE_TLI:
    {
      tHMIAlertType Type = -1;
      float TimeToChange = (float)(pAlert->TLI.TimeChange_ds) / 10.0;
      uint8_t LaneID = pAlert->TLI.LaneId;
      float distStopLine = pAlert->DistanceToEvent_m;

      char *pText = NULL;

      d_printf(D_WARN, NULL,"TA_THREATTYPE_TLI\n");
      switch (pAlert->TLI.CurrentPhase)
      {
        case TA_SIGCOLOR_GREEN:
        {
          Type = HMI_ALERT_TYPE_GREEN_PHASE;
        }
        break;
        case TA_SIGCOLOR_YELLOW:
        {
          Type = HMI_ALERT_TYPE_YELLOW_PHASE;
        }
        break;
        case TA_SIGCOLOR_RED:
        {
          Type = HMI_ALERT_TYPE_RED_PHASE;
        }
        break;
        default:
        {
          Type = -1;
        }
      }
      if (Type == (tHMIAlertType)-1)
      {
        break;
      }
      pText = xxprintf("Time(%.1f), Dist(%.1f), Lane(%d)", TimeToChange, distStopLine, LaneID);
      (void)HMI_Alert(pHMI, Type,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, pText,
                      pAlert->DistanceToEvent_m);
      free(pText);
    }
    break;

    case TA_THREATTYPE_SVA_AIR:
    case TA_THREATTYPE_SVA_HZR:
    case TA_THREATTYPE_SVA_BRK:
    {
      sprintf(AlertStr,"SVA in %dm", (int)pAlert->DistanceToEvent_m);
      d_printf(D_WARN, NULL,"TA_THREATTYPE_SVA: %x\n", pAlert->Type);
      (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_SVA,
                      HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, AlertStr,
                      pAlert->DistanceToEvent_m);
    }
    break;

    // libTIMCSW.so
    case TA_THREATTYPE_CSW:
    {
      OBERx_Threattype_CSW(pAlert, pHMI);
      break;
    }

    // libTIMWorkZone.so
    case TA_THREATTYPE_WZA: // AKA: TA_THREATTYPE_RWW
    {
      OBERx_Threattype_WZA(pAlert, pHMI);
      break;
    }

    // libTIMSpeed.so
    case TA_THREATTYPE_SPEEDLIMIT:
    {
      OBERx_Threattype_SpeedLimit(pAlert, pHMI);
      break;
    }

    // libTIMGeneric.so
    case TA_THREATTYPE_GENERICTIM:
    {
      OBERx_Threattype_GenericTIM(pAlert, pHMI);
      break;
    }

    default:
    {
      dbg_printf("Alert %08"PRIx32" unknown\n", pAlert->Type);
    }
    break;
  }

Exit:
  return;
}

/**
 * @brief OBE Periodic thread processing
 * @param pArg Pointer to @ref OBERx object passed as generic input parameter
 */
static void OBERx_ThreadProc(void *pArg)
{
  dbg_printf("Started thread\n");

  struct OBERx * pOBE = (struct OBERx *) pArg;

  const tQsStatus Status = Msg_QueueInit(QSQUEUE_OBE_RX);
  if (Status != QS_ERR_NONE)
  {
    dbg_printf("Unable to initialise obe-rx message queue\n");
    pthread_exit(NULL);
  }

  // Register the callback with the Ext Module
  const int ExtHandle = Ext_CallbackRegister(&OBERxExtCallback, pOBE);
  if (ExtHandle < 0)
  {
    dbg_printf("Unable to register callback with EXT module\n");
    (void)Msg_QueueDeInit(QSQUEUE_OBE_RX);
    pthread_exit(NULL);
  }
  dbg_printf("Registered Callback!\n");

  pOBE->ThreadState |= OBERX_THREAD_STATE_RUN;

  // Thread loop
  while ((pOBE->ThreadState & OBERX_THREAD_STATE_STOP) == 0)
  {
    const struct timeval Timeout =
    {
      .tv_sec  = pOBE->Params.RxInterval_ms / 1000,
      .tv_usec = (pOBE->Params.RxInterval_ms % 1000) * 1000
    };
    tQsMessage Msg;

    const tQsStatus Stat = Msg_RecvTimed(QSQUEUE_OBE_RX, &Msg, &Timeout);
    if (Stat == QS_ERR_NO_QUEUE)
    {
      dbg_printf("No queue\n");
      // Don't hog the CPU
      Util_Nap(pOBE->Params.RxInterval_ms, NULL);
      continue;
    }
    else if (Stat == QS_ERR_TIMEOUT)
    {
      continue;
    }
    else if (Stat != QS_ERR_NONE)
    {
      dbg_printf("Failed to receive on obe-rx queue\n");
      break;
    }

    switch (Msg.Id)
    {
      case QSMSG_OBE_RX_WSM:
      {
        pOBE->Stats.Rx.Okay++;
        tExtMessage * pExtMsg = Msg.Ref.pPayload;
        if (pExtMsg)
        {
          // Further processing of the message then release it
          Ext_Release(pExtMsg);
        }
        break;
      }
      default:
        break;
    }
  }

  // Deregister the Ext Callback
  (void)Ext_CallbackDeregister(ExtHandle);
  (void)Msg_QueueDeInit(QSQUEUE_OBE_RX);
  pthread_exit(NULL);
}

// Close the doxygen group
/**
 * @}
 */

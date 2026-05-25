/**
 * @addtogroup mod_evw_tx EVW Tx Module
 * @{
 *
 * EVW transmitter thread
 *
 * @file evw-tx.c Module code for EVW transmitter
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "evw-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "j2735asn.h"
#include "p1609-fac-veh-ctrl-if.h"
#include "util.h"
#include "vstate.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include "Example1609_debug.h"

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_EVWTx_DebugLevel

#define dbg_printf(f, a...)                                   \
  do {                                                        \
    fprintf(stdout, "%s(%d): " f, __func__, __LINE__, ## a);  \
  } while (0)

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static tEVWTx *EVWTx_LoadConfig(char *pConfigFileName);

static void EVWTx_ThreadProc(void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate EVWTx object, Setup thread and associated data structures
 * @param ppEVW pointer to EVWTx handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int EVWTx_Open(tEVWTx **ppEVW,
               pthread_attr_t *pAttr,
               char *pConfigFileName)
{
  int Res = -ENOSYS;
  tEVWTx *pEVW;

  // Load config settings
  pEVW = EVWTx_LoadConfig(pConfigFileName);
  if (pEVW == NULL)
  {
    dbg_printf("EVWTx_LoadConfig() == NULL\n");
    goto Error;
  }
  // Init state

  if (pEVW->Enabled)
  {
    // If we've access to CAN data: Use VState to poll siren & lightbar
    const tdefault_stack *pIni = default_stack_Get();
    if (pIni->TxDataSource == default_stack_TxDataSource_Live_Data)
    {
      pEVW->pVStateReader = VState_ReaderRegister();
      if (pEVW->pVStateReader == NULL)
      {
        dbg_printf("Unable to register VState reader to periodically poll lightbar & siren");
      }
    }
    else
    {
      pEVW->pVStateReader = NULL;
    }
    default_stack_Release(&pIni);
  }

  // Assign thread attributes from input param
  pEVW->ThreadAttr = *pAttr;

  // Create thread
  pEVW->ThreadState |= EVWTX_THREAD_STATE_INIT;
  Res = pthread_create(&pEVW->ThreadID,
                       &pEVW->ThreadAttr,
                       (void *) EVWTx_ThreadProc,
                       pEVW);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pEVW->ThreadState = EVWTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppEVW = pEVW;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppEVW = NULL;
  EVWTx_Close(pEVW);
Success:
  dbg_printf("Successfully started the EVWTx\n");
  return Res;
}

/**
 * @brief Stop execution of EVWTx, free the thread and its associated resources
 * @param pEVW EVWTx handle
 */
void EVWTx_Close (tEVWTx *pEVW)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pEVW == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pEVW->ThreadState |= EVWTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pEVW->ThreadState & EVWTX_THREAD_STATE_INIT)
  {
    pthread_join(pEVW->ThreadID, NULL);
  }

  // Release VState reader
  if (pEVW->pVStateReader != NULL)
  {
    VState_ReaderDeregister(pEVW->pVStateReader);
    pEVW->pVStateReader = NULL;
  }

  // Success!
  Res = 0;

Error:
  free(pEVW);

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the EVWTx object
 * @param pEVW EVWTx handle
 */
void EVWTx_PrintStats(tEVWTx *pEVW)
{
  if (pEVW != NULL)
    fprintf(stdout, "EVWTx: Tx (Okay %8u Fail %8u)\n", pEVW->Stats.Tx.Okay, pEVW->Stats.Tx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load 'EVWTx' parameters from the config file
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static tEVWTx *EVWTx_LoadConfig(char *pConfigFileName)
{
  tEVWTx *pEVW = NULL;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for EVWTxTX in config file
  int ConfigVal = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    goto Error;
  }

  // Get the 'EVWTX' Setting
  pSetting = config_lookup(pConfig, EVWTX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", EVWTX_CONFIG_PATH_NAME);
    goto Error;
  }

  // config is open, now look for configuration entries
  pEVW = (tEVWTx *)malloc(sizeof(tEVWTx));

  if (pEVW == NULL)
  {
    dbg_printf("malloc() = NULL\n");
    goto Error;
  }

  memset(pEVW, 0, sizeof(tEVWTx));

  pEVW->Enabled = false;
  pEVW->ApplInterval = EVWTX_CONFIG_VALUE_DEFAULT_EVWAPPLINTERVAL;

  // EVW BSM Enabled
  if (config_setting_lookup_bool(pSetting,
                                 EVWTX_CONFIG_VALUE_NAME_BSMEVWENABLED,
                                 &ConfigVal))
    pEVW->Enabled = (bool)ConfigVal;

  // EVW BSM ApplInterval
  if (config_setting_lookup_bool(pSetting,
                                 EVWTX_CONFIG_VALUE_NAME_EVWAPPLINTERVAL,
                                 &ConfigVal))
    pEVW->ApplInterval = (uint16_t)ConfigVal;


  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!

Error:
  return pEVW;
}

/**
 * @brief EVWTx Periodic thread processing - sends a WSM
 * @param pArg Pointer to EVWTx object passed as generic input parameter
 *
 */
static void EVWTx_ThreadProc(void *pArg)
{
  tEVWTx *pEVW;
  struct timespec Time;

  // pArg cast to EVW object
  pEVW = (tEVWTx *)pArg;

  // "Starting EVWTx Periodic Thread");
  pEVW->ThreadState |= EVWTX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  dbg_printf("In EVWTx_ThreadProc\n");
  if (pEVW->Enabled)
  {
    if (pEVW->pVStateReader == NULL)
    {
      dbg_printf("EVWTx_ThreadProc: Setting BasicVehicleRole to emergency\n");
      (void)P1609VehCtl_SetBasicVehicleRole(SAEBasicVehicleRole_emergency);
    }
  }

  // Thread loop
  while ((pEVW->ThreadState & EVWTX_THREAD_STATE_STOP) == 0)
  {
    const tVStateState *pVState = NULL;

    // Minimum interval: 50ms
    Util_Nap(50, &Time);

    if (pEVW->Enabled)
    {
      // If we've a reader handle, get the latest VState
      if (pEVW->pVStateReader != NULL)
      {
        pVState = VState_ReaderGetBuffer(pEVW->pVStateReader);
      }

      // No VState based on CAN inputs?
      if (pVState == NULL)
      {
        // Just statically enable. *OR* one could poll a VIC connected GPIO
        (void)P1609VehCtl_SetBasicVehicleRole(SAEBasicVehicleRole_emergency);
      }
      else
      {
        // Light Bar?
        if (VSTATE_IS_AVAIL(pVState->LightBarInUse) &&
            VS_GetLightBarInUse(pVState, NULL) == VS_LightBarInUse_InUse)
        {
          (void)P1609VehCtl_SetBasicVehicleRole(SAEBasicVehicleRole_emergency);
        }
        // Siren?
        else if (VSTATE_IS_AVAIL(pVState->SirenInUse) &&
            VS_GetLightBarInUse(pVState, NULL) == VS_SirenInUse_InUse)
        {
          (void)P1609VehCtl_SetBasicVehicleRole(SAEBasicVehicleRole_emergency);
        }
        // Neither?
        else
        {
          (void)P1609VehCtl_ClearBasicVehicleRole();
        }
      }
    }
  }

  // "Leaving Periodic EVWTx Thread");

  if (pEVW->Enabled)
  {
    (void)P1609VehCtl_ClearBasicVehicleRole();
  }

  // exit thread
  (void) pthread_exit(NULL);

}

// Close the doxygen group
/**
 * @}
 */

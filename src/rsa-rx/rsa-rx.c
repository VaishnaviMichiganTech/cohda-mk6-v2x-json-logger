/**
 * @addtogroup mod_rsa_rx RSA Rx Module
 * @{
 *
 * RSA receiver thread
 *
 * @file rsa-rx.c Module code for RSA receiver
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2012 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "rsa-rx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "ext.h"
#include "id-global.h"
#include "j2735-message.h"
#include "j2735asn.h"
#include "j2735asn_def.h"
#include "util.h"
#include "Example1609_debug.h"

#include <errno.h>
#include <pthread.h>
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

#define D_LEVEL Example1609_Example1609_RSARx_DebugLevel

#define dbg_printf(f, a...)                                       \
    do {                                                          \
        { fprintf(stderr, "%s(%d): " f, __func__, __LINE__, ## a); } \
    } while (0)                                                   \


//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int RSARx_LoadConfig (struct RSARx *pRSA,
                             char * pConfigFileName);

static void RSARx_ThreadProc (void *pArg);
static fExt_Callback RSARxExtCallback;

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate RSARx object, Setup thread and associated data structures
 * @param ppRSA pointer to RSA handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int RSARx_Open (struct RSARx **ppRSA,
                pthread_attr_t *pAttr,
                char *pConfigFileName)
{
  int Res;
  struct RSARx *pRSA;

  // Create and check the local structure
  pRSA = (struct RSARx *) malloc(sizeof(struct RSARx));
  if (pRSA == NULL)
  {
    Res = -ENOMEM;
    goto Error;
  }
  memset(pRSA, 0, sizeof(struct RSARx));

  // Load config settings
  Res = RSARx_LoadConfig(pRSA, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("RSARx_LoadConfig() != 0\n");
    goto Error;
  }

  // Init state

  // Assign thread attributes from input param
  pRSA->ThreadAttr = *pAttr;

  // Create RSARx thread
  pRSA->ThreadState |= RSARX_THREAD_STATE_INIT;
  Res = pthread_create(&pRSA->ThreadID,
                       &pRSA->ThreadAttr,
                       (void *)RSARx_ThreadProc,
                       pRSA);
  if (Res != 0)
  {
    dbg_printf("pthread_create() failed\n");
    // Thread creation error
    pRSA->ThreadState = RSARX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppRSA = pRSA;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppRSA = NULL;
  RSARx_Close(pRSA);
Success:
  dbg_printf("Successfully started the rsa-rx\n");
  return Res;
}

/**
 * @brief Stop execution of RSA, free the thread and its associated resources
 * @param pRSA RSA handle
 */
void RSARx_Close (struct RSARx *pRSA)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pRSA == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pRSA->ThreadState |= RSARX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pRSA->ThreadState & RSARX_THREAD_STATE_INIT)
    pthread_join(pRSA->ThreadID, NULL);

  // Success!
  Res = 0;

Error:
  free(pRSA);

  (void)Res;    // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the TPSE object
 * @param pRSA RSA handle
 */
void RSARx_PrintStats (struct RSARx *pRSA)
{
  fprintf(stdout, "RSARx: Rx (Okay %8u Fail %8u)\n",
	  pRSA->Stats.Rx.Okay, pRSA->Stats.Rx.Fail);

}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load RSARX parameters from the config file
 * @param pRSA RSA receiver handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int RSARx_LoadConfig (struct RSARx *pRSA,
                             char *pConfigFileName)
{
  int Res;
  config_t Config; // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting; // setting for RSARX in config file
  int ConfigVal = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pRSA->Params.RxInterval = RSARX_CONFIG_VALUE_DEFAULT_RXINTERVAL;

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the RSARx Setting
  pSetting = config_lookup(pConfig, RSARX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", RSARX_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // Rx thread Interval
  if (config_setting_lookup_int(pSetting,
                                RSARX_CONFIG_VALUE_NAME_RXINTERVAL,
                                &ConfigVal))
    pRSA->Params.RxInterval = (uint32_t) ConfigVal;

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  return Res;
}

/**
 * @brief EXT module Callback function
 *
 * Extracts and counts Road Side Alerts.
 *
 * @param Event EXT event ID
 * @param pMsg EXT data associated with Event ID
 * @param pPriv Private data (tRSARx)
 *
 */
void RSARxExtCallback(tExtEventId Event,
                      tExtMessage *pMsg,
                      void *pPriv)
{
  if (Event != QSMSG_EXT_RX_WSM)
    return;

  if ((pMsg == NULL) || (pPriv == NULL))
    return;

  SAERoadSideAlert *pRSA;
  pRSA = (SAERoadSideAlert *)J2735_MsgExtract(pMsg, SAEDSRCmsgID_roadSideAlert);
  if (pRSA == NULL)
  {
    return;
  }

  dbg_printf("Road Side Alert Message! MsdCount=%d, ITISCodes=%lx\n",
             pRSA->msgCnt, (unsigned long)pRSA->typeEvent);

  struct RSARx *pRSARx = (struct RSARx *)pPriv;
  pRSARx->Stats.Rx.Okay++;
}

/**
 * @brief RSA Periodic thread processing - sends a RSA
 * @param pArg Pointer to RSARx object passed as generic input parameter
 *
 */
static void RSARx_ThreadProc (void *pArg)
{
  struct RSARx *pRSA;
  struct timespec Time;
  int ExtHandle;

  dbg_printf("Started thread\n");

  // pArg cast to TPSE object
  pRSA = (struct RSARx *) pArg;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // "Starting RSA Periodic Thread");
  pRSA->ThreadState |= RSARX_THREAD_STATE_RUN;

  // Register the callback with the Ext Module
  ExtHandle = Ext_CallbackRegister(&RSARxExtCallback, pRSA);

  if (ExtHandle < 0)
  {
    dbg_printf("Unable to register callback with EXT module\n");
    goto Error;
  }

  dbg_printf("Registered Callback!\n");

  // Thread loop
  while ((pRSA->ThreadState & RSARX_THREAD_STATE_STOP) == 0)
  {
    // polling delay
    Util_Nap(pRSA->Params.RxInterval, &Time);
  }

  // Deregister the Ext Callback
  Ext_CallbackDeregister(ExtHandle);

Error:
  // exit thread
  (void) pthread_exit(NULL);

}
// Close the doxygen group
/**
 * @}
 */


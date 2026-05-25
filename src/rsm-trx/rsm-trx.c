/**
 * @addtogroup mod_rsm_trx RSM Tx/Rx Module
 * @{
 *
 * @file rsm-trx.c Example code for RSM transmitter/receiver
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2024 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

// Sub-module includes
#include "rsm-trx.h"

// Application includes
#include "libconfig.h" // used to read configuration file
#include "Example1609_debug.h"

// v2x-lib includes
#include "dot3-wsmp.h"
#include "ext.h"
#include "j2735asn.h"
#include "j2735asn_def.h"
#include "j2735-message.h"
#include "j29454-rsm-if.h"
#include "util.h"
#include "debug-levels.h"

// System includes
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

// .conf: 'Example1609.RSM.DebugLevel = ?`
#define D_LEVEL Example1609_Example1609_RSM_DebugLevel

/// Maximum size of the RSM
#define RSM_WSM_BUF_SIZE      (1386)

// tools/ffasn1c/ffasn1dump -I random -O uper stack/v2x-lib/include/SAE.asn1 RoadSafetyMessage 0 | xxd -i
static const uint8_t TxUPER[] = {
  0x04, 0x00, 0x6b, 0xee, 0x2e, 0xc3, 0x01, 0xcf, 0xd3, 0xe1, 0x11, 0x5f,
  0xa0, 0x0d, 0x1e, 0x2b, 0x28, 0x40, 0x00, 0x0b, 0xf4, 0x50, 0xdf, 0xa3,
  0x39, 0xa1, 0x80, 0x03, 0x0d, 0x21, 0xa5, 0x30, 0x0d, 0xa6, 0x91, 0x38,
  0x01, 0x4c, 0x5c, 0xc2, 0xf9, 0xd9, 0x00, 0x00, 0x03, 0xbf, 0x10, 0x00,
  0x80, 0xb1, 0x80, 0x04, 0x28, 0x03, 0xe0, 0x11, 0x00, 0x00, 0x00, 0x00,
  0x35, 0xdc, 0xd1, 0x96, 0x00, 0x74, 0x00, 0x00, 0x2c, 0xce, 0x20, 0x00,
  0x00, 0x06, 0x55, 0x08, 0xd4, 0x24, 0x78, 0x00, 0xa5, 0xa0, 0x00, 0x15,
  0x4f, 0xc0, 0x00, 0x8b, 0x5b, 0x61, 0xe8, 0x8a, 0x83, 0x48, 0x20, 0x01,
  0x41, 0x65, 0x00, 0x04, 0x00, 0x02, 0x17, 0x69, 0x00, 0x16, 0x00, 0x00,
  0x02, 0x90, 0x20, 0x00, 0x00, 0x00, 0x52, 0x8e, 0x7d, 0x00, 0x84, 0x48,
  0xc3, 0x00, 0x00, 0x00, 0x02, 0x00, 0x5a, 0x40, 0x70, 0x9e, 0xc5, 0x00,
  0xb3, 0x6f, 0xde, 0x08, 0x82, 0x58, 0x21, 0x19, 0xf2, 0x0c, 0x68, 0x18,
  0x7c, 0x69, 0xb8, 0x00, 0x06, 0x10, 0x00, 0x00, 0x47, 0x80, 0x02, 0x2f,
  0xa4, 0x40, 0x39, 0xb1, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x13, 0x00, 0x00, 0x05, 0x2b, 0xc0, 0x00, 0x00, 0x00, 0x50, 0x76,
  0x04, 0x00, 0x00, 0x7c, 0x8f, 0x80, 0x00, 0x06, 0xbe, 0x00, 0x00, 0x01,
  0x0b, 0xa0, 0x00, 0x00, 0x00, 0x11, 0x40, 0xa4, 0x76, 0xb5, 0x07, 0xc8,
  0x25, 0x02, 0xa0, 0x00, 0x40, 0x02, 0x00, 0x24, 0x0f, 0x8e, 0x1d, 0xc1,
  0xc0, 0x00, 0x00, 0x02, 0x44, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x00, 0x5f,
  0x34, 0x05, 0x4c, 0x58, 0x80, 0x97, 0x40, 0x00, 0x28, 0x80, 0x76, 0x32,
  0x0c, 0x78, 0x32, 0x8a, 0x00, 0x00, 0x03, 0x31, 0xa0, 0x00, 0x50, 0x00,
  0x82, 0x20, 0x0a, 0x05, 0x47, 0xfc, 0x00, 0x31, 0x00, 0x02, 0x80, 0x01,
  0x00, 0x96, 0x22, 0xb1, 0x00, 0x27, 0xdb, 0x57, 0x7a, 0x01, 0xa7, 0xee,
  0x7c, 0x66, 0x00, 0x40, 0x10, 0x77, 0xa0, 0xaf, 0xc1, 0x00, 0x00, 0x0a,
  0x41, 0x00, 0x00, 0x2a, 0xfd, 0x00, 0x1a, 0x01, 0x40, 0x14, 0x23, 0xf8,
  0x1d, 0xd0, 0x00, 0x8a, 0x6a, 0xf2, 0x0b, 0x70, 0x00, 0x00, 0x04, 0xc0,
  0x00, 0x18, 0xa0, 0xa2, 0x0a, 0xff, 0x01, 0x2d, 0x00, 0x00, 0xbd, 0x6b,
  0x00, 0x00, 0x00, 0x0e, 0x00, 0x01, 0x45, 0x08, 0x00, 0x04, 0x80, 0x0a,
  0x40, 0x00, 0x00, 0x00, 0xe0, 0x75, 0xa0, 0x00, 0x00, 0x00, 0x0a, 0x0b,
  0x8f, 0x00, 0x00, 0x00, 0x03, 0x9e, 0x20, 0x20, 0x01, 0x44, 0xc6, 0x21,
  0xec, 0x0a, 0xd4, 0x78, 0x01, 0x8e, 0xba, 0x84, 0x00, 0x04, 0x00, 0x18,
  0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0b, 0xc0, 0x01, 0x40
};

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int RSM_LoadConfig (tRSM *pRSM,
                           const char *pConfigFileName);

static void RSM_ThreadProc (void *pArg);

static fExt_Callback RSM_ExtCallback;

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate RSM object, Setup thread and associated data structures
 * @param ppRSM pointer to RSM handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int RSM_Open (tRSM **ppRSM,
              const pthread_attr_t *pAttr,
              const char *pConfigFileName)
{
  int Res;
  tRSM *pRSM;

  d_fnstart(D_DBG, NULL, "\n");

  // Create the local structure
  pRSM = (tRSM *)malloc(sizeof(tRSM));
  if (pRSM == NULL)
  {
    Res = -ENOMEM;
    goto Error;
  }
  memset(pRSM, 0, sizeof(tRSM));

  // Load config settings
  Res = RSM_LoadConfig(pRSM, pConfigFileName);
  if (Res != 0)
  {
    d_printf(D_NOTICE, NULL, "RSM_LoadConfig() = %d\n", Res);
    goto Error;
  }

  // Assign thread attributes from input param
  pRSM->ThreadAttr = *pAttr;

  // Create RSMTx thread
  pRSM->ThreadState |= RSM_THREAD_STATE_INIT;
  Res = pthread_create(&(pRSM->ThreadID),
                       &(pRSM->ThreadAttr),
                       (void *)RSM_ThreadProc,
                       pRSM);
  if (Res != 0)
  {
    d_printf(D_DBG, NULL, "pthread_create() failed\n");
    // Thread creation error
    pRSM->ThreadState = RSM_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppRSM = pRSM;
  Res = 0;

Error:
  if (Res != 0)
  {
    *ppRSM = NULL;
    RSM_Close(pRSM);
  }

  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Stop execution of RSM, free the thread and its associated resources
 * @param pRSM RSM handle
 */
void RSM_Close (tRSM *pRSM)
{
  int Res;

  d_fnstart(D_DBG, NULL, "\n");

  // Catch attempt to close invalid object pointer
  if (pRSM == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pRSM->ThreadState |= RSM_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pRSM->ThreadState & RSM_THREAD_STATE_INIT)
  {
    pthread_join(pRSM->ThreadID, NULL);
  }

  // Success!
  Res = 0;

Error:
  free(pRSM);

  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the RSM object
 * @param pRSM RSM handle
 */
void RSM_PrintStats (tRSM *pRSM)
{
  d_fnstart(D_DBG, NULL, "\n");

  if (pRSM == NULL)
  {
    goto Error;
  }

  if (pRSM->Params.TxInterval_ms != 0)
  {
    fprintf(stdout, "RSM: Tx (Okay %8u Fail %8u)\n",
      pRSM->Stats.Tx.Okay, pRSM->Stats.Tx.Fail);
  }
  if (pRSM->Params.RxEnable == true)
  {
    fprintf(stdout, "RSM: Rx (Net %8u Fac %8u)\n",
      pRSM->Stats.Rx.Net, pRSM->Stats.Rx.Fac);
  }

Error:
  d_fnend(D_DBG, NULL, "()\n");
  return;
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load RSMTx parameters from the config file
 * @param pRSM RSM transmitter handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int RSM_LoadConfig (tRSM *pRSM,
                           const char *pConfigFileName)
{
  int Res;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for 'RSM' in config file
  int ConfigVal = 0;

  d_fnstart(D_DBG, NULL, "\n");

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pRSM->Params.TxInterval_ms = RSM_CONFIG_VALUE_DEFAULT_TXINTERVAL_MS;
  pRSM->Params.RxEnable = RSM_CONFIG_VALUE_DEFAULT_RXENABLE;

  d_printf(D_DBG, NULL, "TxInterval_ms %d RxEnable %d\n",
           pRSM->Params.TxInterval_ms, pRSM->Params.RxEnable);

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    d_printf(D_DBG, NULL, "Could not open %s\n", pConfigFileName);
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'RSM' Setting
  pSetting = config_lookup(pConfig, RSM_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    d_printf(D_DBG, NULL, "config_lookup(%s) failed\n", RSM_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // TxInterval_ms
  if (config_setting_lookup_int(pSetting,
                                RSM_CONFIG_VALUE_NAME_TXINTERVAL_MS,
                                &ConfigVal))
  {
    pRSM->Params.TxInterval_ms = (uint32_t) ConfigVal;
    d_printf(D_DBG, NULL, "TxInterval_ms %d\n", pRSM->Params.TxInterval_ms);
  }
  // RxEnabled
  if (config_setting_lookup_bool(pSetting,
                                 RSM_CONFIG_VALUE_NAME_RXENABLE,
                                 &ConfigVal))
  {
    pRSM->Params.RxEnable = (bool) ConfigVal;
    d_printf(D_DBG, NULL, "RxEnable %d\n", pRSM->Params.RxEnable);
  }

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief EXT module Callback function
 *
 * Extracts and counts RSMs.
 *
 * @note In order not to delay other EXT handlers: \
         Do the minimum filtering required here-in \
         and defer in-depth processing to another thread.
 *
 * @param Event EXT event ID
 * @param pMsg EXT data associated with Event ID
 * @param pPriv Private data (tRSMRx)
 */
static void RSM_ExtCallback(tExtEventId Event,
                            tExtMessage *pExtMsg,
                            void *pPriv)
{
  struct RSM *pRSM;
  struct Dot3WSMPHdr *pWSMHdr;
  uint32_t DSRCMsgID;
  SAERoadSafetyMessage *pMsg;

  if (Event != QSMSG_EXT_RX_WSM)
  {
    goto Exit;
  }

  pRSM = (struct RSM *)pPriv;
  if (pRSM->Params.RxEnable != true)
  {
    goto Exit;
  }

  if ((pExtMsg == NULL) || (pPriv == NULL))
  {
    goto Exit;
  }

  if (pExtMsg->pWSM == NULL)
  {
    goto Exit;
  }
  pWSMHdr = pExtMsg->pWSM;

  d_printf(D_DBG, NULL, "PSID %x\n", ntohl(pWSMHdr->PSID));
  if (ntohl(pWSMHdr->PSID) != J29454_RSM_PSID)
  {
    // Not the expected PSID for RSMs
    goto Exit;
  }

  // Inspect enough to determine the DSRCmsgID
  DSRCMsgID = J2735_MsgInspect(pWSMHdr, NULL, NULL);
  if (DSRCMsgID != SAEDSRCmsgID_roadSafetyMessage)
  {
    // Not the expected DSRCmsgID for RSMs
    goto Exit;
  }
  pRSM->Stats.Rx.Net++;

  d_printf(D_DBG, NULL, "RSM @net Len %u\n", ntohs(pWSMHdr->Length));

  // Find the ASN.1 UPER and decode the RSM
  pMsg = (SAERoadSafetyMessage *)J2735_MsgExtract(pExtMsg, SAEDSRCmsgID_roadSafetyMessage);
  if (pMsg == NULL)
  {
    goto Exit;
  }
  pRSM->Stats.Rx.Fac++;

  // Optionally inspect the ASN.1 contents further
  {
    size_t i;
    uint32_t Hex = 0;
    SAETemporaryID *pTmpId = &(pMsg->commonContainer.eventInfo.eventID.uniqueID);
    for (i = 0; i < pTmpId->len; i++)
    {
      Hex |= (uint32_t)(pTmpId->buf[i]) << (i * 8);
    }
    d_printf(D_DBG, NULL, "RSM @fac ID 0x%08x\n", Hex);
  }

  // Optionally forward the message to another handler
  {
    // Increment the refcount so it's not free()'ed when this callback returns
    Ext_Retain(pExtMsg);

    int Res = -ENOSYS; // Put it in a queue to be proecessed later...
    if (Res)
    {
      // And decrement the refcount when done processing the message
      Ext_Release(pExtMsg);
    }
  }

Exit:
  return;
}


/**
 * @brief Create RSM ASN.1
 * @param pRSM
 * @param[out] ppMsg Message created
 * @return 0 for success. Negative for errors.
 */
static int RSM_CreateMsg (tRSM *pRSM,
                          struct SAERoadSafetyMessage **ppMsg)
{
  int Res;
  struct SAERoadSafetyMessage *pMsg;

  d_fnstart(D_DBG, NULL, "\n");

  if ((pRSM == NULL) || (ppMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  if (0)
  {
    // == Create message contents programatically ==
    pMsg = (struct SAERoadSafetyMessage *)calloc(1, sizeof(struct SAERoadSafetyMessage));
    if (pMsg == NULL)
    {
      Res = -ENOMEM;
      goto Error;
    }
    ; // Fill in pMsg
  }
  else
  {
    // == Populate ASN.1 using pre-defined UPER ==
    ASN1Error Err= {-1,-1,"",""};
    Res = asn1_uper_decode((void **)&pMsg,
                           asn1_type_SAERoadSafetyMessage,
                           TxUPER,
                           sizeof(TxUPER),
                           &Err);
    if (Res < 0)
    {
      d_printf(D_DBG, NULL, "UPER decode fail at %d/%d: %s\n",
                 Err.line_num, Err.bit_pos, Err.msg);
      goto Error;
    }
  }
  *ppMsg = pMsg;
  Res = 0;

Error:
  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Destroy RSM message
 * @param pRSM
 * @param pMsg Message to destroy
 * @return 0 for success. Negative for errors.
 */
static int RSM_DestroyMsg (tRSM *pRSM,
                           struct SAERoadSafetyMessage *pMsg)
{
  int Res;

  d_fnstart(D_DBG, NULL, "\n");

  if ((pRSM == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  asn1_free_value(asn1_type_SAERoadSafetyMessage, pMsg);
  Res = 0;

Error:
  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Transmit RSM
 * @param pRSM RSM Tx module
 * @return 0 for success. Negative for errors.
 */
static int RSM_Transmit (tRSM *pRSM)
{
  int Res;
  struct SAERoadSafetyMessage *pMsg = NULL;

  d_fnstart(D_DBG, NULL, "\n");

  // Create (fill in) the ASN.1 structure
  Res = RSM_CreateMsg(pRSM, &pMsg);
  if (Res != 0)
  {
    goto Error;
  }

  // Transmit the RSM
  Res = J29454RSM_TxReq(pMsg, NULL);
  if (Res == 0)
  {
    pRSM->Stats.Tx.Okay++;
  }
  else
  {
    pRSM->Stats.Tx.Fail++;
  }

Error:
  (void) RSM_DestroyMsg(pRSM, pMsg);
  d_fnend(D_DBG, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief RSM Periodic thread processing - sends a RSM
 * @param pArg Pointer to RSMTx object passed as generic input parameter
 *
 */
static void RSM_ThreadProc (void *pArg)
{
  tRSM *pRSM;
  struct timespec Time;
  int ExtHandle;

  d_fnstart(D_DBG, NULL, "\n");

  // pArg cast to RSMtx object
  pRSM = (tRSM *)pArg;

  // "Starting RSM Periodic Thread");
  pRSM->ThreadState |= RSM_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Register the callback with the Ext Module
  ExtHandle = Ext_CallbackRegister(&RSM_ExtCallback, pRSM);
  if (ExtHandle < 0)
  {
    d_printf(D_DBG, NULL, "Unable to register callback with EXT module\n");
  }

  // Thread loop
  while ((pRSM->ThreadState & RSM_THREAD_STATE_STOP) == 0)
  {
    // Zero indicates that Tx is disabled
    if (pRSM->Params.TxInterval_ms == 0)
    {
      // Snooze a while
      d_printf(D_DBG, NULL, "...\n");
      Util_Nap(500, &Time);
    }
    else
    {
      // Tx the RSM
      Util_Nap(pRSM->Params.TxInterval_ms, &Time);
      (void)RSM_Transmit(pRSM);
    }
  }

  // Deregister the Ext Callback
  Ext_CallbackDeregister(ExtHandle);

  // "Leaving Periodic RSM Thread");
  d_fnend(D_DBG, NULL, "()\n");

  // exit thread
  (void) pthread_exit(NULL);
  return;
}

// Close the doxygen group
/**
 * @}
 */

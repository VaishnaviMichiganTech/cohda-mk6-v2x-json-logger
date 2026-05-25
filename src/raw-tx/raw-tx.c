/**
 * @addtogroup mod_raw_tx Raw WSM Tx Module
 * @{
 *
 * Raw WSM transmitter thread
 *
 * @file raw-tx.c Module code for Raw WSM transmitter
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "raw-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "dot3-wsmp.h"
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

#define D_LEVEL Example1609_Example1609_RawTx_DebugLevel

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

static tRawTx *RawTx_LoadConfig(char *pConfigFileName);

static void RawTx_ThreadProc(void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate RawTx object, Setup thread and associated data structures
 * @param ppRaw pointer to RawTx handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int RawTx_Open(tRawTx **ppRaw,
               pthread_attr_t *pAttr,
               char *pConfigFileName)
{
  int Res = -ENOSYS;
  tRawTx *pRaw;

  // Load config settings
  pRaw = RawTx_LoadConfig(pConfigFileName);
  if (pRaw == NULL)
  {
    dbg_printf("RawTx_LoadConfig() == NULL\n");
    goto Error;
  }
  // Init state
  ;

  // Assign thread attributes from input param
  pRaw->ThreadAttr = *pAttr;

  // Create thread
  pRaw->ThreadState |= RAWTX_THREAD_STATE_INIT;
  Res = pthread_create(&pRaw->ThreadID,
                       &pRaw->ThreadAttr,
                       (void *) RawTx_ThreadProc,
                       pRaw);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pRaw->ThreadState = RAWTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppRaw = pRaw;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppRaw = NULL;
  RawTx_Close(pRaw);
Success:
  dbg_printf("Successfully started the RawTx\n");
  return Res;
}

/**
 * @brief Stop execution of RawTx, free the thread and its associated resources
 * @param pRaw RawTx handle
 */
void RawTx_Close (tRawTx *pRaw)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pRaw == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pRaw->ThreadState |= RAWTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pRaw->ThreadState & RAWTX_THREAD_STATE_INIT)
  {
    pthread_join(pRaw->ThreadID, NULL);
  }

  // Success!
  Res = 0;

Error:
  free(pRaw);

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the RawTx object
 * @param pRaw RawTx handle
 */
void RawTx_PrintStats(tRawTx *pRaw)
{
  int i;

  for (i = 0; i < (int)(pRaw->MsgCnt); i++)
  {
    tRawTxMsg *pMsg = &(pRaw->Msg[i]);

    fprintf(stdout, "RawTx[%d]: Tx (Okay %8u Fail %8u)\n",
	          i, pMsg->Stats.Tx.Okay, pMsg->Stats.Tx.Fail);
  }
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load 'RawTx' parameters from the config file
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static tRawTx *RawTx_LoadConfig(char *pConfigFileName)
{
  tRawTx *pRaw = NULL;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for RawTxTX in config file
  int ConfigVal = 0, MsgCnt = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    goto Error;
  }

  // Get the 'RawTX' Setting
  pSetting = config_lookup(pConfig, RAWTX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", RAWTX_CONFIG_PATH_NAME);
    goto Error;
  }

  // config is open, now look for configuration entries
  MsgCnt = config_setting_length(pSetting);
  if (MsgCnt <= 0)
  {
    dbg_printf("RawTx[] is empty\n");
    goto Error;
  }

  {
    // Create and check the local structure
    const int RawSize = sizeof(tRawTx) + (MsgCnt * sizeof(tRawTxMsg));
    pRaw = (tRawTx *)calloc(RawSize, 1);
    if (pRaw == NULL)
    {
      dbg_printf("malloc() = NULL\n");
      goto Error;
    }
  }

  // Array of messages
  {
    int i;
    for (i = 0; i < MsgCnt; i++)
    {
      tRawTxMsg *pMsg = &(pRaw->Msg[i]);
      config_setting_t *pElem = config_setting_get_elem(pSetting, i);
      if (pElem == NULL)
      {
        dbg_printf("config_setting_get_elem(%s[%d]) failed\n", RAWTX_CONFIG_PATH_NAME, i);
        goto Error;
      }
      // Set defaults
      pMsg->Params.TxInterval = RAWTX_CONFIG_VALUE_DEFAULT_TXINTERVAL;
      { // WSMP defaults
        memset(pMsg->Params.WSMP.Tx.DA, 0xFF, 6);
        pMsg->Params.WSMP.Tx.Priority   = RAWTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY;
        pMsg->Params.WSMP.Tx.ExpiryTime = RAWTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME;
        pMsg->Params.WSMP.Version       = DOT3_WSMP_VERSION_3;
        pMsg->Params.WSMP.ChannelNumber = RAWTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER;
        const tdefault_stack *pIni = default_stack_Get();
        pMsg->Params.WSMP.ChannelNumber = pIni->ForcedControlChanNum;
        default_stack_Release(&pIni);
        pMsg->Params.WSMP.DataRate      = RAWTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE;
        pMsg->Params.WSMP.TxPower       = RAWTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER;
        pMsg->Params.WSMP.PSID          = htonl(RAWTX_CONFIG_VALUE_DEFAULT_WSMP_PSID);
        pMsg->Params.WSMP.Length        = htons(RAWTX_CONFIG_VALUE_DEFAULT_WSMP_LENGTH);
        pMsg->Params.WSMP.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
      }
      memcpy(pMsg->Params.Data,
             RAWTX_CONFIG_VALUE_DEFAULT_WSMP_DATA,
             RAWTX_CONFIG_VALUE_DEFAULT_WSMP_LENGTH);
      // TxInterval
      if (config_setting_lookup_int(pElem,
                                    RAWTX_CONFIG_VALUE_NAME_TXINTERVAL,
                                    &ConfigVal))
      {
        pMsg->Params.TxInterval = (uint32_t) ConfigVal;
      }
      // WSM
      {
        config_setting_t *pWSM;
        pWSM = config_setting_get_member(pElem,
                                         RAWTX_CONFIG_VALUE_NAME_WSMP);
        if (pWSM != NULL)
        {
          // Priority
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_PRIORITY,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.Tx.Priority = (uint8_t) ConfigVal;
          }
          // ExpiryTime
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_EXPIRY_TIME,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.Tx.ExpiryTime = (uint8_t) ConfigVal;
          }
          // ChannelNumber
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.ChannelNumber = (uint8_t) ConfigVal;
          }
          // DataRate
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_DATA_RATE,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.DataRate = (uint8_t) ConfigVal;
          }
          // DataRate
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_TX_POWER,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.TxPower = (uint8_t) ConfigVal;
          }
          // PSID
          if (config_setting_lookup_int(pWSM,
                                        RAWTX_CONFIG_VALUE_NAME_PSID,
                                        &ConfigVal))
          {
            pMsg->Params.WSMP.PSID = htonl((uint32_t) ConfigVal);
          }
          // Data
          {
            config_setting_t *pData;
            pData = config_setting_get_member(pWSM, RAWTX_CONFIG_VALUE_NAME_DATA);
            if (pData != NULL)
            {
              int k;
              int Cnt = config_setting_length(pData);

              if (Cnt > RAWTX_WSM_BUF_SIZE)
              Cnt = RAWTX_WSM_BUF_SIZE;

              for (k = 0; k < Cnt; k++)
              {
                ConfigVal = config_setting_get_int_elem(pData, k);
                pMsg->Params.Data[k] = (uint8_t)ConfigVal;
              }
              pMsg->Params.WSMP.Length = htons((uint16_t) Cnt);
            }
          }
        }
      }

    } // end of for (i < MsgCnt)
    pRaw->MsgCnt = i;
  }

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!

Error:
  return pRaw;
}

/**
 * @brief Transmit the WSM
 * @param pMsg Message to be transmitted
 * @return 0 for success. Negative for errors.
 */
static int RawTx_TransmitWSM(tRawTxMsg *pMsg)
{
  int Res;
  uint8_t *pWSM;
  int WSMSize;

  // Allocate the WSM buffer
  WSMSize = sizeof(struct Dot3WSMPHdr) + RAWTX_WSM_BUF_SIZE;
  pWSM = calloc(1, WSMSize);
  if (pWSM == NULL)
  {
    // "Failed to allocate Tx WSM buffer for encoding.");
    Res = -ENOMEM;
    goto Error;
  }

  // Copy the data into the WSM buffer
  {
    uint8_t *pBER = pWSM + sizeof(struct Dot3WSMPHdr);

    memcpy(pWSM, &(pMsg->Params.WSMP), sizeof(struct Dot3WSMPHdr));
    memcpy(pBER, pMsg->Params.Data, ntohs(pMsg->Params.WSMP.Length));
  }

  // Transmit the WSM.
  // Enable signing (might be fake sig if security is disabled)
  // Also disable Tx logging.
  Res = P1609Tx_SendWSM((struct Dot3WSMPHdr *) pWSM, true, false, -1);
  if (Res != 0)
  {
    // P1609Tx_SendWSM failed
    goto Error;
  }
  else
  {
    // Sent WSM
  }

Error:
  free(pWSM);
  return Res;
}


/**
 * @brief RawTx Periodic thread processing - sends a WSM
 * @param pArg Pointer to RawTx object passed as generic input parameter
 */
static void RawTx_ThreadProc(void *pArg)
{
  tRawTx *pRaw;
  struct timespec Time;

  // pArg cast to TPSE object
  pRaw = (tRawTx *) pArg;

  // "Starting RawTx Periodic Thread");
  pRaw->ThreadState |= RAWTX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while ((pRaw->ThreadState & RAWTX_THREAD_STATE_STOP) == 0)
  {
    int i;

    // Minimum interval: 50ms
    Util_Nap(50, &Time);

    for (i = 0; i < (int)(pRaw->MsgCnt); i++)
    {
      tRawTxMsg *pMsg = &(pRaw->Msg[i]);
      pMsg->Interval += 50;
      if (pMsg->Interval >= pMsg->Params.TxInterval)
      {
        int Res = RawTx_TransmitWSM(pMsg); // Transmit a WSM
        if (Res == 0)
        {
          pMsg->Stats.Tx.Okay++;
        }
        else
        {
          pMsg->Stats.Tx.Fail++;
        }
        pMsg->Interval = 0;
      }

    }
  }

  // "Leaving Periodic RawTx Thread");

  // exit thread
  (void) pthread_exit(NULL);

}

// Close the doxygen group
/**
 * @}
 */

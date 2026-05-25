/**
 * @addtogroup mod_raw_tx Raw WSM Tx Module
 * @{
 *
 * @section mod_raw_tx_dd Raw WSM transmission
 * Send a periodic WSM based entirely on the configuration provided.
 *
 *
 * @file raw-tx.h Raw WSM transmitter definitions
 *
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2012 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __RAW_TX_H_
#define __RAW_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"

#include <pthread.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Maximum size of the WSM
#define RAWTX_WSM_BUF_SIZE      (1386)

/// Location of the RSA transmitter configuration settings
#define RAWTX_CONFIG_PATH_NAME  "Example.RawTx"

// Configuration value names
#define RAWTX_CONFIG_VALUE_NAME_TXINTERVAL      "TxInterval"
#define RAWTX_CONFIG_VALUE_NAME_WSMP            "WSM"
#define RAWTX_CONFIG_VALUE_NAME_PSID            "PSID"
#define RAWTX_CONFIG_VALUE_NAME_PRIORITY        "Priority"
#define RAWTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER  "ChannelNumber"
#define RAWTX_CONFIG_VALUE_NAME_DATA_RATE       "DataRate"
#define RAWTX_CONFIG_VALUE_NAME_TX_POWER        "TxPower"
#define RAWTX_CONFIG_VALUE_NAME_EXPIRY_TIME     "ExpiryTime"
#define RAWTX_CONFIG_VALUE_NAME_LENGTH          "Length"
#define RAWTX_CONFIG_VALUE_NAME_DATA            "Data"

// Configuration value defaults for mandatory items
#define RAWTX_CONFIG_VALUE_DEFAULT_TXINTERVAL           (500)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_PSID            (0xE0545354)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY        (2)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER  (178)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE       (12)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER        (32)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME     (0)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_LENGTH          (11)
#define RAWTX_CONFIG_VALUE_DEFAULT_WSMP_DATA            "\x30\x09\x80\x01\x01\xa1\x00\x82\x02\x00\x00"

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------


/// State IDs for Raw Tx thread (bitmask)
typedef enum RawTxThreadState
{
  /// Not initialized
  RAWTX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  RAWTX_THREAD_STATE_INIT = 0x01,
  /// Running
  RAWTX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  RAWTX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  RAWTX_THREAD_STATE_END  = 0x08,
} eRawTxThreadState;
/// @ref RawTxThreadState
typedef int tRawTxThreadState;

/// RSA transmitter Statistics
typedef struct RawTxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;
} tRawTxStats;

/// Raw transmitter parameters - Stored in Config File
typedef struct RawTxParams
{
  /// Transmit period
  uint32_t TxInterval;
  /// WSMP parameters
  struct Dot3WSMPHdr WSMP;
  /// Pointer to the WSM Data
  uint8_t Data[RAWTX_WSM_BUF_SIZE];
} tRawTxParams;

/// Raw transmitter array element
typedef struct RawTxMsg
{
  /// For transmit period operation
  uint32_t Interval;
  /// Configuration
  tRawTxParams Params;
  /// Module statistics
  tRawTxStats Stats;
} tRawTxMsg;

/// Raw transmitter state
typedef struct RawTx
{
  /// ID of RSA transmitter thread
  pthread_t ThreadID;
  /// RSA transmitter thread state
  tRawTxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Array of messages
  uint32_t MsgCnt;
  tRawTxMsg Msg[];
} tRawTx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int RawTx_Open(tRawTx **ppRaw,
               pthread_attr_t *pAttr,
               char *pConfigFileName);

void RawTx_Close(tRawTx *pRaw);

void RawTx_PrintStats(tRawTx *pRaw);

#endif // __RAW_TX_H_
/**
 * @}
 */

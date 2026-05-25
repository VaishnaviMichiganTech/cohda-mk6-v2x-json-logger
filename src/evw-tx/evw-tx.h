/**
 * @addtogroup mod_evw_tx EVW Tx Module
 * @{
 *
 * @section mod_evw_tx_dd EVW transmission
 * Send a periodic EVW packet based entirely on the configuration provided.
 *
 *
 * @file evw-tx.h EVW transmitter definitions
 *
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __EVW_TX_H_
#define __EVW_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "vstate.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the EVW transmitter configuration settings
#define EVWTX_CONFIG_PATH_NAME                   "Example.EVWTx"
#define EVWTX_CONFIG_VALUE_NAME_BSMEVWENABLED    "Enabled"
#define EVWTX_CONFIG_VALUE_NAME_EVWAPPLINTERVAL  "ApplInterval"
#define EVWTX_CONFIG_VALUE_DEFAULT_EVWAPPLINTERVAL (1000) // mSec

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------


/// State IDs for EVW Tx thread
typedef enum EVWTxThreadState
{
  /// Not initialized
  EVWTX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  EVWTX_THREAD_STATE_INIT = 0x01,
  /// Running
  EVWTX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  EVWTX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  EVWTX_THREAD_STATE_END  = 0x08,
} eEVWTxThreadState;
/// @ref EVWTxThreadState
typedef int tEVWTxThreadState;

/// RSA transmitter Statistics
typedef struct EVWTxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;
} tEVWTxStats;

/// EVW transmitter state
typedef struct EVWTx
{
  /// ID of EVW polling thread
  pthread_t ThreadID;
  /// RSA transmitter thread state
  tEVWTxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Stats
  tEVWTxStats Stats;
  /// EVW BSM enabled?
  bool Enabled;
  /// Transmit interval
  uint16_t ApplInterval;
  /// Next transmit timestamp
  uint64_t NextTxTimeMS;
  // VState handle for polling CAN signals
  tUtilBufferQueueReader *pVStateReader;
} tEVWTx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int EVWTx_Open(tEVWTx **ppEVW,
               pthread_attr_t *pAttr,
               char *pConfigFileName);

void EVWTx_Close(tEVWTx *pEVW);

void EVWTx_PrintStats(tEVWTx *pEVW);

#endif // __EVW_TX_H_
/**
 * @}
 */

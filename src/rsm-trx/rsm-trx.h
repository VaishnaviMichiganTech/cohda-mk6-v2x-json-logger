/**
 * @addtogroup mod_rsm_trx RSM Tx/Rx Module
 * @{
 *
 * @section mod_rsm_tx_dd RSM transmitter
 * Example RSM periodic transmitter.
 *
 * @section mod_rsm_rx_dd RSM receiver
 * Example RSA receiver that prints out the details of any received RSMs
 *
 * @file rsm-trx.h RSM definitions
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2024 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __RSM_TRX_H_
#define __RSM_TRX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"
#include "j29454-rsm-if.h"

#include <pthread.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the RSM transmitter configuration settings
#define RSM_CONFIG_PATH_NAME                          "Example.RSM"

// Configuration value names
#define RSM_CONFIG_VALUE_NAME_TXINTERVAL_MS           "TxInterval_ms"
#define RSM_CONFIG_VALUE_NAME_RXENABLE                "RxEnable"

// Configuration value defaults (all disabled)
#define RSM_CONFIG_VALUE_DEFAULT_TXINTERVAL_MS        (0)
#define RSM_CONFIG_VALUE_DEFAULT_RXENABLE             (false)

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

/// State IDs for RSM Tx thread (bitmask)
typedef enum RSMThreadState
{
  /// Not initialized
  RSM_THREAD_STATE_NONE = 0x00,
  /// Initializing
  RSM_THREAD_STATE_INIT = 0x01,
  /// Running
  RSM_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  RSM_THREAD_STATE_STOP = 0x04,
  /// Stopped
  RSM_THREAD_STATE_END  = 0x08,
} eRSMThreadState;
/// @ref RSMThreadState
typedef int tRSMThreadState;

/// RSM statistics
typedef struct RSMStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;
  /// Receive counters
  struct {
    uint32_t Net;
    uint32_t Fac;
  } Rx;
} tRSMStats;

/// RSM parameters - Stored in .cfg File
typedef struct RSMParams
{
  /// Transmit period
  uint32_t TxInterval_ms;
  /// Receive handling enabled?
  uint32_t RxEnable;

} tRSMParams;

/// RSM transmitter state
typedef struct RSM
{
  /// ID of RSM transmitter thread
  pthread_t ThreadID;
  /// RSM transmitter thread state
  tRSMThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  tRSMStats Stats;
  /// Configuration
  tRSMParams Params;
} tRSM;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int RSM_Open (tRSM **ppRSM,
              const pthread_attr_t *pAttr,
              const char *pConfigFileName);

void RSM_Close (tRSM *pRSM);

void RSM_PrintStats (tRSM *pRSM);

#endif // __RSM_TRX_H_
/**
 * @}
 */

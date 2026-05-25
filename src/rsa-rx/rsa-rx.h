/**
 * @addtogroup mod_rsa_rx RSA Rx Module
 * @{
 *
 * @section mod_rsa_rx_dd What Does It Do
 * Example RSA receiver. Creates a thread to listen to WSM messages.
 * Decodes all WSM messages and prints out the details of any RSA messages
 * received.
 *
 * @file rsa-rx.h RSA receiver definitions
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2010 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __RSA_RX_H_
#define __RSA_RX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include <pthread.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the RSA receiver configuration settings
#define RSARX_CONFIG_PATH_NAME  "Example.RSARx"

// Configuration value names
#define RSARX_CONFIG_VALUE_NAME_RXINTERVAL      "RxInterval"

// Configuration value defaults for mandatory items
#define RSARX_CONFIG_VALUE_DEFAULT_RXINTERVAL           (500)

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------


/// State IDs for RSA Rx thread (bitmask)
typedef enum RSARxThreadState
{
  /// Not initialized
  RSARX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  RSARX_THREAD_STATE_INIT = 0x01,
  /// Running
  RSARX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  RSARX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  RSARX_THREAD_STATE_END  = 0x08,
} eRSARxThreadState;
/// @ref RSARxThreadState
typedef int tRSARxThreadState;

/// RSA receiver Statistics
typedef struct RSARxStats
{
  /// Receive counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Rx;

} tRSARxStats;

/// RSA receiver parameters - Stored in Config File
typedef struct RSARxParams
{
  /// period
  uint32_t RxInterval; // just for the thread sleep

} tRSARxParams;

/// RSA receiver state
typedef struct RSARx
{
  /// ID of RSA receiver thread
  pthread_t ThreadID;
  /// RSA receiver thread state
  tRSARxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  struct RSARxStats Stats;
  /// Configuration
  struct RSARxParams Params;
} tRSARx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int RSARx_Open (struct RSARx **ppRSA,
                pthread_attr_t *pAttr,
                char *pConfigFileName);

void RSARx_Close (struct RSARx *pRSA);

void RSARx_PrintStats (struct RSARx *pRSA);

#endif // __RSA_RX_H_
/**
 * @}
 */

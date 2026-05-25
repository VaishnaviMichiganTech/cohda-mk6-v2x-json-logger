/**
 * @addtogroup mod_tc_client TC Client Module
 * @{
 *
 * @section mod_tc_client_dd What Does It Do
 * Registers with @ref tc_api to receive a particular incoming TC messages.
 * When a message is received information about the threat is displayed.
 *
 * @file tc-client.h TC Client definitions
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2014 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __TC_CLIENT_H_
#define __TC_CLIENT_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the CANVState configuration settings
#define TCCLIENT_CONFIG_PATH_NAME  "Example.TCClient"

// Configuration value names
#define TCCLIENT_CONFIG_VALUE_NAME_VERBOSE         "Verbose"
#define TCCLIENT_CONFIG_VALUE_NAME_POLLINGINTERVAL "PollingInterval"

// Configuration value defaults for mandatory items
#define TCCLIENT_CONFIG_VALUE_DEFAULT_VERBOSE         false
#define TCCLIENT_CONFIG_VALUE_DEFAULT_POLLINGINTERVAL 0

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
typedef enum TCClientThreadState
{
  /// Not initialized
  TCCLIENT_THREAD_STATE_NONE = 0x00,
  /// Initializing
  TCCLIENT_THREAD_STATE_INIT = 0x01,
  /// Running
  TCCLIENT_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  TCCLIENT_THREAD_STATE_STOP = 0x04,
  /// Stopped
  TCCLIENT_THREAD_STATE_END  = 0x08,
} eTCClientThreadState;

typedef struct TCClientState
{
  // configuration information
  bool     Verbose;
  uint32_t PollingInterval;

  // thread information
  pthread_t ThreadID;
  unsigned  ThreadState;

  // state handles
  unsigned clientID;
} tTCClientState;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int TCClient_Open (tTCClientState **ppState,
                   const pthread_attr_t *pAttr,
                   const char *pConfigFileName);

void TCClient_Close (tTCClientState *pState);


#endif // __TC_CLIENT_H_
/**
 * @}
 */

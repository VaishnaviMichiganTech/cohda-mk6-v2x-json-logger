/**
 * @addtogroup app_ntrip Ntrip client to DSRC forwarder
 * @{
 *
 * @file
 */

//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __NTRIP_RX_H_
#define __NTRIP_RX_H_


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

/// Ntrip receive specific state information
typedef struct NtripRx
{
  /// @ref eNtripRxState
  uint32_t State;

  /// Socket file handle
  int Fd;
  // optional username & password
  char Credentials[64];
  // Ntrip caster hostname or address
  char Hostname[96];
  // Ntrip caster port number
  char Port[16];
  // Ntrip caster mountpoint
  char Mountpoint[64];

  /// Timestamp of last GGA
  /// (used to honour GGA_Interval config parameter)
  uint64_t TimestampGGA;

  /// Thread related
  struct {
    /// Handle
    pthread_t Id;
  } Thread;

} tNtripRx;


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

int NtripRx_Init(void);
void NtripRx_Exit(void);

#if defined(UNITTEST)
#endif

#endif // __NTRIP_RX_H_

/**
 * @}
 */

#pragma once
/**
 * @addtogroup app_ntrip Ntrip client to DSRC forwarder
 * @{
 *
 * @file
 */

//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __NTRIP_H__
#define __NTRIP_H__

// ----------------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------------
#include "Ntrip-rx.h"

#include "Ntrip_debug.h"
#include "Ntrip_defn.h"

#include <stdbool.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/// Ntrip configurable parameters
typedef struct NtripParam
{
  /// Enabled?
  bool Enable;
  /// Log verbosity level
  int DebugLevel;
} tNtripParam;

/// Top level Ntrip state information
typedef struct Ntrip
{
  bool Active;                  ///<
  bool NA;                      ///< NA specific behaviour
  bool EU;                      ///< EU specific behaviour
  bool CN;                      ///< EU specific behaviour
  /// Ntrip confguration parameters (from the config)
  tNtrip_Cust_App_Ntrip Param;
  /// Client functionality
  tNtripRx Rx;
} tNtrip;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

/// Singleton
extern tNtrip Ntrip;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

int  Ntrip_Init(void);
void Ntrip_Exit(void);

#if defined(UNITTEST)
tNtrip *Ntrip_Ptr(void);
#endif

#endif // __NTRIP_H__
/**
 * @}
 */

/**
 * @addtogroup mod_obe_rx OBE Rx Module
 * @{
 *
 * @section mod_obe_rx_dd What Does It Do
 * Demonstrates the usage of the @ref ta_api to enable FCW, EEBL and ICW
 * applications for an On-Board Equipment. Also demonstrates the usage of the
 * @ref hmi_api for displaying alerts to a driver via some HMI.
 *
 * @file obe-rx.h OBE receiver definitions
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2014 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __OBE_RX_H_
#define __OBE_RX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "default_stack_defn.h"
#include "hmi.h"
#include "TA.h"

// used to read configuration file
#include "libconfig.h"

#include <pthread.h>
#include <stdint.h>
//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the OBE receiver configuration settings
#define OBERX_CONFIG_PATH_NAME                "Example.OBE"

/// Configuration value names
#define OBERX_CONFIG_VALUE_NAME_RXINTERVAL    "RxInterval"

/// Configuration value defaults for mandatory items
#define OBERX_CONFIG_VALUE_DEFAULT_RXINTERVAL (500)

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

/// Manage the thread state using bit flags
typedef enum OBERxThreadState
{
  /// Not initialized
  OBERX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  OBERX_THREAD_STATE_INIT = 0x01,
  /// Running
  OBERX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  OBERX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  OBERX_THREAD_STATE_END  = 0x08,
} eOBERxThreadState;
/// @ref OBERxThreadState
typedef uint8_t tOBERxThreadState;

/// OBE receiver Statistics
typedef struct OBERxStats
{
  /// Receive counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Rx;

} tOBERxStats;

/// OBE receiver parameters - Stored in Config File
typedef struct OBERxParams
{
  /// period
  uint32_t RxInterval_ms; // just for the thread sleep

} tOBERxParams;

/// OBE receiver state
typedef struct OBERx
{
  /// ID of OBE receiver thread
  pthread_t ThreadID;
  /// OBE receiver thread state
  tOBERxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  struct OBERxStats Stats;
  /// Configuration
  struct OBERxParams Params;
  // HMI object
  struct HMI *pHMI;
  // TM Params
  tdefault_stack_Cohda_TM TMParams;
} tOBERx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int OBERx_Open(struct OBERx **ppOBE,
               pthread_attr_t *pAttr,
               char *pConfigFileName);

void OBERx_Close(struct OBERx *pOBE);

void OBERx_PrintStats(struct OBERx *pOBE);

void OBERx_ReadCustomHMIAlert(const config_t *pConfig,
                              tHMI *pHMI,
                              const char *pPath);

void OBERx_Threattype_CSW(const tTAAlert *pAlert,
                          tHMI *pHMI);

void OBERx_Threattype_WZA(const tTAAlert *pAlert,
                          tHMI *pHMI);

void OBERx_Threattype_SpeedLimit(const tTAAlert *pAlert,
                                 tHMI *pHMI);

void OBERx_Threattype_GenericTIM(const tTAAlert *pAlert,
                                 tHMI *pHMI);

#endif // __OBE_RX_H_
/**
 * @}
 */

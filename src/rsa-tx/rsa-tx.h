/**
 * @addtogroup mod_rsa_tx RSA Tx Module
 * @{
 *
 * @section mod_rsa_tx_dd Periodic RSA transmitter
 * Sends a RSA message periodically.
 *
 *
 * @file rsa-tx.h RSA transmitter definitions
 *
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2012 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __RSA_TX_H_
#define __RSA_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"
#include "j2735-road-side-alert.h"

#include <pthread.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the RSA transmitter configuration settings
#define RSATX_CONFIG_PATH_NAME  "Example.RSATx"

// Configuration value names
#define RSATX_CONFIG_VALUE_NAME_TXINTERVAL      "TxInterval"
#define RSATX_CONFIG_VALUE_NAME_WSMP            "WSMP"
#define RSATX_CONFIG_VALUE_NAME_PSID            "PSID"
#define RSATX_CONFIG_VALUE_NAME_CHANNEL_NUMBER  "ChannelNumber"
#define RSATX_CONFIG_VALUE_NAME_DATA_RATE       "DataRate"
#define RSATX_CONFIG_VALUE_NAME_TX_POWER        "TxPower"
#define RSATX_CONFIG_VALUE_NAME_EXPIRY_TIME     "ExpiryTime"
#define RSATX_CONFIG_VALUE_NAME_TYPE_EVENT      "TypeEvent"
#define RSATX_CONFIG_VALUE_NAME_DESCRIPTION     "Description"
#define RSATX_CONFIG_VALUE_NAME_PRIORITY        "Priority"
#define RSATX_CONFIG_VALUE_NAME_HEADING         "Heading"
#define RSATX_CONFIG_VALUE_NAME_EXTENT          "Extent"
#define RSATX_CONFIG_VALUE_NAME_POSITION        "Position"
#define RSATX_CONFIG_VALUE_NAME_FURTHERINFO     "FurtherInfo"

// Configuration value defaults for mandatory items
#define RSATX_CONFIG_VALUE_DEFAULT_TXINTERVAL           (500)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_PSID            (0xE0525341)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY        (2)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER  (172)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE       (12)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER        (32)
#define RSATX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME     (0)
#define RSATX_CONFIG_VALUE_DEFAULT_TYPE_EVENT           (0xC0DA)
//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

/// State IDs for RSA Tx thread (bitmask)
typedef enum RSATxThreadState
{
  /// Not initialized
  RSATX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  RSATX_THREAD_STATE_INIT = 0x01,
  /// Running
  RSATX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  RSATX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  RSATX_THREAD_STATE_END  = 0x08,
} eRSATxThreadState;
/// @ref RSATxThreadState
typedef int tRSATxThreadState;

/// RSA transmitter Statistics
typedef struct RSATxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;
} tRSATxStats;

/// RSA transmitter parameters - Stored in Config File
typedef struct RSATxParams
{
  /// Transmit period
  uint32_t TxInterval;

  /// WSMP parameters
  struct Dot3WSMPHdr WSMP;

  /// ASN.1 structure config
  struct RoadSideAlertCfg Cfg;
  /// ASN.1 value
  uint16_t TypeEvent;
  /// ASN.1 value
  uint16_t Description[8];
  /// ASN.1 value
  uint8_t Priority;
  /// ASN.1 value
  uint16_t Heading;
  /// ASN.1 value
  uint8_t Extent;
  /// ASN.1 value(s)
  struct {
    /// ASN.1 value
    uint32_t Lon;
    /// ASN.1 value
    uint32_t Lat;
  } Position;
  /// ASN.1 value
  uint8_t FurtherInfo[2];

} tRSATxParams;

/// RSA transmitter state
typedef struct RSATx
{
  /// ID of RSA transmitter thread
  pthread_t ThreadID;
  /// RSA transmitter thread state
  tRSATxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// LPH handle
  void *pLPH;
  /// ASN.1 value INTEGER (0..127)
  int MsgCount;
  /// Module statistics
  struct RSATxStats Stats;
  /// Configuration
  struct RSATxParams Params;
} tRSATx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int RSATx_Open (struct RSATx **ppRSA,
                pthread_attr_t *pAttr,
                char *pConfigFileName);

void RSATx_Close (struct RSATx *pRSA);

void RSATx_PrintStats (struct RSATx *pRSA);

#endif // __RSA_TX_H_
/**
 * @}
 */

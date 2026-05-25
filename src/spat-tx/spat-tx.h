/**
 * @addtogroup mod_spat_tx SPAT Tx Module
 * @{
 *
 * @section mod_spat_tx_dd SPAT transmission
 * Send a SPAT message based entirely on the configuration provided.
 *
 * @file spat-tx.h SPAT transmitter definitions
 *
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __SPAT_TX_H_
#define __SPAT_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"
#include "j2735-spat.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the SPAT configuration settings
#define SPATTX_CONFIG_PATH_NAME  "Example.SPAT"

// SPAT Configuration value names
#define SPATTX_CONFIG_VALUE_NAME_WSMP            "WSMP"
#define SPATTX_CONFIG_VALUE_NAME_PSID            "PSID"
#define SPATTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER  "ChannelNumber"
#define SPATTX_CONFIG_VALUE_NAME_DATA_RATE       "DataRate"
#define SPATTX_CONFIG_VALUE_NAME_TX_POWER        "TxPower"
#define SPATTX_CONFIG_VALUE_NAME_EXPIRY_TIME     "ExpiryTime"
#define SPATTX_CONFIG_VALUE_NAME_TYPE_EVENT      "TypeEvent"
#define SPATTX_CONFIG_VALUE_NAME_DESCRIPTION     "Description"
#define SPATTX_CONFIG_VALUE_NAME_PRIORITY        "Priority"
#define SPATTX_CONFIG_VALUE_NAME_HEADING         "Heading"
#define SPATTX_CONFIG_VALUE_NAME_EXTENT          "Extent"
#define SPATTX_CONFIG_VALUE_NAME_POSITION        "Position"
#define SPATTX_CONFIG_VALUE_NAME_FURTHERINFO     "FurtherInfo"
#define SPATTX_CONFIG_VALUE_NAME_SPATENABLED             "Enabled"
#define SPATTX_CONFIG_VALUE_NAME_SPATDYNAMICPHASES       "DynamicPhases"
#define SPATTX_CONFIG_VALUE_NAME_SPATAPPLINTERVAL        "ApplInterval"
#define SPATTX_CONFIG_VALUE_NAME_SPATINTERSECTIONID      "IntersectionId"
#define SPATTX_CONFIG_VALUE_NAME_SPATREDTIME             "RedPhasePeriod"
#define SPATTX_CONFIG_VALUE_NAME_SPATGREENTIME           "GreenPhasePeriod"
#define SPATTX_CONFIG_VALUE_NAME_SPATYELLOWTIME          "YellowPhasePeriod"
#define SPATTX_CONFIG_VALUE_NAME_SPATMOVEMENETS          "Movements"
#define SPATTX_CONFIG_VALUE_NAME_SPATSIGNALGROUP         "SignalGroup"
#define SPATTX_CONFIG_VALUE_NAME_SPATCURRENTSTATE        "CurrentState"
#define SPATTX_CONFIG_VALUE_NAME_SPATTIMETOCHANGE        "TimeToChange"
#define SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICE         "SpeedAdvice"
#define SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICESPEED    "Speed"
#define SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICEDISTANCE "Distance"
#define SPATTX_CONFIG_VALUE_NAME_SPATSPEEDADVICETYPE     "Type"

#define SPATTX_CONFIG_VALUE_DEFAULT_SPATDYNAMICPHASES    (false)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATAPPLINTERVAL     (100) // mSec
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATINTERSECTIONID   (0x5566)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATREDTIME          (260)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATGREENTIME        (100)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATYELLOWTIME       (30)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATSIGNALGROUP      (0)
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATCURRENTSTATE     (0) // Unavailble/Off
#define SPATTX_CONFIG_VALUE_DEFAULT_SPATTIMETOCHANGE     (36002) // Undefined
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_PSID            (0x8002)
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY        (7)
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER  (172)
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE       (12)
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER        (20)
#define SPATTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME     (0)
#define SPATTX_CONFIG_VALUE_DEFAULT_TYPE_EVENT           (0xC0DA)



//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
typedef struct SPATSpeedAdvisory
{
  uint16_t Speed_tenthmps;
  uint16_t Distance_m;
  uint8_t  Type;
} tSPATSpeedAdvisory;


typedef struct SPATMovementParams
{
  uint8_t CurrentState;
  uint8_t SignalGroup;
  uint16_t TimeToChange;
  /// Number of SpeedAdvisories for this Movement
  uint8_t NumSpeedAdvisories;
  /// Intersection lanes config
  tSPATSpeedAdvisory *pSpeedAdvisories;
} tSPATMovementParams;


/// State IDs for SPAT Tx thread
typedef enum SPATTxThreadState
{
  /// Not initialized
  SPATTX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  SPATTX_THREAD_STATE_INIT = 0x01,
  /// Running
  SPATTX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  SPATTX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  SPATTX_THREAD_STATE_END  = 0x08,
} eSPATTxThreadState;
/// @ref SPATTxThreadState
typedef int tSPATTxThreadState;

/// SPAT transmitter Statistics
typedef struct SPATTxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;

} tSPATTxStats;

/// SPAT parameters - Stored in Config File
typedef struct SPATTxParams
{
  bool Enabled;
  /// WSMP parameters
  struct Dot3WSMPHdr WSMP;
  /// ASN.1 structure config
  struct SPATCfg Cfg;
  /// When set, SPAT phases keeps changing each Tx (using phases timing below)
  bool DynamicPhases;
  /// Spat transmission interval (mSec)
  uint16_t ApplInterval;
  /// Next SPAT
  uint64_t NextTxTimeMS;
  /// RSU intersection Id
  uint32_t IntersectionId;
  /// Red state reload time period (tength of sec)
  uint16_t RedPhasePeriod;
  /// Green state reload time period (tength of sec)
  uint16_t GreenPhasePeriod;
  /// Yellow state reload time period (tength of sec)
  uint16_t YellowPhasePeriod;
  /// Number of movement states for this SPAT msg
  uint8_t NumMovements;
  /// All current movement states
  tSPATMovementParams *pMovements;
} tSPATTxParams;

/// SPAT state
typedef struct SPATTx
{
  /// ID of SPAT thread
  pthread_t ThreadID;
  /// SPAT thread state
  tSPATTxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  struct SPATTxStats Stats;
  /// ASN.1 structure
  struct SAESPAT *pASN;
  /// Configuration
  struct SPATTxParams Params;
} tSPATTx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int SPATTx_Open (struct SPATTx **ppSPAT,
                 pthread_attr_t *pAttr,
                 char *pConfigFileName);

void SPATTx_Close (struct SPATTx *pSPAT);

void SPATTx_PrintStats (struct SPATTx *pSPAT);

#endif // __SPAT_TX_H_
/**
 * @}
 */

/**
 * @addtogroup mod_sdsm_tx Sensor Data Sharing Tx Module
 * @{
 *
 * @section mod_sdsm_tx_dd SDSM transmission
 * Send a SDSM message based entirely on the configuration provided.
 *
 * @file sdsm-tx.h SDSM transmitter definitions
 *
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2022 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __SDSM_TX_H_
#define __SDSM_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"
#include "j3224-sdsm.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the SDSM configuration settings
#define SDSMTX_CONFIG_PATH_NAME  "Example.SDSM"
#define SDSMTX_CONFIG_VALUE_NAME_OBJECTS  "Objects"

#define SDSMTX_CONFIG_VALUE_NAME_ID       "objectID"
#define SDSMTX_CONFIG_VALUE_NAME_TYPE     "objType"
#define SDSMTX_CONFIG_VALUE_NAME_TYPECONF "objTypeCfd"
#define SDSMTX_CONFIG_VALUE_NAME_OFFSETX  "offsetX"
#define SDSMTX_CONFIG_VALUE_NAME_OFFSETY  "offsetY"
#define SDSMTX_CONFIG_VALUE_NAME_OFFSETZ  "offsetZ"
#define SDSMTX_CONFIG_VALUE_NAME_SPEED    "speed"
#define SDSMTX_CONFIG_VALUE_NAME_HEADING  "heading"

#define SDSMTX_CONFIG_VALUE_NAME_WSMP            "WSMP"
#define SDSMTX_CONFIG_VALUE_NAME_PSID            "PSID"
#define SDSMTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER  "ChannelNumber"
#define SDSMTX_CONFIG_VALUE_NAME_DATA_RATE       "DataRate"
#define SDSMTX_CONFIG_VALUE_NAME_TX_POWER        "TxPower"
#define SDSMTX_CONFIG_VALUE_NAME_EXPIRY_TIME     "ExpiryTime"
#define SDSMTX_CONFIG_VALUE_NAME_PRIORITY        "Priority"

#define SDSMTX_CONFIG_VALUE_NAME_SDSMENABLED             "Enabled"
#define SDSMTX_CONFIG_VALUE_NAME_SDSMINTERVAL_MS         "Interval_ms"
#define SDSMTX_CONFIG_VALUE_NAME_OBJECTMAXAGE_MS         "ObjectMaxAge_ms"
#define SDSMTX_CONFIG_VALUE_NAME_SDSMDATASOURCE          "DataSource"

#define SDSMTX_CONFIG_VALUE_DEFAULT_SDSMINTERVAL_MS      (200) // mSec
#define SDSMTX_CONFIG_VALUE_DEFAULT_OBJECTMAXAGE_MS      (1500) // mSec
#define SDSMTX_CONFIG_VALUE_DEFAULT_SDSMDATASOURCE       (1)

#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_PSID            (0x8010) // p-encoding of 0x90
#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY        (7)
#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER  (172)
#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE       (12)
#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER        (20)
#define SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME     (0)

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------

/// State IDs for SDSM Tx thread
typedef enum SDSMTxThreadState
{
  /// Not initialized
  SDSMTX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  SDSMTX_THREAD_STATE_INIT = 0x01,
  /// Running
  SDSMTX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  SDSMTX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  SDSMTX_THREAD_STATE_END  = 0x08,
} eSDSMTxThreadState;
/// @ref SDSMTxThreadState
typedef int tSDSMTxThreadState;

/// SDSM object data source
typedef enum SDSMTxDataSource
{
  SDSMTX_DATA_SOURCE_CFG = 0,
  SDSMTX_DATA_SOURCE_UDP = 1,
} eSDSMTxDataSource;

/// SDSM transmitter Statistics
typedef struct SDSMTxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;

} tSDSMTxStats;

typedef struct SAEDetectedObjectDataSnapshot
{
  uint64_t Time_ms; // Unix time in ms
  int Latitude;  // J2735 Latitude
  int Longitude; // J2735 Longitude
  int Elevation; // J2735 Elevation
  SAEDetectedObjectData Object;
} tSAEDetectedObjectDataSnapshot;

/// SDSM parameters - Stored in Config File
typedef struct SDSMTxParams
{
  bool Enabled;
  /// Object data source
  int DataSource;
  /// WSMP parameters
  struct Dot3WSMPHdr WSMP;
  /// sdsm transmission interval (mSec)
  uint16_t Interval_ms;
  /// Maximum object age (mSec)
  uint16_t ObjectMaxAge_ms;
  /// Number of objects
  uint16_t NumObjects;
  /// Objects
  tSAEDetectedObjectDataSnapshot *pObjects;
} tSDSMTxParams;

/// SDSM state
typedef struct SDSMTx
{
  /// ID of SDSM thread
  pthread_t ThreadID;
  /// SDSM Tx thread state
  tSDSMTxThreadState ThreadState;
  /// ID of object Rx thread
  pthread_t ObjectRx_ThreadID;
  /// Object Rx thread state
  tSDSMTxThreadState ObjectRx_ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  struct SDSMTxStats Stats;
  /// Configuration
  struct SDSMTxParams Params;
} tSDSMTx;

typedef struct ObjectData
{
  // Object ID
  uint32_t ObjectID;
  // J3224's object type
  //  unknown (0), vehicle (1), vru (2), animal (3)
  SAEObjectType ObjectType;
  // Timestamp of object data in unix time [ms]
  uint64_t Time_ms;
  /// J2735's DE_Latitude.
  /// Range: -900000000..900000001 (-90..90) Units: 1/10 micro degree
  SAELatitude Lat;
  /// J2735's DE_Longitude.
  /// Range -1799999999..1800000001 (-180..180) Units: 1/10 micro degree
  SAELongitude Long;
  /// J2735's DE_Elevation
  /// -4096..61439
  /// -- In units of 10 cm steps above or below the reference ellipsoid
  /// -- Providing a range of -409.5 to + 6143.9 meters
  /// -- The value -4096 shall be used when Unknown is to be sent
  SAEElevation Elev;
  /// J2735's DE_Speed
  /// Range: 0..8190 (0..163.8 m/s) Units: 0.02 m/s
  /// Special value 8191 indicates that speed is unavailable
  SAESpeed Speed;
  /// J2735's DE_Heading
  /// Range: 0..28800 (0 to 359.9875 degrees) Units: 0.0125 degrees
  SAEHeading Heading;
} tObjectData;

typedef struct ObjectDataList
{
  // 0xdeadbeef
  uint32_t Magic;
  // Number of objects
  int NumObjects;
  // Pointer to the object data (actually it just follows this structure)
  tObjectData Objects[0];
} tObjectDataList;


//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int SDSMTx_Open (struct SDSMTx **ppSDSM,
                 pthread_attr_t *pAttr,
                 char *pConfigFileName);

void SDSMTx_Close (struct SDSMTx *pSDSM);

void SDSMTx_PrintStats (struct SDSMTx *pSDSM);

#endif // __SDSM_TX_H_
/**
 * @}
 */

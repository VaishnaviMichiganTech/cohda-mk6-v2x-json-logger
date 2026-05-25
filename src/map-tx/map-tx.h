/**
 * @addtogroup mod_map_tx MAP Tx Module
 * @{
 *
 * @section mod_map_tx_dd MAP transmission
 * Send a MAP message based entirely on the configuration provided.
 *
 * @file
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __MAP_TX_H_
#define __MAP_TX_H_

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "dot3-wsmp.h"
#include "j2735-map.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Location of the MAP configuration settings
#define MAPTX_CONFIG_PATH_NAME  "Example.MAP"

// MAP Configuration value names
#define MAPTX_CONFIG_VALUE_NAME_WSMP            "WSMP"
#define MAPTX_CONFIG_VALUE_NAME_PSID            "PSID"
#define MAPTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER  "ChannelNumber"
#define MAPTX_CONFIG_VALUE_NAME_DATA_RATE       "DataRate"
#define MAPTX_CONFIG_VALUE_NAME_TX_POWER        "TxPower"
#define MAPTX_CONFIG_VALUE_NAME_EXPIRY_TIME     "ExpiryTime"
#define MAPTX_CONFIG_VALUE_NAME_TYPE_EVENT      "TypeEvent"
#define MAPTX_CONFIG_VALUE_NAME_DESCRIPTION     "Description"
#define MAPTX_CONFIG_VALUE_NAME_PRIORITY        "Priority"
#define MAPTX_CONFIG_VALUE_NAME_HEADING         "Heading"
#define MAPTX_CONFIG_VALUE_NAME_EXTENT          "Extent"
#define MAPTX_CONFIG_VALUE_NAME_POSITION        "Position"
#define MAPTX_CONFIG_VALUE_NAME_FURTHERINFO     "FurtherInfo"
#define MAPTX_CONFIG_VALUE_NAME_MAPENABLED         "Enabled"
#define MAPTX_CONFIG_VALUE_NAME_MAPAPPLINTERVAL    "ApplInterval"
#define MAPTX_CONFIG_VALUE_NAME_MAPINTERSECTIONID  "IntersectionId"
#define MAPTX_CONFIG_VALUE_NAME_MAPLATITUDE        "Latitude"
#define MAPTX_CONFIG_VALUE_NAME_MAPLONGITUDE       "Longitude"
#define MAPTX_CONFIG_VALUE_NAME_MAPLANES           "Lanes"
#define MAPTX_CONFIG_VALUE_NAME_MAPLANEID          "LaneId"
#define MAPTX_CONFIG_VALUE_NAME_MAPLANEDIRECTION   "Direction"
#define MAPTX_CONFIG_VALUE_NAME_MAPNODES           "Nodes"
#define MAPTX_CONFIG_VALUE_NAME_MAPNODEX           "X"
#define MAPTX_CONFIG_VALUE_NAME_MAPNODEY           "Y"
#define MAPTX_CONFIG_VALUE_NAME_MAPCONNECTIONS     "Connections"
#define MAPTX_CONFIG_VALUE_NAME_MAPSIGNALGROUP     "SignalGroup"
#define MAPTX_CONFIG_VALUE_NAME_MAPCONNECTSTO      "ConnectsTo"
#define MAPTX_CONFIG_VALUE_NAME_MAPMANEUVER        "Maneuver"

#define MAPTX_CONFIG_VALUE_DEFAULT_MAPAPPLINTERVAL      (1000) // mSec
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPINTERSECTIONID    (0x5566)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPLATITUDE          (-340000000)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPLONGITUDE         (1380000000)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPLANEID            (0)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPNODEX             (0) // Segment length = 0
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPNODEY             (0) // Segment length = 0
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPSIGNALGROUP       (0)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPCONNECTSTO        (0)
#define MAPTX_CONFIG_VALUE_DEFAULT_MAPMANEUVER          (0) // Undefined
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_PSID            (0x8002)
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY        (7)
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER  (172)
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE       (12)
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER        (20)
#define MAPTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME     (0)
#define MAPTX_CONFIG_VALUE_DEFAULT_TYPE_EVENT           (0xC0DA)


//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
typedef struct MAPLaneNodeParams
{
  int X;
  int Y;
} tMAPLaneNodeParams;

typedef struct MAPLaneConnectionParams
{
  int ConnectsTo;
  uint8_t SignalGroup;
  uint8_t Maneuver;
} tMAPLaneConnectionParams;

typedef struct MAPLaneParams
{
  int LaneId;
  int NumNodes;
  int NumConnections;
  bool egressLane;
  bool ingressLane;
  tMAPLaneNodeParams *pNodes;
  tMAPLaneConnectionParams *pConnections;
} tMAPLaneParams;

/// State IDs for MAP Tx thread
typedef enum MAPTxThreadState
{
  /// Not initialized
  MAPTX_THREAD_STATE_NONE = 0x00,
  /// Initializing
  MAPTX_THREAD_STATE_INIT = 0x01,
  /// Running
  MAPTX_THREAD_STATE_RUN  = 0x02,
  /// Stopping
  MAPTX_THREAD_STATE_STOP = 0x04,
  /// Stopped
  MAPTX_THREAD_STATE_END  = 0x08,
} eMAPTxThreadState;
/// @ref MAPTxThreadState
typedef int tMAPTxThreadState;

/// MAP transmitter Statistics
typedef struct MAPTxStats
{
  /// Transmit counters
  struct {
    uint32_t Okay;
    uint32_t Fail;
  } Tx;

} tMAPTxStats;

/// MAP parameters - Stored in Config File
typedef struct MAPTxParams
{
  bool Enabled;
  /// WSMP parameters
  struct Dot3WSMPHdr WSMP;
  /// ASN.1 structure config
  struct MAPCfg Cfg;
  /// Map transmission interval (mSec)
  uint16_t ApplInterval;
  /// Next MAP
  uint64_t NextTxTimeMS;
  /// RSU intersection Id
  uint32_t IntersectionId;
  /// RSU latitude in tenths of microdegrees
  int32_t Latitude;
  /// RSU longitude in tenths of microdegrees
  int32_t Longitude;
  /// Number of "vehicle" lanes in this intersection
  uint8_t NumLanes;
  /// Intersection lanes config
  tMAPLaneParams *pLanes;
} tMAPTxParams;

/// MAP state
typedef struct MAPTx
{
  /// ID of MAP thread
  pthread_t ThreadID;
  /// MAP thread state
  tMAPTxThreadState ThreadState;
  /// Attributes used for thread
  pthread_attr_t ThreadAttr;
  /// Module statistics
  struct MAPTxStats Stats;
  /// Configuration
  struct MAPTxParams Params;
} tMAPTx;

//-----------------------------------------------------------------------------
// Function Declarations
//-----------------------------------------------------------------------------

int MAPTx_Open (struct MAPTx **ppMAP,
                 pthread_attr_t *pAttr,
                 char *pConfigFileName);

void MAPTx_Close (struct MAPTx *pMAP);

void MAPTx_PrintStats (struct MAPTx *pMAP);

#endif // __MAP_TX_H_
/**
 * @}
 */

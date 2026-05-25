/**
 * @addtogroup mod_map_tx MAP Tx Module
 * @{
 *
 * @internal
 *
 */
//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "map-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "conf.h"
#include "debug-levels.h"
#include "default_stack_defn.h"
#include "dot3-wsmp.h"
#include "j2735-common.h"
#include "j2735-map.h"
#include "j2735asn.h"
#include "p1609-tx-msg.h"
#include "util.h"
#include "Example1609_debug.h"

#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_MAPTx_DebugLevel

#define dbg_printf(f, a...)                                       \
    do {                                                          \
        fprintf(stdout, "%s(%d): " f, __func__, __LINE__, ## a);  \
    } while (0)                                                   \

/// Maximum size of the WSM
// TODO: is this sufficient?
#define MAPTX_WSM_BUF_SIZE      (1386)

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int MAPTx_LoadConfig (struct MAPTx *pMAP,
                             char * pConfigFileName);

static void MAPTx_ThreadProc (void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

extern pthread_mutex_t P1609Tx_SendWSM_Mutex;

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate MAPTx object, Setup thread and associated data structures
 * @param ppMAP pointer to MAPTx handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int MAPTx_Open (struct MAPTx **ppMAP,
                pthread_attr_t *pAttr,
                char *pConfigFileName)
{
  int Res;
  struct MAPTx *pMAP;

  // Create and check the local structure
  pMAP = (struct MAPTx *) malloc(sizeof(struct MAPTx));
  if (pMAP == NULL)
  {
    dbg_printf("malloc() = NULL\n");
    Res = -ENOMEM;
    goto Error;
  }
  memset(pMAP, 0, sizeof(struct MAPTx));

  // Load config settings
  Res = MAPTx_LoadConfig(pMAP, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("MAPTx_LoadConfig() != 0\n");
    goto Error;
  }
  // Init state
  ;

  // Assign thread attributes from input param
  pMAP->ThreadAttr = *pAttr;

  // Create thread
  pMAP->ThreadState |= MAPTX_THREAD_STATE_INIT;
  Res = pthread_create(&pMAP->ThreadID,
                       &pMAP->ThreadAttr,
                       (void *) MAPTx_ThreadProc,
                       pMAP);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pMAP->ThreadState = MAPTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppMAP = pMAP;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppMAP = NULL;
  MAPTx_Close(pMAP);
Success:
  dbg_printf("Successfully started the MAPTx\n");
  return Res;
}

/**
 * @brief Stop execution of MAPTx, free the thread and its associated resources
 * @param pMAP MAPTx handle
 */
void MAPTx_Close (struct MAPTx *pMAP)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pMAP == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pMAP->ThreadState |= MAPTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pMAP->ThreadState & MAPTX_THREAD_STATE_INIT)
    pthread_join(pMAP->ThreadID, NULL);

  // Free up allocated config parameters
  pMAP->Params.Enabled = false;

  int ix;
  for (ix = 0; ix < pMAP->Params.NumLanes; ix++)
  {
    if (pMAP->Params.pLanes != NULL)
    {
      if (pMAP->Params.pLanes[ix].pNodes != NULL)
      {
        free(pMAP->Params.pLanes[ix].pNodes);
      }
      if (pMAP->Params.pLanes[ix].pConnections != NULL)
      {
        free(pMAP->Params.pLanes[ix].pConnections);
      }
    }
  }
  free(pMAP->Params.pLanes);

  // Success!
  Res = 0;

Error:
  free(pMAP);

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the MAPTx object
 * @param pMAP MAPTx handle
 */
void MAPTx_PrintStats (struct MAPTx *pMAP)
{
  fprintf(stdout, "MAPTx: Tx (Okay %8u Fail %8u)\n",
	  pMAP->Stats.Tx.Okay, pMAP->Stats.Tx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Read a latitude/longitude
 *
 * @param pLatitude  Where to store the retrieved latitude
 * @param pLongitude Where to store the retrieved longitude
 * @param pAltitude  Where to store the retrieved altitude
 * @param pSetting Section to interogate
 * @param pLatitudeName  Name of the latitude parameter
 * @param pLongitudeName Name of the latitude parameter
 * @param pAltitudeName  Name of the altitude parameter
 */
static void ReadLatLong(int32_t * pLatitude,
                        int32_t * pLongitude,
                        int32_t * pAltitude,
                        const config_setting_t *pSetting,
                        const char * pLatitudeName,
                        const char * pLongitudeName,
                        const char * pAltitudeName)
{
  BUILD_BUG_ON(sizeof(int) < sizeof(int32_t));
  d_assert(pLatitude);
  d_assert(pLongitude);
  d_assert(pAltitude);
  d_assert(pSetting);
  d_assert(pLatitudeName);
  d_assert(pLongitudeName);
  d_assert(pAltitudeName);

  d_fnstart(D_INTERN, NULL, "(%p,%p,%p,%p{%s},%s,%s,%s)\n",
            pLatitude, pLongitude, pAltitude,
            pSetting,
            SAFE(pSetting->name),
            SAFE(pLatitudeName), SAFE(pLongitudeName), SAFE(pAltitudeName));

  int ConfigVal = 0;
  // Latitude
  if (config_setting_lookup_int(pSetting,
                                pLatitudeName,
                                &ConfigVal))
  {
    *pLatitude = (int32_t) ConfigVal;
  }
  else
  {
    d_printf(D_WARN, NULL, "Latitude unavail, %p{%s}[%s]\n",
             pSetting, SAFE(pSetting->name), SAFE(pLatitudeName));
  }

  // EventLongitude
  if (config_setting_lookup_int(pSetting,
                                pLongitudeName,
                                &ConfigVal))
  {
    *pLongitude = (int32_t) ConfigVal;
  }
  else
  {
    d_printf(D_WARN, NULL, "Longitude unavail, %p{%s}[%s]\n",
             pSetting, SAFE(pSetting->name), SAFE(pLongitudeName));
  }

  // Altitude
  if (config_setting_lookup_int(pSetting,
                                pAltitudeName,
                                &ConfigVal))
  {
    *pAltitude = (int32_t) ConfigVal;
  }
  else
  {
    d_printf(D_WARN, NULL, "Altitude unavail, %p{%s}[%s]\n",
             pSetting, SAFE(pSetting->name), SAFE(pAltitudeName));
  }

  d_fnend(D_INTERN, NULL, "(%p,%p,%p,%p{%s},%s,%s,%s): (%"PRId32",%"PRId32",%"PRId32")\n",
          pLatitude, pLongitude, pAltitude,
          pSetting,
          SAFE(pSetting->name),
          SAFE(pLatitudeName), SAFE(pLongitudeName), SAFE(pAltitudeName),
          *pLatitude, *pLongitude,*pLatitude);
}

/**
 * @brief Load 'MAPTx' parameters from the config file
 * @param pMAP MAP transmitter handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int MAPTx_LoadConfig (struct MAPTx *pMAP,
                             char *pConfigFileName)
{
  int Res;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for MAP in config file
  int ConfigVal = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pMAP->Params.Enabled = false;
  pMAP->Params.ApplInterval = MAPTX_CONFIG_VALUE_DEFAULT_MAPAPPLINTERVAL;
  pMAP->Params.IntersectionId = MAPTX_CONFIG_VALUE_DEFAULT_MAPINTERSECTIONID;
  pMAP->Params.Latitude = MAPTX_CONFIG_VALUE_DEFAULT_MAPLATITUDE;
  pMAP->Params.Longitude = MAPTX_CONFIG_VALUE_DEFAULT_MAPLONGITUDE;
  pMAP->Params.NumLanes = 0;
  pMAP->Params.pLanes = NULL;

  // WSMP defaults
  {
    memset(pMAP->Params.WSMP.Tx.DA, 0xFF, 6);
    pMAP->Params.WSMP.Tx.Priority   = MAPTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY;
    pMAP->Params.WSMP.Tx.ExpiryTime = MAPTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME;
    pMAP->Params.WSMP.Version       = DOT3_WSMP_VERSION_3;
    pMAP->Params.WSMP.ChannelNumber = MAPTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER;
    const tdefault_stack *pIni = default_stack_Get();
    pMAP->Params.WSMP.ChannelNumber = pIni->ContinuousChanNum;
    default_stack_Release(&pIni);
    pMAP->Params.WSMP.DataRate      = MAPTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE;
    pMAP->Params.WSMP.TxPower       = MAPTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER;
    pMAP->Params.WSMP.PSID          = htonl(MAPTX_CONFIG_VALUE_DEFAULT_WSMP_PSID);
    pMAP->Params.WSMP.Length        = htons(0);
    pMAP->Params.WSMP.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
  }

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'MAPTx' Setting
  pSetting = config_lookup(pConfig, MAPTX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", MAPTX_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // WSMP
  {
    config_setting_t *pWSMP;
    pWSMP = config_setting_get_member(pSetting,
                                      MAPTX_CONFIG_VALUE_NAME_WSMP);
    if (pWSMP != NULL)
    {
      // "pWSMP %p\n", pWSMP);

      // Priority
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_PRIORITY,
                                    &ConfigVal))
        pMAP->Params.WSMP.Tx.Priority = (uint8_t) ConfigVal;
      // ExpiryTime
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_EXPIRY_TIME,
                                    &ConfigVal))
        pMAP->Params.WSMP.Tx.ExpiryTime = (uint8_t) ConfigVal;
      // ChannelNumber
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER,
                                    &ConfigVal))
        pMAP->Params.WSMP.ChannelNumber = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_DATA_RATE,
                                    &ConfigVal))
        pMAP->Params.WSMP.DataRate = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_TX_POWER,
                                    &ConfigVal))
        pMAP->Params.WSMP.TxPower = (uint8_t) ConfigVal;
      // PSID
      if (config_setting_lookup_int(pWSMP,
                                    MAPTX_CONFIG_VALUE_NAME_PSID,
                                    &ConfigVal))
        pMAP->Params.WSMP.PSID = htonl((uint32_t) ConfigVal);
    }
  }

  // MAP Enabled
  if (config_setting_lookup_bool(pSetting,
                                 MAPTX_CONFIG_VALUE_NAME_MAPENABLED,
                                 &ConfigVal))
    pMAP->Params.Enabled = (bool) ConfigVal;
  d_printf(D_DEBUG, 0, "MAP.Enabled = %d\n", pMAP->Params.Enabled);

  // Duration
  if (config_setting_lookup_int(pSetting,
                                MAPTX_CONFIG_VALUE_NAME_MAPAPPLINTERVAL,
                                &ConfigVal))
    pMAP->Params.ApplInterval = (uint16_t) ConfigVal;
  d_printf(D_DEBUG, 0, "MAP.ApplInterval = %d\n", pMAP->Params.ApplInterval);

  // Intersection ID
  if (config_setting_lookup_int(pSetting,
                                MAPTX_CONFIG_VALUE_NAME_MAPINTERSECTIONID,
                                &ConfigVal))
    pMAP->Params.IntersectionId = (uint32_t) ConfigVal;
  d_printf(D_DEBUG, 0, "MAP.IntersectionId = 0x%x\n", pMAP->Params.IntersectionId);

  // Latitude & Longitude
  int32_t Altitude_ignored;
  ReadLatLong(&pMAP->Params.Latitude, &pMAP->Params.Longitude,
              &Altitude_ignored,
              pSetting,
              MAPTX_CONFIG_VALUE_NAME_MAPLATITUDE,
              MAPTX_CONFIG_VALUE_NAME_MAPLONGITUDE,
              "Altitude_ignored");
  d_printf(D_DEBUG, 0, "MAP.Latitude = %d\n", pMAP->Params.Latitude);
  d_printf(D_DEBUG, 0, "MAP.Longitude = %d\n", pMAP->Params.Longitude);

  // Construct intersection lanes
  config_setting_t *pLanes;
  pLanes = config_setting_get_member(pSetting, MAPTX_CONFIG_VALUE_NAME_MAPLANES);
  if (pLanes != NULL)
  {
    int NumberOfLanes = config_setting_length(pLanes);
    pMAP->Params.NumLanes = NumberOfLanes;
    d_printf(D_DEBUG, 0, "MAP.NumLanes = %d\n", pMAP->Params.NumLanes);

    if (NumberOfLanes > 0)
    {
      pMAP->Params.pLanes = (tMAPLaneParams *)
        calloc(sizeof(tMAPLaneParams), NumberOfLanes);
      if (pMAP->Params.pLanes != NULL)
      {
        int i;
        for (i = 0; i < NumberOfLanes; i++)
        {
          config_setting_t *pLane = config_setting_get_elem(pLanes, i);
          pMAP->Params.pLanes[i].LaneId = MAPTX_CONFIG_VALUE_DEFAULT_MAPLANEID;
          pMAP->Params.pLanes[i].NumNodes = 0;
          pMAP->Params.pLanes[i].NumConnections = 0;
          pMAP->Params.pLanes[i].egressLane = false;
          pMAP->Params.pLanes[i].ingressLane = true; // as this what was previously set before egressLane/ingressLane config added
          pMAP->Params.pLanes[i].pNodes = NULL;
          pMAP->Params.pLanes[i].pConnections = NULL;

          if (pLane != NULL)
          {
            if (config_setting_lookup_int(pLane,
                                          MAPTX_CONFIG_VALUE_NAME_MAPLANEID,
                                          &ConfigVal))
              pMAP->Params.pLanes[i].LaneId =  (int) ConfigVal;
            d_printf(D_DEBUG, 0, "MAP.pLanes[%d].LaneId = %d\n",
                     i, pMAP->Params.pLanes[i].LaneId);

            if (config_setting_lookup_int(pLane,
                                          MAPTX_CONFIG_VALUE_NAME_MAPLANEDIRECTION,
                                          &ConfigVal))
            {
              if (((int) ConfigVal) & 0x01)
                pMAP->Params.pLanes[i].egressLane = true;
              else
                pMAP->Params.pLanes[i].egressLane = false;
              if (((int) ConfigVal) & 0x02)
                pMAP->Params.pLanes[i].ingressLane = true;
              else
                pMAP->Params.pLanes[i].ingressLane = false;
            }

            // Construct lane nodes:
            config_setting_t *pNodes;
            pNodes = config_setting_get_member(pLane, MAPTX_CONFIG_VALUE_NAME_MAPNODES);
            if (pNodes != NULL)
            {
              int NumberOfNodes = config_setting_length(pNodes);
              pMAP->Params.pLanes[i].NumNodes = NumberOfNodes;
              d_printf(D_DEBUG, 0, "MAP.pLanes[%d].NumNodes = %d\n",
                       i, pMAP->Params.pLanes[i].NumNodes);
              if (NumberOfNodes > 0)
              {
                pMAP->Params.pLanes[i].pNodes = (tMAPLaneNodeParams *)
                  calloc(sizeof(tMAPLaneNodeParams), NumberOfNodes);
                if (pMAP->Params.pLanes[i].pNodes != NULL)
                {
                  int j;
                  for (j = 0; j < NumberOfNodes; j++)
                  {
                    config_setting_t *pNode = config_setting_get_elem(pNodes, j);
                    pMAP->Params.pLanes[i].pNodes[j].X =
                      MAPTX_CONFIG_VALUE_DEFAULT_MAPNODEX;
                    pMAP->Params.pLanes[i].pNodes[j].Y =
                      MAPTX_CONFIG_VALUE_DEFAULT_MAPNODEY;

                    if (pNode != NULL)
                    {
                      if (config_setting_lookup_int(pNode,
                                                    MAPTX_CONFIG_VALUE_NAME_MAPNODEX,
                                                    &ConfigVal))
                        pMAP->Params.pLanes[i].pNodes[j].X =  (int) ConfigVal;
                      d_printf(D_DEBUG, 0, "MAP.pLanes[%d].pNodes[%d].X = %d\n",
                               i, j, pMAP->Params.pLanes[i].pNodes[j].X);

                      if (config_setting_lookup_int(pNode,
                                                    MAPTX_CONFIG_VALUE_NAME_MAPNODEY,
                                                    &ConfigVal))
                        pMAP->Params.pLanes[i].pNodes[j].Y =  (int) ConfigVal;
                      d_printf(D_DEBUG, 0, "MAP.pLanes[%d].pNodes[%d].Y = %d\n",
                               i, j, pMAP->Params.pLanes[i].pNodes[j].Y);
                    }
                  }
                }
              }
            }

            // Construct lane connections:
            config_setting_t *pConnections;
            pConnections = config_setting_get_member(pLane, MAPTX_CONFIG_VALUE_NAME_MAPCONNECTIONS);
            if (pConnections != NULL)
            {
              int NumberOfConnections = config_setting_length(pConnections);
              pMAP->Params.pLanes[i].NumConnections = NumberOfConnections;
              d_printf(D_DEBUG, 0, "MAP.pLanes[%d].NumConnections = %d\n",
                       i, pMAP->Params.pLanes[i].NumConnections);
              if (NumberOfConnections > 0)
              {
                pMAP->Params.pLanes[i].pConnections =(tMAPLaneConnectionParams *)
                  calloc(sizeof(tMAPLaneConnectionParams), NumberOfConnections);
                if (pMAP->Params.pLanes[i].pConnections != NULL)
                {
                  int j;
                  for (j = 0; j < NumberOfConnections; j++)
                  {
                    config_setting_t *pConnection = config_setting_get_elem(pConnections, j);
                    pMAP->Params.pLanes[i].pConnections[j].SignalGroup =
                      MAPTX_CONFIG_VALUE_DEFAULT_MAPSIGNALGROUP;
                    pMAP->Params.pLanes[i].pConnections[j].ConnectsTo =
                      MAPTX_CONFIG_VALUE_DEFAULT_MAPCONNECTSTO;
                    pMAP->Params.pLanes[i].pConnections[j].Maneuver =
                      MAPTX_CONFIG_VALUE_DEFAULT_MAPMANEUVER;

                    if (pConnection != NULL)
                    {
                      if (config_setting_lookup_int(pConnection,
                                                    MAPTX_CONFIG_VALUE_NAME_MAPSIGNALGROUP,
                                                    &ConfigVal))
                        pMAP->Params.pLanes[i].pConnections[j].SignalGroup =  (uint8_t) ConfigVal;
                      d_printf(D_DEBUG, 0, "MAP.pLanes[%d].pConnections[%d].SignalGroup = %d\n",
                               i, j, pMAP->Params.pLanes[i].pConnections[j].SignalGroup);

                      if (config_setting_lookup_int(pConnection,
                                                    MAPTX_CONFIG_VALUE_NAME_MAPCONNECTSTO,
                                                    &ConfigVal))
                        pMAP->Params.pLanes[i].pConnections[j].ConnectsTo =  (int) ConfigVal;
                      d_printf(D_DEBUG, 0, "MAP.pLanes[%d].pConnections[%d].ConnectsTo = %d\n",
                               i, j, pMAP->Params.pLanes[i].pConnections[j].ConnectsTo);

                      if (config_setting_lookup_int(pConnection,
                                                    MAPTX_CONFIG_VALUE_NAME_MAPMANEUVER,
                                                    &ConfigVal))
                        pMAP->Params.pLanes[i].pConnections[j].Maneuver =  (uint8_t) ConfigVal;
                      d_printf(D_DEBUG, 0, "MAP.pLanes[%d].pConnections[%d].Maneuver = %d\n",
                               i, j, pMAP->Params.pLanes[i].pConnections[j].Maneuver);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  return Res;
}

/**
 * @brief Create MAP message
 * @param pMAP MAP Tx module
 * @param[out] ppMsg Created MAP Message
 * @param pCfg MAP configuration
 * @return 0 for success, negative for errors
 */
static int MAPTx_CreateMAP (struct MAPTx *pMAP,
                            struct SAEMapData **ppMsg,
                            struct MAPCfg *pCfg)
{
  int Res;

  if ((pMAP == NULL) || (ppMsg == NULL) || (pCfg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735MAP_Create(ppMsg, pCfg, J2735_CREATE_ALL);

Error:
  return Res;
}

/**
 * @brief Populate MAP message
 * @param pMAP MAP Tx module
 * @param[out] ppMsg MAP Message to be populated
 * @param pCfg MAP configuration
 * @return 0 for success, negative for errors
 */
static int MAPTx_PopulateMAP (struct MAPTx *pMAP,
                              struct SAEMapData *pMapData,
                              struct MAPCfg *pCfg)
{
  int Res;

  (void)pCfg;

  if (pMapData)
  {
    int i, j;
    SAEIntersectionGeometry *pIntersection;
    SAEGenericLane *pLane;
    SAENodeXY *pNode;
    SAEConnection *pConnection;

    pMapData->msgIssueRevision = 0;

    pMapData->intersections_option = true;
    // Yes, count is hardcoded as we assume MAP contains only one intersection
    pMapData->intersections.count = 1;
    pMapData->intersections.tab = (SAEIntersectionGeometry *)
      asn1_mallocz(pMapData->intersections.count * sizeof(SAEIntersectionGeometry));
    d_assert(pMapData->intersections.tab);
    pIntersection = &pMapData->intersections.tab[0];

    pIntersection->id.id = pMAP->Params.IntersectionId;
    // change of revision is required to send same intersection with new data
    pIntersection->revision = 0;
    pIntersection->refPoint.lat = pMAP->Params.Latitude;
    pIntersection->refPoint.Long = pMAP->Params.Longitude;

    // Long offset fix for testing purposes
    const tdefault_stack * pIni = default_stack_Get();
    int LongOffsetAdj = (int)(pIni->Cohda_ReplayLongOffset * 10000000);
    default_stack_Release(&pIni);
    if (LongOffsetAdj)
    {
      pIntersection->refPoint.Long += LongOffsetAdj;

      // handle LARGE offsets
      if (pIntersection->refPoint.Long > 1800000000)
      {
        pIntersection->refPoint.Long -= 3600000000UL;
      }
      else if (pIntersection->refPoint.Long < -1800000000)
      {
        pIntersection->refPoint.Long += 3600000000UL;
      }

      d_printf(D_DEBUG, 0, "MAP Longitude refPoint modified to %d (offset %d)\n",
               pIntersection->refPoint.Long,
               LongOffsetAdj);
    }

    pIntersection->laneSet.count = pMAP->Params.NumLanes;
    pIntersection->laneSet.tab = (SAEGenericLane *)
      asn1_mallocz(pIntersection->laneSet.count * sizeof(SAEGenericLane));
    d_assert(pIntersection->laneSet.tab);

    for (i=0; i < pMAP->Params.NumLanes; i++)
    {
      pLane = &pIntersection->laneSet.tab[i];
      pLane->laneID = pMAP->Params.pLanes[i].LaneId;

      // Hardcoded direction and lane types. Other directions and types are not
      // considered by spat/map framework
      pLane->laneAttributes.laneType.choice = SAELaneTypeAttributes_vehicle;
      J2735_SetBitString(&pLane->laneAttributes.laneType.u.vehicle, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      J2735_SetBitString(&pLane->laneAttributes.directionalUse, 2, pMAP->Params.pLanes[i].ingressLane, pMAP->Params.pLanes[i].egressLane);
      J2735_SetBitString(&pLane->laneAttributes.sharedWith, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

      pLane->nodeList.choice = SAENodeListXY_nodes;
      pLane->nodeList.u.nodes.count = pMAP->Params.pLanes[i].NumNodes;
      pLane->nodeList.u.nodes.tab = (SAENodeXY*)
        asn1_mallocz(pLane->nodeList.u.nodes.count * sizeof(SAENodeXY));
      d_assert(pLane->nodeList.u.nodes.tab);

      for (j=0; j < pMAP->Params.pLanes[i].NumNodes; j++)
      {
        pNode = &pLane->nodeList.u.nodes.tab[j];
        pNode->delta.choice = SAENodeOffsetPointXY_node_XY6;
        pNode->delta.u.node_XY6.x = pMAP->Params.pLanes[i].pNodes[j].X;
        pNode->delta.u.node_XY6.y = pMAP->Params.pLanes[i].pNodes[j].Y;
      }

      if (pMAP->Params.pLanes[i].NumConnections)
      {
        pLane->connectsTo_option = true;
        pLane->connectsTo.count = pMAP->Params.pLanes[i].NumConnections;
        pLane->connectsTo.tab = (SAEConnection*)
          asn1_mallocz(pLane->connectsTo.count * sizeof(SAEConnection));
        d_assert(pLane->connectsTo.tab);

        for (j=0; j < pMAP->Params.pLanes[i].NumConnections; j++)
        {
          pConnection = &pLane->connectsTo.tab[j];
          pConnection->signalGroup_option = true;
          pConnection->signalGroup = pMAP->Params.pLanes[i].pConnections[j].SignalGroup;
          pConnection->connectingLane.lane = pMAP->Params.pLanes[i].pConnections[j].ConnectsTo;

          if(pMAP->Params.pLanes[i].pConnections[j].Maneuver == 1)
          {
            // Straight maneuver: first bit of first byte only is set
            pConnection->connectingLane.maneuver_option = true;
            J2735_SetBitString(&pConnection->connectingLane.maneuver, 12, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
          }
          else if(pMAP->Params.pLanes[i].pConnections[j].Maneuver == 2)
          {
            // Left maneuver: second bit of first byte only is set
            pConnection->connectingLane.maneuver_option = true;
            J2735_SetBitString(&pConnection->connectingLane.maneuver, 12, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
          }
          else if(pMAP->Params.pLanes[i].pConnections[j].Maneuver == 3)
          {
            // Right maneuver: third bit of first byte only is set
            pConnection->connectingLane.maneuver_option = true;
            J2735_SetBitString(&pConnection->connectingLane.maneuver, 12, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
          }
          else
          {
            // No maneuver
            pConnection->connectingLane.maneuver_option = false;
            J2735_SetBitString(&pConnection->connectingLane.maneuver, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
          }
        }
      }
    }
    Res = 0;
  }
  else
  {
    Res = -EINVAL;
    goto Error;
  }


Error:
  return Res;
}

/**
 * @brief Encode MAP message
 * @param pMAP
 * @param pMsg Message to be encoded
 * @param[out] pBuf Buffer that contains encoded message
 * @param BufSize Buffer size
 * @return Size of encoded message, negative for errors
 */
static int MAPTx_EncodeMAP (struct MAPTx *pMAP,
                            struct SAEMapData *pMsg,
                            uint8_t *pBuf,
                            int BufSize)
{
  int Res;
  int RVal;

  (void)pMAP;

  // Encode the MAP to a buffer
  RVal = J2735MAP_Encode(pMsg, pBuf, BufSize);
  if (RVal == -1)
  {
    // "Failed to encode MAP at %s.",
    // RVal.failed_type ? RVal.failed_type->name : "unknown");
    Res = -EAGAIN;
  }
  else
  {
    // Encoding finished successfully
    // "Encoded MAP has %d bytes.", RVal);
    Res = RVal;
  }

  return Res;
}

/**
 * @brief Populate WSM header
 * @param pMAP
 * @param[out] pHdr WSMP header to be popualted
 * @param Len Size of WSM paylaod
 */
void MAPTx_PopulateWSMHeader (struct MAPTx *pMAP,
                              struct Dot3WSMPHdr *pHdr,
                              int Len)
{
  memcpy(pHdr, &(pMAP->Params.WSMP), sizeof(struct Dot3WSMPHdr));
  pHdr->Length = htons(Len);

  // "WSM Header Tx to MAC %02x:%02x:%02x:%02x:%02x:%02x: "
  // "Priority %d ChanNbr %d DataRate %d "
  // "TxPwr %d PSID %08x Load %08x)\n",
  // pHdr->Tx.DA[0],
  // pHdr->Tx.DA[1],
  // pHdr->Tx.DA[2],
  // pHdr->Tx.DA[3],
  // pHdr->Tx.DA[4],
  // pHdr->Tx.DA[5],
  // pHdr->Tx.Priority,
  // pHdr->ChannelNumber,
  // pHdr->DataRate,
  // pHdr->TxPower,
  // ntohl(pHdr->PSID),
  // ntohl(pHdr->ChannelLoad));

}

/**
 * @brief Destroy a MAP Message
 * @param pMAP
 * @param pMsg MAP message
 * @return 0 for success. Negative for errors.
 */
static int MAPTx_DestroyMAP (struct MAPTx *pMAP,
                             struct SAEMapData *pMsg)
{
  int Res;

  if ((pMAP == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735MAP_Free(pMsg, J2735_FREE_ALL);

Error:
  return Res;
}

/**
 * @brief Transmit MAP messages
 * @param pMAP MAP Tx module
 * @return 0 for success. Negative for errors.
 */
static int MAPTx_TransmitMAP (struct MAPTx *pMAP)
{
  int Res;
  struct SAEMapData *pASN = NULL;
  uint8_t *pWSM = NULL;
  int WSMSize;

  // Create the ASN.1 structure
  Res = MAPTx_CreateMAP(pMAP, &pASN, &(pMAP->Params.Cfg));
  if (Res != 0)
  {
    goto Error;
  }

  // Fill in the ASN.1 structure
  Res = MAPTx_PopulateMAP(pMAP, pASN, &(pMAP->Params.Cfg));
  if (Res != 0)
  {
    goto Error;
  }

  // Allocate the WSM buffer
  WSMSize = sizeof(struct Dot3WSMPHdr) + MAPTX_WSM_BUF_SIZE; // TODO: is the BUF_SIZE sufficient?
  pWSM = calloc(1, WSMSize);
  if (pWSM == NULL)
  {
    // "Failed to allocate Tx WSM buffer for encoding.");
    Res = -ENOMEM;
    goto Error;
  }

  // Encode the ASN.1 structure into the WSM buffer
  {
    uint8_t *pUPER = pWSM + sizeof(struct Dot3WSMPHdr);
    int UPERSize = WSMSize - sizeof(struct Dot3WSMPHdr);

    Res = MAPTx_EncodeMAP(pMAP, pASN, pUPER, UPERSize);
    if (Res < 0)
    {
      goto Error;
    }
    // A positive return value means that the encoding worked
    WSMSize = Res;
  }

  // Add the WSM Header to the UPER encoded buffer
  MAPTx_PopulateWSMHeader(pMAP, (struct Dot3WSMPHdr *) pWSM, WSMSize);

  // Transmit the WSM.
  // Enable signing (might be fake sig if security is disabled)
  // Also disable Tx logging.

  pthread_mutex_lock(&P1609Tx_SendWSM_Mutex);
  Res = P1609Tx_SendWSM((struct Dot3WSMPHdr *) pWSM, true, false, -1);
  pthread_mutex_unlock(&P1609Tx_SendWSM_Mutex);
  if (Res != 0)
  {
    // "P1609Tx_SendWSM failed");
    goto Error;
  }
  else
  {
    // "Sent MAP (%d bytes)", WSMSize);
  }

Error:
  if (pWSM)
    free(pWSM);
  if (pASN)
    (void) MAPTx_DestroyMAP(pMAP, pASN);
  return Res;
}

/**
 * @brief MAPTx Periodic thread processing - sends a MAP
 * @param pArg Pointer to MAPTx object passed as generic input parameter
 *
 */
static void MAPTx_ThreadProc (void *pArg)
{
  struct MAPTx *pMAP;
  struct timespec Time;

  pMAP = (struct MAPTx *) pArg;

  // "Starting MAPTx Periodic Thread");
  pMAP->ThreadState |= MAPTX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while ((pMAP->ThreadState & MAPTX_THREAD_STATE_STOP) == 0)
  {
    // polling delay
    Util_Nap(pMAP->Params.ApplInterval, &Time);

    if (pMAP->Params.Enabled)
    {
      int Res = MAPTx_TransmitMAP(pMAP); // Transmit a MAP
      if (Res == 0)
        pMAP->Stats.Tx.Okay++;
      else
        pMAP->Stats.Tx.Fail++;
    }
  }

  // "Leaving Periodic MAPTx Thread");

  // exit thread
  (void) pthread_exit(NULL);

}

// Close the doxygen group
/**
 * @}
 */

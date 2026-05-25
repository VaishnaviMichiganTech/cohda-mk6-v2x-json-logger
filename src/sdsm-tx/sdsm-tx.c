/**
 * @addtogroup mod_sdsm_tx Sensor Data Sharing Tx Module
 * @{
 *
 * @file
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "Example1609_defn.h"
#include "sdsm-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "asn1defs.h"
#include "debug-levels.h"
#include "dot3-wsmp.h"
#include "j2735-common.h"
#include "j3224-sdsm.h"
#include "j2735asn.h"
#include "p1609-tx-msg.h"
#include "udp.h"
#include "util.h"
#include "Vector.h"

#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_SPATTx_DebugLevel

#define dbg_printf(f, a...)                                       \
    do {                                                          \
        fprintf(stdout, "%s(%d): " f, __func__, __LINE__, ## a);  \
    } while (0)                                                   \

/// Maximum size of the WSM
// TODO: is this sufficient?
#define SDSMTX_WSM_BUF_SIZE      (1386)

/// Limit some parameters
#define LIMIT(var, un, max, min)                \
  do {                                          \
    if ((var) != (un)) {                        \
      if ((var) > (max)) {                      \
        (var) = (max);                          \
      } else if ((var) <= (min)) {              \
        (var) = (min);                          \
      }                                         \
    }                                           \
  } while(0)

#define MAX_OBJECTS 20

#define EARTH_CIRCUMFERENCE_POLES_m   40008000
#define EARTH_CIRCUMFERENCE_EQUATOR_m 40075160

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int SDSMTx_LoadConfig (struct SDSMTx *pSDSM,
                              char * pConfigFileName);

static void SDSMTx_ThreadProc (void *pArg);

static void SDSMTx_ObjectRxThread (void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

extern pthread_mutex_t P1609Tx_SendWSM_Mutex;

// LPH handle
void *pLPH = NULL;

// FD for object data socket
int ObjectFd = -1;

//int EpollFd = -1;

// Object list
tSAEDetectedObjectDataSnapshot *pObjects = NULL;

// Number of objects in object list
int NumObjects = 0;

// Object list mutex
pthread_mutex_t ObjectMutex;

struct timeval Object_tv;

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------

/**
 * @brief Allocate SDSMTx object, Setup thread and associated data structures
 * @param ppSDSM pointer to SDSMTx handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int SDSMTx_Open (struct SDSMTx **ppSDSM,
                pthread_attr_t *pAttr,
                char *pConfigFileName)
{
  d_fnstart(D_API, NULL, "(%p, %p, %s)\n",
            ppSDSM, pAttr, pConfigFileName);

  (void)pConfigFileName;

  int Res;
  struct SDSMTx *pSDSM = NULL;

  pthread_mutex_init(&ObjectMutex, NULL);

  const tExample1609_Example1609 * pExample = Example1609_Get();

  // Open socket for receiving object data
  if (pExample->SDSM_Object_Port != 0)
  {
    ObjectFd = UDP_OpenSrvPort(pExample->SDSM_Object_Port);
    if (ObjectFd < 0)
    {
      d_error(D_ERR, NULL, "Failed to create object Rx socket: %d\n", ObjectFd);
      Res = -ENODEV;
      goto Error;
    }
  }

#if 0
  /// Set up epoll
  EpollFd = Res = epoll_create(1);
  if (Res < 0)
  {
    d_error(D_ERR, NULL, "epoll_create failed: %s\n", strerror(errno));
    Res = -ENODEV;
    goto Error;
  }

  struct epoll_event Event;
  Event.events = EPOLLIN;
  Event.data.fd = ObjectFd;

  Res = epoll_ctl(EpollFd, EPOLL_CTL_ADD, ObjectFd, &Event);
  if (Res < 0)
  {
    d_printf(D_ERR, NULL, "Error - epoll_ctl(%d): %s\n",
             ObjectFd, strerror(errno));
    Res = -ENODEV;
    goto Error;
  }
#endif

  // LPH reader
  pLPH = LPH_Open();
  if (pLPH == NULL)
  {
    dbg_printf("LPH_Open() == NULL\n");
    Res = -ENODEV;
    goto Error;
  }

  // Create and check the local structure
  pSDSM = (struct SDSMTx *) malloc(sizeof(struct SDSMTx));
  if (pSDSM == NULL)
  {
    dbg_printf("malloc() == NULL\n");
    Res = -ENOMEM;
    goto Error;
  }
  memset(pSDSM, 0, sizeof(struct SDSMTx));

  // Load config settings
  Res = SDSMTx_LoadConfig(pSDSM, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("SDSMTx_LoadConfig() != 0\n");
    goto Error;
  }

  // Assign thread attributes from input param
  pSDSM->ThreadAttr = *pAttr;

  // Create thread
  pSDSM->ThreadState |= SDSMTX_THREAD_STATE_INIT;
  Res = pthread_create(&pSDSM->ThreadID,
                       &pSDSM->ThreadAttr,
                       (void *) SDSMTx_ThreadProc,
                       pSDSM);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pSDSM->ThreadState = SDSMTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Create thread
  pSDSM->ObjectRx_ThreadState |= SDSMTX_THREAD_STATE_INIT;
  Res = pthread_create(&pSDSM->ObjectRx_ThreadID,
                       &pSDSM->ThreadAttr,
                       (void *) SDSMTx_ObjectRxThread,
                       pSDSM);
  if (Res != 0)
  {
    dbg_printf("pthread_create(0 != 0\n");
    // Thread creation error
    pSDSM->ObjectRx_ThreadState = SDSMTX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppSDSM = pSDSM;
  Res = 0;
  goto Success;

Error:
  *ppSDSM = NULL;
  SDSMTx_Close(pSDSM);
  Example1609_Release(&pExample);

Success:
  dbg_printf("Successfully started the SDSMTx\n");
  d_fnend(D_API, NULL, "(%p, %p, %s): %d\n",
          ppSDSM, pAttr, pConfigFileName, Res);
  return Res;
}

/**
 * @brief Stop execution of SDSMTx, free the thread and its associated resources
 * @param pSDSM SDSMTx handle
 */
void SDSMTx_Close (struct SDSMTx *pSDSM)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pSDSM == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  if (pLPH)
  {
    LPH_Close(pLPH);
    pLPH = NULL;
  }

  // Signal thread to terminate
  pSDSM->ThreadState |= SDSMTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pSDSM->ThreadState & SDSMTX_THREAD_STATE_INIT)
  {
    pthread_join(pSDSM->ThreadID, NULL);
  }

  // Signal thread to terminate
  pSDSM->ObjectRx_ThreadState |= SDSMTX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pSDSM->ObjectRx_ThreadState & SDSMTX_THREAD_STATE_INIT)
  {
    pthread_join(pSDSM->ObjectRx_ThreadID, NULL);
  }

  if (ObjectFd >= 0)
  {
    close(ObjectFd);
  }

#if 0
  if (EpollFd >= 0)
  {
    close(EpollFd);
  }
#endif

  // Free up allocated config parameters
  pSDSM->Params.Enabled = false;

  // Free object data
  if (pSDSM->Params.pObjects)
  {
    free(pSDSM->Params.pObjects);
  }

  // Success!
  Res = 0;

Error:
  free(pSDSM);

  (void)Res;   // value not currently used
  return;
}


/**
 * @brief Prints the statistics from the SDSMTx object
 * @param pSDSM SDSMTx handle
 */
void SDSMTx_PrintStats (struct SDSMTx *pSDSM)
{
  fprintf(stdout, "SDSMTx: Tx (Okay %8u Fail %8u)\n",
	  pSDSM->Stats.Tx.Okay, pSDSM->Stats.Tx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load 'SDSMTx' parameters from the config file
 * @param pSDSM SDSM transmitter handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int SDSMTx_LoadConfig (struct SDSMTx *pSDSM,
                             char *pConfigFileName)
{
  int Res = -ENOSYS;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for SDSM in config file
  int ConfigVal = 0;
  int NumObjs = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pSDSM->Params.Enabled = false;
  pSDSM->Params.Interval_ms = SDSMTX_CONFIG_VALUE_DEFAULT_SDSMINTERVAL_MS;
  pSDSM->Params.ObjectMaxAge_ms = SDSMTX_CONFIG_VALUE_DEFAULT_OBJECTMAXAGE_MS;
  pSDSM->Params.DataSource = SDSMTX_CONFIG_VALUE_DEFAULT_SDSMDATASOURCE;

  // WSMP defaults
  {
    memset(pSDSM->Params.WSMP.Tx.DA, 0xFF, 6);
    pSDSM->Params.WSMP.Tx.Priority   = SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY;
    pSDSM->Params.WSMP.Tx.ExpiryTime = SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME;
    pSDSM->Params.WSMP.Version       = DOT3_WSMP_VERSION_3;
    pSDSM->Params.WSMP.ChannelNumber = SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER;
    const tdefault_stack *pIni = default_stack_Get();
    pSDSM->Params.WSMP.ChannelNumber = pIni->ContinuousChanNum;
    default_stack_Release(&pIni);
    pSDSM->Params.WSMP.DataRate      = SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE;
    pSDSM->Params.WSMP.TxPower       = SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER;
    pSDSM->Params.WSMP.PSID          = htonl(SDSMTX_CONFIG_VALUE_DEFAULT_WSMP_PSID);
    pSDSM->Params.WSMP.Length        = htons(0);
    pSDSM->Params.WSMP.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
  }

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'SDSMTx' Setting
  pSetting = config_lookup(pConfig, SDSMTX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", SDSMTX_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // WSMP
  {
    config_setting_t *pWSMP;
    pWSMP = config_setting_get_member(pSetting,
                                      SDSMTX_CONFIG_VALUE_NAME_WSMP);
    if (pWSMP != NULL)
    {
      // "pWSMP %p\n", pWSMP);

      // Priority
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_PRIORITY,
                                    &ConfigVal))
        pSDSM->Params.WSMP.Tx.Priority = (uint8_t) ConfigVal;
      // ExpiryTime
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_EXPIRY_TIME,
                                    &ConfigVal))
        pSDSM->Params.WSMP.Tx.ExpiryTime = (uint8_t) ConfigVal;
      // ChannelNumber
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_CHANNEL_NUMBER,
                                    &ConfigVal))
        pSDSM->Params.WSMP.ChannelNumber = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_DATA_RATE,
                                    &ConfigVal))
        pSDSM->Params.WSMP.DataRate = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_TX_POWER,
                                    &ConfigVal))
        pSDSM->Params.WSMP.TxPower = (uint8_t) ConfigVal;
      // PSID
      if (config_setting_lookup_int(pWSMP,
                                    SDSMTX_CONFIG_VALUE_NAME_PSID,
                                    &ConfigVal))
        pSDSM->Params.WSMP.PSID = htonl((uint32_t) ConfigVal);
    }
  }

  // SDSM Enabled
  if (config_setting_lookup_bool(pSetting,
                                 SDSMTX_CONFIG_VALUE_NAME_SDSMENABLED,
                                 &ConfigVal))
  {
    pSDSM->Params.Enabled = (bool) ConfigVal;
  }
  d_printf(D_DEBUG, 0, "SDSM.Enabled = %d\n", pSDSM->Params.Enabled);

  // SDSM object data source
  if (config_setting_lookup_int(pSetting,
                                SDSMTX_CONFIG_VALUE_NAME_SDSMDATASOURCE,
                                &ConfigVal))
  {
    pSDSM->Params.DataSource = ConfigVal;
  }
  else
  {
    d_printf(D_WARN, NULL, "Did not find 'DataSource' config\n");
  }
  d_printf(D_DEBUG, 0, "SDSM.DataSource = %d\n", pSDSM->Params.DataSource);

  // Duration
  if (config_setting_lookup_int(pSetting,
                                SDSMTX_CONFIG_VALUE_NAME_SDSMINTERVAL_MS,
                                &ConfigVal))
  {
    pSDSM->Params.Interval_ms = (uint16_t) ConfigVal;
  }
  d_printf(D_DEBUG, 0, "SDSM.Interval_ms = %d\n", pSDSM->Params.Interval_ms);

  // Object max age
  if (config_setting_lookup_int(pSetting,
                                SDSMTX_CONFIG_VALUE_NAME_OBJECTMAXAGE_MS,
                                &ConfigVal))
  {
    pSDSM->Params.ObjectMaxAge_ms = (uint16_t) ConfigVal;
  }
  d_printf(D_DEBUG, 0, "SDSM.ObjectMaxAge_ms = %d\n", pSDSM->Params.ObjectMaxAge_ms);


  // SDSM Objects
  if (pSDSM->Params.DataSource == SDSMTX_DATA_SOURCE_CFG)
  {
    config_setting_t *pObjs;
    pObjs = config_setting_get_member(pSetting,
                                      SDSMTX_CONFIG_VALUE_NAME_OBJECTS);
    if (!pObjs)
    {
      dbg_printf("config_lookup(%s) failed\n", SDSMTX_CONFIG_VALUE_NAME_OBJECTS);
      Res = -EINVAL;
      goto Error;
    }
    NumObjs = pSDSM->Params.NumObjects = config_setting_length(pObjs);
    d_printf(D_DEBUG, NULL, "SDSM.NumObjects = %d\n", NumObjs);

    if (NumObjs <= 0)
    {
      pSDSM->Params.NumObjects = 0;
      d_error(D_ERR, NULL, "%d objects\n", NumObjs);
      Res = -EINVAL;
      goto Error;
    }

    pSDSM->Params.pObjects = (tSAEDetectedObjectDataSnapshot *)
      calloc(sizeof(tSAEDetectedObjectDataSnapshot), NumObjs);
    if (!pSDSM->Params.pObjects)
    {
      pSDSM->Params.NumObjects = 0;
      d_error(D_ERR, NULL, "Unable to allocate %d objects\n",
              NumObjs);
      Res = -EINVAL;
      goto Error;
    }

    int i;
    for (i = 0; i < NumObjs; ++i)
    {
      tSAEDetectedObjectDataSnapshot *pObject = &(pSDSM->Params.pObjects[i]);
      // initialise some defaults
      pObject->Object.detObjCommon.timeConfidence = SAETimeConfidence_unavailable;
      pObject->Object.detObjCommon.measurementTime = 123; // FIXME
      pObject->Object.detObjCommon.pos.offsetZ_option = true;
      pObject->Object.detObjCommon.posConfidence.pos = SAEPositionConfidence_unavailable;
      pObject->Object.detObjCommon.posConfidence.elevation = SAEElevationConfidence_unavailable;
      pObject->Object.detObjCommon.speedConfidence = SAESpeedConfidence_unavailable;
      pObject->Object.detObjCommon.speedZ_option = false;
      pObject->Object.detObjCommon.speedConfidenceZ_option = false;
      pObject->Object.detObjCommon.accel4way_option = false;
      pObject->Object.detObjCommon.accCfdX_option = false;
      pObject->Object.detObjCommon.accCfdY_option = false;
      pObject->Object.detObjCommon.accCfdZ_option = false;
      pObject->Object.detObjCommon.accCfdYaw_option = false;
      pObject->Object.detObjOptData_option = false;

      // Get object from configuration
      config_setting_t *pConf = config_setting_get_elem(pObjs, i);
      if (!pConf)
      {
        d_printf(D_WARN, NULL, "Unable to get object %d\n", i);
        continue;
      }

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_ID,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.objectID = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].Id = %d\n",
               i, pObject->Object.detObjCommon.objectID);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_TYPE,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.objType = (uint8_t) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].objType = %d\n",
               i, pObject->Object.detObjCommon.objType);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_TYPECONF,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.objTypeCfd = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].objTypeCfd = %d\n",
               i, pObject->Object.detObjCommon.objTypeCfd);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_OFFSETX,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.pos.offsetX = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].pos.offsetX = %d\n",
               i, pObject->Object.detObjCommon.pos.offsetX);


      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_OFFSETY,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.pos.offsetY = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].pos.offsetY = %d\n",
               i, pObject->Object.detObjCommon.pos.offsetY);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_OFFSETZ,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.pos.offsetZ = (int) ConfigVal;
        pObject->Object.detObjCommon.pos.offsetZ_option = true;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].pos.offsetZ = %d\n",
               i, pObject->Object.detObjCommon.pos.offsetZ);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_SPEED,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.speed = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].speed = %d\n",
               i, pObject->Object.detObjCommon.speed);

      if (config_setting_lookup_int(pConf,
                                    SDSMTX_CONFIG_VALUE_NAME_HEADING,
                                    &ConfigVal))
      {
        pObject->Object.detObjCommon.heading = (int) ConfigVal;
      }
      d_printf(D_DEBUG, 0, "SDSM.pObjects[%d].heading = %d\n",
               i, pObject->Object.detObjCommon.heading);
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
 * @brief Destroy a SDSM Message
 * @param pSDSM
 * @param pMsg SDSM message
 * @return 0 for success. Negative for errors.
 */
static int SDSMTx_DestroySDSM (struct SDSMTx *pSDSM,
                               struct SAESensorDataSharingMessage *pMsg)
{
  int Res;

  if ((pSDSM == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J3224SDSM_Free(pMsg);

Error:
  return Res;
}


/**
 * @brief Create SDSM message
 * @param pSDSM SDSM Tx module
 * @param[out] ppMsg Created SDSM Message
 * @return 0 for success, negative for errors
 */
static int SDSMTx_CreateSDSM (struct SDSMTx *pSDSM,
                              struct SAESensorDataSharingMessage **ppMsg)
{
  int Res;

  if ((pSDSM == NULL) || (ppMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J3224SDSM_Create(ppMsg);

Error:
  return Res;
}


/**
 * @brief Encode SDSM message
 * @param pSDSM
 * @param pMsg Message to be encoded
 * @param[out] pBuf Buffer that contains encoded message
 * @param BufSize Buffer size
 * @return Size of encoded message, negative for errors
 */
static int SDSMTx_EncodeSDSM (struct SDSMTx *pSDSM,
                              struct SAESensorDataSharingMessage *pMsg,
                              uint8_t *pBuf,
                              int BufSize)
{
  int Res;
  int RVal;

  (void)pSDSM;

  // Encode the SDSM to a buffer
  RVal = J3224SDSM_Encode(pMsg, pBuf, BufSize);
  if (RVal == -1)
  {
    // "Failed to encode SDSM at %s.",
    // RVal.failed_type ? RVal.failed_type->name : "unknown");
    Res = -EAGAIN;
  }
  else
  {
    // Encoding finished successfully
    // "Encoded SDSM has %d bytes.", RVal);
    Res = RVal;
  }

  return Res;
}


/**
 * FIXME: copied from src/fac-1609/p1609-tx/p1609-tx-msg.c
 */
static void PopulatePositionalAccuracy(SAEPositionalAccuracy * pSAEPositionalAccuracy,
                                       const tLPHPos * pLPHPos)
{
  d_assert(pSAEPositionalAccuracy);
  d_assert(pLPHPos);

  // halve 95% confidence error to get 68% confidence (one std dev)
  const float SemiMajorAxis_m = pLPHPos->PositionalAccuracy.SemiMajorAxis_m / 2;
  const float SemiMinorAxis_m = pLPHPos->PositionalAccuracy.SemiMinorAxis_m / 2;
  const float SemiMajorAxisOrientation_deg =
    pLPHPos->PositionalAccuracy.SemiMajorAxisOrientation_deg;

  if (isnan(SemiMajorAxis_m))
  {
    pSAEPositionalAccuracy->semiMajor = SAESemiMajorAxisAccuracy_Limit_Max;
  }
  else if (SemiMajorAxis_m >= 12.7f)
  {
    pSAEPositionalAccuracy->semiMajor = SAESemiMajorAxisAccuracy_Limit_Max - 1;
  }
  else
  {
    pSAEPositionalAccuracy->semiMajor = rintf(SemiMajorAxis_m / 0.05f);
  }

  if (isnan(SemiMinorAxis_m))
  {
    pSAEPositionalAccuracy->semiMinor = SAESemiMinorAxisAccuracy_Limit_Max;
  }
  else if (SemiMinorAxis_m >= 12.7f)
  {
    pSAEPositionalAccuracy->semiMinor = SAESemiMinorAxisAccuracy_Limit_Max - 1;
  }
  else
  {
    pSAEPositionalAccuracy->semiMinor = rintf(SemiMinorAxis_m / 0.05f);
  }

  if (isnan(SemiMajorAxisOrientation_deg))
  {
    pSAEPositionalAccuracy->orientation = SAESemiMajorAxisOrientation_Limit_Max;
  }
  else
  {
    pSAEPositionalAccuracy->orientation = rintf(SemiMajorAxisOrientation_deg / 0.0054932479f);
  }

  if (pSAEPositionalAccuracy->orientation < SAESemiMajorAxisOrientation_Limit_Min ||
      pSAEPositionalAccuracy->orientation > SAESemiMajorAxisOrientation_Limit_Max)
  {
    d_printf(D_WARN, NULL, "SemiMajorAxisOrientation out of range %d\n",
             pSAEPositionalAccuracy->orientation);
  }

  LIMIT(pSAEPositionalAccuracy->orientation,
        SAESemiMajorAxisOrientation_Limit_Max,
        SAESemiMajorAxisOrientation_Limit_Max - 1,
        SAESemiMajorAxisOrientation_Limit_Min);
}


/**
 * @brief Compute SDSM time of measurement value given position reference time
 *        and the measurement time in absolute time
 * @param posRefT_ms Position reference time (mS) since UNIX epoch
 * @param objAbsT_ms Object measurement time in absolute time (mS) since UNIX epoch
 * @return the object ToM delta value SAEMeasurementTimeOffset (mS)
 */
static int16_t ComputeObjToM(uint64_t PosRefT_ms, uint64_t objAbsT_ms)
{
  //d_printf(D_WARN, NULL, "(%" PRIu64 ", %" PRIu64 ")\n", PosRefT_ms, objAbsT_ms);
#if 1
  uint64_t abs_diff = (objAbsT_ms > PosRefT_ms)
    ? (objAbsT_ms - PosRefT_ms)
    : (PosRefT_ms - objAbsT_ms);
  // J3224 DE_MeasurementTimeOffset (-1500..1500)
  if (abs_diff > 1500)
  {
    abs_diff = 1500;
  }
  return (objAbsT_ms > PosRefT_ms)
      ?  (int16_t)abs_diff
      : -(int16_t)abs_diff;
#else
  int64_t Diff_ms = objAbsT_ms - PosRefT_ms;
  if (Diff_ms > 1500)
  {
    Diff_ms = 1500;
  }
  else if (Diff_ms < -1500)
  {
    Diff_ms = -1500;
  }
  return Diff_ms;
#endif
}


/**
 * @return distance between two latitudes in metres in units 10cm
 *         (-32767..32767)
 */
static int DeltaLat2dY(double Lat1_deg,
                       double Lat2_deg)
{
  d_fnstart(D_INTERN, NULL, "(%.5f,%.5f)\n", Lat1_deg, Lat2_deg);

  double lat_1deg_metres = EARTH_CIRCUMFERENCE_POLES_m/360.0;
  double dY_m = (Lat2_deg - Lat1_deg) * lat_1deg_metres;

  d_printf(D_WARN, NULL, "1 degree of latitude ~= %fm\n", EARTH_CIRCUMFERENCE_POLES_m / 360.0);
  d_printf(D_WARN, NULL, "Delta latitude = %f degrees = %f m\n", Lat2_deg - Lat1_deg, dY_m);

  int D_10cm = dY_m * 10;
  if (D_10cm > 32767)
  {
    D_10cm = 32767;
  }
  else if (D_10cm < -32767)
  {
    D_10cm = -32767;
  }

  d_fnend(D_INTERN, NULL, "(%.5f,%.5f): %d\n", Lat1_deg, Lat2_deg, D_10cm);
  return D_10cm;
}


/**
 * @return Distance between two longitudes at a specific latitude in units
 *         of 10cm (-32767..32767)
 */
static int DeltaLon2dX(double Lon1_deg,
                       double Lon2_deg,
                       double Lat_deg)
{
  d_fnstart(D_INTERN, NULL, "(%.5f,%.5f,%.5f)\n", Lon1_deg, Lon2_deg, Lat_deg);

#define DEGS2RAD (M_PI/180)
  double cosine_lat = cos(Lat_deg * DEGS2RAD);
  double lon_1deg_metres = cosine_lat * EARTH_CIRCUMFERENCE_EQUATOR_m / 360.0;

  double dX_m = (Lon2_deg - Lon1_deg) * lon_1deg_metres;

  d_printf(D_WARN, NULL, "Delta longitude = %f degrees = %f m\n", Lon2_deg - Lon1_deg, dX_m);

  int D_10cm = dX_m * 10;
  if (D_10cm > 32767)
  {
    D_10cm = 32767;
  }
  else if (D_10cm < -32767)
  {
    D_10cm = -32767;
  }
  d_fnend(D_INTERN, NULL, "(%.5f,%.5f,%.5f): %d\n", Lon1_deg, Lon2_deg, Lat_deg, D_10cm);
  return D_10cm;
}


static void CompueteXYZOffsets(tSAEDetectedObjectDataSnapshot *pObject,
                               const struct LPHPos *pLPHPos)
{
  int DeltaElev_10cm  = (pObject->Elevation - pLPHPos->Elevation);
#if 0
  (void)DeltaLat2dY(0,0);
  (void)DeltaLon2dX(0,0,0);

  float DeltaLat_deg  = (pObject->Latitude  - pLPHPos->Latitude) * 1e-7;
  float DeltaLong_deg = (pObject->Longitude - pLPHPos->Longitude) * 1e-7;

  float OffsetX_m = dLon2dX(DeltaLong_deg, pLPHPos->Latitude * 1e-7);
  float OffsetY_m = dLat2dY(DeltaLat_deg);

  pObject->Object.detObjCommon.pos.offsetX = rint(OffsetX_m * 1e1);
  pObject->Object.detObjCommon.pos.offsetY = rint(OffsetY_m * 1e1);
#else
  pObject->Object.detObjCommon.pos.offsetX = DeltaLon2dX(pLPHPos->Longitude * 1e-7,
                                                         pObject->Longitude * 1e-7,
                                                         pLPHPos->Latitude * 1e-7);

  pObject->Object.detObjCommon.pos.offsetY = DeltaLat2dY(pLPHPos->Latitude * 1e-7,
                                                         pObject->Latitude * 1e-7);
#endif
  pObject->Object.detObjCommon.pos.offsetZ = DeltaElev_10cm;

  return;
}



/**
 * @brief Populate SDSM message
 * @param pSDSM SDSM Tx module
 * @param[out] ppMsg SDSM Message to be populated
 * @return 0 for success, negative for errors
 */
static int SDSMTx_PopulateSDSM (struct SDSMTx *pSDSMTx,
                                struct SAESensorDataSharingMessage *pSDSM,
                                tSAEDetectedObjectDataSnapshot *pObjs,
                                const int NumObjs,
                                const int DataSource)
{
  d_fnstart(D_API, NULL, "(%p, %p)\n", pSDSMTx, pSDSM);
  int Ret = J3224_SUCCESS;

  if (pSDSMTx && pSDSM)
  {
    // DE_TemporaryID
    pSDSM->sourceID.buf = (uint8_t *)asn1_mallocz(4);
    if (pSDSM->sourceID.buf == NULL)
    {
      d_error(D_WARN, NULL, "unable to allocate ID\n");
      Ret = J3224_FAILURE;
      goto Error;
    }
    else
    {
      pSDSM->sourceID.len = 4;

      uint32_t TempID = P1609TX_GetTempID();
      *((uint32_t*)pSDSM->sourceID.buf) = htonl(TempID);
    }

    // DE_MsgCount
    {
      static uint8_t MsgCnt = 0;
      pSDSM->msgCnt = MsgCnt;
      MsgCnt = (MsgCnt + 1) % 128;
    }

    struct LPHPos LPHPos = {0};
    if (LPH_Position(pLPH, &LPHPos))
    {
      d_error(D_ERR, NULL, "Unable to obtain position\n");
      Ret = J3224_FAILURE;
      goto Error;
    }
    else // DF_Position3D
    {
      // DE_Latitude
      pSDSM->refPos.lat  = LPHPos.Latitude;
      // DE_Longitude
      pSDSM->refPos.Long = LPHPos.Longitude;
      // DE_Elevation
      pSDSM->refPos.elevation = LPHPos.Elevation;
      pSDSM->refPos.elevation_option = true;
    }
    uint64_t PosTimeUTC_ms = (LPHPos.Time.tv_sec * 1000) + (LPHPos.Time.tv_usec / 1000);

    d_printf(D_INFO, NULL, "refPos (%16.12f, %16.12f, %f) @ %f\n",
             LPHPos.Latitude * 1e-7,
             LPHPos.Longitude * 1e-7,
             LPHPos.Elevation * 1e-1,
             PosTimeUTC_ms * 1e-3);

    struct tm TimeStamp;
    if (gmtime_r(&(LPHPos.Time.tv_sec), &TimeStamp) == NULL)
    {
      d_error(D_ERR, NULL, "Unable to obtain convert timestamp %lu\n", (unsigned long)LPHPos.Time.tv_sec);
      Ret = J3224_FAILURE;
      goto Error;
    }
    else // DF_DDateTime
    {
      // tm_year is number of years since 1900
      // J2735 DDateTime year is gregorian year
      pSDSM->sDSMTimeStamp.year_option = true;
      pSDSM->sDSMTimeStamp.year = TimeStamp.tm_year + 1900;

      // tm_mon is number of months since January (0-11)
      // J2735 DDateTime month is month of the year (1-12)
      pSDSM->sDSMTimeStamp.month_option = true;
      pSDSM->sDSMTimeStamp.month = TimeStamp.tm_mon + 1;

      // Day of the month (1-31)
      pSDSM->sDSMTimeStamp.day_option = true;
      pSDSM->sDSMTimeStamp.day = TimeStamp.tm_mday;

      // Number of hours past midnight (0-23)
      pSDSM->sDSMTimeStamp.hour_option = true;
      pSDSM->sDSMTimeStamp.hour = TimeStamp.tm_hour;

      // Number of minutes past the hour (0-59)
      pSDSM->sDSMTimeStamp.minute_option = true;
      pSDSM->sDSMTimeStamp.minute = TimeStamp.tm_min;

      // tm_sec is number of seconds after the minute
      // J2735 DDateTime second is number of milliseconds after the minute
      pSDSM->sDSMTimeStamp.second_option = true;
      pSDSM->sDSMTimeStamp.second = TimeStamp.tm_sec*1000 + LPHPos.Time.tv_usec/1000;
    }

    // DF_PositionConfidenceSet
    PopulatePositionalAccuracy(&pSDSM->refPosXYConf, &LPHPos);

    if (NumObjs)
    {
      // Allocate object list
      pSDSM->objects.tab =
        (SAEDetectedObjectData *)asn1_mallocz(NumObjs * sizeof(SAEDetectedObjectData));

      if (pSDSM->objects.tab)
      {
        size_t i;

        pSDSM->objects.count = NumObjs;
        for (i = 0; i < pSDSM->objects.count; ++i)
        {
          // Set the object measurement time based on the reference position time
          pSDSM->objects.tab[i].detObjCommon.measurementTime =
              ComputeObjToM(PosTimeUTC_ms, pObjs[i].Time_ms);

          if (DataSource == SDSMTX_DATA_SOURCE_UDP)
          {
            // Set the position offset based on the reference position
            CompueteXYZOffsets(&pObjs[i], &LPHPos);
          }
          // Otherwise the offsets are set at init from the static configuration

          d_printf(D_NOTICE, NULL, "%2zu: [%f %f %f] %fm/s %feg %dms \n", i,
                   pObjs[i].Object.detObjCommon.pos.offsetX * 1e-1,
                   pObjs[i].Object.detObjCommon.pos.offsetY * 1e-1,
                   pObjs[i].Object.detObjCommon.pos.offsetZ * 1e-1,
                   pObjs[i].Object.detObjCommon.speed * 0.02,
                   pObjs[i].Object.detObjCommon.heading * 0.0125,
                   pSDSM->objects.tab[i].detObjCommon.measurementTime);

          asn1_copy_value(asn1_type_SAEDetectedObjectData,
                          &pSDSM->objects.tab[i],
                          &pObjs[i].Object);

        }
      }
      else
      {
        d_error(D_ERR, NULL, "SDSM object list allocation failed\n");
        pSDSM->objects.count = 0;
      }
    }

  }
  else
  {
    Ret = -EINVAL;
  }

Error:
  d_fnend(D_API, NULL, "(%p, %p): %d\n", pSDSMTx, pSDSM, Ret);
  return Ret;
} // SDSMTx_PopulateSDSM


/**
 * @brief Populate WSM header
 * @param pSDSM
 * @param[out] pHdr WSMP header to be popualted
 * @param Len Size of WSM paylaod
 */
void SDSMTx_PopulateWSMHeader (struct SDSMTx *pSDSM,
                               struct Dot3WSMPHdr *pHdr,
                               int Len)
{
  memcpy(pHdr, &(pSDSM->Params.WSMP), sizeof(struct Dot3WSMPHdr));
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
 * @brief Transmit SDSM messages
 * @param pSDSM SDSM Tx module
 * @return 0 for success. Negative for errors.
 */
static int SDSMTx_TransmitSDSM (struct SDSMTx *pSDSM,
                                tSAEDetectedObjectDataSnapshot *pObjs,
                                const int Num,
                                const int DataSource)
{
  int Res;
  int WSMSize;
  uint8_t *pWSM = NULL;
  struct SAESensorDataSharingMessage *pASN = NULL;

  // Create the ASN.1 structure
  Res = SDSMTx_CreateSDSM(pSDSM, &pASN);
  if (Res != 0)
  {
    pASN = NULL;
    goto Error;
  }
  d_assert(pASN != NULL);

  // Fill in the ASN.1 structure
  Res = SDSMTx_PopulateSDSM(pSDSM, pASN, pObjs, Num, DataSource);
  if (Res != 0)
  {
    goto Error;
  }

  // Allocate the WSM buffer
  WSMSize = sizeof(struct Dot3WSMPHdr) + SDSMTX_WSM_BUF_SIZE; // TODO: is the BUF_SIZE sufficient?
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

    Res = SDSMTx_EncodeSDSM(pSDSM, pASN, pUPER, UPERSize);
    if (Res < 0)
    {
      goto Error;
    }
    // A positive return value means that the encoding worked
    WSMSize = Res;
  }

  // Add the WSM Header to the BER encoded buffer
  SDSMTx_PopulateWSMHeader(pSDSM, (struct Dot3WSMPHdr *) pWSM, WSMSize);

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
    // "Sent SDSM (%d bytes)", WSMSize);
  }

Error:
  if (pWSM)
    free(pWSM);
  if (pASN)
    (void) SDSMTx_DestroySDSM(pSDSM, pASN);
  return Res;
}


static void SDSMTx_ObjectRxThread (void *pArg)
{
  d_fnstart(D_API, NULL, "(%p)\n", pArg);

  //int ErrCount = 0;
  struct SDSMTx *pSDSM;
  struct timespec Time;
  //struct epoll_event Events[1];
  // Wake up every 1 second so we don't block too long on exit
  int Timeout = 1000;

  char Buffer[1024] = {0};

  pSDSM = (struct SDSMTx *) pArg;

  pSDSM->ObjectRx_ThreadState |= SDSMTX_THREAD_STATE_RUN;

  // Thread loop
  while ((pSDSM->ObjectRx_ThreadState & SDSMTX_THREAD_STATE_STOP) == 0)
  {
#if 0
    int NFds;

    NFds = epoll_wait(EpollFd, Events, 1, Timeout);
    if (!NFds)
    {
      d_printf(D_DEBUG, NULL, "epoll_wait timeout\n");
      continue;
    }
    if (NFds < 0)
    {
      // If epoll_wait fails, try again up to MAX_RETRIES
      d_error(D_ERR, NULL, "epoll_wait failed: %s\n", strerror(errno));
      if (++ErrCount < 100)
      {
        Util_Nap(200, &Time);
      }
      else
      {
        break;
      }
    }
#else
    struct pollfd FD = {
      .fd      = ObjectFd,
      .events  = (POLLIN | POLLPRI),
      .revents = 0,
    };

    // poll() returns >0 if descriptor is readable, 0 if timeout, -1 if error
    int Res = poll(&FD, 1, Timeout);
    // preserve errno before calling case d_printf
    const int PollErrno = errno;
    if (Res < 0)
    {
      if (PollErrno == EINTR)
      {
        d_printf(D_INFO, NULL, "poll(%d) interrupted (%d %d).\n", FD.fd, Res, PollErrno);
        continue;
      }
      else
      {
        d_printf(D_ERR, NULL, "poll(%d) error (%d %d).\n", FD.fd, Res, PollErrno);
        break;
      }
    }
    else if (Res == 0)
    {
      d_printf(D_DBG, NULL,
               "poll(%d) timeout @ %" PRIu64 "\n", FD.fd, Util_Now());
      continue;
    }
#endif

    struct timeval tv;
    ssize_t Len = recv(ObjectFd, Buffer, sizeof(Buffer), 0);
    gettimeofday(&tv, NULL);
    if (Len <= 0)
    {
      d_error(D_WARN, NULL, "recv %s", strerror(errno));
    }
    else
    {
      d_printf(D_WARN, NULL, "Received %zu bytes\n", Len);

      tObjectDataList *ObjectList = (tObjectDataList *)Buffer;

      if (ObjectList->Magic != 0xdeadbeef)
      {
        d_error(D_WARN, NULL, "Message failed magic check 0x%x != 0xdeadbeef\n",
                ObjectList->Magic);
      }
      else
      {
        d_printf(D_WARN, NULL, "Received %d objects\n", ObjectList->NumObjects);

        ssize_t ExpectedLen = sizeof(tObjectDataList) +
          (ObjectList->NumObjects * sizeof(tObjectData));

        if (Len < ExpectedLen)
        {
          d_error(D_WARN, NULL, "recv message truncated: got %zu expected %zu\n",
                  Len, ExpectedLen);
        }

        pthread_mutex_lock(&ObjectMutex);
        {
          // Save timestamp
          Object_tv = tv;

          // Free old object list
          if (pObjects)
          {
            free(pObjects);
          }
          // Allocate new object list
          NumObjects = ObjectList->NumObjects;
          pObjects = malloc(sizeof(tSAEDetectedObjectDataSnapshot) * NumObjects);

          // Populate the new object list
          int i;
          for (i = 0; i < NumObjects; ++i)
          {
            pObjects[i].Time_ms   = ObjectList->Objects[i].Time_ms;
            pObjects[i].Latitude  = ObjectList->Objects[i].Lat;
            pObjects[i].Longitude = ObjectList->Objects[i].Long;
            pObjects[i].Elevation = ObjectList->Objects[i].Elev;

            pObjects[i].Object.detObjCommon.objType = ObjectList->Objects[i].ObjectType;
            pObjects[i].Object.detObjCommon.objTypeCfd = 0;
            pObjects[i].Object.detObjCommon.objectID = ObjectList->Objects[i].ObjectID;
            pObjects[i].Object.detObjCommon.measurementTime = 0; // This will be set at transmit time
            pObjects[i].Object.detObjCommon.timeConfidence = SAETimeConfidence_unavailable;
            pObjects[i].Object.detObjCommon.pos.offsetX = 0; // This will be set at transmit time
            pObjects[i].Object.detObjCommon.pos.offsetY = 0; // This will be set at transmit time
            pObjects[i].Object.detObjCommon.pos.offsetZ = 0; // This will be set at transmit time
            pObjects[i].Object.detObjCommon.pos.offsetZ_option = true;
            pObjects[i].Object.detObjCommon.posConfidence.pos = SAEPositionConfidence_unavailable;
            pObjects[i].Object.detObjCommon.posConfidence.elevation = SAEElevationConfidence_unavailable;
            pObjects[i].Object.detObjCommon.speed = ObjectList->Objects[i].Speed;
            pObjects[i].Object.detObjCommon.speedConfidence = SAESpeedConfidence_unavailable;
            pObjects[i].Object.detObjCommon.speedZ_option = false;
            pObjects[i].Object.detObjCommon.speedConfidenceZ_option = false;
            pObjects[i].Object.detObjCommon.heading = ObjectList->Objects[i].Heading;
            pObjects[i].Object.detObjCommon.headingConf = SAEHeadingConfidence_unavailable;
            pObjects[i].Object.detObjCommon.accel4way_option = false;
            pObjects[i].Object.detObjCommon.accCfdX_option = false;
            pObjects[i].Object.detObjCommon.accCfdY_option = false;
            pObjects[i].Object.detObjCommon.accCfdZ_option = false;
            pObjects[i].Object.detObjCommon.accCfdYaw_option = false;
            pObjects[i].Object.detObjOptData_option = false;
          }
        }
        pthread_mutex_unlock(&ObjectMutex);
      }
    }

    // polling delay
    Util_Nap(pSDSM->Params.Interval_ms, &Time);
  }

  d_fnend(D_API, NULL, "(%p)\n", pArg);

  // exit thread
  (void) pthread_exit(NULL);
}


/**
 * @brief SDSMTx Periodic thread processing - sends a SDSM
 * @param pArg Pointer to SDSMTx object passed as generic input parameter
 */
static void SDSMTx_ThreadProc (void *pArg)
{
  d_fnstart(D_API, NULL, "(%p)\n", pArg);

  struct SDSMTx *pSDSM;
  struct timespec Time;

  pSDSM = (struct SDSMTx *) pArg;

  // "Starting SDSMTx Periodic Thread");
  pSDSM->ThreadState |= SDSMTX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while ((pSDSM->ThreadState & SDSMTX_THREAD_STATE_STOP) == 0)
  {
    int i;
    for (i = 0; i < pSDSM->Params.NumObjects; ++i)
    {
      pSDSM->Params.pObjects[i].Time_ms = Util_Now();
    }

    if (pSDSM->Params.Enabled)
    {
      // FIXME: magic number
      if (pSDSM->Params.DataSource == SDSMTX_DATA_SOURCE_CFG)
      {
        d_printf(D_INFO, NULL, "Transmit SDSM from config data\n");

        // Transmit a SDSM
        int Res = SDSMTx_TransmitSDSM(pSDSM,
                                      pSDSM->Params.pObjects,
                                      pSDSM->Params.NumObjects,
                                      pSDSM->Params.DataSource);
        if (Res == 0)
        {
          pSDSM->Stats.Tx.Okay++;
        }
        else
        {
          pSDSM->Stats.Tx.Fail++;
        }
      }
      else
      {
        struct timeval tv, tv_diff;
        int ObjectAge_ms;
        tSAEDetectedObjectDataSnapshot *Objects = NULL;

        d_printf(D_INFO, NULL, "Transmit SDSM from UDP data\n");

        pthread_mutex_lock(&ObjectMutex);
        gettimeofday(&tv, NULL);
        Objects = (tSAEDetectedObjectDataSnapshot *)
          malloc(sizeof(tSAEDetectedObjectDataSnapshot) * NumObjects);
        memcpy(Objects, pObjects, sizeof(tSAEDetectedObjectDataSnapshot) * NumObjects);
        pthread_mutex_unlock(&ObjectMutex);

        timersub(&tv, &Object_tv, &tv_diff);
        ObjectAge_ms = (tv_diff.tv_sec * 1000) + (tv_diff.tv_usec / 1000);

        if (ObjectAge_ms <= pSDSM->Params.ObjectMaxAge_ms)
        {
          // Transmit a SDSM
          int Res = SDSMTx_TransmitSDSM(pSDSM,
                                        Objects,
                                        NumObjects,
                                        pSDSM->Params.DataSource);
          if (Res == 0)
          {
            pSDSM->Stats.Tx.Okay++;
          }
          else
          {
            pSDSM->Stats.Tx.Fail++;
          }
        }
        else
        {
          d_printf(D_INFO, NULL, "Object data too old (%d ms)\n", ObjectAge_ms);
        }
        free(Objects);
      }
    }

    // polling delay
    Util_Nap(pSDSM->Params.Interval_ms, &Time);
  }

  d_fnend(D_API, NULL, "(%p)\n", pArg);

  // exit thread
  (void) pthread_exit(NULL);
}



// Close the doxygen group
/**
 * @}
 */

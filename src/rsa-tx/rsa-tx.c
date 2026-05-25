/**
 * @addtogroup mod_rsa_tx RSA Tx Module
 * @{
 *
 * RSA transmitter thread
 *
 * @file rsa-tx.c Module code for RSA transmitter
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2012 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "rsa-tx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "dot3-wsmp.h"
#include "j2735-common.h"
#include "j2735-full-position-vector.h"
#include "j2735-road-side-alert.h"
#include "j2735asn.h"
#include "lph.h"
#include "p1609-tx-msg.h"
#include "util.h"
#include "Example1609_debug.h"

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_RSATx_DebugLevel

#define dbg_printf(f, a...)                                       \
    do {                                                          \
        fprintf(stderr, "%s(%d): " f, __func__, __LINE__, ## a);  \
    } while (0)                                                   \

/// Maximum size of the WSM
#define RSATX_WSM_BUF_SIZE      (1386)

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int RSATx_LoadConfig (struct RSATx *pRSA,
                             char * pConfigFileName);

static void RSATx_ThreadProc (void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------


/**
 * @brief Allocate RSA object, Setup thread and associated data structures
 * @param ppRSA pointer to RSA handle (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or an negative errno
 */
int RSATx_Open (struct RSATx **ppRSA,
                pthread_attr_t *pAttr,
                char *pConfigFileName)
{
  int Res;
  struct RSATx *pRSA;

  // Create and check the local structure
  pRSA = (struct RSATx *) malloc(sizeof(struct RSATx));
  if (pRSA == NULL)
  {
    Res = -ENOMEM;
    goto Error;
  }
  memset(pRSA, 0, sizeof(struct RSATx));

  // Load config settings
  Res = RSATx_LoadConfig(pRSA, pConfigFileName);
  if (Res != 0)
  {
    dbg_printf("RSATx_LoadConfig() != 0\n");
    goto Error;
  }
  // Init state
  pRSA->MsgCount = rand();

  // Assign thread attributes from input param
  pRSA->ThreadAttr = *pAttr;

  // Setup reader for LPH
  pRSA->pLPH = LPH_Open();
  if (pRSA->pLPH == NULL)
  {
    dbg_printf("LPH_Open() = NULL\n");
    // LPH registration error
    Res = -EPIPE;
    goto Error;
  }

  // Create RSATx thread
  pRSA->ThreadState |= RSATX_THREAD_STATE_INIT;
  Res = pthread_create(&pRSA->ThreadID,
                       &pRSA->ThreadAttr,
                       (void *) RSATx_ThreadProc,
                       pRSA);
  if (Res != 0)
  {
    dbg_printf("pthread_create() failed\n");
    // Thread creation error
    pRSA->ThreadState = RSATX_THREAD_STATE_NONE;
    goto Error;
  }

  // Success!
  *ppRSA = pRSA;
  Res = 0;
  goto Success;

Error:
  dbg_printf("Error!\n");
  *ppRSA = NULL;
  RSATx_Close(pRSA);
  Success: dbg_printf("Successfully started the rsa-tx\n");
  return Res;
}

/**
 * @brief Stop execution of RSA, free the thread and its associated resources
 * @param pRSA RSA handle
 */
void RSATx_Close (struct RSATx *pRSA)
{
  int Res;

  // Catch attempt to close invalid object pointer
  if (pRSA == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Signal thread to terminate
  pRSA->ThreadState |= RSATX_THREAD_STATE_STOP;
  // Wait for thread termination
  if (pRSA->ThreadState & RSATX_THREAD_STATE_INIT)
    pthread_join(pRSA->ThreadID, NULL);

  // De-register LPH connection
  LPH_Close(pRSA->pLPH);
  pRSA->pLPH = NULL;

  // Success!
  Res = 0;

Error:
  free(pRSA);

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Prints the statistics from the RSATx object
 * @param pRSA RSA handle
 */
void RSATx_PrintStats (struct RSATx *pRSA)
{
  fprintf(stdout, "RSATx: Tx (Okay %8u Fail %8u)\n",
	  pRSA->Stats.Tx.Okay, pRSA->Stats.Tx.Fail);
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load RSATx parameters from the config file
 * @param pRSA RSA transmitter handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int RSATx_LoadConfig (struct RSATx *pRSA,
                             char *pConfigFileName)
{
  int Res;
  config_t Config;             // configuration object read from config file
  config_t *pConfig;
  config_setting_t *pSetting;  // setting for RSATx in config file
  int ConfigVal = 0;
  char *pConfigString;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pRSA->Params.TxInterval = RSATX_CONFIG_VALUE_DEFAULT_TXINTERVAL;
  pRSA->Params.TypeEvent  = RSATX_CONFIG_VALUE_DEFAULT_TYPE_EVENT;

  // WSMP defaults
  {
    memset(pRSA->Params.WSMP.Tx.DA, 0xFF, 6);
    pRSA->Params.WSMP.Tx.Priority   = RSATX_CONFIG_VALUE_DEFAULT_WSMP_PRIORITY;
    pRSA->Params.WSMP.Tx.ExpiryTime = RSATX_CONFIG_VALUE_DEFAULT_WSMP_EXPIRY_TIME;
    pRSA->Params.WSMP.Version       = DOT3_WSMP_VERSION_3;
    pRSA->Params.WSMP.ChannelNumber = RSATX_CONFIG_VALUE_DEFAULT_WSMP_CHANNEL_NUMBER;
    const tdefault_stack *pIni = default_stack_Get();
    pRSA->Params.WSMP.ChannelNumber = pIni->ContinuousChanNum;
    default_stack_Release(&pIni);
    pRSA->Params.WSMP.DataRate      = RSATX_CONFIG_VALUE_DEFAULT_WSMP_DATA_RATE;
    pRSA->Params.WSMP.TxPower       = RSATX_CONFIG_VALUE_DEFAULT_WSMP_TX_POWER;
    pRSA->Params.WSMP.PSID          = htonl(RSATX_CONFIG_VALUE_DEFAULT_WSMP_PSID);
    pRSA->Params.WSMP.Length        = htons(0);
    pRSA->Params.WSMP.HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
  }

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    dbg_printf("Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'RSATx' Setting
  pSetting = config_lookup(pConfig, RSATX_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    dbg_printf("config_lookup(%s) failed\n", RSATX_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries

  // TxInterval
  if (config_setting_lookup_int(pSetting,
                                RSATX_CONFIG_VALUE_NAME_TXINTERVAL,
                                &ConfigVal))
    pRSA->Params.TxInterval = (uint32_t) ConfigVal;

  // WSMP
  {
    config_setting_t *pWSMP;
    pWSMP = config_setting_get_member(pSetting,
                                      RSATX_CONFIG_VALUE_NAME_WSMP);
    if (pWSMP != NULL)
    {
      // "pWSMP %p\n", pWSMP);

      // Priority
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_PRIORITY,
                                    &ConfigVal))
        pRSA->Params.WSMP.Tx.Priority = (uint8_t) ConfigVal;
      // ExpiryTime
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_EXPIRY_TIME,
                                    &ConfigVal))
        pRSA->Params.WSMP.Tx.ExpiryTime = (uint8_t) ConfigVal;
      // ChannelNumber
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_CHANNEL_NUMBER,
                                    &ConfigVal))
        pRSA->Params.WSMP.ChannelNumber = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_DATA_RATE,
                                    &ConfigVal))
        pRSA->Params.WSMP.DataRate = (uint8_t) ConfigVal;
      // DataRate
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_TX_POWER,
                                    &ConfigVal))
        pRSA->Params.WSMP.TxPower = (uint8_t) ConfigVal;
      // PSID
      if (config_setting_lookup_int(pWSMP,
                                    RSATX_CONFIG_VALUE_NAME_PSID,
                                    &ConfigVal))
        pRSA->Params.WSMP.PSID = htonl((uint32_t) ConfigVal);
    }
  }

  // TypeEvent
  if (config_setting_lookup_int(pSetting,
                                RSATX_CONFIG_VALUE_NAME_TYPE_EVENT,
                                &ConfigVal))
    pRSA->Params.TypeEvent = (uint16_t) ConfigVal;

  // Description
  {
    config_setting_t *pDescription;
    pDescription = config_setting_get_member(pSetting,
                                             RSATX_CONFIG_VALUE_NAME_DESCRIPTION);
    // "pDescription %p\n", pDescription);
    if (pDescription != NULL)
    {
      int i;
      int Cnt = config_setting_length(pDescription);

      for (i = 0; i < Cnt; i++)
      {
        ConfigVal = config_setting_get_int_elem(pDescription, i);
        pRSA->Params.Description[i] = (uint16_t) ConfigVal;
        // "pRSA->Params.Description[%d] %d\n", i, pRSA->Params.Description[i]);
      }
      pRSA->Params.Cfg.description = Cnt;
      // "pRSA->Params.Cfg.description %d\n", pRSA->Params.Cfg.description);
    }
  }

  // Priority
  if (config_setting_lookup_int(pSetting,
                                RSATX_CONFIG_VALUE_NAME_PRIORITY,
                                &ConfigVal))
  {
    pRSA->Params.Priority = (uint8_t) ConfigVal;
    pRSA->Params.Cfg.priority = 1;
    // "pRSA->Params.Priority %d\n", pRSA->Params.Priority);
    // "pRSA->Params.Cfg.priority %d\n", pRSA->Params.Cfg.priority);
  }

  // Heading
  if (config_setting_lookup_int(pSetting,
                                RSATX_CONFIG_VALUE_NAME_HEADING,
                                &ConfigVal))
  {
    pRSA->Params.Heading = (uint16_t) ConfigVal;
    pRSA->Params.Cfg.heading = 1;
    // "pRSA->Params.Heading %d\n", pRSA->Params.Heading);
    // "pRSA->Params.Cfg.heading %d\n", pRSA->Params.Cfg.heading);
  }

  // Extent
  if (config_setting_lookup_int(pSetting,
                                RSATX_CONFIG_VALUE_NAME_EXTENT,
                                &ConfigVal))
  {
    pRSA->Params.Extent = (uint8_t) ConfigVal;
    pRSA->Params.Cfg.extent = 1;
    // "pRSA->Params.Extent %d\n", pRSA->Params.Extent);
    // "pRSA->Params.Cfg.extent %d\n", pRSA->Params.Cfg.extent);
  }

  // Position
  {
    config_setting_t *pPosition;
    pPosition = config_setting_get_member(pSetting,
                                          RSATX_CONFIG_VALUE_NAME_POSITION);
    if (pPosition != NULL)
    {
      // "pPosition %p\n", pPosition);
      if (config_setting_length(pPosition) >= 2)
      {
        ConfigVal = config_setting_get_int_elem(pPosition, 0);
        pRSA->Params.Position.Lon = (uint32_t) ConfigVal;
        ConfigVal = config_setting_get_int_elem(pPosition, 1);
        pRSA->Params.Position.Lat = (uint32_t) ConfigVal;
      }
      pRSA->Params.Cfg.position = 1;
    }
  }

  // FurtherInfo
  if (config_setting_lookup_string(pSetting,
                                   RSATX_CONFIG_VALUE_NAME_FURTHERINFO,
                                   (const char **) &pConfigString))
  {
    // "ConfigString '%s'\n", pConfigString);
    pRSA->Params.FurtherInfo[0] = pConfigString[0];
    pRSA->Params.FurtherInfo[1] = pConfigString[1];
    pRSA->Params.Cfg.furtherInfoID = 1;
    // "pRSA->Params.FurtherInfo %d %d\n",
    // pRSA->Params.FurtherInfo[0],
    // pRSA->Params.FurtherInfo[1]);
    // "pRSA->Params.Cfg.furtherInfoID %d\n",
    // pRSA->Params.Cfg.furtherInfoID);
  }

  // Update complete - close the configuration
  config_destroy(pConfig);

  // Success!
  Res = 0;

Error:
  return Res;
}


/**
 * @brief Generate full position content
 * @param pFPV Pointer to a FullPositionVector object
 * @param pCfg Pointer to the ASN.1 object config specification
 */
static int RSATx_PopulateFullPositionVector (struct SAEFullPositionVector *pFPV,
                                             struct SAEFullPositionVectorCfg *pCfg,
                                             const struct LPHPos *pLPHPos)
{
  int Res;

  // Populate Full Position Vector Mandatory Fields
  pFPV->Long = pLPHPos->Longitude;
  pFPV->lat  = pLPHPos->Latitude;

  // "FPV Long=%d, Lat=%d.", pFPV->Long, pFPV->lat);

  // Populate Full Position Vector Optional Fields
  if (pCfg->utcTime)
  {
    struct tm DateAndTime;

    // Use time stamp from the LPH (corresponding to when the pos info was taken)
    if (!gmtime_r(&(pLPHPos->Time.tv_sec), &DateAndTime))
    {
      Res = -errno;
      goto Error;
    }

    if (pCfg->utcTimeConfig.Year)
    {
      pFPV->utcTime.year = DateAndTime.tm_year + 1900; // years since 1900
    }

    if (pCfg->utcTimeConfig.Month)
    {
      pFPV->utcTime.month = DateAndTime.tm_mon + 1; // Jan == 0
    }

    if (pCfg->utcTimeConfig.Day)
    {
      pFPV->utcTime.day = DateAndTime.tm_mday;
    }

    if (pCfg->utcTimeConfig.Hour)
    {
      pFPV->utcTime.hour = DateAndTime.tm_hour;
    }

    if (pCfg->utcTimeConfig.Min)
    {
      pFPV->utcTime.minute = DateAndTime.tm_min;
    }

    if (pCfg->utcTimeConfig.Sec)
    {
      // FPV DSecond is 0-60999 representing ms in a minute (60001-60999 used
      // to represent a leap second)
      pFPV->utcTime.second = (DateAndTime.tm_sec * 1000) +
                             (pLPHPos->Time.tv_usec / 1000);
    }
  }

  if (pCfg->elevation)
  {
    pFPV->elevation = pLPHPos->Elevation;
  }

  if (pCfg->heading)
  {
    pFPV->heading = pLPHPos->Heading;
  }

  if (pCfg->posAccuracy)
  {
    // "posAccuracy not supported");
  }

  if (pCfg->timeConfidence)
  {
    // "timeConfidence not supported");
  }

  if (pCfg->posConfidence)
  {
    // "posConfidence not supported");
  }

  if (pCfg->speedConfidence)
  {
    // "speedConfidence not supported");
  }

  // Success!
  Res = 0;

Error:
  return Res;
}


/**
 * @brief Create RSA message
 * @param pRSA
 * @param[out] ppMsg Message created
 * @param pCfg RSA configuration
 * @return 0 for success. Negative for errors.
 */
static int RSATx_CreateRSA (struct RSATx *pRSA,
                            struct SAERoadSideAlert **ppMsg,
                            struct RoadSideAlertCfg *pCfg)
{
  int Res;

  if ((pRSA == NULL) || (ppMsg == NULL) || (pCfg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735RoadSideAlert_Create(ppMsg, pCfg, J2735_CREATE_ALL);

Error:
  return Res;
}

/**
 * @brief Destroy RSA message
 * @param pRSA
 * @param pMsg Message to destroy
 * @return 0 for success. Negative for errors.
 */
static int RSATx_DestroyRSA (struct RSATx *pRSA,
                             struct SAERoadSideAlert *pMsg)
{
  int Res;

  if ((pRSA == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  Res = J2735RoadSideAlert_Free(pMsg, J2735_FREE_ALL);

Error:
  return Res;
}

/**
 * @brief Populate RSA message
 * @param pRSA
 * @param pMsg Message to populate
 * @param pCfg RSA configuration
 * @return 0 for success. Negative for errors.
 */
static int RSATx_PopulateRSA (struct RSATx *pRSA,
                              struct SAERoadSideAlert *pMsg,
                              struct RoadSideAlertCfg *pCfg)
{
  int Res = -ENOSYS;
  const struct LPHPos *pLPHPos = NULL; // populated with current position info

  (void)pCfg;

  // Get current position info for RSA content
  {
    const struct LPHData *pLPHData = NULL;

    int Ret = LPH_Data(pRSA->pLPH, &pLPHData);
    if ((Ret == 0) && (pLPHData != NULL))
      pLPHPos = (const struct LPHPos *) &(pLPHData->Pos);
  }

  // Populate mandatory fields
  /// ?? pMsg->msgID = SAEDSRCmsgID_roadSideAlert;
  pMsg->msgCnt = 0x7F & pRSA->MsgCount++;
  pMsg->typeEvent = pRSA->Params.TypeEvent;

  // Optional fields

  if (pRSA->Params.Cfg.description != 0)
  {
    SAEITIScodes *pList = pMsg->description.tab;
    size_t i;
    for (i = 0; i < pMsg->description.count; i++)
    {
      pList[i] = pRSA->Params.Description[i];
    }
  }

  if (pRSA->Params.Cfg.priority != 0)
  {
    pMsg->priority.buf[0] = pRSA->Params.Priority;
    pMsg->priority.len = 1;
  }

  if (pRSA->Params.Cfg.heading != 0)
  {
    pMsg->heading.buf[0] = (pRSA->Params.Heading & 0xFF00) >> 8;
    pMsg->heading.buf[1] = (pRSA->Params.Heading & 0x00FF) >> 0;
    pMsg->heading.len = 16;
  }

  if (pRSA->Params.Cfg.extent != 0)
  {
    pMsg->extent = pRSA->Params.Extent;
  }

  if (pRSA->Params.Cfg.position != 0)
  {
    struct LPHPos LPHPos;

    if (pLPHPos == NULL)
    {
      Res = -EPIPE;
      goto Error;
    }
    memcpy(&LPHPos, pLPHPos, sizeof(struct LPHPos));

    // Overide the GPS position (if requested in config file)
    if (pRSA->Params.Position.Lon != 0)
      LPHPos.Longitude = pRSA->Params.Position.Lon;
    if (pRSA->Params.Position.Lat != 0)
      LPHPos.Latitude = pRSA->Params.Position.Lat;

    // Populate the FPV
    int FPVRes = RSATx_PopulateFullPositionVector(&pMsg->position,
                                                  &(pRSA->Params.Cfg.positionCfg),
                                                  &LPHPos);
    if (FPVRes != 0)
    {
      // "Failed to populate RSA's FPV %d.", FPVRes);
      goto Error;
    }
  }

  if (pRSA->Params.Cfg.furtherInfoID != 0)
  {
    pMsg->furtherInfoID.buf[0] = pRSA->Params.FurtherInfo[0];
    pMsg->furtherInfoID.buf[1] = pRSA->Params.FurtherInfo[1];
    pMsg->furtherInfoID.len = 2;
  }

  // Success!
  Res = 0;

Error:
  return Res;
}

/**
 * @brief Encode RSA message
 * @param pRSA
 * @param pMsg Message to be encoded
 * @param[out] pBuf Buffer that contains encoded message
 * @param BufSize Buffer size
 * @return Size of encoded message, negative for errors
 */
static int RSATx_EncodeRSA (struct RSATx *pRSA,
                            struct SAERoadSideAlert *pMsg,
                            uint8_t *pBuf,
                            int BufSize)
{
  int Res;
  int RVal;

  (void)pRSA;

  // Encode the RSA to a buffer
  RVal = J2735RoadSideAlert_Encode(pMsg, pBuf, BufSize);
  if (RVal == -1)
  {
    // "Failed to encode RSA at %s.",
    // RVal.failed_type ? RVal.failed_type->name : "unknown");
    Res = -EAGAIN;
  }
  else
  {
    // Encoding finished successfully
    // "Encoded RSA has %d bytes.", RVal);
    Res = RVal;
  }

  return Res;
}

/**
 * @brief Populate WSM header
 * @param pRSA
 * @param[out] pHdr WSMP header to be popualted
 * @param Len Size of WSM paylaod
 */
void RSATx_PopulateWSMHeader (struct RSATx *pRSA,
                              struct Dot3WSMPHdr *pHdr,
                              int Len)
{
  memcpy(pHdr, &(pRSA->Params.WSMP), sizeof(struct Dot3WSMPHdr));
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
 * @brief Transmit RSA messages
 * @param pRSA RSA Tx module
 * @return 0 for success. Negative for errors.
 */
static int RSATx_TransmitRSA (struct RSATx *pRSA)
{
  int Res;
  struct SAERoadSideAlert *pASN = NULL;
  uint8_t *pWSM = NULL;
  int WSMSize;

  // Create the ASN.1 structure
  Res = RSATx_CreateRSA(pRSA, &pASN, &(pRSA->Params.Cfg));
  if (Res != 0)
  {
    goto Error;
  }

  // Fill in the ASN.1 structure
  Res = RSATx_PopulateRSA(pRSA, pASN, &(pRSA->Params.Cfg));
  if (Res != 0)
  {
    goto Error;
  }

  // Allocate the WSM buffer
  WSMSize = sizeof(struct Dot3WSMPHdr) + RSATX_WSM_BUF_SIZE;
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

    Res = RSATx_EncodeRSA(pRSA, pASN, pUPER, UPERSize);
    if (Res < 0)
    {
      goto Error;
    }
    // A positive return value means that the encoding worked
    WSMSize = Res;
  }

  // Add the WSM Header to the BER encoded buffer
  RSATx_PopulateWSMHeader(pRSA, (struct Dot3WSMPHdr *) pWSM, WSMSize);

  // Transmit the WSM.
  // Enable signing (might be fake sig if security is disabled).
  // Also disable Tx logging.
  Res = P1609Tx_SendWSM((struct Dot3WSMPHdr *) pWSM, true, false, -1);
  if (Res != 0)
  {
    // P1609Tx_SendWSM failed
    goto Error;
  }
  else
  {
    // Sent RSA
  }

Error:
  if (pWSM)
    free(pWSM);
  if (pASN)
    (void) RSATx_DestroyRSA(pRSA, pASN);
  return Res;
}


/**
 * @brief RSA Periodic thread processing - sends a RSA
 * @param pArg Pointer to RSATx object passed as generic input parameter
 *
 */
static void RSATx_ThreadProc (void *pArg)
{
  struct RSATx *pRSA;
  struct timespec Time;

  // pArg cast to RSAtx object
  pRSA = (struct RSATx *) pArg;

  // "Starting RSA Periodic Thread");
  pRSA->ThreadState |= RSATX_THREAD_STATE_RUN;

  // Initialise the timespec struct for the polling loop
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while ((pRSA->ThreadState & RSATX_THREAD_STATE_STOP) == 0)
  {
    // polling delay
    Util_Nap(pRSA->Params.TxInterval, &Time);

    // Transmit an RSA
    int Res = RSATx_TransmitRSA(pRSA);

    if (Res == 0)
      pRSA->Stats.Tx.Okay++;
    else
      pRSA->Stats.Tx.Fail++;
  }

  // "Leaving Periodic RSA Thread");

  // exit thread
  (void) pthread_exit(NULL);

}

// Close the doxygen group
/**
 * @}
 */

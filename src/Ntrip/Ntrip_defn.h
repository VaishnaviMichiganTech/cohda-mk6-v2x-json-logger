#pragma once
/**
 * @addtogroup conf_if_module_Ntrip Ntrip Configuration service
 * @{
 *
 * @file
 *
 * Configuration file parameters for Ntrip
 */
//------------------------------------------------------------------------------
// Copyright (c) 2021 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------
#define Ntrip_CRC ((uint64_t)(0xc5822991b87b06d1ull))
#include "Ntrip_debug.h"

#include "conf_if.h"
#include "conf_common.h"

#include "conf_common.h" // for blobs

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C"
{
#endif

bool Ntrip_CheckCRC(uint64_t CRC);

#define Ntrip_Cust_App_Ntrip_DebugLevel_PARAM "Cust_App_Ntrip.DebugLevel"
#define Ntrip_Cust_App_Ntrip_DebugLevel_MIN -1
#define Ntrip_Cust_App_Ntrip_DebugLevel_DEF -1
#define Ntrip_Cust_App_Ntrip_DebugLevel_MAX 255

#define Ntrip_Cust_App_Ntrip_ENABLE_PARAM "Cust_App_Ntrip.ENABLE"
#define Ntrip_Cust_App_Ntrip_ENABLE_DEF false

#define Ntrip_Cust_App_Ntrip_URI_PARAM "Cust_App_Ntrip.URI"
#define Ntrip_Cust_App_Ntrip_URI_DEF ""

#define Ntrip_Cust_App_Ntrip_GGA_Interval_PARAM "Cust_App_Ntrip.GGA_Interval"
#define Ntrip_Cust_App_Ntrip_GGA_Interval_MIN 0
#define Ntrip_Cust_App_Ntrip_GGA_Interval_DEF 5000
#define Ntrip_Cust_App_Ntrip_GGA_Interval_MAX 60000

#define Ntrip_Cust_App_Ntrip_WSM_TxPSID_PARAM "Cust_App_Ntrip.WSM_TxPSID"
#define Ntrip_Cust_App_Ntrip_WSM_TxPSID_MIN 0x0ul
#define Ntrip_Cust_App_Ntrip_WSM_TxPSID_DEF 0x8001ul
#define Ntrip_Cust_App_Ntrip_WSM_TxPSID_MAX 0xeffffffful

#define Ntrip_Cust_App_Ntrip_WSM_TxPrio_PARAM "Cust_App_Ntrip.WSM_TxPrio"
#define Ntrip_Cust_App_Ntrip_WSM_TxPrio_MIN 0
#define Ntrip_Cust_App_Ntrip_WSM_TxPrio_DEF 0
#define Ntrip_Cust_App_Ntrip_WSM_TxPrio_MAX 7

#define Ntrip_Cust_App_Ntrip_WSM_TxChannel_PARAM "Cust_App_Ntrip.WSM_TxChannel"
#define Ntrip_Cust_App_Ntrip_WSM_TxChannel_MIN 172
#define Ntrip_Cust_App_Ntrip_WSM_TxChannel_DEF 178
#define Ntrip_Cust_App_Ntrip_WSM_TxChannel_MAX 184

#define Ntrip_Cust_App_Ntrip_WSM_TxDataRate_PARAM "Cust_App_Ntrip.WSM_TxDataRate"
#define Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MIN 5
#define Ntrip_Cust_App_Ntrip_WSM_TxDataRate_DEF 12
#define Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MAX 54

#define Ntrip_Cust_App_Ntrip_WSM_TxPower_PARAM "Cust_App_Ntrip.WSM_TxPower"
#define Ntrip_Cust_App_Ntrip_WSM_TxPower_MIN 0
#define Ntrip_Cust_App_Ntrip_WSM_TxPower_DEF 20
#define Ntrip_Cust_App_Ntrip_WSM_TxPower_MAX 33

#define Ntrip_Cust_App_Ntrip_HopLimit_PARAM "Cust_App_Ntrip.HopLimit"
#define Ntrip_Cust_App_Ntrip_HopLimit_MIN 0
#define Ntrip_Cust_App_Ntrip_HopLimit_DEF 10
#define Ntrip_Cust_App_Ntrip_HopLimit_MAX 255

#define Ntrip_Cust_App_Ntrip_RTCM_Revision_PARAM "Cust_App_Ntrip.RTCM_Revision"
#define Ntrip_Cust_App_Ntrip_RTCM_Revision_MIN 0
#define Ntrip_Cust_App_Ntrip_RTCM_Revision_DEF 2
#define Ntrip_Cust_App_Ntrip_RTCM_Revision_MAX 3

#define Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_PARAM "Cust_App_Ntrip.RTCM_MaxPayloads"
#define Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MIN 1
#define Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_DEF 5
#define Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MAX 5

typedef struct Ntrip_Cust_App_Ntrip
{
  /**
   * URI default is ""
   * Ntrip caster URI with the prefix "ntrip://"
   * e.g. "ntrip://username:password^ntrip.example.com:80/mountpoint"
   */
  const char * URI;

  /**
   * DebugLevel default is -1, min/max -1/255
   * if != -1, override debug level for Ntrip module only
   */
  int32_t DebugLevel;

  /**
   * GGA_Interval default is 5000, min/max 0/60000
   * Update interval for re-sending the local GGA string to the server [ms]
   */
  int32_t GGA_Interval;

  /**
   * WSM_TxPSID default is 0x8001ul, min/max 0x0ul/0xeffffffful
   *# WSM transmit parameters (when ProtocolMode = 1 AKA 1609) ##
   * PSID (p-encoded)
   */
  uint32_t WSM_TxPSID;

  /**
   * WSM_TxPrio default is 0, min/max 0/7
   * Priority in the range of 0 (lowest) through 7
   */
  int32_t WSM_TxPrio;

  /**
   * WSM_TxChannel default is 178, min/max 172/184
   * Channel number
   */
  int32_t WSM_TxChannel;

  /**
   * WSM_TxDataRate default is 12, min/max 5/54
   * Data rate (12 = 6Mbps)
   */
  int32_t WSM_TxDataRate;

  /**
   * WSM_TxPower default is 20, min/max 0/33
   * The transmit power used for the WSM [dBm] (J2945/1 2016 Table 21)
   */
  int32_t WSM_TxPower;

  /**
   * HopLimit default is 10, min/max 0/255
   *# ETSIRTCM transmit parameters (when ProtocolMode = 0 AKA ETSI) ##
   * Hop Limit
   */
  int32_t HopLimit;

  /**
   * RTCM_Revision default is 2, min/max 0/3
   * RTCM Revision
   * 0 - Unknown (ITSRTCM_Revision_unknown)
   * 1 - RTCM Revision 2 (ITSRTCM_Revision_rtcmRev2)
   * 2 - RTCM Revision 3 (ITSRTCM_Revision_rtcmRev3)
   * 3 - Reserved (ITSRTCM_Revision_reserved)
   */
  int32_t RTCM_Revision;

  /**
   * RTCM_MaxPayloads default is 5, min/max 1/5
   * RTCM3 payloads count
   * Maximum RTCM3 payloads per V2X RTCM message
   * Supporting more items could make V2X RTCM message larger than allowed MTU
   */
  int32_t RTCM_MaxPayloads;

  /**
   * ENABLE default is false
   * Ntrip enabled?
   */
  bool ENABLE;
} tNtrip_Cust_App_Ntrip;

#define Ntrip_Cust_App_Ntrip_NUM_SUB (0)
#define Ntrip_Cust_App_Ntrip_NUM_FLD (12)

/// compiled-in defaults
extern const tNtrip_Cust_App_Ntrip Ntrip_Cust_App_Ntrip_DEF;
/// Internals.... only for unit test...
const struct Conf_Callbacks * Ntrip_Callbacks(void);

/**
 * @brief Get a R/O copy.
 *
 * Release with @ref Ntrip_Release.
 *
 * @return Read-only instance of @ref Ntrip_Cust_App_Ntrip.
 */
const tNtrip_Cust_App_Ntrip * Ntrip_Get(void);

/**
 * @brief Get a R/W copy.
 *
 * Discard with @ref Ntrip_Cust_App_Ntrip_Clean
 * or commit and retain with with @ref Ntrip_Put.
 * or commit and discard with with @ref Ntrip_Push.
 *
 * @return Read/Write instance of @ref Ntrip_Cust_App_Ntrip.
 */
tNtrip_Cust_App_Ntrip * Ntrip_Clone(void);

/**
 * @brief Release a R/O copy.
 *
 * Discard with a copy obtained with @ref Ntrip_Get
 * *ppNtrip_Cust_App_Ntrip is set to NULL to help avoid use-after-free.
 *
 * @param ppNtrip_Cust_App_Ntrip Pointer to tNtrip_Cust_App_Ntrip to be cleaned up a f free(3)'d
 */
void Ntrip_Release(const tNtrip_Cust_App_Ntrip ** ppNtrip_Cust_App_Ntrip);

/**
 * @brief Commit a R/O copy.
 *
 * Deep clean pNtrip_Cust_App_Ntrip.
 * pNtrip_Cust_App_Ntrip is not altered or free(3)'d
 *
 * @param pNtrip_Cust_App_Ntrip Pointer to tNtrip_Cust_App_Ntrip to update parameters with.
 */
void Ntrip_Put(const tNtrip_Cust_App_Ntrip * pNtrip_Cust_App_Ntrip);

/**
 * @brief Commit and clean up ppNtrip_Cust_App_Ntrip.
 *
 * Applies *ppNtrip_Cust_App_Ntrip as the new configuration.
 * *ppNtrip_Cust_App_Ntrip is set to NULL to help avoid use-after-free.
 * Use with @ref Ntrip_Clone.
 *
 * @param ppNtrip_Cust_App_Ntrip Pointer to tNtrip_Cust_App_Ntrip to update parameters with and clean up.
 */
void Ntrip_Push(tNtrip_Cust_App_Ntrip ** ppNtrip_Cust_App_Ntrip);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Ntrip_Cust_App_Ntrip_Dump(
  FILE * pFile,
  const tNtrip_Cust_App_Ntrip * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Parse the given key/value
 *
 * @param pKey Key to assign it to
 * @param pValue Value to parse
 * @param pParse where to store it
 * @param Mode Deprecated?
 *
 * @return 0 if parsed without error
 */
int  Ntrip_Cust_App_Ntrip_Parse(
  const char * pKey,
  const char * pValue,
  tNtrip_Cust_App_Ntrip * pParse,
  int Mode);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Ntrip_Cust_App_Ntrip_Check(
  const tNtrip_Cust_App_Ntrip * pCheck);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Ntrip_Cust_App_Ntrip_Copy with pSrc set to NULL
 */
int  Ntrip_Cust_App_Ntrip_Init(
  tNtrip_Cust_App_Ntrip * pInit);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Ntrip_Cust_App_Ntrip_Clean(
  tNtrip_Cust_App_Ntrip * pClean);

/**
 * @brief Deep copy
 *
 * Calls @ref Ntrip_Cust_App_Ntrip_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Ntrip_Cust_App_Ntrip_Copy(
  tNtrip_Cust_App_Ntrip * pDst,
  const tNtrip_Cust_App_Ntrip * pSrc);

/**
 * @brief Deep Compare
 *
 * Note that floating point comparisons are used.
 *
 * @param pA one
 * @param pB the other one
 *
 * @return 0 iff *pA is identical to *pB
 */
int  Ntrip_Cust_App_Ntrip_Comp(
  const tNtrip_Cust_App_Ntrip * pA,
  const tNtrip_Cust_App_Ntrip * pB);

/**
 * @brief Update "DebugLevel"s
 *
 * update main DebugLevel and friends.
 *
 * @param pRO Config to update DebugLevels with
 * @param DebugLevel Default Debug Level
 */
void Ntrip_Cust_App_Ntrip_UpdateDebug(
  const tNtrip_Cust_App_Ntrip * pRO,
  int DebugLevel);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

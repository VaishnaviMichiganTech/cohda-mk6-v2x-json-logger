#pragma once
/**
 * @addtogroup conf_if_module_Example1609 Example1609 Configuration service
 * @{
 *
 * @file
 *
 * Configuration file parameters for Example1609
 */
//------------------------------------------------------------------------------
// Copyright (c) 2021 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------
#define Example1609_CRC ((uint64_t)(0x6b5cb17d85bb5ef1ull))
#include "Example1609_debug.h"

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

bool Example1609_CheckCRC(uint64_t CRC);

#define Example1609_Example1609_DebugLevel_PARAM "Example1609.DebugLevel"
#define Example1609_Example1609_DebugLevel_MIN -1
#define Example1609_Example1609_DebugLevel_DEF -1
#define Example1609_Example1609_DebugLevel_MAX 255

#define Example1609_Example1609_SDSM_Object_Port_PARAM "Example1609.SDSM_Object_Port"
#define Example1609_Example1609_SDSM_Object_Port_MIN 0
#define Example1609_Example1609_SDSM_Object_Port_DEF 3157
#define Example1609_Example1609_SDSM_Object_Port_MAX 65535


#define Example1609_Example1609_CANVState_DebugLevel_PARAM "Example1609.CANVState.DebugLevel"
#define Example1609_Example1609_CANVState_DebugLevel_MIN -1
#define Example1609_Example1609_CANVState_DebugLevel_DEF -1
#define Example1609_Example1609_CANVState_DebugLevel_MAX 255

typedef struct Example1609_Example1609_CANVState
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_CANVState;

#define Example1609_Example1609_CANVState_NUM_SUB (0)
#define Example1609_Example1609_CANVState_NUM_FLD (1)


#define Example1609_Example1609_EVWTx_DebugLevel_PARAM "Example1609.EVWTx.DebugLevel"
#define Example1609_Example1609_EVWTx_DebugLevel_MIN -1
#define Example1609_Example1609_EVWTx_DebugLevel_DEF -1
#define Example1609_Example1609_EVWTx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_EVWTx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_EVWTx;

#define Example1609_Example1609_EVWTx_NUM_SUB (0)
#define Example1609_Example1609_EVWTx_NUM_FLD (1)


#define Example1609_Example1609_MAPTx_DebugLevel_PARAM "Example1609.MAPTx.DebugLevel"
#define Example1609_Example1609_MAPTx_DebugLevel_MIN -1
#define Example1609_Example1609_MAPTx_DebugLevel_DEF -1
#define Example1609_Example1609_MAPTx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_MAPTx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_MAPTx;

#define Example1609_Example1609_MAPTx_NUM_SUB (0)
#define Example1609_Example1609_MAPTx_NUM_FLD (1)


#define Example1609_Example1609_Main_DebugLevel_PARAM "Example1609.Main.DebugLevel"
#define Example1609_Example1609_Main_DebugLevel_MIN -1
#define Example1609_Example1609_Main_DebugLevel_DEF -1
#define Example1609_Example1609_Main_DebugLevel_MAX 255

typedef struct Example1609_Example1609_Main
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_Main;

#define Example1609_Example1609_Main_NUM_SUB (0)
#define Example1609_Example1609_Main_NUM_FLD (1)


#define Example1609_Example1609_OBERx_DebugLevel_PARAM "Example1609.OBERx.DebugLevel"
#define Example1609_Example1609_OBERx_DebugLevel_MIN -1
#define Example1609_Example1609_OBERx_DebugLevel_DEF -1
#define Example1609_Example1609_OBERx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_OBERx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_OBERx;

#define Example1609_Example1609_OBERx_NUM_SUB (0)
#define Example1609_Example1609_OBERx_NUM_FLD (1)


#define Example1609_Example1609_PosExt_DebugLevel_PARAM "Example1609.PosExt.DebugLevel"
#define Example1609_Example1609_PosExt_DebugLevel_MIN -1
#define Example1609_Example1609_PosExt_DebugLevel_DEF -1
#define Example1609_Example1609_PosExt_DebugLevel_MAX 255

typedef struct Example1609_Example1609_PosExt
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_PosExt;

#define Example1609_Example1609_PosExt_NUM_SUB (0)
#define Example1609_Example1609_PosExt_NUM_FLD (1)


#define Example1609_Example1609_RawTx_DebugLevel_PARAM "Example1609.RawTx.DebugLevel"
#define Example1609_Example1609_RawTx_DebugLevel_MIN -1
#define Example1609_Example1609_RawTx_DebugLevel_DEF -1
#define Example1609_Example1609_RawTx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_RawTx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_RawTx;

#define Example1609_Example1609_RawTx_NUM_SUB (0)
#define Example1609_Example1609_RawTx_NUM_FLD (1)


#define Example1609_Example1609_RSARx_DebugLevel_PARAM "Example1609.RSARx.DebugLevel"
#define Example1609_Example1609_RSARx_DebugLevel_MIN -1
#define Example1609_Example1609_RSARx_DebugLevel_DEF -1
#define Example1609_Example1609_RSARx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_RSARx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_RSARx;

#define Example1609_Example1609_RSARx_NUM_SUB (0)
#define Example1609_Example1609_RSARx_NUM_FLD (1)


#define Example1609_Example1609_RSATx_DebugLevel_PARAM "Example1609.RSATx.DebugLevel"
#define Example1609_Example1609_RSATx_DebugLevel_MIN -1
#define Example1609_Example1609_RSATx_DebugLevel_DEF -1
#define Example1609_Example1609_RSATx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_RSATx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_RSATx;

#define Example1609_Example1609_RSATx_NUM_SUB (0)
#define Example1609_Example1609_RSATx_NUM_FLD (1)


#define Example1609_Example1609_SPATTx_DebugLevel_PARAM "Example1609.SPATTx.DebugLevel"
#define Example1609_Example1609_SPATTx_DebugLevel_MIN -1
#define Example1609_Example1609_SPATTx_DebugLevel_DEF -1
#define Example1609_Example1609_SPATTx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_SPATTx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_SPATTx;

#define Example1609_Example1609_SPATTx_NUM_SUB (0)
#define Example1609_Example1609_SPATTx_NUM_FLD (1)


#define Example1609_Example1609_TCClient_DebugLevel_PARAM "Example1609.TCClient.DebugLevel"
#define Example1609_Example1609_TCClient_DebugLevel_MIN -1
#define Example1609_Example1609_TCClient_DebugLevel_DEF -1
#define Example1609_Example1609_TCClient_DebugLevel_MAX 255

typedef struct Example1609_Example1609_TCClient
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_TCClient;

#define Example1609_Example1609_TCClient_NUM_SUB (0)
#define Example1609_Example1609_TCClient_NUM_FLD (1)


#define Example1609_Example1609_TIMGenericRx_DebugLevel_PARAM "Example1609.TIMGenericRx.DebugLevel"
#define Example1609_Example1609_TIMGenericRx_DebugLevel_MIN -1
#define Example1609_Example1609_TIMGenericRx_DebugLevel_DEF -1
#define Example1609_Example1609_TIMGenericRx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_TIMGenericRx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_TIMGenericRx;

#define Example1609_Example1609_TIMGenericRx_NUM_SUB (0)
#define Example1609_Example1609_TIMGenericRx_NUM_FLD (1)


#define Example1609_Example1609_SDSMTx_DebugLevel_PARAM "Example1609.SDSMTx.DebugLevel"
#define Example1609_Example1609_SDSMTx_DebugLevel_MIN -1
#define Example1609_Example1609_SDSMTx_DebugLevel_DEF -1
#define Example1609_Example1609_SDSMTx_DebugLevel_MAX 255

typedef struct Example1609_Example1609_SDSMTx
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_SDSMTx;

#define Example1609_Example1609_SDSMTx_NUM_SUB (0)
#define Example1609_Example1609_SDSMTx_NUM_FLD (1)


#define Example1609_Example1609_RSM_DebugLevel_PARAM "Example1609.RSM.DebugLevel"
#define Example1609_Example1609_RSM_DebugLevel_MIN -1
#define Example1609_Example1609_RSM_DebugLevel_DEF -1
#define Example1609_Example1609_RSM_DebugLevel_MAX 255

typedef struct Example1609_Example1609_RSM
{
  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;
} tExample1609_Example1609_RSM;

#define Example1609_Example1609_RSM_NUM_SUB (0)
#define Example1609_Example1609_RSM_NUM_FLD (1)

typedef struct Example1609_Example1609
{
  tExample1609_Example1609_CANVState CANVState;

  tExample1609_Example1609_EVWTx EVWTx;

  tExample1609_Example1609_MAPTx MAPTx;

  tExample1609_Example1609_Main Main;

  tExample1609_Example1609_OBERx OBERx;

  tExample1609_Example1609_PosExt PosExt;

  tExample1609_Example1609_RawTx RawTx;

  tExample1609_Example1609_RSARx RSARx;

  tExample1609_Example1609_RSATx RSATx;

  tExample1609_Example1609_SPATTx SPATTx;

  tExample1609_Example1609_TCClient TCClient;

  tExample1609_Example1609_TIMGenericRx TIMGenericRx;

  tExample1609_Example1609_SDSMTx SDSMTx;

  tExample1609_Example1609_RSM RSM;

  /**
   * DebugLevel default is -1, min/max -1/255
   */
  int32_t DebugLevel;

  /**
   * SDSM_Object_Port default is 3157, min/max 0/65535
   * UDP port for reception of objects
   * data from camera classifier
   */
  int32_t SDSM_Object_Port;
} tExample1609_Example1609;

#define Example1609_Example1609_NUM_SUB (14)
#define Example1609_Example1609_NUM_FLD (2)

/// compiled-in defaults
extern const tExample1609_Example1609 Example1609_Example1609_DEF;
/// Internals.... only for unit test...
const struct Conf_Callbacks * Example1609_Callbacks(void);

/**
 * @brief Get a R/O copy.
 *
 * Release with @ref Example1609_Release.
 *
 * @return Read-only instance of @ref Example1609_Example1609.
 */
const tExample1609_Example1609 * Example1609_Get(void);

/**
 * @brief Get a R/W copy.
 *
 * Discard with @ref Example1609_Example1609_Clean
 * or commit and retain with with @ref Example1609_Put.
 * or commit and discard with with @ref Example1609_Push.
 *
 * @return Read/Write instance of @ref Example1609_Example1609.
 */
tExample1609_Example1609 * Example1609_Clone(void);

/**
 * @brief Release a R/O copy.
 *
 * Discard with a copy obtained with @ref Example1609_Get
 * *ppExample1609_Example1609 is set to NULL to help avoid use-after-free.
 *
 * @param ppExample1609_Example1609 Pointer to tExample1609_Example1609 to be cleaned up a f free(3)'d
 */
void Example1609_Release(const tExample1609_Example1609 ** ppExample1609_Example1609);

/**
 * @brief Commit a R/O copy.
 *
 * Deep clean pExample1609_Example1609.
 * pExample1609_Example1609 is not altered or free(3)'d
 *
 * @param pExample1609_Example1609 Pointer to tExample1609_Example1609 to update parameters with.
 */
void Example1609_Put(const tExample1609_Example1609 * pExample1609_Example1609);

/**
 * @brief Commit and clean up ppExample1609_Example1609.
 *
 * Applies *ppExample1609_Example1609 as the new configuration.
 * *ppExample1609_Example1609 is set to NULL to help avoid use-after-free.
 * Use with @ref Example1609_Clone.
 *
 * @param ppExample1609_Example1609 Pointer to tExample1609_Example1609 to update parameters with and clean up.
 */
void Example1609_Push(tExample1609_Example1609 ** ppExample1609_Example1609);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_Dump(
  FILE * pFile,
  const tExample1609_Example1609 * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_CANVState_Dump(
  FILE * pFile,
  const tExample1609_Example1609_CANVState * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_EVWTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_EVWTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_MAPTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_MAPTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_Main_Dump(
  FILE * pFile,
  const tExample1609_Example1609_Main * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_OBERx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_OBERx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_PosExt_Dump(
  FILE * pFile,
  const tExample1609_Example1609_PosExt * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_RSARx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSARx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_RSATx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSATx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_RSM_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSM * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_RawTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RawTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_SDSMTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_SDSMTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_SPATTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_SPATTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_TCClient_Dump(
  FILE * pFile,
  const tExample1609_Example1609_TCClient * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits);

/**
 * @brief Write to file
 *
 * @param pFile where to dump
 * @param pDump what to dump
 * @param SkipDefault skip values at their default
 * @param SkipDisabled skip structure where DISABLED is false
 * @param ShowLimits show min/max
 */
void Example1609_Example1609_TIMGenericRx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_TIMGenericRx * pDump,
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
int  Example1609_Example1609_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609 * pParse,
  int Mode);

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
int  Example1609_Example1609_CANVState_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_CANVState * pParse,
  int Mode);

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
int  Example1609_Example1609_EVWTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_EVWTx * pParse,
  int Mode);

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
int  Example1609_Example1609_MAPTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_MAPTx * pParse,
  int Mode);

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
int  Example1609_Example1609_Main_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_Main * pParse,
  int Mode);

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
int  Example1609_Example1609_OBERx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_OBERx * pParse,
  int Mode);

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
int  Example1609_Example1609_PosExt_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_PosExt * pParse,
  int Mode);

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
int  Example1609_Example1609_RSARx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSARx * pParse,
  int Mode);

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
int  Example1609_Example1609_RSATx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSATx * pParse,
  int Mode);

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
int  Example1609_Example1609_RSM_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSM * pParse,
  int Mode);

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
int  Example1609_Example1609_RawTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RawTx * pParse,
  int Mode);

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
int  Example1609_Example1609_SDSMTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_SDSMTx * pParse,
  int Mode);

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
int  Example1609_Example1609_SPATTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_SPATTx * pParse,
  int Mode);

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
int  Example1609_Example1609_TCClient_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_TCClient * pParse,
  int Mode);

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
int  Example1609_Example1609_TIMGenericRx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_TIMGenericRx * pParse,
  int Mode);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_Check(
  const tExample1609_Example1609 * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_CANVState_Check(
  const tExample1609_Example1609_CANVState * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_EVWTx_Check(
  const tExample1609_Example1609_EVWTx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_MAPTx_Check(
  const tExample1609_Example1609_MAPTx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_Main_Check(
  const tExample1609_Example1609_Main * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_OBERx_Check(
  const tExample1609_Example1609_OBERx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_PosExt_Check(
  const tExample1609_Example1609_PosExt * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_RSARx_Check(
  const tExample1609_Example1609_RSARx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_RSATx_Check(
  const tExample1609_Example1609_RSATx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_RSM_Check(
  const tExample1609_Example1609_RSM * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_RawTx_Check(
  const tExample1609_Example1609_RawTx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_SDSMTx_Check(
  const tExample1609_Example1609_SDSMTx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_SPATTx_Check(
  const tExample1609_Example1609_SPATTx * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_TCClient_Check(
  const tExample1609_Example1609_TCClient * pCheck);

/**
 * @brief Sanity Check
 *
 * @param pCheck what to check
 *
 * @return 0 iff sane
 */
int  Example1609_Example1609_TIMGenericRx_Check(
  const tExample1609_Example1609_TIMGenericRx * pCheck);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_Init(
  tExample1609_Example1609 * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_CANVState_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_CANVState_Init(
  tExample1609_Example1609_CANVState * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_EVWTx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_EVWTx_Init(
  tExample1609_Example1609_EVWTx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_MAPTx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_MAPTx_Init(
  tExample1609_Example1609_MAPTx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_Main_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_Main_Init(
  tExample1609_Example1609_Main * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_OBERx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_OBERx_Init(
  tExample1609_Example1609_OBERx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_PosExt_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_PosExt_Init(
  tExample1609_Example1609_PosExt * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_RSARx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_RSARx_Init(
  tExample1609_Example1609_RSARx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_RSATx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_RSATx_Init(
  tExample1609_Example1609_RSATx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_RSM_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_RSM_Init(
  tExample1609_Example1609_RSM * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_RawTx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_RawTx_Init(
  tExample1609_Example1609_RawTx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_SDSMTx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_SDSMTx_Init(
  tExample1609_Example1609_SDSMTx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_SPATTx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_SPATTx_Init(
  tExample1609_Example1609_SPATTx * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_TCClient_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_TCClient_Init(
  tExample1609_Example1609_TCClient * pInit);

/**
 * @brief Load with compiled-in defaults
 *
 * This is probably not what you want.
 * Try @ref Example1609_Example1609_TIMGenericRx_Copy with pSrc set to NULL
 */
int  Example1609_Example1609_TIMGenericRx_Init(
  tExample1609_Example1609_TIMGenericRx * pInit);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_Clean(
  tExample1609_Example1609 * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_CANVState_Clean(
  tExample1609_Example1609_CANVState * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_EVWTx_Clean(
  tExample1609_Example1609_EVWTx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_MAPTx_Clean(
  tExample1609_Example1609_MAPTx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_Main_Clean(
  tExample1609_Example1609_Main * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_OBERx_Clean(
  tExample1609_Example1609_OBERx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_PosExt_Clean(
  tExample1609_Example1609_PosExt * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_RSARx_Clean(
  tExample1609_Example1609_RSARx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_RSATx_Clean(
  tExample1609_Example1609_RSATx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_RSM_Clean(
  tExample1609_Example1609_RSM * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_RawTx_Clean(
  tExample1609_Example1609_RawTx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_SDSMTx_Clean(
  tExample1609_Example1609_SDSMTx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_SPATTx_Clean(
  tExample1609_Example1609_SPATTx * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_TCClient_Clean(
  tExample1609_Example1609_TCClient * pClean);

/**
 * @brief Clean up
 *
 * pClean is not free(d)'d
 *
 * @param pClean what to clean
 */
void Example1609_Example1609_TIMGenericRx_Clean(
  tExample1609_Example1609_TIMGenericRx * pClean);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_Copy(
  tExample1609_Example1609 * pDst,
  const tExample1609_Example1609 * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_CANVState_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_CANVState_Copy(
  tExample1609_Example1609_CANVState * pDst,
  const tExample1609_Example1609_CANVState * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_EVWTx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_EVWTx_Copy(
  tExample1609_Example1609_EVWTx * pDst,
  const tExample1609_Example1609_EVWTx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_MAPTx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_MAPTx_Copy(
  tExample1609_Example1609_MAPTx * pDst,
  const tExample1609_Example1609_MAPTx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_Main_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_Main_Copy(
  tExample1609_Example1609_Main * pDst,
  const tExample1609_Example1609_Main * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_OBERx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_OBERx_Copy(
  tExample1609_Example1609_OBERx * pDst,
  const tExample1609_Example1609_OBERx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_PosExt_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_PosExt_Copy(
  tExample1609_Example1609_PosExt * pDst,
  const tExample1609_Example1609_PosExt * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_RSARx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_RSARx_Copy(
  tExample1609_Example1609_RSARx * pDst,
  const tExample1609_Example1609_RSARx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_RSATx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_RSATx_Copy(
  tExample1609_Example1609_RSATx * pDst,
  const tExample1609_Example1609_RSATx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_RSM_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_RSM_Copy(
  tExample1609_Example1609_RSM * pDst,
  const tExample1609_Example1609_RSM * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_RawTx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_RawTx_Copy(
  tExample1609_Example1609_RawTx * pDst,
  const tExample1609_Example1609_RawTx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_SDSMTx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_SDSMTx_Copy(
  tExample1609_Example1609_SDSMTx * pDst,
  const tExample1609_Example1609_SDSMTx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_SPATTx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_SPATTx_Copy(
  tExample1609_Example1609_SPATTx * pDst,
  const tExample1609_Example1609_SPATTx * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_TCClient_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_TCClient_Copy(
  tExample1609_Example1609_TCClient * pDst,
  const tExample1609_Example1609_TCClient * pSrc);

/**
 * @brief Deep copy
 *
 * Calls @ref Example1609_Example1609_TIMGenericRx_Clean on pSrc
 * Use current settings if pSrc is NULL
 */
int  Example1609_Example1609_TIMGenericRx_Copy(
  tExample1609_Example1609_TIMGenericRx * pDst,
  const tExample1609_Example1609_TIMGenericRx * pSrc);

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
int  Example1609_Example1609_Comp(
  const tExample1609_Example1609 * pA,
  const tExample1609_Example1609 * pB);

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
int  Example1609_Example1609_CANVState_Comp(
  const tExample1609_Example1609_CANVState * pA,
  const tExample1609_Example1609_CANVState * pB);

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
int  Example1609_Example1609_EVWTx_Comp(
  const tExample1609_Example1609_EVWTx * pA,
  const tExample1609_Example1609_EVWTx * pB);

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
int  Example1609_Example1609_MAPTx_Comp(
  const tExample1609_Example1609_MAPTx * pA,
  const tExample1609_Example1609_MAPTx * pB);

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
int  Example1609_Example1609_Main_Comp(
  const tExample1609_Example1609_Main * pA,
  const tExample1609_Example1609_Main * pB);

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
int  Example1609_Example1609_OBERx_Comp(
  const tExample1609_Example1609_OBERx * pA,
  const tExample1609_Example1609_OBERx * pB);

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
int  Example1609_Example1609_PosExt_Comp(
  const tExample1609_Example1609_PosExt * pA,
  const tExample1609_Example1609_PosExt * pB);

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
int  Example1609_Example1609_RSARx_Comp(
  const tExample1609_Example1609_RSARx * pA,
  const tExample1609_Example1609_RSARx * pB);

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
int  Example1609_Example1609_RSATx_Comp(
  const tExample1609_Example1609_RSATx * pA,
  const tExample1609_Example1609_RSATx * pB);

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
int  Example1609_Example1609_RSM_Comp(
  const tExample1609_Example1609_RSM * pA,
  const tExample1609_Example1609_RSM * pB);

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
int  Example1609_Example1609_RawTx_Comp(
  const tExample1609_Example1609_RawTx * pA,
  const tExample1609_Example1609_RawTx * pB);

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
int  Example1609_Example1609_SDSMTx_Comp(
  const tExample1609_Example1609_SDSMTx * pA,
  const tExample1609_Example1609_SDSMTx * pB);

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
int  Example1609_Example1609_SPATTx_Comp(
  const tExample1609_Example1609_SPATTx * pA,
  const tExample1609_Example1609_SPATTx * pB);

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
int  Example1609_Example1609_TCClient_Comp(
  const tExample1609_Example1609_TCClient * pA,
  const tExample1609_Example1609_TCClient * pB);

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
int  Example1609_Example1609_TIMGenericRx_Comp(
  const tExample1609_Example1609_TIMGenericRx * pA,
  const tExample1609_Example1609_TIMGenericRx * pB);

/**
 * @brief Update "DebugLevel"s
 *
 * update main DebugLevel and friends.
 *
 * @param pRO Config to update DebugLevels with
 * @param DebugLevel Default Debug Level
 */
void Example1609_Example1609_UpdateDebug(
  const tExample1609_Example1609 * pRO,
  int DebugLevel);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

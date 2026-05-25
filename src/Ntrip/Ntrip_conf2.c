#include "Ntrip_defn.h"

#include "conf_common.h"
#include "conf_if.h"
#include "conf_internal.h"

#include <stdlib.h>

static const tConf_Callbacks Callbacks_Ntrip_Cust_App_Ntrip =
{
  .CRC                  = Ntrip_CRC, // 0xc5822991b87b06d1
  .pConf_Callback_Check = (fConf_Callback_Check *)Ntrip_Cust_App_Ntrip_Check,
  .pConf_Callback_Clean = (fConf_Callback_Clean *)Ntrip_Cust_App_Ntrip_Clean,
  .pConf_Callback_Comp  = (fConf_Callback_Comp  *)Ntrip_Cust_App_Ntrip_Comp,
  .pConf_Callback_Copy  = (fConf_Callback_Copy  *)Ntrip_Cust_App_Ntrip_Copy,
  .pConf_Callback_Dump  = (fConf_Callback_Dump  *)Ntrip_Cust_App_Ntrip_Dump,
  .pConf_Callback_Init  = (fConf_Callback_Init  *)Ntrip_Cust_App_Ntrip_Init,
  .pConf_Callback_Parse = (fConf_Callback_Parse *)Ntrip_Cust_App_Ntrip_Parse,
  .pConf_Callback_Debug = (fConf_Callback_Debug *)(void*)Ntrip_Cust_App_Ntrip_UpdateDebug,
  .pPrefix              = "Cust_App_Ntrip.",
  .Size                 = sizeof(tNtrip_Cust_App_Ntrip),
///  .MaxParamNameLen      = 31,
};

static struct Conf_Registered * pConf_Registered = NULL;

bool Ntrip_CheckCRC(uint64_t CRC)
{
  return ((Ntrip_CRC == 0xc5822991b87b06d1ull)
          &&
          (Ntrip_CRC == CRC));
}

const tNtrip_Cust_App_Ntrip * Ntrip_Get(void)
{
  const void * pConf = Conf2_Get(pConf_Registered);
  const tNtrip_Cust_App_Ntrip * pNtrip_Cust_App_Ntrip  = (const tNtrip_Cust_App_Ntrip *)pConf;
  if (pNtrip_Cust_App_Ntrip == NULL)
  {
    return &Ntrip_Cust_App_Ntrip_DEF;
  }
  return pNtrip_Cust_App_Ntrip;
}

tNtrip_Cust_App_Ntrip * Ntrip_Clone(void)
{
  const tNtrip_Cust_App_Ntrip * pGet = Ntrip_Get();
  if (pGet == NULL)
  {
    return NULL;
  }
  tNtrip_Cust_App_Ntrip * pClone = (tNtrip_Cust_App_Ntrip *)calloc(1, sizeof(tNtrip_Cust_App_Ntrip));
  CONF_DBG_ALLOC(pClone, tNtrip_Cust_App_Ntrip);
  if (pClone != NULL)
  {
    int Res = Ntrip_Cust_App_Ntrip_Copy(pClone, pGet);
    if (Res != 0)
    {
      Ntrip_Cust_App_Ntrip_Clean(pClone);
      free(pClone);
      pClone = NULL;
    }
  }
  Ntrip_Release(&pGet);
  return pClone;
}

void Ntrip_Release(const tNtrip_Cust_App_Ntrip ** ppNtrip_Cust_App_Ntrip)
{
  if ((ppNtrip_Cust_App_Ntrip == NULL) || (*ppNtrip_Cust_App_Ntrip == NULL))
  {
    return;
  }
  if (*ppNtrip_Cust_App_Ntrip == &Ntrip_Cust_App_Ntrip_DEF)
  {
    *ppNtrip_Cust_App_Ntrip = NULL;
    return;
  }
  Conf2_Release(pConf_Registered, (const void **)ppNtrip_Cust_App_Ntrip);
}

void Ntrip_Put(const tNtrip_Cust_App_Ntrip * pNtrip_Cust_App_Ntrip)
{
  Conf2_Put(pConf_Registered, (const void *)pNtrip_Cust_App_Ntrip);
}

void Ntrip_Push(tNtrip_Cust_App_Ntrip ** ppNtrip_Cust_App_Ntrip)
{
  if ((ppNtrip_Cust_App_Ntrip  != NULL)
      &&
      (*ppNtrip_Cust_App_Ntrip != NULL))
  {
    Ntrip_Put(*ppNtrip_Cust_App_Ntrip);
    Ntrip_Cust_App_Ntrip_Clean(*ppNtrip_Cust_App_Ntrip);
    free(*ppNtrip_Cust_App_Ntrip);
    *ppNtrip_Cust_App_Ntrip = NULL;
  }
}

static void Ntrip_Load(void)   __attribute__((constructor));
static void Ntrip_UnLoad(void) __attribute__((destructor));
void Ntrip_Load(void)
{
  Conf2_Init("Ntrip");
  pConf_Registered = Conf2_Register(&Callbacks_Ntrip_Cust_App_Ntrip);
}

void Ntrip_UnLoad(void)
{
  Conf2_DeRegister(&Callbacks_Ntrip_Cust_App_Ntrip);
  Conf2_Exit("Ntrip");
}
void Ntrip_Cust_App_Ntrip_Dump(
  FILE * pFile,
  const tNtrip_Cust_App_Ntrip * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  if (SkipDisabled && !pDump->ENABLE) return;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 31;
  Dump_Debug(pFile, "Cust_App_Ntrip.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
  Dump_bool(pFile, "Cust_App_Ntrip.ENABLE", pDump->ENABLE, false, /*MaxParamNameLen, */SkipDefault);
  Dump_String(pFile, "Cust_App_Ntrip.URI", pDump->URI, "", /*MaxParamNameLen, */SkipDefault);
  Dump_int32_t(pFile, "Cust_App_Ntrip.GGA_Interval", pDump->GGA_Interval, 5000, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 60000);
  Dump_uint32_t(pFile, "Cust_App_Ntrip.WSM_TxPSID", pDump->WSM_TxPSID, 0x8001ul, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0x0ul, 0xeffffffful);
  Dump_int32_t(pFile, "Cust_App_Ntrip.WSM_TxPrio", pDump->WSM_TxPrio, 0, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 7);
  Dump_int32_t(pFile, "Cust_App_Ntrip.WSM_TxChannel", pDump->WSM_TxChannel, 178, /*MaxParamNameLen, */SkipDefault, ShowLimits, 172, 184);
  Dump_int32_t(pFile, "Cust_App_Ntrip.WSM_TxDataRate", pDump->WSM_TxDataRate, 12, /*MaxParamNameLen, */SkipDefault, ShowLimits, 5, 54);
  Dump_int32_t(pFile, "Cust_App_Ntrip.WSM_TxPower", pDump->WSM_TxPower, 20, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 33);
  Dump_int32_t(pFile, "Cust_App_Ntrip.HopLimit", pDump->HopLimit, 10, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 255);
  Dump_int32_t(pFile, "Cust_App_Ntrip.RTCM_Revision", pDump->RTCM_Revision, 2, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 3);
  Dump_int32_t(pFile, "Cust_App_Ntrip.RTCM_MaxPayloads", pDump->RTCM_MaxPayloads, 5, /*MaxParamNameLen, */SkipDefault, ShowLimits, 1, 5);
}

int Ntrip_Cust_App_Ntrip_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Ntrip_Cust_App_Ntrip_debug(
  const tNtrip_Cust_App_Ntrip * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Ntrip_Cust_App_Ntrip_DebugLevel = DebugLevel;
}
void Ntrip_Cust_App_Ntrip_UpdateDebug(
  const tNtrip_Cust_App_Ntrip * pRO,
  int DebugLevel)
{
  if ((DebugLevel < 0) || (DebugLevel > 255))
  {
    DebugLevel = 4;
  }
  _Ntrip_Cust_App_Ntrip_debug(pRO, DebugLevel);
}
int  Ntrip_Cust_App_Ntrip_Parse(
  const char * pKey,
  const char * pValue,
  tNtrip_Cust_App_Ntrip * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Ntrip_Cust_App_Ntrip_DebugLevel_MIN, Ntrip_Cust_App_Ntrip_DebugLevel_MAX, Mode);
  }
  if (!strcmp(pKey, "ENABLE"))
  {
    return Parse_bool(pValue, &pParse->ENABLE);
  }
  if (!strcmp(pKey, "URI"))
  {
    return Parse_String(pValue, &pParse->URI);
  }
  if (!strcmp(pKey, "GGA_Interval"))
  {
    return Parse_int32_t(pValue, &pParse->GGA_Interval, Ntrip_Cust_App_Ntrip_GGA_Interval_MIN, Ntrip_Cust_App_Ntrip_GGA_Interval_MAX, Mode);
  }
  if (!strcmp(pKey, "WSM_TxPSID"))
  {
    return Parse_uint32_t(pValue, &pParse->WSM_TxPSID, Ntrip_Cust_App_Ntrip_WSM_TxPSID_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPSID_MAX, Mode);
  }
  if (!strcmp(pKey, "WSM_TxPrio"))
  {
    return Parse_int32_t(pValue, &pParse->WSM_TxPrio, Ntrip_Cust_App_Ntrip_WSM_TxPrio_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPrio_MAX, Mode);
  }
  if (!strcmp(pKey, "WSM_TxChannel"))
  {
    return Parse_int32_t(pValue, &pParse->WSM_TxChannel, Ntrip_Cust_App_Ntrip_WSM_TxChannel_MIN, Ntrip_Cust_App_Ntrip_WSM_TxChannel_MAX, Mode);
  }
  if (!strcmp(pKey, "WSM_TxDataRate"))
  {
    return Parse_int32_t(pValue, &pParse->WSM_TxDataRate, Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MIN, Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MAX, Mode);
  }
  if (!strcmp(pKey, "WSM_TxPower"))
  {
    return Parse_int32_t(pValue, &pParse->WSM_TxPower, Ntrip_Cust_App_Ntrip_WSM_TxPower_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPower_MAX, Mode);
  }
  if (!strcmp(pKey, "HopLimit"))
  {
    return Parse_int32_t(pValue, &pParse->HopLimit, Ntrip_Cust_App_Ntrip_HopLimit_MIN, Ntrip_Cust_App_Ntrip_HopLimit_MAX, Mode);
  }
  if (!strcmp(pKey, "RTCM_Revision"))
  {
    return Parse_int32_t(pValue, &pParse->RTCM_Revision, Ntrip_Cust_App_Ntrip_RTCM_Revision_MIN, Ntrip_Cust_App_Ntrip_RTCM_Revision_MAX, Mode);
  }
  if (!strcmp(pKey, "RTCM_MaxPayloads"))
  {
    return Parse_int32_t(pValue, &pParse->RTCM_MaxPayloads, Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MIN, Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Ntrip_Cust_App_Ntrip_Check(
  const tNtrip_Cust_App_Ntrip * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Ntrip_Cust_App_Ntrip_DebugLevel_MIN, Ntrip_Cust_App_Ntrip_DebugLevel_MAX, Ntrip_Cust_App_Ntrip_DebugLevel_PARAM);
  ret |= Check_int32_t(pCheck->GGA_Interval, Ntrip_Cust_App_Ntrip_GGA_Interval_MIN, Ntrip_Cust_App_Ntrip_GGA_Interval_MAX, Ntrip_Cust_App_Ntrip_GGA_Interval_PARAM);
  ret |= Check_uint32_t(pCheck->WSM_TxPSID, Ntrip_Cust_App_Ntrip_WSM_TxPSID_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPSID_MAX, Ntrip_Cust_App_Ntrip_WSM_TxPSID_PARAM);
  ret |= Check_int32_t(pCheck->WSM_TxPrio, Ntrip_Cust_App_Ntrip_WSM_TxPrio_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPrio_MAX, Ntrip_Cust_App_Ntrip_WSM_TxPrio_PARAM);
  ret |= Check_int32_t(pCheck->WSM_TxChannel, Ntrip_Cust_App_Ntrip_WSM_TxChannel_MIN, Ntrip_Cust_App_Ntrip_WSM_TxChannel_MAX, Ntrip_Cust_App_Ntrip_WSM_TxChannel_PARAM);
  ret |= Check_int32_t(pCheck->WSM_TxDataRate, Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MIN, Ntrip_Cust_App_Ntrip_WSM_TxDataRate_MAX, Ntrip_Cust_App_Ntrip_WSM_TxDataRate_PARAM);
  ret |= Check_int32_t(pCheck->WSM_TxPower, Ntrip_Cust_App_Ntrip_WSM_TxPower_MIN, Ntrip_Cust_App_Ntrip_WSM_TxPower_MAX, Ntrip_Cust_App_Ntrip_WSM_TxPower_PARAM);
  ret |= Check_int32_t(pCheck->HopLimit, Ntrip_Cust_App_Ntrip_HopLimit_MIN, Ntrip_Cust_App_Ntrip_HopLimit_MAX, Ntrip_Cust_App_Ntrip_HopLimit_PARAM);
  ret |= Check_int32_t(pCheck->RTCM_Revision, Ntrip_Cust_App_Ntrip_RTCM_Revision_MIN, Ntrip_Cust_App_Ntrip_RTCM_Revision_MAX, Ntrip_Cust_App_Ntrip_RTCM_Revision_PARAM);
  ret |= Check_int32_t(pCheck->RTCM_MaxPayloads, Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MIN, Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MAX, Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_PARAM);
  return ret;
}
extern const tNtrip_Cust_App_Ntrip Ntrip_Cust_App_Ntrip_DEF;
int  Ntrip_Cust_App_Ntrip_Init(
  tNtrip_Cust_App_Ntrip * pInit)
{
  return Ntrip_Cust_App_Ntrip_Copy(pInit, &Ntrip_Cust_App_Ntrip_DEF);
}
void Ntrip_Cust_App_Ntrip_Clean(
  tNtrip_Cust_App_Ntrip * pClean)
{
  if (pClean == NULL) return;
  Clean_String(&pClean->URI);
}
int  Ntrip_Cust_App_Ntrip_Copy(
  tNtrip_Cust_App_Ntrip * pDst,
  const tNtrip_Cust_App_Ntrip * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tNtrip_Cust_App_Ntrip * pNtrip = Ntrip_Get();
    if (pNtrip == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = pNtrip;
    Res = Ntrip_Cust_App_Ntrip_Copy(pDst, pSrc);
    Ntrip_Release(&pNtrip);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  pDst->ENABLE = pSrc->ENABLE;
  Res |=   Copy_String(&pDst->URI, pSrc->URI);
  pDst->GGA_Interval = pSrc->GGA_Interval;
  pDst->WSM_TxPSID = pSrc->WSM_TxPSID;
  pDst->WSM_TxPrio = pSrc->WSM_TxPrio;
  pDst->WSM_TxChannel = pSrc->WSM_TxChannel;
  pDst->WSM_TxDataRate = pSrc->WSM_TxDataRate;
  pDst->WSM_TxPower = pSrc->WSM_TxPower;
  pDst->HopLimit = pSrc->HopLimit;
  pDst->RTCM_Revision = pSrc->RTCM_Revision;
  pDst->RTCM_MaxPayloads = pSrc->RTCM_MaxPayloads;
  return Res;
}
int  Ntrip_Cust_App_Ntrip_Comp(
  const tNtrip_Cust_App_Ntrip * pA,
  const tNtrip_Cust_App_Ntrip * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  Res = Comp_bool(pA->ENABLE, pB->ENABLE);
  if (Res != 0) return Res;
  Res = Comp_String(pA->URI, pB->URI);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->GGA_Interval, pB->GGA_Interval);
  if (Res != 0) return Res;
  Res = Comp_uint32_t(pA->WSM_TxPSID, pB->WSM_TxPSID);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->WSM_TxPrio, pB->WSM_TxPrio);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->WSM_TxChannel, pB->WSM_TxChannel);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->WSM_TxDataRate, pB->WSM_TxDataRate);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->WSM_TxPower, pB->WSM_TxPower);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->HopLimit, pB->HopLimit);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->RTCM_Revision, pB->RTCM_Revision);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->RTCM_MaxPayloads, pB->RTCM_MaxPayloads);
  if (Res != 0) return Res;
  return Res;
}
const tNtrip_Cust_App_Ntrip Ntrip_Cust_App_Ntrip_DEF =
{
  .URI = Ntrip_Cust_App_Ntrip_URI_DEF,
  .DebugLevel = Ntrip_Cust_App_Ntrip_DebugLevel_DEF,
  .GGA_Interval = Ntrip_Cust_App_Ntrip_GGA_Interval_DEF,
  .WSM_TxPSID = Ntrip_Cust_App_Ntrip_WSM_TxPSID_DEF,
  .WSM_TxPrio = Ntrip_Cust_App_Ntrip_WSM_TxPrio_DEF,
  .WSM_TxChannel = Ntrip_Cust_App_Ntrip_WSM_TxChannel_DEF,
  .WSM_TxDataRate = Ntrip_Cust_App_Ntrip_WSM_TxDataRate_DEF,
  .WSM_TxPower = Ntrip_Cust_App_Ntrip_WSM_TxPower_DEF,
  .HopLimit = Ntrip_Cust_App_Ntrip_HopLimit_DEF,
  .RTCM_Revision = Ntrip_Cust_App_Ntrip_RTCM_Revision_DEF,
  .RTCM_MaxPayloads = Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_DEF,
  .ENABLE = Ntrip_Cust_App_Ntrip_ENABLE_DEF,
};

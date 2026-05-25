#include "Example1609_defn.h"

#include "conf_common.h"
#include "conf_if.h"
#include "conf_internal.h"

#include <stdlib.h>

static const tConf_Callbacks Callbacks_Example1609_Example1609 =
{
  .CRC                  = Example1609_CRC, // 0x6b5cb17d85bb5ef1
  .pConf_Callback_Check = (fConf_Callback_Check *)Example1609_Example1609_Check,
  .pConf_Callback_Clean = (fConf_Callback_Clean *)Example1609_Example1609_Clean,
  .pConf_Callback_Comp  = (fConf_Callback_Comp  *)Example1609_Example1609_Comp,
  .pConf_Callback_Copy  = (fConf_Callback_Copy  *)Example1609_Example1609_Copy,
  .pConf_Callback_Dump  = (fConf_Callback_Dump  *)Example1609_Example1609_Dump,
  .pConf_Callback_Init  = (fConf_Callback_Init  *)Example1609_Example1609_Init,
  .pConf_Callback_Parse = (fConf_Callback_Parse *)Example1609_Example1609_Parse,
  .pConf_Callback_Debug = (fConf_Callback_Debug *)(void*)Example1609_Example1609_UpdateDebug,
  .pPrefix              = "Example1609.",
  .Size                 = sizeof(tExample1609_Example1609),
///  .MaxParamNameLen      = 35,
};

static struct Conf_Registered * pConf_Registered = NULL;

bool Example1609_CheckCRC(uint64_t CRC)
{
  return ((Example1609_CRC == 0x6b5cb17d85bb5ef1ull)
          &&
          (Example1609_CRC == CRC));
}

const tExample1609_Example1609 * Example1609_Get(void)
{
  const void * pConf = Conf2_Get(pConf_Registered);
  const tExample1609_Example1609 * pExample1609_Example1609  = (const tExample1609_Example1609 *)pConf;
  if (pExample1609_Example1609 == NULL)
  {
    return &Example1609_Example1609_DEF;
  }
  return pExample1609_Example1609;
}

tExample1609_Example1609 * Example1609_Clone(void)
{
  const tExample1609_Example1609 * pGet = Example1609_Get();
  if (pGet == NULL)
  {
    return NULL;
  }
  tExample1609_Example1609 * pClone = (tExample1609_Example1609 *)calloc(1, sizeof(tExample1609_Example1609));
  CONF_DBG_ALLOC(pClone, tExample1609_Example1609);
  if (pClone != NULL)
  {
    int Res = Example1609_Example1609_Copy(pClone, pGet);
    if (Res != 0)
    {
      Example1609_Example1609_Clean(pClone);
      free(pClone);
      pClone = NULL;
    }
  }
  Example1609_Release(&pGet);
  return pClone;
}

void Example1609_Release(const tExample1609_Example1609 ** ppExample1609_Example1609)
{
  if ((ppExample1609_Example1609 == NULL) || (*ppExample1609_Example1609 == NULL))
  {
    return;
  }
  if (*ppExample1609_Example1609 == &Example1609_Example1609_DEF)
  {
    *ppExample1609_Example1609 = NULL;
    return;
  }
  Conf2_Release(pConf_Registered, (const void **)ppExample1609_Example1609);
}

void Example1609_Put(const tExample1609_Example1609 * pExample1609_Example1609)
{
  Conf2_Put(pConf_Registered, (const void *)pExample1609_Example1609);
}

void Example1609_Push(tExample1609_Example1609 ** ppExample1609_Example1609)
{
  if ((ppExample1609_Example1609  != NULL)
      &&
      (*ppExample1609_Example1609 != NULL))
  {
    Example1609_Put(*ppExample1609_Example1609);
    Example1609_Example1609_Clean(*ppExample1609_Example1609);
    free(*ppExample1609_Example1609);
    *ppExample1609_Example1609 = NULL;
  }
}

static void Example1609_Load(void)   __attribute__((constructor));
static void Example1609_UnLoad(void) __attribute__((destructor));
void Example1609_Load(void)
{
  Conf2_Init("Example1609");
  pConf_Registered = Conf2_Register(&Callbacks_Example1609_Example1609);
}

void Example1609_UnLoad(void)
{
  Conf2_DeRegister(&Callbacks_Example1609_Example1609);
  Conf2_Exit("Example1609");
}
void Example1609_Example1609_Dump(
  FILE * pFile,
  const tExample1609_Example1609 * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
  Example1609_Example1609_CANVState_Dump(pFile, &pDump->CANVState, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_EVWTx_Dump(pFile, &pDump->EVWTx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_MAPTx_Dump(pFile, &pDump->MAPTx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_Main_Dump(pFile, &pDump->Main, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_OBERx_Dump(pFile, &pDump->OBERx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_PosExt_Dump(pFile, &pDump->PosExt, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_RawTx_Dump(pFile, &pDump->RawTx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_RSARx_Dump(pFile, &pDump->RSARx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_RSATx_Dump(pFile, &pDump->RSATx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_SPATTx_Dump(pFile, &pDump->SPATTx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_TCClient_Dump(pFile, &pDump->TCClient, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_TIMGenericRx_Dump(pFile, &pDump->TIMGenericRx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_SDSMTx_Dump(pFile, &pDump->SDSMTx, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Example1609_Example1609_RSM_Dump(pFile, &pDump->RSM, /*MaxParamNameLen,*/ SkipDefault, SkipDisabled, ShowLimits);
  Dump_int32_t(pFile, "Example1609.SDSM_Object_Port", pDump->SDSM_Object_Port, 3157, /*MaxParamNameLen, */SkipDefault, ShowLimits, 0, 65535);
}
void Example1609_Example1609_CANVState_Dump(
  FILE * pFile,
  const tExample1609_Example1609_CANVState * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.CANVState.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_EVWTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_EVWTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.EVWTx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_MAPTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_MAPTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.MAPTx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_Main_Dump(
  FILE * pFile,
  const tExample1609_Example1609_Main * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.Main.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_OBERx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_OBERx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.OBERx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_PosExt_Dump(
  FILE * pFile,
  const tExample1609_Example1609_PosExt * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.PosExt.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_RSARx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSARx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.RSARx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_RSATx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSATx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.RSATx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_RSM_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RSM * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.RSM.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_RawTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_RawTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.RawTx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_SDSMTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_SDSMTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.SDSMTx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_SPATTx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_SPATTx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.SPATTx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_TCClient_Dump(
  FILE * pFile,
  const tExample1609_Example1609_TCClient * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.TCClient.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}
void Example1609_Example1609_TIMGenericRx_Dump(
  FILE * pFile,
  const tExample1609_Example1609_TIMGenericRx * pDump,
  /*int MaxParamNameLen,*/
  bool SkipDefault,
  bool SkipDisabled,
  bool ShowLimits)
{
  if (pFile == NULL) return;
  if (pDump == NULL) return;
  (void)SkipDisabled;
  (void)ShowLimits;
///  MaxParamNameLen = MaxParamNameLen ?: 35;
  Dump_Debug(pFile, "Example1609.TIMGenericRx.DebugLevel", pDump->DebugLevel, -1, /*MaxParamNameLen, */SkipDefault, ShowLimits, -1, 255);
}

int Example1609_Example1609_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
int Example1609_Example1609_CANVState_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_CANVState_debug(
  const tExample1609_Example1609_CANVState * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_CANVState_DebugLevel = DebugLevel;
}
int Example1609_Example1609_EVWTx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_EVWTx_debug(
  const tExample1609_Example1609_EVWTx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_EVWTx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_MAPTx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_MAPTx_debug(
  const tExample1609_Example1609_MAPTx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_MAPTx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_Main_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_Main_debug(
  const tExample1609_Example1609_Main * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_Main_DebugLevel = DebugLevel;
}
int Example1609_Example1609_OBERx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_OBERx_debug(
  const tExample1609_Example1609_OBERx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_OBERx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_PosExt_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_PosExt_debug(
  const tExample1609_Example1609_PosExt * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_PosExt_DebugLevel = DebugLevel;
}
int Example1609_Example1609_RawTx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_RawTx_debug(
  const tExample1609_Example1609_RawTx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_RawTx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_RSARx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_RSARx_debug(
  const tExample1609_Example1609_RSARx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_RSARx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_RSATx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_RSATx_debug(
  const tExample1609_Example1609_RSATx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_RSATx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_SPATTx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_SPATTx_debug(
  const tExample1609_Example1609_SPATTx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_SPATTx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_TCClient_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_TCClient_debug(
  const tExample1609_Example1609_TCClient * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_TCClient_DebugLevel = DebugLevel;
}
int Example1609_Example1609_TIMGenericRx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_TIMGenericRx_debug(
  const tExample1609_Example1609_TIMGenericRx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_TIMGenericRx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_SDSMTx_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_SDSMTx_debug(
  const tExample1609_Example1609_SDSMTx * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_SDSMTx_DebugLevel = DebugLevel;
}
int Example1609_Example1609_RSM_DebugLevel = 4; //default_stack_Cohda_DebugLevel_WARN
static void _Example1609_Example1609_RSM_debug(
  const tExample1609_Example1609_RSM * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_RSM_DebugLevel = DebugLevel;
}
static void _Example1609_Example1609_debug(
  const tExample1609_Example1609 * pRO,
  int DebugLevel)
{
  (void)DebugLevel;
  if (pRO == NULL) return;
  if (pRO->DebugLevel != -1)
  {
    DebugLevel = pRO->DebugLevel;
  }
  Example1609_Example1609_DebugLevel = DebugLevel;
  _Example1609_Example1609_CANVState_debug(&pRO->CANVState, DebugLevel);
  _Example1609_Example1609_EVWTx_debug(&pRO->EVWTx, DebugLevel);
  _Example1609_Example1609_MAPTx_debug(&pRO->MAPTx, DebugLevel);
  _Example1609_Example1609_Main_debug(&pRO->Main, DebugLevel);
  _Example1609_Example1609_OBERx_debug(&pRO->OBERx, DebugLevel);
  _Example1609_Example1609_PosExt_debug(&pRO->PosExt, DebugLevel);
  _Example1609_Example1609_RawTx_debug(&pRO->RawTx, DebugLevel);
  _Example1609_Example1609_RSARx_debug(&pRO->RSARx, DebugLevel);
  _Example1609_Example1609_RSATx_debug(&pRO->RSATx, DebugLevel);
  _Example1609_Example1609_SPATTx_debug(&pRO->SPATTx, DebugLevel);
  _Example1609_Example1609_TCClient_debug(&pRO->TCClient, DebugLevel);
  _Example1609_Example1609_TIMGenericRx_debug(&pRO->TIMGenericRx, DebugLevel);
  _Example1609_Example1609_SDSMTx_debug(&pRO->SDSMTx, DebugLevel);
  _Example1609_Example1609_RSM_debug(&pRO->RSM, DebugLevel);
}
void Example1609_Example1609_UpdateDebug(
  const tExample1609_Example1609 * pRO,
  int DebugLevel)
{
  if ((DebugLevel < 0) || (DebugLevel > 255))
  {
    DebugLevel = 4;
  }
  _Example1609_Example1609_debug(pRO, DebugLevel);
}
int  Example1609_Example1609_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609 * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_DebugLevel_MIN, Example1609_Example1609_DebugLevel_MAX, Mode);
  }
  if (!strncmp(pKey, "CANVState.", 9 + 1))
  {
    return Example1609_Example1609_CANVState_Parse(pKey + (9 + 1), pValue, &pParse->CANVState, Mode);
  }
  if (!strncmp(pKey, "EVWTx.", 5 + 1))
  {
    return Example1609_Example1609_EVWTx_Parse(pKey + (5 + 1), pValue, &pParse->EVWTx, Mode);
  }
  if (!strncmp(pKey, "MAPTx.", 5 + 1))
  {
    return Example1609_Example1609_MAPTx_Parse(pKey + (5 + 1), pValue, &pParse->MAPTx, Mode);
  }
  if (!strncmp(pKey, "Main.", 4 + 1))
  {
    return Example1609_Example1609_Main_Parse(pKey + (4 + 1), pValue, &pParse->Main, Mode);
  }
  if (!strncmp(pKey, "OBERx.", 5 + 1))
  {
    return Example1609_Example1609_OBERx_Parse(pKey + (5 + 1), pValue, &pParse->OBERx, Mode);
  }
  if (!strncmp(pKey, "PosExt.", 6 + 1))
  {
    return Example1609_Example1609_PosExt_Parse(pKey + (6 + 1), pValue, &pParse->PosExt, Mode);
  }
  if (!strncmp(pKey, "RawTx.", 5 + 1))
  {
    return Example1609_Example1609_RawTx_Parse(pKey + (5 + 1), pValue, &pParse->RawTx, Mode);
  }
  if (!strncmp(pKey, "RSARx.", 5 + 1))
  {
    return Example1609_Example1609_RSARx_Parse(pKey + (5 + 1), pValue, &pParse->RSARx, Mode);
  }
  if (!strncmp(pKey, "RSATx.", 5 + 1))
  {
    return Example1609_Example1609_RSATx_Parse(pKey + (5 + 1), pValue, &pParse->RSATx, Mode);
  }
  if (!strncmp(pKey, "SPATTx.", 6 + 1))
  {
    return Example1609_Example1609_SPATTx_Parse(pKey + (6 + 1), pValue, &pParse->SPATTx, Mode);
  }
  if (!strncmp(pKey, "TCClient.", 8 + 1))
  {
    return Example1609_Example1609_TCClient_Parse(pKey + (8 + 1), pValue, &pParse->TCClient, Mode);
  }
  if (!strncmp(pKey, "TIMGenericRx.", 12 + 1))
  {
    return Example1609_Example1609_TIMGenericRx_Parse(pKey + (12 + 1), pValue, &pParse->TIMGenericRx, Mode);
  }
  if (!strncmp(pKey, "SDSMTx.", 6 + 1))
  {
    return Example1609_Example1609_SDSMTx_Parse(pKey + (6 + 1), pValue, &pParse->SDSMTx, Mode);
  }
  if (!strncmp(pKey, "RSM.", 3 + 1))
  {
    return Example1609_Example1609_RSM_Parse(pKey + (3 + 1), pValue, &pParse->RSM, Mode);
  }
  if (!strcmp(pKey, "SDSM_Object_Port"))
  {
    return Parse_int32_t(pValue, &pParse->SDSM_Object_Port, Example1609_Example1609_SDSM_Object_Port_MIN, Example1609_Example1609_SDSM_Object_Port_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_CANVState_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_CANVState * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_CANVState_DebugLevel_MIN, Example1609_Example1609_CANVState_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_EVWTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_EVWTx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_EVWTx_DebugLevel_MIN, Example1609_Example1609_EVWTx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_MAPTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_MAPTx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_MAPTx_DebugLevel_MIN, Example1609_Example1609_MAPTx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_Main_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_Main * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_Main_DebugLevel_MIN, Example1609_Example1609_Main_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_OBERx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_OBERx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_OBERx_DebugLevel_MIN, Example1609_Example1609_OBERx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_PosExt_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_PosExt * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_PosExt_DebugLevel_MIN, Example1609_Example1609_PosExt_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_RSARx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSARx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_RSARx_DebugLevel_MIN, Example1609_Example1609_RSARx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_RSATx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSATx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_RSATx_DebugLevel_MIN, Example1609_Example1609_RSATx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_RSM_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RSM * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_RSM_DebugLevel_MIN, Example1609_Example1609_RSM_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_RawTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_RawTx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_RawTx_DebugLevel_MIN, Example1609_Example1609_RawTx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_SDSMTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_SDSMTx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_SDSMTx_DebugLevel_MIN, Example1609_Example1609_SDSMTx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_SPATTx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_SPATTx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_SPATTx_DebugLevel_MIN, Example1609_Example1609_SPATTx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_TCClient_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_TCClient * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_TCClient_DebugLevel_MIN, Example1609_Example1609_TCClient_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_TIMGenericRx_Parse(
  const char * pKey,
  const char * pValue,
  tExample1609_Example1609_TIMGenericRx * pParse,
  int Mode)
{
  (void)Mode;
  if (pParse == NULL) return CONF_ERROR_NULL;
  if (!strcmp(pKey, "DebugLevel"))
  {
    return Parse_Debug(pValue, &pParse->DebugLevel, Example1609_Example1609_TIMGenericRx_DebugLevel_MIN, Example1609_Example1609_TIMGenericRx_DebugLevel_MAX, Mode);
  }
  return CONF_ERROR_UNK;
}
int  Example1609_Example1609_Check(
  const tExample1609_Example1609 * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_DebugLevel_MIN, Example1609_Example1609_DebugLevel_MAX, Example1609_Example1609_DebugLevel_PARAM);
  ret |= Example1609_Example1609_CANVState_Check(&pCheck->CANVState);
  ret |= Example1609_Example1609_EVWTx_Check(&pCheck->EVWTx);
  ret |= Example1609_Example1609_MAPTx_Check(&pCheck->MAPTx);
  ret |= Example1609_Example1609_Main_Check(&pCheck->Main);
  ret |= Example1609_Example1609_OBERx_Check(&pCheck->OBERx);
  ret |= Example1609_Example1609_PosExt_Check(&pCheck->PosExt);
  ret |= Example1609_Example1609_RawTx_Check(&pCheck->RawTx);
  ret |= Example1609_Example1609_RSARx_Check(&pCheck->RSARx);
  ret |= Example1609_Example1609_RSATx_Check(&pCheck->RSATx);
  ret |= Example1609_Example1609_SPATTx_Check(&pCheck->SPATTx);
  ret |= Example1609_Example1609_TCClient_Check(&pCheck->TCClient);
  ret |= Example1609_Example1609_TIMGenericRx_Check(&pCheck->TIMGenericRx);
  ret |= Example1609_Example1609_SDSMTx_Check(&pCheck->SDSMTx);
  ret |= Example1609_Example1609_RSM_Check(&pCheck->RSM);
  ret |= Check_int32_t(pCheck->SDSM_Object_Port, Example1609_Example1609_SDSM_Object_Port_MIN, Example1609_Example1609_SDSM_Object_Port_MAX, Example1609_Example1609_SDSM_Object_Port_PARAM);
  return ret;
}
int  Example1609_Example1609_CANVState_Check(
  const tExample1609_Example1609_CANVState * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_CANVState_DebugLevel_MIN, Example1609_Example1609_CANVState_DebugLevel_MAX, Example1609_Example1609_CANVState_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_EVWTx_Check(
  const tExample1609_Example1609_EVWTx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_EVWTx_DebugLevel_MIN, Example1609_Example1609_EVWTx_DebugLevel_MAX, Example1609_Example1609_EVWTx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_MAPTx_Check(
  const tExample1609_Example1609_MAPTx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_MAPTx_DebugLevel_MIN, Example1609_Example1609_MAPTx_DebugLevel_MAX, Example1609_Example1609_MAPTx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_Main_Check(
  const tExample1609_Example1609_Main * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_Main_DebugLevel_MIN, Example1609_Example1609_Main_DebugLevel_MAX, Example1609_Example1609_Main_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_OBERx_Check(
  const tExample1609_Example1609_OBERx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_OBERx_DebugLevel_MIN, Example1609_Example1609_OBERx_DebugLevel_MAX, Example1609_Example1609_OBERx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_PosExt_Check(
  const tExample1609_Example1609_PosExt * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_PosExt_DebugLevel_MIN, Example1609_Example1609_PosExt_DebugLevel_MAX, Example1609_Example1609_PosExt_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_RSARx_Check(
  const tExample1609_Example1609_RSARx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_RSARx_DebugLevel_MIN, Example1609_Example1609_RSARx_DebugLevel_MAX, Example1609_Example1609_RSARx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_RSATx_Check(
  const tExample1609_Example1609_RSATx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_RSATx_DebugLevel_MIN, Example1609_Example1609_RSATx_DebugLevel_MAX, Example1609_Example1609_RSATx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_RSM_Check(
  const tExample1609_Example1609_RSM * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_RSM_DebugLevel_MIN, Example1609_Example1609_RSM_DebugLevel_MAX, Example1609_Example1609_RSM_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_RawTx_Check(
  const tExample1609_Example1609_RawTx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_RawTx_DebugLevel_MIN, Example1609_Example1609_RawTx_DebugLevel_MAX, Example1609_Example1609_RawTx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_SDSMTx_Check(
  const tExample1609_Example1609_SDSMTx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_SDSMTx_DebugLevel_MIN, Example1609_Example1609_SDSMTx_DebugLevel_MAX, Example1609_Example1609_SDSMTx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_SPATTx_Check(
  const tExample1609_Example1609_SPATTx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_SPATTx_DebugLevel_MIN, Example1609_Example1609_SPATTx_DebugLevel_MAX, Example1609_Example1609_SPATTx_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_TCClient_Check(
  const tExample1609_Example1609_TCClient * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_TCClient_DebugLevel_MIN, Example1609_Example1609_TCClient_DebugLevel_MAX, Example1609_Example1609_TCClient_DebugLevel_PARAM);
  return ret;
}
int  Example1609_Example1609_TIMGenericRx_Check(
  const tExample1609_Example1609_TIMGenericRx * pCheck)
{
  int ret = 0;
  if (pCheck == NULL) return CONF_ERROR_NULL;
  ret |= Check_int32_t(pCheck->DebugLevel, Example1609_Example1609_TIMGenericRx_DebugLevel_MIN, Example1609_Example1609_TIMGenericRx_DebugLevel_MAX, Example1609_Example1609_TIMGenericRx_DebugLevel_PARAM);
  return ret;
}
extern const tExample1609_Example1609 Example1609_Example1609_DEF;
int  Example1609_Example1609_Init(
  tExample1609_Example1609 * pInit)
{
  return Example1609_Example1609_Copy(pInit, &Example1609_Example1609_DEF);
}
int  Example1609_Example1609_CANVState_Init(
  tExample1609_Example1609_CANVState * pInit)
{
  return Example1609_Example1609_CANVState_Copy(pInit, &Example1609_Example1609_DEF.CANVState);
}
int  Example1609_Example1609_EVWTx_Init(
  tExample1609_Example1609_EVWTx * pInit)
{
  return Example1609_Example1609_EVWTx_Copy(pInit, &Example1609_Example1609_DEF.EVWTx);
}
int  Example1609_Example1609_MAPTx_Init(
  tExample1609_Example1609_MAPTx * pInit)
{
  return Example1609_Example1609_MAPTx_Copy(pInit, &Example1609_Example1609_DEF.MAPTx);
}
int  Example1609_Example1609_Main_Init(
  tExample1609_Example1609_Main * pInit)
{
  return Example1609_Example1609_Main_Copy(pInit, &Example1609_Example1609_DEF.Main);
}
int  Example1609_Example1609_OBERx_Init(
  tExample1609_Example1609_OBERx * pInit)
{
  return Example1609_Example1609_OBERx_Copy(pInit, &Example1609_Example1609_DEF.OBERx);
}
int  Example1609_Example1609_PosExt_Init(
  tExample1609_Example1609_PosExt * pInit)
{
  return Example1609_Example1609_PosExt_Copy(pInit, &Example1609_Example1609_DEF.PosExt);
}
int  Example1609_Example1609_RawTx_Init(
  tExample1609_Example1609_RawTx * pInit)
{
  return Example1609_Example1609_RawTx_Copy(pInit, &Example1609_Example1609_DEF.RawTx);
}
int  Example1609_Example1609_RSARx_Init(
  tExample1609_Example1609_RSARx * pInit)
{
  return Example1609_Example1609_RSARx_Copy(pInit, &Example1609_Example1609_DEF.RSARx);
}
int  Example1609_Example1609_RSATx_Init(
  tExample1609_Example1609_RSATx * pInit)
{
  return Example1609_Example1609_RSATx_Copy(pInit, &Example1609_Example1609_DEF.RSATx);
}
int  Example1609_Example1609_SPATTx_Init(
  tExample1609_Example1609_SPATTx * pInit)
{
  return Example1609_Example1609_SPATTx_Copy(pInit, &Example1609_Example1609_DEF.SPATTx);
}
int  Example1609_Example1609_TCClient_Init(
  tExample1609_Example1609_TCClient * pInit)
{
  return Example1609_Example1609_TCClient_Copy(pInit, &Example1609_Example1609_DEF.TCClient);
}
int  Example1609_Example1609_TIMGenericRx_Init(
  tExample1609_Example1609_TIMGenericRx * pInit)
{
  return Example1609_Example1609_TIMGenericRx_Copy(pInit, &Example1609_Example1609_DEF.TIMGenericRx);
}
int  Example1609_Example1609_SDSMTx_Init(
  tExample1609_Example1609_SDSMTx * pInit)
{
  return Example1609_Example1609_SDSMTx_Copy(pInit, &Example1609_Example1609_DEF.SDSMTx);
}
int  Example1609_Example1609_RSM_Init(
  tExample1609_Example1609_RSM * pInit)
{
  return Example1609_Example1609_RSM_Copy(pInit, &Example1609_Example1609_DEF.RSM);
}
void Example1609_Example1609_Clean(
  tExample1609_Example1609 * pClean)
{
  if (pClean == NULL) return;
  Example1609_Example1609_CANVState_Clean(&pClean->CANVState);
  Example1609_Example1609_EVWTx_Clean(&pClean->EVWTx);
  Example1609_Example1609_MAPTx_Clean(&pClean->MAPTx);
  Example1609_Example1609_Main_Clean(&pClean->Main);
  Example1609_Example1609_OBERx_Clean(&pClean->OBERx);
  Example1609_Example1609_PosExt_Clean(&pClean->PosExt);
  Example1609_Example1609_RawTx_Clean(&pClean->RawTx);
  Example1609_Example1609_RSARx_Clean(&pClean->RSARx);
  Example1609_Example1609_RSATx_Clean(&pClean->RSATx);
  Example1609_Example1609_SPATTx_Clean(&pClean->SPATTx);
  Example1609_Example1609_TCClient_Clean(&pClean->TCClient);
  Example1609_Example1609_TIMGenericRx_Clean(&pClean->TIMGenericRx);
  Example1609_Example1609_SDSMTx_Clean(&pClean->SDSMTx);
  Example1609_Example1609_RSM_Clean(&pClean->RSM);
}
void Example1609_Example1609_CANVState_Clean(
  tExample1609_Example1609_CANVState * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_EVWTx_Clean(
  tExample1609_Example1609_EVWTx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_MAPTx_Clean(
  tExample1609_Example1609_MAPTx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_Main_Clean(
  tExample1609_Example1609_Main * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_OBERx_Clean(
  tExample1609_Example1609_OBERx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_PosExt_Clean(
  tExample1609_Example1609_PosExt * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_RSARx_Clean(
  tExample1609_Example1609_RSARx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_RSATx_Clean(
  tExample1609_Example1609_RSATx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_RSM_Clean(
  tExample1609_Example1609_RSM * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_RawTx_Clean(
  tExample1609_Example1609_RawTx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_SDSMTx_Clean(
  tExample1609_Example1609_SDSMTx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_SPATTx_Clean(
  tExample1609_Example1609_SPATTx * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_TCClient_Clean(
  tExample1609_Example1609_TCClient * pClean)
{
  if (pClean == NULL) return;
}
void Example1609_Example1609_TIMGenericRx_Clean(
  tExample1609_Example1609_TIMGenericRx * pClean)
{
  if (pClean == NULL) return;
}
int  Example1609_Example1609_Copy(
  tExample1609_Example1609 * pDst,
  const tExample1609_Example1609 * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = pExample1609;
    Res = Example1609_Example1609_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  Res |= Example1609_Example1609_CANVState_Copy(&pDst->CANVState, &pSrc->CANVState);
  Res |= Example1609_Example1609_EVWTx_Copy(&pDst->EVWTx, &pSrc->EVWTx);
  Res |= Example1609_Example1609_MAPTx_Copy(&pDst->MAPTx, &pSrc->MAPTx);
  Res |= Example1609_Example1609_Main_Copy(&pDst->Main, &pSrc->Main);
  Res |= Example1609_Example1609_OBERx_Copy(&pDst->OBERx, &pSrc->OBERx);
  Res |= Example1609_Example1609_PosExt_Copy(&pDst->PosExt, &pSrc->PosExt);
  Res |= Example1609_Example1609_RawTx_Copy(&pDst->RawTx, &pSrc->RawTx);
  Res |= Example1609_Example1609_RSARx_Copy(&pDst->RSARx, &pSrc->RSARx);
  Res |= Example1609_Example1609_RSATx_Copy(&pDst->RSATx, &pSrc->RSATx);
  Res |= Example1609_Example1609_SPATTx_Copy(&pDst->SPATTx, &pSrc->SPATTx);
  Res |= Example1609_Example1609_TCClient_Copy(&pDst->TCClient, &pSrc->TCClient);
  Res |= Example1609_Example1609_TIMGenericRx_Copy(&pDst->TIMGenericRx, &pSrc->TIMGenericRx);
  Res |= Example1609_Example1609_SDSMTx_Copy(&pDst->SDSMTx, &pSrc->SDSMTx);
  Res |= Example1609_Example1609_RSM_Copy(&pDst->RSM, &pSrc->RSM);
  pDst->SDSM_Object_Port = pSrc->SDSM_Object_Port;
  return Res;
}
int  Example1609_Example1609_CANVState_Copy(
  tExample1609_Example1609_CANVState * pDst,
  const tExample1609_Example1609_CANVState * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->CANVState;
    Res = Example1609_Example1609_CANVState_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_EVWTx_Copy(
  tExample1609_Example1609_EVWTx * pDst,
  const tExample1609_Example1609_EVWTx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->EVWTx;
    Res = Example1609_Example1609_EVWTx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_MAPTx_Copy(
  tExample1609_Example1609_MAPTx * pDst,
  const tExample1609_Example1609_MAPTx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->MAPTx;
    Res = Example1609_Example1609_MAPTx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_Main_Copy(
  tExample1609_Example1609_Main * pDst,
  const tExample1609_Example1609_Main * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->Main;
    Res = Example1609_Example1609_Main_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_OBERx_Copy(
  tExample1609_Example1609_OBERx * pDst,
  const tExample1609_Example1609_OBERx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->OBERx;
    Res = Example1609_Example1609_OBERx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_PosExt_Copy(
  tExample1609_Example1609_PosExt * pDst,
  const tExample1609_Example1609_PosExt * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->PosExt;
    Res = Example1609_Example1609_PosExt_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_RSARx_Copy(
  tExample1609_Example1609_RSARx * pDst,
  const tExample1609_Example1609_RSARx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->RSARx;
    Res = Example1609_Example1609_RSARx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_RSATx_Copy(
  tExample1609_Example1609_RSATx * pDst,
  const tExample1609_Example1609_RSATx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->RSATx;
    Res = Example1609_Example1609_RSATx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_RSM_Copy(
  tExample1609_Example1609_RSM * pDst,
  const tExample1609_Example1609_RSM * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->RSM;
    Res = Example1609_Example1609_RSM_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_RawTx_Copy(
  tExample1609_Example1609_RawTx * pDst,
  const tExample1609_Example1609_RawTx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->RawTx;
    Res = Example1609_Example1609_RawTx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_SDSMTx_Copy(
  tExample1609_Example1609_SDSMTx * pDst,
  const tExample1609_Example1609_SDSMTx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->SDSMTx;
    Res = Example1609_Example1609_SDSMTx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_SPATTx_Copy(
  tExample1609_Example1609_SPATTx * pDst,
  const tExample1609_Example1609_SPATTx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->SPATTx;
    Res = Example1609_Example1609_SPATTx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_TCClient_Copy(
  tExample1609_Example1609_TCClient * pDst,
  const tExample1609_Example1609_TCClient * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->TCClient;
    Res = Example1609_Example1609_TCClient_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_TIMGenericRx_Copy(
  tExample1609_Example1609_TIMGenericRx * pDst,
  const tExample1609_Example1609_TIMGenericRx * pSrc)
{
  int Res = 0;
  if (pDst == NULL) return CONF_ERROR_NULL;
  if (pSrc == pDst) return CONF_ERROR_INVALID;
  if (pSrc == NULL)
  {
    const tExample1609_Example1609 * pExample1609 = Example1609_Get();
    if (pExample1609 == NULL)
    {
      return CONF_ERROR_NULL;
    }
    pSrc = &pExample1609->TIMGenericRx;
    Res = Example1609_Example1609_TIMGenericRx_Copy(pDst, pSrc);
    Example1609_Release(&pExample1609);
    return Res;
  }
  pDst->DebugLevel = pSrc->DebugLevel;
  return Res;
}
int  Example1609_Example1609_Comp(
  const tExample1609_Example1609 * pA,
  const tExample1609_Example1609 * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_CANVState_Comp(&pA->CANVState, &pB->CANVState);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_EVWTx_Comp(&pA->EVWTx, &pB->EVWTx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_MAPTx_Comp(&pA->MAPTx, &pB->MAPTx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_Main_Comp(&pA->Main, &pB->Main);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_OBERx_Comp(&pA->OBERx, &pB->OBERx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_PosExt_Comp(&pA->PosExt, &pB->PosExt);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_RawTx_Comp(&pA->RawTx, &pB->RawTx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_RSARx_Comp(&pA->RSARx, &pB->RSARx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_RSATx_Comp(&pA->RSATx, &pB->RSATx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_SPATTx_Comp(&pA->SPATTx, &pB->SPATTx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_TCClient_Comp(&pA->TCClient, &pB->TCClient);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_TIMGenericRx_Comp(&pA->TIMGenericRx, &pB->TIMGenericRx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_SDSMTx_Comp(&pA->SDSMTx, &pB->SDSMTx);
  if (Res != 0) return Res;
  Res = Example1609_Example1609_RSM_Comp(&pA->RSM, &pB->RSM);
  if (Res != 0) return Res;
  Res = Comp_int32_t(pA->SDSM_Object_Port, pB->SDSM_Object_Port);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_CANVState_Comp(
  const tExample1609_Example1609_CANVState * pA,
  const tExample1609_Example1609_CANVState * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_EVWTx_Comp(
  const tExample1609_Example1609_EVWTx * pA,
  const tExample1609_Example1609_EVWTx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_MAPTx_Comp(
  const tExample1609_Example1609_MAPTx * pA,
  const tExample1609_Example1609_MAPTx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_Main_Comp(
  const tExample1609_Example1609_Main * pA,
  const tExample1609_Example1609_Main * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_OBERx_Comp(
  const tExample1609_Example1609_OBERx * pA,
  const tExample1609_Example1609_OBERx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_PosExt_Comp(
  const tExample1609_Example1609_PosExt * pA,
  const tExample1609_Example1609_PosExt * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_RSARx_Comp(
  const tExample1609_Example1609_RSARx * pA,
  const tExample1609_Example1609_RSARx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_RSATx_Comp(
  const tExample1609_Example1609_RSATx * pA,
  const tExample1609_Example1609_RSATx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_RSM_Comp(
  const tExample1609_Example1609_RSM * pA,
  const tExample1609_Example1609_RSM * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_RawTx_Comp(
  const tExample1609_Example1609_RawTx * pA,
  const tExample1609_Example1609_RawTx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_SDSMTx_Comp(
  const tExample1609_Example1609_SDSMTx * pA,
  const tExample1609_Example1609_SDSMTx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_SPATTx_Comp(
  const tExample1609_Example1609_SPATTx * pA,
  const tExample1609_Example1609_SPATTx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_TCClient_Comp(
  const tExample1609_Example1609_TCClient * pA,
  const tExample1609_Example1609_TCClient * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
int  Example1609_Example1609_TIMGenericRx_Comp(
  const tExample1609_Example1609_TIMGenericRx * pA,
  const tExample1609_Example1609_TIMGenericRx * pB)
{
  int Res = 0;
  if (pA == NULL) return CONF_ERROR_NULL;
  if (pB == NULL) return CONF_ERROR_NULL;
  Res = Comp_int32_t(pA->DebugLevel, pB->DebugLevel);
  if (Res != 0) return Res;
  return Res;
}
const tExample1609_Example1609 Example1609_Example1609_DEF =
{
  .CANVState = {
    .DebugLevel = Example1609_Example1609_CANVState_DebugLevel_DEF,
  },
  .EVWTx = {
    .DebugLevel = Example1609_Example1609_EVWTx_DebugLevel_DEF,
  },
  .MAPTx = {
    .DebugLevel = Example1609_Example1609_MAPTx_DebugLevel_DEF,
  },
  .Main = {
    .DebugLevel = Example1609_Example1609_Main_DebugLevel_DEF,
  },
  .OBERx = {
    .DebugLevel = Example1609_Example1609_OBERx_DebugLevel_DEF,
  },
  .PosExt = {
    .DebugLevel = Example1609_Example1609_PosExt_DebugLevel_DEF,
  },
  .RawTx = {
    .DebugLevel = Example1609_Example1609_RawTx_DebugLevel_DEF,
  },
  .RSARx = {
    .DebugLevel = Example1609_Example1609_RSARx_DebugLevel_DEF,
  },
  .RSATx = {
    .DebugLevel = Example1609_Example1609_RSATx_DebugLevel_DEF,
  },
  .SPATTx = {
    .DebugLevel = Example1609_Example1609_SPATTx_DebugLevel_DEF,
  },
  .TCClient = {
    .DebugLevel = Example1609_Example1609_TCClient_DebugLevel_DEF,
  },
  .TIMGenericRx = {
    .DebugLevel = Example1609_Example1609_TIMGenericRx_DebugLevel_DEF,
  },
  .SDSMTx = {
    .DebugLevel = Example1609_Example1609_SDSMTx_DebugLevel_DEF,
  },
  .RSM = {
    .DebugLevel = Example1609_Example1609_RSM_DebugLevel_DEF,
  },
  .DebugLevel = Example1609_Example1609_DebugLevel_DEF,
  .SDSM_Object_Port = Example1609_Example1609_SDSM_Object_Port_DEF,
};

/**
 * @addtogroup mod_tim_generic_rx TIM Generic ITIS Match Module
 * @{
 *
 * @file tim-generic-rx.c Module code for generic TIM
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "tim-generic-rx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "j2735-message.h"
#include "j2735asn.h"
#include "ldm.h"
#include "lph.h"
#include "tim-region.h"
#include "tim.h"
#include "TA.h"
#include "Example1609_debug.h"

#include <debug-levels.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_TIMGenericRx_DebugLevel

#define GENERIC_ALERT_MAX_DISTANCE_CM (10000)

/// theth-microdegress for ~1KM (lat/lon offset)
#define GENERIC_LDM_AOI_LATLON_DEFAULT (90000)
#define GENERIC_LDM_AOI_ELEV_DEFAULT (99)

/// Max number of cached TIM
#define MAX_TIMGENERIC_CACHE_RECORDS   (20)

#define TIMGENERICRX_CONFIG_ENABLE        "Example.APP.TIMGenericRx"
#define TIMGENERICRX_CONFIG_PATHNAME      "Example.TIMGenericRx"
#define TIMGENERICRX_SETTING_SEVERITY_MAX "SeverityMax"
#define TIMGENERICRX_SETTING_SEVERITY_MIN "SeverityMin"
#define TIMGENERICRX_SETTING_ITIS_CODES   "ITISCodes"

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/// Speed limit specific warning details
typedef struct TIMGenericData
{
  // Identifier for the TIM
  int64_t ID;
  // The ITIS code associated with the TIM
  SAEITIScodes ITISCode;
  // The TIM region
  tTIMRegion *pRegion;
} tTIMGenericRxData;

/// Cached TIM data
typedef struct TIMGenericRxCache
{
  unsigned int NumberOfRecords;
  pthread_mutex_t Lock;
  tTIMGenericRxData Records[MAX_TIMGENERIC_CACHE_RECORDS];
} tTIMGenericRxCache;

/// TIMGeneric receiver parameters - Stored in Config File
typedef struct TIMGenericRxParams
{
  /// Generic TIM enabled?
  /// __Boolean, default: "False"__
  bool ENABLE;
  /// Generic TIM severity max (0:off, 1:lowest, 255:highest) @ref eTASeverity
  /// __Integer, default: 95, min/max 0/255__
  int32_t SEVERITY_MAX;
  /// Speed Limit severity min (0:off, 1:lowest, 255:highest) @ref eTASeverity
  /// __Integer, default: 81, min/max 0/255__
  int32_t SEVERITY_MIN;
  /// Generic TIM ITIS Code match (0 = disabled, any valid ITIS code enables the Generic TIM match)\n
  /// These are evaluated in order - 0 - 4.  If a match is found evaluation stops, therefore giving\n
  /// the ITIS match an implicit priorority order.
  /// __Integer, default: 0, min/max 0/65535__
  int32_t * ITIS_CODES;
  size_t    NUM_ITIS_CODES;
} tTIMGenericRxParams;

/// Globals
typedef struct TIMGenericRx
{
  /// Cache of all TIMs currently being evaluated
  tTIMGenericRxCache Cache;
  ///Threat mask
  tTAThreatTypes Threats;
  /// The LDM Handle
  int LDMHandle;

  /// Loaded params
  tTIMGenericRxParams Params;
} tTIMGenericRx;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
static void TIMGenericRx_LDMCallback(size_t NumOfItemsFound,
                                     void *pPriv,
                                     const tLDM_QueryResult *pData);
static int TIMGenericRx_Evaluate(const tTA_HV *pHV,
                                 tTIMGenericRxData *pTIMData,
                                 tTAAlert *pAlert);

static fTA_NonRVThreatApplication TIMGenericRx_Eval;

static int TIMGenericRx_LoadConfig(tTIMGenericRx *pTIMGenericRx,
                                   const char * pConfigFileName);
//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
/// Singleton
static tTIMGenericRx TIMGenericRx;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Free TIM regions
 * @param pData TIM Rx Data
 * @param NumData Number of TIM regions
 * @return Zero for success or a negative errno
 */
static int TIMGenericRx_FreeRegions(tTIMGenericRxData *pData,
                                    int NumData)
{
  int Res = 0, Idx = 0;

  if (pData == NULL)
  {
    d_printf(D_ERR, NULL, "Generic TIM Data List is NULL - Not freeing\n");
    Res = -1;
    goto Exit;
  }

  if (NumData == 0)
  {
    d_printf(D_ERR, NULL, "No records in Generic TIM Cache - Not freeing\n");
    Res = 0;
    goto Exit;
  }

  for (Idx = 0; Idx < NumData; Idx++)
  {
    if (pData[Idx].pRegion == NULL)
    {
      d_printf(D_ERR, NULL, "Region is NULL - Not freeing\n");
      continue;
    }
    TIMRegion_Free(pData[Idx].pRegion);
    Res++;
  }

Exit:
  return Res;
}

/**
 * @brief Register area of interest to LDM
 * @return Zero for success or a negative errno
 */
static int TIMGenericRx_LDMDefaultAOI()
{
  int Res = 0;
  tLDMQuery Query;

  Query.Type = LDM_MSG_TIM;
  Query.AOI.AoiSizeLat = GENERIC_LDM_AOI_LATLON_DEFAULT;
  Query.AOI.AoiSizeLon = GENERIC_LDM_AOI_LATLON_DEFAULT;
  Query.AOI.AoiSizeElv = GENERIC_LDM_AOI_ELEV_DEFAULT;

  // If this is the first entry, the LDMHandle will need to be initialised
  if (TIMGenericRx.LDMHandle == 0)
  {
    TIMGenericRx.LDMHandle = LDM_SubscribeQuery(&Query, TIMGenericRx_LDMCallback, NULL);
    Res = TIMGenericRx.LDMHandle;
    if (TIMGenericRx.LDMHandle < 0)
    {
      Res = -1;
      d_printf(D_ERR, NULL, "Failed to subscribe to LDM\n");
    }
    else
    {
      d_printf(D_DBG, NULL, "Subscribed to LDM\n");
    }
  }
  else
  {
    Res = LDM_UpdateQuery(&(Query.AOI), TIMGenericRx.LDMHandle);
    if (Res != 0)
    {
      d_printf(D_ERR, NULL, "Failed to set the LDM AOI to default\n");
    }
    else
    {
      d_printf(D_DBG, NULL, "Updated LDM Query\n");
    }
  }

  return Res;
}

/**
 * @brief LDM callback
 * @param NumOfRecordsFound number of records found
 * @param pPriv Private pointer that was provided in LDM_SubscribeQuery()
 * @param pData point to memory holding query results (variable sized array of tLDM_QueryResult)
 */
static void TIMGenericRx_LDMCallback(size_t NumOfItemsFound,
                                     void *pPriv,
                                     const tLDM_QueryResult *pData)
{
  int MaxNumber = 0;
  int NumGenericData = 0;
  int i;
  const tLDMBBox *pBbox;
  tTIMGenericRxData *pTIMData = NULL;

  (void)pPriv;

  // TIM Generic enabled?
  if (!TIMGenericRx.Params.ENABLE)
  {
    d_printf(D_ERR, NULL, "Disabled\n");
    goto Error;
  }

  if (pData == NULL)
  {
    d_printf(D_ERR, NULL, "Missing LDM query pointer\n");
    goto Error;
  }

  if (NumOfItemsFound == 0)
  {
    // None found, so Exit (remembering to clear previously cached results)
    d_printf(D_ERR, NULL, "No TIMs of interest from LDM\n");
    goto Exit;
  }

  for (i = 0; i < (int)NumOfItemsFound; i++)
  {
    const tLDM_QueryResult * pQR = pData + i;
    if (pQR->Type == LDM_MSG_TIM)
    {
      pBbox = &pQR->Bbox;

      struct J2735Msg *pJ2735 = NULL;
      int Res = J2735_MsgPERDecode(&pJ2735,
                                   (char *)&(pQR->Data[0]),
                                   pQR->DataLen);
      if ((Res < 0) || (!pJ2735))
      {
        // decode fail
        d_printf(D_ERR, NULL, "Unable to decode TIM message\n");
        // Try the next one
        continue;
      }
      struct SAETravelerInformation *pTIMMsg = (struct SAETravelerInformation *)pJ2735->pTIM;
      SAETravelerDataFrame *pTIMInfo = &(pTIMMsg->dataFrames.tab[0]);

      SAEITIScodes ITIS = pTIMInfo->content.u.advisory.tab[0].item.u.itis;

      tTIMGenericRxData *_pTIMData = (tTIMGenericRxData *)realloc(pTIMData, (NumGenericData + 1)*sizeof(*pTIMData));
      if (!_pTIMData)
      {
        d_printf(D_ERR, NULL, "Failed to allocate memory for TIM data\n");
        J2735_MsgFree(pJ2735);
        pJ2735 = NULL;
        goto Error;
      }
      pTIMData = _pTIMData;

      tTIMRegion *pRegion = TIMRegion_ExtractFromTIM(pTIMInfo);

      pTIMData[NumGenericData].ITISCode = ITIS;
      pTIMData[NumGenericData].pRegion = pRegion;
      pTIMData[NumGenericData].ID = pQR->MsgId;

      d_printf(D_DBG, NULL, "Adding to Generic TIM Cache: MsgId = %"PRIx64" MinLat = %d "
               "MaxLat = %d, MinLon = %d MaxLon = %d MinElev = %d MaxElev = %d "
               "ValidFrom = %d ValidTo = %d pMsg = %p, Len = %zd\n",
               pQR->MsgId, pBbox->MinLat, pBbox->MaxLat, pBbox->MinLon,
               pBbox->MaxLon, pBbox->MinElev, pBbox->MaxElev, pBbox->ValidFrom,
               pBbox->ValidTo, pQR->Data, pQR->DataLen);

      NumGenericData++;

      //free the J2735 message created in the decoding process
      J2735_MsgFree(pJ2735);
    }
  }

  // if we have few enough records ...
  if (NumGenericData < MAX_TIMGENERIC_CACHE_RECORDS)
  {
    MaxNumber = NumGenericData;
  }
  else
  {
    MaxNumber = MAX_TIMGENERIC_CACHE_RECORDS;
  }

Exit:
  // update the processing cache
  pthread_mutex_lock(&TIMGenericRx.Cache.Lock);

  // Need to free any regions that are already stored in the cache as we will be
  // overwriting them very shortly
  TIMGenericRx_FreeRegions(&(TIMGenericRx.Cache.Records[0]),
                           TIMGenericRx.Cache.NumberOfRecords);

  // Clear cache now that new results are in
  memset(TIMGenericRx.Cache.Records, 0, sizeof(TIMGenericRx.Cache.Records));
  if (pTIMData != NULL)
  {
    memcpy(TIMGenericRx.Cache.Records, pTIMData, (sizeof(*pTIMData) * MaxNumber));
  }
  TIMGenericRx.Cache.NumberOfRecords = MaxNumber;
  pthread_mutex_unlock(&TIMGenericRx.Cache.Lock);

  // All good
  goto Success;

Error:
  // There was an error so we have not cached anything - okay to free the
  // regions before we free up the temporary storage of pTIMData;
  TIMGenericRx_FreeRegions(pTIMData, NumGenericData);
Success:
  // Since we are bailing out clean up any memory we have already
  // allocated.
  if (pTIMData != NULL)
  {
    free(pTIMData);
  }
  return;
}

/**
 * @brief Set TIM Alert Level according to current position
 * @param pRegion TIM Regsion to be checked
 * @param pLPHPos Current vehicle position
 * @return TIM Alert Level
 */
static int TIMGenericRx_SetAlertLevel(const tTIMRegion *pRegion, struct LPHPos *pLPHPos)
{
  tTASeverity AlertLevel = TA_SEVERITY_MINIMUM;

  // Check if the Vehicle is in the region defined by the TIM
  tPointInRegionResult InRegion = TIMRegion_PointInRegion(pRegion, pLPHPos);
  d_printf(D_DBG, NULL, "Vehicle currently %s with regard to TIM\n",
           (InRegion == POINT_INSIDE_AREAS ? "'Inside Areas'" : "'Outside Region'"));
  if (InRegion == POINT_INSIDE_AREAS)
  {
    AlertLevel = TIMGenericRx.Params.SEVERITY_MAX;
  }

  return AlertLevel;
}

/**
 * @brief Evaluate the RV for a potential TIM ITIS matches
 * @param[in] pHV where we are
 * @param[in] pTIM TIM RSU under consideration
 * @param[out] pAlert TA State for the remote
 */
static int TIMGenericRx_Evaluate(const tTA_HV *pHV,
                                 tTIMGenericRxData *pTIMData,
                                 tTAAlert *pAlert)
{
  int Res = TA_SEVERITY_MINIMUM;

  // TIM Generic ITIS match enabled?
  if (!(TIMGenericRx.Params.ENABLE))
  {
    d_printf(D_DBG, NULL, "(%p): Disabled\n", pHV);
    return -ENODEV;
  }

  if (pTIMData->ID == 0)
  {
    // This is an empty entry and so is the end of the list to be evaluated
    // We should never get here though
    d_printf(D_ERR, NULL, "Failed to retrieve TIM data for evaluation - NULL TIM\n");
    return -EINVAL;
  }

  // Do TIM ITIS match here
  {
    tTASeverity AlertLevel = TA_SEVERITY_CANCEL;

#define CHECK_ITIS_CODE(NUM, ITISCode)                                  \
    do {                                                                \
      d_printf(D_DBG, NULL, "Try to match ITIS Code %d, %d\n", TIMGenericRx.Params.ITIS_CODES[NUM], ITISCode); \
      if ((TIMGenericRx.Params.ITIS_CODES[NUM]) &&                \
          (TIMGenericRx.Params.ITIS_CODES[NUM] == ITISCode)) {         \
        tTASeverity _AlertLevel = TIMGenericRx_SetAlertLevel(pTIMData->pRegion, (struct LPHPos *)pHV->pLPHPos); \
        if (_AlertLevel > AlertLevel) {                                 \
          AlertLevel = _AlertLevel;                                     \
        }                                                               \
      }                                                                 \
    } while(0)

    int i;
    for (i = 0; i < (int)(TIMGenericRx.Params.NUM_ITIS_CODES); ++i)
    {
      CHECK_ITIS_CODE(i, pTIMData->ITISCode);
    }

    if (AlertLevel >= pAlert->Level)
    {
      Res = AlertLevel;
      pAlert->GenericTIM.ITISCode = pTIMData->ITISCode;
    }
  }
  return Res;
}


/**
 * @brief Assess a TIM ITIS code for match
 * @param[in] pHV Data for the HV
 * @param[out] pAlert RV alert state (modified by this func)
 * @param[in] pCurrentAlert Current TA alert state
 * @param[in,out] pPriv local storage, not used
 *
 */
static void TIMGenericRx_Eval(const tTA_HV *pHV,
                              tTAAlert *pAlert,
                              const tTAAlert *pCurrentAlert,
                              void *pPriv)
{
  int Res = -ENOSYS;

  (void)pPriv;

  // TIM Generic enabled?
  if (!(TIMGenericRx.Params.ENABLE))
  {
    d_printf(D_DBG, NULL, "(%p): Disabled\n", pHV);
    goto Error;
  }

  // Bogus parameters?
  if ((pHV == NULL) ||
      (pHV->pVector == NULL)   ||
      (pAlert == NULL))
  {
    d_printf(D_ERR, NULL, "(%p %p %p %p)\n",
             pHV, pAlert, pCurrentAlert, pPriv);
    Res = -EINVAL;
    goto Error;
  }

  Res = pthread_mutex_trylock(&TIMGenericRx.Cache.Lock);

  if (Res)
  {
    d_printf(D_ERR, NULL, "Failed to get lock\n");
    goto Error;
  }

  // Fetch TIM data
  int TIMCount = TIMGenericRx.Cache.NumberOfRecords;
  tTASeverity GenericLevel = TA_SEVERITY_CANCEL;

  if (TIMCount > 0)
  {
    // Loop through the TIM messages
    int TIMIndex;
    for (TIMIndex = 0; TIMIndex < TIMCount; TIMIndex++)
    {
      tTIMGenericRxData *pTIMData = &(TIMGenericRx.Cache.Records[TIMIndex]);
      Res = TIMGenericRx_Evaluate(pHV , pTIMData, pAlert);

      // Update currently tracked alert levels if appropriate
      if (Res > pAlert->Level)
      {
        pAlert->Level = Res;
      }
      if (Res >= GenericLevel)
      {
        GenericLevel = Res;
        pAlert->ID = pTIMData->ID;
      }
    }

    if (GenericLevel == pAlert->Level)
    {
      // Use the highest Res as alert level
      Res = GenericLevel;
    }
    else
    {
      Res = TA_SEVERITY_CANCEL;
    }

    // Sanity check the severity
    if ((Res < TA_SEVERITY_CANCEL) || (Res > TA_SEVERITY_MITIGATE))
    {
      d_printf(D_ERR, NULL, "GenericRx_Evaluate() = %d\n", Res);
      goto Exit;
    }

    // Does the alert level indicate an Generic TIM warning?
    if (Res > TA_SEVERITY_TRACKING)
    {
      // initialise the alert structure with the relevant information
      pAlert->Type      = TA_THREATTYPE_GENERICTIM;
      pAlert->Level     = Res;

      // Set Generic TIM flag
      TA_SetThreat(&TIMGenericRx.Threats, TA_THREATTYPE_GENERICTIM,
                   pAlert->ID, "Generic TIM");
    }
    else if (Res >= TA_SEVERITY_CANCEL)
    {
      // Clear the Generic TIM flag in this RV
      TA_ClearThreat(&TIMGenericRx.Threats, TA_THREATTYPE_GENERICTIM,
                     pAlert->ID, "Generic TIM");

      // If we were sending a generic TIM and now we're not, send a CANCEL
      if ((pCurrentAlert != NULL) &&
          (pCurrentAlert->Type == TA_THREATTYPE_GENERICTIM))
      {
        // Inform registered alert call back function to cancel alert.
        pAlert->Type  = TA_THREATTYPE_GENERICTIM;
        pAlert->Level = TA_SEVERITY_CANCEL;

        // Invoke registered alert call back function.
        TA_CancelAlert(pAlert);

        // Reset the threat level to aid periodic processing
        pAlert->Level = Res;

        d_printf(D_DBG, NULL, "(%"PRIx64"): %d, Stop Generic TIM Warning!\n", pAlert->ID, Res);
      }
    }
  }
  else
  {
    if (TIMGenericRx.Threats & TA_THREATTYPE_GENERICTIM)
    {
      // Clear the Generic TIM Warning flag in this RV
      TA_ClearThreat(&TIMGenericRx.Threats, TA_THREATTYPE_GENERICTIM,
                     pAlert->ID, "Generic TIM");

      // Currently alerting, but now no TIMSs to handle
      pAlert->Type  = TA_THREATTYPE_GENERICTIM;
      pAlert->Level = TA_SEVERITY_CANCEL;

      // Invoke registered alert call back function.
      TA_CancelAlert(pAlert);

      // Reset the threat level to aid periodic processing
      pAlert->Level = Res;

      d_printf(D_DBG, NULL, "(%"PRIx64"): %d, No Generic TIM Warning!\n", pAlert->ID, Res);
    }
  }

  // Done.
Exit:
  pthread_mutex_unlock(&TIMGenericRx.Cache.Lock);
Error:

  (void)Res;   // value not currently used
  return;
}

/**
 * @brief Initialize TIM ITIS Code match handler
 * @param pConfigFileName Configuration filename
 * @return Success: 0, Error: @ref TA_ErrCode or negative errno
 */
int TIMGenericRx_Open(const char *pConfigFileName)
{
  int Res;

  // Load config settings
  Res = TIMGenericRx_LoadConfig(&TIMGenericRx, pConfigFileName);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "TIMGenericRx_LoadConfig() != 0\n");
    goto Error;
  }

  // Start the TIM reception
  Res = TIM_Init();
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "Failed TIM_Init() == %d\n", Res);
    goto Error;
  }

  // Initialise the LDM if it hasn't already
  Res = LDM_Init();
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "Failed to intialise the LDM - not initialising Generic TIM Check\n");
    goto Error;
  }

  Res = pthread_mutex_init(&TIMGenericRx.Cache.Lock, NULL);
  if (Res != 0)
  {
    // Lock init failed
    d_printf(D_ERR, NULL, "pthread_mutex_init failed %d\n", Res);
    Res = -ENODEV;
    goto Error;
  }

  // Regsiter with LDM for TIM message updates.  Initially the TIM handler will
  // be notified of all TIMS within 1KM bounding box.
  Res = TIMGenericRx_LDMDefaultAOI();
  if(Res < 0)
  {
    d_printf(D_ERR, NULL, "LDM_SubscribeQuery() = %d\n", Res);
    goto LDMSubscribeError;
  }

  Res = TA_RegisterNonRVApplication(TIMGenericRx_Eval,
                                    TIMGenericRx.Params.SEVERITY_MAX,
                                    NULL,
                                    NULL, "TIMGen");
  // registered?
  if (Res)
  {
    // not going to register with LDM
    TIMGenericRx.LDMHandle = -1;
    d_printf(D_ERR, NULL, "() TA_RegisterNonRVApplication failed: %d\n", Res);
    goto TARegisterError;
  }

  // All good if we get here
  goto Success;

TARegisterError:
LDMSubscribeError:
  LDM_Deinit();
Error:
Success:

  return Res;
}

/**
 * @brief De-initialize the TIM ITIS Match handler
 * @return Zero for success or a negative errno
 */
int TIMGenericRx_Close(void)
{
  if (TIMGenericRx.Params.ITIS_CODES)
  {
    free(TIMGenericRx.Params.ITIS_CODES);
    TIMGenericRx.Params.ITIS_CODES = NULL;
  }

  int Res = TA_DeregisterNonRVApplication(TIMGenericRx_Eval,
                                          TIMGenericRx.Params.SEVERITY_MAX);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "Failed to deregister TIM Generic TA callback\n");
  }

  // Deregsiter with LDM
  if (TIMGenericRx.LDMHandle >= 0)
  {
    (void)LDM_UnsubscribeQuery(TIMGenericRx.LDMHandle);
    TIMGenericRx.LDMHandle = -1;
  }
  LDM_Deinit();

  // Free any regions that are currently stored in the cache
  TIMGenericRx_FreeRegions(&(TIMGenericRx.Cache.Records[0]),
                           TIMGenericRx.Cache.NumberOfRecords);

  // unreg first to avoid access the lock in callback
  pthread_mutex_destroy(&TIMGenericRx.Cache.Lock);

  Res = 0; // Nothing to do

  TIM_Deinit();
  return Res;
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Load TIMGenericRx parameters from the config file
 * @param pOBE OBE receiver handle
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int TIMGenericRx_LoadConfig(tTIMGenericRx *pTIMGenericRx,
                                   const char *pConfigFileName)
{
  int Res = -ENOSYS;
  config_t Config; // configuration object read from config file
  config_t *pConfig = NULL;
  config_setting_t * pSetting = NULL, * pITISCodes = NULL;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Set defaults
  pTIMGenericRx->Params.ENABLE = false;
  pTIMGenericRx->Params.SEVERITY_MAX = 95;
  pTIMGenericRx->Params.SEVERITY_MIN = 81;
  pTIMGenericRx->Params.NUM_ITIS_CODES = 0;

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    d_printf(D_ERR, NULL, "Could not open %s\n", pConfigFileName);
    Res = -EINVAL;
    goto Error;
  }

  int Enable = 0;
  if (config_lookup_bool(pConfig,
                         TIMGENERICRX_CONFIG_ENABLE,
                         &Enable) != CONFIG_TRUE)
  {
    d_printf(D_ERR, NULL, "TIMGenericRx not found in config\n");
    goto Error;
  }
  if (!Enable)
  {
    d_printf(D_ERR, NULL, "TIMGenericRx not enabled\n");
    goto Error;
  }
  pTIMGenericRx->Params.ENABLE = true;

  // Get the TIMGenericRx Setting
  pSetting = config_lookup(pConfig, TIMGENERICRX_CONFIG_PATHNAME);
  if (pSetting == NULL)
  {
    d_printf(D_ERR, NULL, "config_lookup(%s) failed\n", TIMGENERICRX_CONFIG_PATHNAME);
    goto Error;
  }
  if (config_setting_lookup_int(pSetting,
                                TIMGENERICRX_SETTING_SEVERITY_MAX,
                                &pTIMGenericRx->Params.SEVERITY_MAX) != CONFIG_TRUE)
  {
    d_printf(D_ERR, NULL, "SevityMax not found\n");
    goto Error;
  }
  if (config_setting_lookup_int(pSetting,
                                TIMGENERICRX_SETTING_SEVERITY_MIN,
                                &pTIMGenericRx->Params.SEVERITY_MIN) != CONFIG_TRUE)
  {
    d_printf(D_ERR, NULL, "SevityMin not found\n");
    goto Error;
  }

  pITISCodes = config_setting_get_member(pSetting, TIMGENERICRX_SETTING_ITIS_CODES);
  if (!pITISCodes)
  {
    d_printf(D_ERR, NULL, "Failed to find ITIS codes\n");
    goto Error;
  }

  pTIMGenericRx->Params.NUM_ITIS_CODES = config_setting_length(pITISCodes);
  pTIMGenericRx->Params.ITIS_CODES =
    calloc(sizeof(int), pTIMGenericRx->Params.NUM_ITIS_CODES);
  int i;
  for (i = 0; i < (int)(pTIMGenericRx->Params.NUM_ITIS_CODES); ++i)
  {
    pTIMGenericRx->Params.ITIS_CODES[i] = config_setting_get_int_elem(pITISCodes, i);
  }

  // Success!
  Res = 0;

Error:
  // Update complete - close the configuration
  config_destroy(pConfig);
  return Res;
}


/**
 * @}
 */

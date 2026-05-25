/**
 * @addtogroup mod_obe_rx OBE Rx Module
 * @{
 *
 * @file tim-rx.c Module code for handling TIM messages
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2016 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "obe-rx.h"

#include "libconfig.h" // used to read configuration file

// v2x-lib includes
#include "debug-levels.h"
#include "hmi.h"
#include "tim-region.h"
#include "TA.h"
#include "Example1609_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_OBERx_DebugLevel

/// ITIS code for right arrow
#define ITIS_RIGHT_ARROW (13609)

/// ITIS code for left arrow
#define ITIS_LEFT_ARROW (13610)

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------

/**
 * @brief Read HMI configuration
 * @param pConfig Config structure
 * @param[out] pHMI HMI configs to load
 * @param pPath Path to HMI configs
 */
void OBERx_ReadCustomHMIAlert(const config_t *pConfig,
                              tHMI *pHMI,
                              const char *pPath)
{
  config_setting_t * pSetting;
  const char *pIcon = NULL;
  const char *pLeftIcon = NULL;
  const char *pRightIcon = NULL;
  const char *pAudio = NULL;
  const char *pLeftAudio = NULL;
  const char *pRightAudio = NULL;
  const char *pText = NULL;
  int ITISCode = -1;
  int SeverityLow = 0;
  int SeverityHigh = 255;
  config_setting_t * pSeverityRange;

  d_fnstart(D_ERR, NULL, "(%p, %p, %s)\n", pConfig, pHMI, pPath);

  config_setting_t * pConfigurations = config_lookup(pConfig, pPath);
  if (pConfigurations != NULL)
  {
    int NumberOfConfigs = config_setting_length(pConfigurations);
    d_printf(D_DEBUG, NULL, "Number of configs = %d\n", NumberOfConfigs);
    int i = 0;
    for (; i < NumberOfConfigs; i++)
    {
      pSetting = config_setting_get_elem(pConfigurations, i);
      if (pSetting == NULL)
      {
        d_error(D_ERR, NULL, "Error getting the next hmi configuration block\n");
        goto Exit;
      }
      pSeverityRange = config_setting_get_member(pSetting, "SeverityRange");
      // severity range is a 2 element list
      // element[0] severity low
      // element[1] severity high
      if ((pSeverityRange == NULL) ||
          (config_setting_length(pSeverityRange) != 2))
      {
        d_error(D_ERR, NULL, "HMI SeverityRange error for %s", pPath);
        goto Exit;
      }
      SeverityLow  = config_setting_get_int_elem(pSeverityRange, 0);
      SeverityHigh = config_setting_get_int_elem(pSeverityRange, 1);

      // must specify either Icon and possibly LeftIcon and RightIcon
      config_setting_lookup_string(pSetting, "Icon", &pIcon);
      config_setting_lookup_string(pSetting, "LeftIcon", &pLeftIcon);
      config_setting_lookup_string(pSetting, "RightIcon", &pRightIcon);
      // must specify either Audio and possibly LeftAudio and RightAudio
      config_setting_lookup_string(pSetting, "Audio", &pAudio);
      config_setting_lookup_string(pSetting, "LeftAudio", &pLeftAudio);
      config_setting_lookup_string(pSetting, "RightAudio", &pRightAudio);
      config_setting_lookup_string(pSetting, "Text", &pText);
      config_setting_lookup_int(pSetting, "ITIS", &ITISCode);
      if ((pIcon || (pLeftIcon && pRightIcon)) &&
          (pAudio || (pLeftAudio && pRightAudio)) &&
          pText &&
          ITISCode != -1)
      {
        d_assert((pIcon || (pLeftIcon && pRightIcon)) &&
                 (pAudio || (pLeftAudio && pRightAudio)) &&
                 pText);
        d_printf(D_WARN, NULL, "Custom alert:\n"
                 "\t SeverityLow: %d\n"
                 "\t SeverityHigh: %d\n"
                 "\t LeftIcon: %s\n"
                 "\tRightIcon: %s\n"
                 "\t     Icon: %s\n"
                 "\t LeftAudio: %s\n"
                 "\tRightAudio: %s\n"
                 "\t     Audio: %s\n"
                 "\t     Text: %s\n"
                 "\t ITISCode: %d\n",
                 SeverityLow,
                 SeverityHigh,
                 pLeftIcon ? pLeftIcon : "(unspecified)",
                 pRightIcon ? pRightIcon : "(unspecified)",
                 pIcon ? pIcon : "(unspecified)",
                 pLeftAudio ? pLeftAudio : "(unspecified)",
                 pRightAudio ? pRightAudio : "(unspecified)",
                 pAudio ? pAudio : "(unspecified)",
                 pText,
                 ITISCode);
        if (pIcon && pAudio && pText)
        {
          HMI_AddAlertConfig(pHMI,
                             ITISCode,
                             HMI_ALERT_DIRECTION_UNKNOWN,
                             SeverityLow,
                             SeverityHigh,
                             pIcon,
                             pAudio,
                             pText);

          if (pLeftIcon || pLeftAudio)
          {
            HMI_AddAlertConfig(pHMI,
                               ITISCode,
                               HMI_ALERT_DIRECTION_LEFT,
                               SeverityLow,
                               SeverityHigh,
                               pLeftIcon ? pLeftIcon : pIcon,
                               pLeftAudio ? pLeftAudio : pAudio,
                               pText);
          }
          if (pRightIcon || pRightAudio)
          {
            HMI_AddAlertConfig(pHMI,
                               ITISCode,
                               HMI_ALERT_DIRECTION_RIGHT,
                               SeverityLow,
                               SeverityHigh,
                               pRightIcon ? pRightIcon : pIcon,
                               pRightAudio ? pRightAudio : pAudio,
                               pText);
          }
        }
      }
    }
    goto Exit;
  }
  d_printf(D_WARN, NULL, "Missing custom alert HMI configuration\n");
Exit:
  d_fnend(D_DEBUG, NULL, "()\n");
  return;
}

/**
 * @brief Issue CSW HMI Alert
 * @param pAlert Alert
 * @param pHMI HMI configuration
 */
void OBERx_Threattype_CSW(const tTAAlert *pAlert,
                          tHMI *pHMI)
{
  d_printf(D_TST, NULL, "()\n");
  d_assert(pAlert);
  d_assert(pHMI);

  const tHMIAlertSeverity Severity = pAlert->Level;
  const tHMIAlertDirection Direction =
      (pAlert->CSW.AlertDirection == ITIS_RIGHT_ARROW) ?
       HMI_ALERT_DIRECTION_RIGHT : HMI_ALERT_DIRECTION_LEFT;
  tHMIAlertConfig *pConfig;
  tHMIErrCode Err;
  Err = HMI_GetAlertConfig(pHMI,
                           HMI_ALERT_TYPE_CSW,
                           Direction, Severity,
                           &pConfig);

  if (Err != HMI_ERR_NONE)
  {
    d_printf(D_WARN, NULL,
             "No CSW alert configured in HMI for Direction %u, Severity %u\n",
             Direction, Severity);
  }
  else
  {
    // if we have information to determine the speed and direction of the curve
    // is AND the pText for the alert contains a %u and a %s placeholder, we can
    // replace the %u with the speed and the %s with the direction, otherwise
    // the original pText will be used as is
    unsigned int i, NumPC = 0;

    // use provided pText if it includes %d / %u and %s placeholders
    for (i = 0; pConfig->pText != NULL && i < strlen(pConfig->pText); i++)
    {
      if (pConfig->pText[i] == '%')
        NumPC++;
    }

    // allow two format directives, one must be %u / %d and one must be %s
    if (NumPC == 2 &&
        (strstr(pConfig->pText, "%u") != NULL ||
         strstr(pConfig->pText, "%d") != NULL) &&
         strstr(pConfig->pText, "%s") != NULL)
    {
      char Warning[1024];
      snprintf(Warning, sizeof(Warning), pConfig->pText,
               pAlert->CSW.SpeedLimit,
               (Direction == HMI_ALERT_DIRECTION_LEFT ? "Left" : "Right"));
      Warning[sizeof(Warning) - 1] = '\0';

      // replace the string in pAlert
      free(pConfig->pText);
      pConfig->pText = strdup(Warning);
    }
  }

  d_printf(D_WARN, NULL, "Alerting for active CSW (in %s)\n",
           pAlert->CSW.Region == POINT_INSIDE_APPROACH ? "approach" : "offsets");

  // HMI
  if (pHMI)
    HMI_Alert(pHMI,
              HMI_ALERT_TYPE_CSW,
              Direction,
              Severity,
              pConfig ? pConfig->pText : NULL,
              pAlert->DistanceToEvent_m);

  if (pConfig)
    HMI_FreeAlertConfig(pConfig);

  return;
}

/**
 * @brief Issue Generic TIM HMI Alert
 * @param pAlert Alert
 * @param pHMI HMI configuration
 */
void OBERx_Threattype_GenericTIM(const tTAAlert *pAlert,
                                 tHMI *pHMI)
{
  d_printf(D_TST, NULL, "()\n");
  d_assert(pAlert);
  d_assert(pHMI);

  // Generic ITIS Code match
  tHMIAlertSeverity Severity = pAlert->Level;
  int ITISCode =  pAlert->GenericTIM.ITISCode;
  tHMIAlertConfig *pConfig;
  tHMIErrCode Err;
  Err = HMI_GetAlertConfig(pHMI,
                           ITISCode,
                           HMI_ALERT_DIRECTION_UNKNOWN,
                           Severity,
                           &pConfig);

  if (Err != HMI_ERR_NONE)
  {
    d_printf(D_WARN, NULL,
             "No Generic alert configured in HMI for ITIS Code %d Severity %u\n",
             ITISCode, Severity);
  }
  else if (pHMI)
  {
    d_printf(D_DEBUG, NULL, "Alerting for generic TIM ITIS code %d match\n", ITISCode);
    HMI_Alert(pHMI, ITISCode, HMI_ALERT_DIRECTION_UNKNOWN, Severity, NULL, pAlert->DistanceToEvent_m);
  }

  if (pConfig)
  {
    HMI_FreeAlertConfig(pConfig);
  }
}

void OBERx_Threattype_WZA(const tTAAlert *pAlert,
                          tHMI *pHMI)
{
  d_printf(D_TST, NULL, "()\n");
  d_assert(pAlert);
  d_assert(pHMI);
  char AlertStr[40] = {0,};

  size_t Len = 0;
  if (pAlert->RWW.SpeedLimitApplicable)
  {
    Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
             "%ukph ", pAlert->RWW.SpeedLimit_kmph);
    AlertStr[sizeof(AlertStr) - 1] = '\0';
  }

  if (pAlert->RWW.RWWDirection == 0 /* RWW_AHEAD */)
  {
    Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
                    "Start %dm ",
                   (int)pAlert->RWW.Distance2RWW_mm / 1000);
  }
  else if (pAlert->RWW.RWWDirection == 1 /* RWW_BEHIND */)
  {
    Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
                    "End %dm ",
                   (int)pAlert->RWW.Distance2EndRWW_mm / 1000);
  }

  AlertStr[sizeof(AlertStr) - 1] = '\0';

  if (pAlert->RWW.RWWDirection == 1 /* RWW_BEHIND */)
  {
    // For LHD vehicles
    if (pAlert->RWW.InnerHardShoulderStatus >= 0)
    {
      Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
                      "|%s|", (pAlert->RWW.InnerHardShoulderStatus == 2 ? "↑" :
                             (pAlert->RWW.InnerHardShoulderStatus == 1 ? "T" :
                              "_")));
    }
    int i;
    for (i = pAlert->RWW.NumberOfLanes - 1; i >= 0; i--)
    {
      uint8_t NumBytes = (uint8_t)((pAlert->RWW.NumberOfLanes+7)/8);
      uint8_t Lane = (pAlert->RWW.ClosedLanes >> (((NumBytes*8) - 1) - (i + 1))) & 0x1;
      Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
                      "|%s", Lane ? "T" : "↑");
      AlertStr[sizeof(AlertStr) - 1] = '\0';
    }
    Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len, "|");
    AlertStr[sizeof(AlertStr) - 1] = '\0';
    // add hardshoulder bit
    if (pAlert->RWW.OuterHardShoulderStatus >= 0)
    {
      Len += snprintf(AlertStr + Len, sizeof(AlertStr) - Len,
                      "|%s|", (pAlert->RWW.OuterHardShoulderStatus == 2 ? "↑" :
                             (pAlert->RWW.OuterHardShoulderStatus == 1 ? "T" :
                              "_")));
    }
    // TODO: For RHD vehicles
    (void)Len;
  }

  d_printf(D_TST, NULL,"TA_THREATTYPE_WZA\n");
  (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_ROADWORK,
                  HMI_ALERT_DIRECTION_UNKNOWN, pAlert->Level, AlertStr,
                  pAlert->DistanceToEvent_m);

  return;
}

void OBERx_Threattype_SpeedLimit(const tTAAlert *pAlert,
                                 tHMI *pHMI)
{
  d_printf(D_TST, NULL, "()\n");
  d_assert(pAlert);

  d_printf(D_TST, NULL,"TA_THREATTYPE_SPEEDLIMIT\n");

  const tHMIAlertSeverity Severity = pAlert->Level;
  const tHMIAlertDirection Direction = HMI_ALERT_DIRECTION_UNKNOWN;
  tHMIAlertConfig *pConfig = NULL;
  tHMIErrCode Err;
  Err = HMI_GetAlertConfig(pHMI,
                           HMI_ALERT_TYPE_SPEEDLIMIT,
                           Direction, Severity,
                           &pConfig);
  if (Err != HMI_ERR_NONE)
  {
    d_printf(D_WARN, NULL,
             "No TIMSPD alert configured in HMI for severity %u\n",
             Severity);
  }
  else if (pHMI)
  {
    if (pConfig->pText)
    {
      char AlertStr[96] = {0,};
      // replace the string with the speed limit value appended
      size_t Len = snprintf(AlertStr, sizeof(AlertStr) - 1, "%s : %ukph ",
                            pConfig->pText, pAlert->SpeedLimit.SpeedLimit);
      AlertStr[Len] = '\0';
      free(pConfig->pText);
      pConfig->pText = strdup(AlertStr);
    }
    (void)HMI_Alert(pHMI, HMI_ALERT_TYPE_SPEEDLIMIT,
                    Direction,
                    Severity,
                    pConfig->pText,
                    pAlert->DistanceToEvent_m);
  }

  if (pConfig)
  {
    HMI_FreeAlertConfig(pConfig);
  }

  return;
}

/**
 * @}
 */

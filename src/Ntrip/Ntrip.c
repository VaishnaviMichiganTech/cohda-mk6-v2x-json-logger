/**
 * @addtogroup app_ntrip Ntrip client to DSRC forwarder
 * @{
 *
 *
 * @dontinclude Ntrip.c
 */

//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "Ntrip.h"
#include "Ntrip-rx.h"

// v2x-lib includes
#include "conf2.h"
#include "debug-levels.h"
#include "default_stack_defn.h"
#include "v2xlib-plugin-if.h"

#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Ntrip_Cust_App_Ntrip_DebugLevel

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

/// Singleton
tNtrip Ntrip;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

#if defined(UNITTEST)
tNtrip *Ntrip_Ptr(void)
{
  if (Ntrip.Active == true)
    return &Ntrip;
  else
    return NULL;
}
#endif

/**
 * @brief Initialize Ntrip
 * @return Success: 0, Error: @ref TA_ErrCode or negative errno
 */
int Ntrip_Init(void)
{
  int Res;

  memset(&Ntrip, 0, sizeof(Ntrip));

  // Get the Ntrip specific configuration (from the .conf file)
  (void)Ntrip_Cust_App_Ntrip_Copy(&Ntrip.Param, NULL);

  d_fnstart(D_TST, 0, "()\n");

  // Select EU/NA mode
  if (Conf2_GetProtocolMode() == default_stack_ProtocolMode_ITS)
  {
    Ntrip.EU = true;
  }
  else if (Conf2_GetProtocolMode() == default_stack_ProtocolMode_1609)
  {
    Ntrip.NA = true;
  }
  else
  {
    Res = 0;
    goto Exit;
  }

  if (Ntrip.Param.ENABLE == 0)
  {
    Res = 0;
    d_printf(D_NOTICE, 0, "Ntrip not enabled (%d)\n", Ntrip.Param.ENABLE);
    goto Exit;
  }

  // Start reception
  Res = NtripRx_Init();
  if (Res != 0)
  {
    d_printf(D_WARN, 0, "NtripRx_Init() == %d\n", Res);
    goto Exit;
  }

  // Success
  Ntrip.Active = true;

Exit:
  d_fnend(D_TST, 0, "() = %d\n", Res);
  return Res;

}

/**
 * @brief De-initialize the Ntrip feature
 */
void Ntrip_Exit(void)
{
  d_fnstart(D_TST, 0, "()\n");

  NtripRx_Exit();

  Ntrip.Active = false;

  Ntrip_Cust_App_Ntrip_Clean(&Ntrip.Param);

  d_fnend(D_TST, 0, "()\n");
  return;
}


/**
 * @brief V2XLib plugin init wrapper
 */
static int Ntrip_PluginInit(void)
{
  return Ntrip_Init();
}

/**
 * @brief V2XLib plugin exit wrapper
 */
static void Ntrip_PluginExit (void)
{
  Ntrip_Exit();
}

/**
 * @brief V2XLib plugin definition
 */
V2XLIB_PLUGIN("Ntrip client -> DSRC forwarder",
              Ntrip_PluginInit, Ntrip_PluginExit);

/**
 * @}
 */

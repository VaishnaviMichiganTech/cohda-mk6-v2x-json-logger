/**
 * @addtogroup mod_tc_client TC Client Module
 * @{
 *
 * @file tc-client.c TC Client
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2014 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "tc-client.h"

#include "debug-levels.h"
#include "libconfig.h"
#include "lph.h"
#include "util.h"
#include "TC.h"
#include "TC_Err.h"
#include "Vector.h"
#include "Example1609_debug.h"

#include <errno.h>
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

#define D_LEVEL Example1609_Example1609_TCClient_DebugLevel

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

/// load configuration file
static int TCClient_LoadConfig (tTCClientState * pState,
                                const char *pConfigFileName);

/// the thread function which periodically dumps remotes
static void * TCClient_ThreadProc (void *pArg);

/// TC callback - add a remote
TC_CallbackAdd    TCClient_Add;
/// TC callback - update an existing remote
TC_CallbackUpdate TCClient_Update;
/// TC callback - purge a remote
TC_CallbackPurge  TCClient_Purge;

/// print a remote
void TCClient_PrintRemote(const tTCClientState * pState,
                          const char * pTag,
                          const tTCRemote * pRemote);

/// print a remote by ID
void TCClient_PrintID(const tTCClientState * pState,
                      const char * pTag, uint64_t id);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

static const unsigned InvalidClientId = (unsigned)(-1);

//------------------------------------------------------------------------------
// API Functions
//------------------------------------------------------------------------------

/**
 * @brief Allocate TCClient state object, Setup thread and associated data structures
 * @param ppState pointer to a TCClient state object (populated by this function)
 * @param pAttr POSIX thread attributes
 * @param pConfigFileName Configuration filename
 * @return Zero for success or a negative errno
 */
int TCClient_Open (tTCClientState ** ppState,
                   const pthread_attr_t *pAttr,
                   const char *pConfigFileName)
{
  d_fnstart(D_INTERN, 0, "(%p,%p,\"%s\")\n",
            ppState, pAttr, pConfigFileName);
  int Res;
  tTC_ErrCode tcErr = TC_ERR_NONE;
  tTCClientState *pState = NULL;

  // make sure we're given a valid pointer
  if (!ppState)
  {
    Res = -EINVAL;
    d_error(D_ERR, NULL, "\"ppState\" pointer must not be NULL\n");
    goto Error;
  }

  if (*ppState)
  {
    Res = -EINVAL;
    d_error(D_ERR, NULL, "\"ppState\" seems to already be initialised\n");
    goto Error;
  }

  // allocate and check the state structure
  *ppState = (tTCClientState *)calloc(1, sizeof(tTCClientState));

  // check the memory allocation
  if (!(*ppState))
  {
    Res = -ENOMEM;
    d_error(D_ERR, NULL, "Unable to allocate \"state\"\n");
    goto Error;
  }

  // State allocated
  pState = *ppState;

  // set the client ID to INVALID
  pState->clientID = InvalidClientId;

  // Load config settings
  Res = TCClient_LoadConfig(pState, pConfigFileName);
  if (Res != 0)
  {
    d_error(D_ERR, NULL, "TCClient_LoadConfig() != 0\n");
    goto Error;
  }

  // Create thread (if required)
  if (pState->PollingInterval)
  {
    pState->ThreadState |= TCCLIENT_THREAD_STATE_INIT;
    Res = pthread_create(&(pState->ThreadID),
                         pAttr,
                         TCClient_ThreadProc,
                         pState);
    if (Res != 0)
    {
      d_error(D_ERR, NULL, "pthread_create() failed\n");
      goto Error;
    }
  }

  tcErr = TC_Open(&(pState->clientID), TCClient_Add, TCClient_Update, TCClient_Purge, NULL, pState);
  if (tcErr != TC_ERR_NONE)
  {
    Res = -EINVAL;
    d_error(D_ERR, NULL, "TC_Open() failed: %d, %s\n", tcErr, TC_ErrToStr(tcErr));
    pState->clientID = InvalidClientId;
    goto Error;
  }

  // Success!
  Res = 0;

Error:
  if (Res)
  {
    d_fnend(D_ERR, NULL, "Error!: %d\n", Res);
    // if we got as far as opening a client, close it again.
    if (pState)
    {
      TCClient_Close(pState);
    }
  }
  else
  {
    d_fnend(D_ERR, NULL, "Successfully started the TCClient\n");
  }
  return Res;
}

/**
 * @brief Stop execution of TC, free the thread and its associated resources
 * @param pState Client State Handle
 */
void TCClient_Close (tTCClientState * pState)
{
  d_fnstart(D_ERR, NULL, "(%p)\b", pState);

  if (!pState)
  {
    d_fnend(D_ERR, NULL, "Unable to close NULL\n");
    return;
  }

  if(pState->ThreadState)
  {
    d_printf(D_DEBUG, 0, "Stopping thread\n");
    pState->ThreadState |= TCCLIENT_THREAD_STATE_STOP;
    pthread_join(pState->ThreadID, NULL);
    d_printf(D_DEBUG, 0, "Thread Stopped\n");
  }

  // if we had a valid client ID handle then close it.
  if (pState->clientID != InvalidClientId)
  {
    tTC_ErrCode tcErr = TC_Close(pState->clientID);
    if (tcErr)
    {
      d_error(D_ERR, NULL, "TC_Close(%u) failed: %d, %s\n",
              pState->clientID,
              tcErr, TC_ErrToStr(tcErr));
    }
    else
    {
      d_printf(D_DEBUG, 0, "Closed TC\n");
    }
  }
  else
  {
    d_printf(D_DEBUG, 0, "Cannot close TC, invalid handle\n");
  }
  memset(pState, -1, sizeof(*pState));
  free(pState);
  d_fnend(D_ERR, NULL, "()\n");
}

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

/**
 * @brief Print out a Remote
 * @param pState The TCClient state information
 * @param pTag A tag to prefix the remote with
 * @param pRemote The remote to print
 */
void TCClient_PrintRemote(const tTCClientState * pState,
                          const char * pTag,
                          const tTCRemote * pRemote)
{
  if (!pRemote)
  {
    printf("%s: MISSING!\n", pTag);
    return;
  }
  if (pState->Verbose)
  {
    printf("%s: ", pTag);
    TC_fprintf(stdout, pRemote);
    printf("\n"); // eventually add cached ASN
  }
  else
  {
    printf("%s: %"PRIx64"\n", pTag, pRemote->ID);
  }
}

/**
 * @brief Print out a Remote given it's ID
 * @param pState The TCClient state information
 * @param pTag A tag to prefix the remote with
 * @param id The ID of the remote to print
 */
void TCClient_PrintID(const tTCClientState * pState,
                      const char * pTag, uint64_t id)
{
  const tTCRemote * pRemote = TC_Access(id);
  if(pRemote)
  {
    TCClient_PrintRemote(pState, pTag, pRemote);
    TC_Release(pRemote);
  }
  else
  {
    printf("%s: %"PRIx64" MISSING!\n", pTag, id);
  }
}


/**
 * @brief Load TCCLIENT parameters from the config file
 * @param pState TC Client State
 * @param pConfigFileName Filename of the config file
 * @return Zero for success or a negative errno
 */
static int TCClient_LoadConfig (tTCClientState * pState,
                                const char *pConfigFileName)
{
  int Res;
  config_t Config; // configuration object read from config file
  config_t *pConfig = 0;
  config_setting_t *pSetting; // setting for TC in config file
  int ConfigVal = 0;

  pConfig = &Config;
  config_init(pConfig); // initialise the config object

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pConfigFileName) != CONFIG_TRUE)
  {
    d_error(D_ERR, NULL,  "Could not open %s\n", pConfigFileName);
    // "libconfig: %s at line %d\n", config_error_text (pConfig),
    Res = -EINVAL;
    goto Error;
  }

  // Get the 'TCCLIENT' Setting
  pSetting = config_lookup(pConfig, TCCLIENT_CONFIG_PATH_NAME);
  if (pSetting == NULL)
  {
    d_error(D_ERR, NULL,  "config_lookup(%s) failed\n",
            TCCLIENT_CONFIG_PATH_NAME);
    Res = -EINVAL;
    goto Error;
  }

  // config is open, now look for configuration entries
#define GET_OPT(flag, opt, typ)                                 \
  do                                                            \
  {                                                             \
    if (config_setting_lookup_ ## typ(                          \
          pSetting,                                             \
          TCCLIENT_CONFIG_VALUE_NAME_ ## opt,                   \
          &ConfigVal))                                          \
    {                                                           \
      pState->flag = (typ)ConfigVal;                             \
      d_printf(D_INTERN, 0, #flag ": %d\n", ConfigVal);         \
    }                                                           \
    else                                                        \
    {                                                           \
      pState->flag = TCCLIENT_CONFIG_VALUE_DEFAULT_ ## opt;      \
      d_printf(D_INTERN, 0, #flag ": %d (default)\n",           \
               ConfigVal);                                      \
    }                                                           \
  }                                                             \
  while(0)

  GET_OPT(Verbose,         VERBOSE,         bool);
  GET_OPT(PollingInterval, POLLINGINTERVAL, int);

#undef GET_OPT

  // Success!
  Res = 0;

Error:
  config_destroy(pConfig);
  return Res;
}

/**
 * @brief Callback for added remotes
 *
 * @param pAdded The added remote
 * @param pLocalVector where we are
 * @param pLPHPP our current path prediction
 * @param pLPHPH our current path history
 * @param pParam registration parameter (tTCClientState*)
 */
void * TCClient_Add(const tTCRemote * pAdded,
                    const tVector * pLocalVector,
                    const tLPHPP  * pLPHPP,
                    const tLPHPH  * pLPHPH,
                    void * pParam)
{
  d_fnstart(D_INTERN, 0, "(%p, %p, %p, %p, %p)\n",
            pAdded, pLocalVector, pLPHPP, pLPHPH, pParam);
  tTCClientState * pState = (tTCClientState *)pParam;
  TCClient_PrintRemote(pState, "ADD", pAdded);
  // return the low 32-bits of the ID as a fake pointer.
  // in the logs verify that the 'pointer' is maintained
  return (void*)(unsigned long)pAdded->ID;
}

/**
 * @brief Callback for updated remotes
 *
 * @param pUpdated The updated remote
 * @param pLocalVector where we are
 * @param pLPHPP our current path prediction
 * @param pLPHPH our current path history
 * @param pUser per-user parameter for the remote
 * @param pPrevUpdated last time the remote was updated
 * @param pParam registration parameter (tTCClientState*)
 */
void TCClient_Update(const tTCRemote * pUpdated,
                     const tVector * pLocalVector,
                     const tLPHPP  * pLPHPP,
                     const tLPHPH  * pLPHPH,
                     void * pUser,
                     const struct timeval * pPrevUpdated,
                     void * pParam)
{
  d_fnstart(D_INTERN, 0, "(%p, %p, %p, %p, %p, %p, %p)\n",
            pUpdated, pLocalVector, pLPHPP, pLPHPH, pUser, pPrevUpdated, pParam);
  tTCClientState * pState = (tTCClientState *)pParam;
  TCClient_PrintRemote(pState, "UPDATE", pUpdated);
}

/**
 * @brief Callback for purged remotes.
 *
 * @param pPurged The remote about to be removed
 * @param pUser per-user parameter for the remote
 * @param pParam registration parameter (tTCClientState*)
 */
void TCClient_Purge(const tTCRemote * pPurged,
                    void * pUser,
                    void * pParam)
{
  d_fnstart(D_INTERN, 0, "(%p, %p, %p)\n",
            pPurged, pUser, pParam);
  tTCClientState * pState = (tTCClientState *)pParam;
  TCClient_PrintRemote(pState, "REMOVED", pPurged);
}

/**
 * @brief TC Client Periodic thread processing - print remotes
 * @param pArg Pointer to TCClient state object passed as generic input parameter
 *
 */
static void * TCClient_ThreadProc (void *pArg)
{
  d_fnstart(D_INTERN, 0, "(%p)\n", pArg);
  tTCClientState * pState = (tTCClientState *)pArg;

  pState->ThreadState |= TCCLIENT_THREAD_STATE_RUN;
  d_printf(D_DEBUG, NULL, "Started thread\n");

  // Initialise the timespec struct for the polling loop
  struct timespec Time;
  clock_gettime(CLOCK_MONOTONIC, &Time);

  // Thread loop
  while (!(pState->ThreadState & TCCLIENT_THREAD_STATE_STOP))
  {
    d_printf(D_INTERN, 0, "napping...\n");
    // polling delay
    Util_Nap(pState->PollingInterval, &Time);
    d_printf(D_INTERN, 0, "iterating...\n");
    /// iterate and print
    uint64_t id = 0;
    const tTCRemote * pRemote = TC_Iterate(&id);
    while(pRemote)
    {
      TCClient_PrintRemote(pState, "ITT", pRemote);
      pRemote = TC_ReleaseIterate(pRemote, &id);
    }
  }

  // exit thread
  pState->ThreadState |= TCCLIENT_THREAD_STATE_END;
  d_fnend(D_INTERN, 0, "(%p)\n", pArg);
  pthread_exit(NULL);
}

// Close the doxygen group
/**
 * @}
 */

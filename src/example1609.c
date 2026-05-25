/**
 * @addtogroup v2v_example Example IEEE 1609 Application
 * @{
 *
 *  @section v2v_example_overview Overview of the Example Application
 *
 *  This example application shipped with the SDK demonstrates how to use
 *  the Cohda Wireless Stack APIs to develop a V2X application. Full
 *  source code for the application is provided and this can be used as
 *  a basis for the development of new customer applications.
 *
 *  The example application can be run in either an on-board equipment
 *  (OBE) configuration, intended to demonstrate the basic functionality
 *  for a V2X OBE device, or it can be run as an example road-side
 *  equipment (RSE) configuration.
 *
 *  @section v2v_example_building Compiling the Example Application
 *
 *  The application can be compiled for an MKx device by compiling it
 *  with the appropriate mkx-tarball Makefile target (where mkx is
 *  replaced by mk5, or x86 as appropriate in the following):
 *
 *  @code
 *  cd ~/mkx/stack/apps/example1609
 *  make mkx-tarball
 *  @endcode
 *
 *  This will produce the tarball example1609-mkx.tgz which contains the example
 *  application and all required configuration etc. This should be
 *  copied to the MKx device and extracted under the /mnt/ubi directory
 *  as follows:
 *
 *  @code
 *  scp example1609-mkx.tgz user@IP_OF_MKX:/tmp
 *  ssh user@IP_OF_MKX
 *  cd /mnt/ubi
 *  mv /tmp/example1609-mkx.tgz .
 *  tar xzf example1609-mkx.tgz
 *  @endcode
 *
 *  @section v2v_example_running Running the IEEE 1609 Example Application
 *
 *  The application is managed via the rc.example1609 script which is used
 *  to launch the application with a particular configuration and to
 *  then stop the application as follows:
 *
 *  @note The example application requires root permissions when run
 *
 *  @code
 *  # launch the application in OBU configuration
 *  ./rc.example1609 start obu
 *
 *  # stop the application
 *  ./rc.example1609 stop
 *
 *  # launch the application in RSU configuration
 *  ./rc.example1609 start rsu
 *
 *  # stop the application
 *  ./rc.example1609 stop
 *
 *  sudo ./example1609 -f <appl-config> -c <stack-config>
 *
 *  @endcode
 *
 *  @section v2v_example_config Configuring the Example Application
 *
 *  The application is configured using the standard libconfig library.
 *  For more information concerning libconfig in general please see
 *  http://www.hyperrealm.com/libconfig/libconfig_manual.html
 *
 *  The files obu.cfg and rsu.cfg contain the libconfig configuration
 *  for the application in the OBU and RSU modes respectively.
 *  Also present are corresponding stack configuration file (suffix .conf)
 *
 *  @section v2v_example_details Basic details of the Example Application
 *
 *  @example example1609/src/example1609.c
 *  This file contains the main parts of the example application.
 *
 *  The basic operation of the application can be summarised as:
 *
 *   - Determine configuration filenames (application and stack) from command-line arguments
 *   - Parse application configuration file to determine which modules to enable using libconfig
 *   - Initialise basic scheduling parameters for application threads via pthreads
 *   - Setup signal handlers for application so it can be stopped
 *   - Initialise platform using stack configuration file via libPlat_Init()
 *   - Initialise stack components GPSRX, CAN, LPH, VState and EXT
 *     - GPSRX_LibraryInit()
 *     - CANRx_LibraryInit()
 *     - LPH_Init()
 *     - VState_LibraryInit()
 *     - Ext_Init()
 *   - Initialise 1609 Facilities layer via lib1609Fac_Init()
 *   - Start platform components via libPlat_Start()
 *   - Start LPH, VState, EXT component threads
 *     - LPH_ThreadInit()
 *     - VState_ThreadInit()
 *     - Ext_ThreadInit()
 *   - Start Stacks
 *     - Start 1609 Facilities layer via lib1609Fac_Start()
 *   - Start TIM Generic Reception
 *     - TIMGenericRx_Open()
 *   - Start the configured application module components (ie. RSATx, RSARx etc)
 *     - RSATx_Open()
 *     - RSARx_Open()
 *     - RawTx_Open()
 *     - EVWTx_Open()
 *     - OBERx_Open()
 *     - CANVState_Open()
 *     - SPAT_Open()
 *     - MAP_Open()
 *   - Create watchdog thread to periodically print application statistics
 *   - Wait for application to exit via received signal
 *
 *  The example application contains a number of separate modules which
 *  implement the actual OBU / RSU functions - @ref mod_rsa_rx
 *  and @ref mod_evw_tx for OBE and @ref mod_rsa_tx,
 *  @ref mod_raw_tx, @ref mod_spat_tx and @ref mod_map_tx for the RSE.
 *
 *  @ref mod_can_vstate describes how to interface with
 *  the CAN and VState subsystems. These are described in their own sections.
 *
 *  @example example1609/src/rsa-rx/rsa-rx.c
 *
 *  Provides an example of receiving Road Side Alert (RSA) Messages and is
 *  documented in @ref mod_rsa_rx
 *
 *  @example example1609/src/rsa-tx/rsa-tx.c
 *
 *  Provides an example of transmitting Road Side Alert (RSA) Messages and is
 *  documented in @ref mod_rsa_tx
 *
 *  @example example1609/src/raw-tx/raw-tx.c
 *
 *  Provides an example of transmitting raw Wave Service Messages (WSMs) and is
 *  documented in @ref mod_raw_tx
 *
 *  @example example1609/src/evw-tx/evw-tx.c
 *
 *  Provides an example of transmitting BSMs with Emergency Vehicle Warning
 *  information and is  documented in @ref mod_evw_tx
 *
 *  @example example1609/src/can-vstate/can-vstate.c
 *
 *  Provides an example of receiving a message from the CAN bus and then
 *  updating VState accordingly from the data within the CAN message and
 *  is documented in @ref mod_can_vstate
 *
 *  @example example1609/src/tc-client/tc-client.c
 *
 *  Provides an example of registering as a TC Client to receive notifications
 *  of remote vehicles as documented in @ref tc_api
 *
 *  @example example1609/src/tim-generic-rx/tim-generic-rx.c
 *
 *  Provides an example of receiving a TIM message via the @ref cohda_ldm
 *  and raising alerts based on a simple mapping of ITIS code sequences to @ref
 *  hmi_api
 *
 *  @example example1609/src/obe-rx/obe-rx.c
 *
 *  Provides an example of a basic OBE using the @ref tm_api, @ref ta_api and
 *  @ref hmi_api to show warnings for Forward Collision Warning, Emergency
 *  Electronic Brake Light and Intersection Collision Warning.
 *
 *  @example example1609/src/spat-tx/spat-tx.c
 *
 *  Provides an example transmitting SPaT messages and is documented in
 *  @ref mod_spat_tx.
 *
 *  @example example1609/src/map-tx/map-tx.c
 *
 *  Provides an example transmitting MAP messages and is documented in
 *  @ref mod_map_tx.
 *
 *  @example example1609/src/pos-ext/pos-ext.c
 *
 *  Provides an example using external position interface.
 *  @ref mod_pos_ext.
*
 * @file
 *
 * Example application with both OBE and RSE functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2018 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

// Example Application
//    - Initialises and starts the stack library components
//    - Initialises the app threads
//    - Monitors signals (exits on user CTRL-C)

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

// Example app includes
#include "can-vstate.h"
#include "evw-tx.h"
#include "map-tx.h"
#include "obe-rx.h"
#include "pos-ext.h"
#include "raw-tx.h"
#include "rsa-rx.h"
#include "rsa-tx.h"
#include "rsm-trx.h"
#include "spat-tx.h"
#include "sdsm-tx.h"
#include "tc-client.h"
#include "tim-generic-rx.h"
#include "spatmap-log/spatmap-log.h"
#include "sdsm-log/sdsm-log.h" //addedbyvaishnavi
#include "bsm-log/bsm-log.h"   //addedbyvaishnavi
#include "tx-log/tx-log.h"     //addedbyvaishnavi
#ifdef PVDM
#include "PVDM.h"
#endif

// Third-party includes
#include "libconfig.h"

// Stack (v2x-lib) includes
#include "can-rx.h"
#include "can-vsc3.h"
#include "debug-levels.h"
#include "ext.h"
#include "gps-rx.h"
#include "id-global.h"
#include "lib1609fac-mgmt.h"
#ifdef POSIX_1609
#include "libnet1609.h"
#endif // POSIX_1609
#include "libplat-mgmt.h"
#include "log.h"
#include "lph.h"
#include "util.h"
#include "v2xlib-plugin.h"
#include "vstate.h"
#include "TC.h"
#include "Example1609_debug.h"

/// Standard includes
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#if defined(__QNX__)
// QNX defines the union but not the type
typedef union sigval sigval_t;
#endif

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Example1609_Example1609_DebugLevel

/// Stack size (at least 16384, multiple of 16)
#ifdef __aarch64__
/// Need larger stacksize for 64-bit architectures
#define APP_THREAD_STACK_SIZE   262144
#else
#define APP_THREAD_STACK_SIZE   65536
#endif

/// Commandline option for the .cfg file
#define OPTS_TAG_APP_CFG_FILENAME   "-f"
/// Commandline option for the stack config file
#define OPTS_TAG_STACK_CFG_FILENAME  "-c"
/// Commandline option for the extra stack config file
#define OPTS_TAG_EXTRA_CFG_FILENAME  "-C"
/// Commandline option for allowing to override limits of configuration values
#define OPTS_TAG_CONF_ALLOW          "-a"
/// Watchdog thread period
#define WATCHDOG_PERIOD_S       (10)
/// Will try this number of graceful exits before forced exit is resorted to
#define ATTEMPTS_BEFORE_FORCED_EXIT 19
/// Configuration filename max length
#define CONFIG_NAME_MAX_LEN 255

//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------

/// App state
// cppcheck-suppress ctuOneDefinitionRuleViolation
typedef struct App
{
  /// Example app configuration file name
  char pAppConfigFilename[CONFIG_NAME_MAX_LEN];

  /// Example stack configuration file name
  char pStackConfigFilename[CONFIG_NAME_MAX_LEN];

  /// Extra configuration file name
  char pExtraConfigFilename[CONFIG_NAME_MAX_LEN];

  /// App configuration (from config file)
  struct
  {
    bool Watchdog;
    bool RSATx;
    bool RSARx;
    bool RawTx;
    bool EVWTx;
    bool OBERx;
    bool CANVState;
    bool TCClient;
    bool PosExt;
    bool CANVSC3;
    bool TIMGenericRx;
    bool SPATTx;
    bool MAPTx;
    bool SDSMTx;
    bool RSM;
    bool SPATMAPLog; //addedbyvaishnavi
    bool SDSMLog;
    bool BSMLog;     //addedbyvaishnavi
  } Cfg;

  /// Application Module handles
  struct
  {
    tRSATx *pRSATx;
    tRSARx *pRSARx;
    tRawTx *pRawTx;
    tEVWTx *pEVWTx;
    tOBERx *pOBERx;
    tCANVState1609 *pCANVState;
    tTCClientState *pTCClient;
    tSPATTx *pSPATTx;
    tMAPTx *pMAPTx;
    tSDSMTx *pSDSMTx;
    tRSM *pRSM;
  } Hdl;

  /// Whether TIM Generic Rx module is active
  bool TIMGenericRxActive;

  int StackActive;
} tApp;

//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

static int Attr_Init(pthread_attr_t *pAttr);

static int Stack_Init (tApp *pApp, bool ExtraConfigFilename, bool ConfAllow);

static void Stack_Exit ();

static int App_Init (tApp *pApp);

static void App_Exit (void);

static void *SignalThread (void *pArg);

static void SignalHandler (int Sig);

static void TimeoutHandler (sigval_t SigVal);

static void PrintUsage (void);

static void *WatchdogThread (void *pArg);

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------

/// Thread attributes variable
static pthread_attr_t Attr;

/// Watchdog thread
static pthread_t Watchdog_ThreadId;
static int WatchdogThreadHalted = 0;

/// Signal set
static sigset_t SigSet;
/// Signal handler thread
static pthread_t Signal_ThreadId;
/// Signal handler mutex
static pthread_mutex_t SigMutex;
/// Signal handler termination flag
static bool TerminateFlag = false;

/// P1609Tx_SendWSM mutex
pthread_mutex_t P1609Tx_SendWSM_Mutex;

#if defined(V2XATK)
/// ATK functions called by Aerolink
void *(*ll_get)(const char *dev_name);
int (*ll_init)(const char *dev_name);
#endif

/// App state
static tApp _App;

/**
 * @brief Starts stack components and then launches application module threads
 * @param Argc Number of command-line arguments
 * @param ppArgv Array of command-line arguments
 * @return 0 on success, error code on error
 */
int main (int Argc, char ** ppArgv)
{
  int Stat;
  int ArgIndex;
  int GotAppConfigFilename;
  int GotStackConfigFilename;
  bool ConfAllow = false, ExtraConfigFilename = false;
  int ErrCode = 0;

  tApp *pApp = &_App;

  D_LEVEL_INIT();

  printf("\nStarting Example Application...\n");

  // Clear the application state
  memset(pApp, 0x00, sizeof(tApp));

  if (Argc < 5)
  {
    // Force user to specify config file locations
    PrintUsage();
    goto Exit;
  }

  // Get app config filename from input params
  GotAppConfigFilename = 0;
  for (ArgIndex = 0; ArgIndex < Argc; ArgIndex++)
  {
    if (strcmp(ppArgv[ArgIndex], OPTS_TAG_APP_CFG_FILENAME) == 0)
    {
      if ((ArgIndex + 1) < Argc)
      {
        strncpy_t(pApp->pAppConfigFilename, ppArgv[ArgIndex + 1], CONFIG_NAME_MAX_LEN);
        GotAppConfigFilename = 1;
        break;
      }
      else
      {
        fprintf(stderr, "Error: expecting app config filename after %s tag\n",
               OPTS_TAG_APP_CFG_FILENAME);
        PrintUsage();
        goto Exit;
      }
    }
  }
  if (GotAppConfigFilename)
    printf("Reading app configuration from %s...\n",
            pApp->pAppConfigFilename);
  else
  {
    fprintf(stderr,
            "Error: app config file not specified - use \"%s <filename>\"\n",
            OPTS_TAG_APP_CFG_FILENAME);
    PrintUsage();
    goto Exit;
  }

  // Get stack config filename from input params
  GotStackConfigFilename = 0;
  for (ArgIndex = 0; ArgIndex < Argc; ArgIndex++)
  {
    if (strcmp(ppArgv[ArgIndex], OPTS_TAG_STACK_CFG_FILENAME) == 0)
    {
      if ((ArgIndex + 1) < Argc)
      {
        strncpy_t(pApp->pStackConfigFilename, ppArgv[ArgIndex + 1], CONFIG_NAME_MAX_LEN);
        GotStackConfigFilename = 1;
        break;
      }
      else
      {
        fprintf(stderr, "Error: expecting stack config filename after %s tag\n",
               OPTS_TAG_STACK_CFG_FILENAME);
        PrintUsage();
        goto Exit;
      }
    }
  }
  if (GotStackConfigFilename)
    printf("Reading stack configuration from %s...\n", pApp->pStackConfigFilename);
  else
  {
    fprintf(stderr,
            "Error: stack config file not specified - use \"%s <filename>\"\n",
            OPTS_TAG_STACK_CFG_FILENAME);
    PrintUsage();
    goto Exit;
  }

  // Get extra stack config filename from input params
  for (ArgIndex = 0; ArgIndex < Argc; ArgIndex++)
  {
    if (strcmp(ppArgv[ArgIndex], OPTS_TAG_EXTRA_CFG_FILENAME) == 0)
    {
      if ((ArgIndex + 1) < Argc)
      {
        strncpy_t(pApp->pExtraConfigFilename, ppArgv[ArgIndex + 1], CONFIG_NAME_MAX_LEN);
        ExtraConfigFilename = true;
        break;
      }
      else
      {
        fprintf(stderr, "Error: expecting extra stack config filename after %s tag\n",
                OPTS_TAG_EXTRA_CFG_FILENAME);
        PrintUsage();
        goto Exit;
      }
    }
  }
  if (ExtraConfigFilename)
  {
    printf("Reading extra stack configuration from %s...\n",
           pApp->pExtraConfigFilename);
  }

  // Get whether to allow config values to exceed their limits
  for (ArgIndex = 0; ArgIndex < Argc; ArgIndex++)
  {
    if (strcmp(ppArgv[ArgIndex], OPTS_TAG_CONF_ALLOW) == 0)
      ConfAllow = true;
  }

  // Reset the application state
  Stat = App_Init(pApp);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "App_Init() failed, returned %d\n", Stat);
    goto Exit;
  }

  // Init thread attribute data
  Stat = Attr_Init(&Attr);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "Attr_Init() failed, returned %d\n", Stat);
    goto Exit;
  }

  // Set signal mask for other threads, and block
  sigemptyset(&SigSet);
  sigaddset(&SigSet, SIGINT);
  sigaddset(&SigSet, SIGTERM);
  sigaddset(&SigSet, SIGQUIT);
  sigaddset(&SigSet, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &SigSet, NULL);

  // Initialise signal handler termination flag and mutex
  TerminateFlag = false;
  pthread_mutex_init(&SigMutex, NULL);
  pthread_mutex_init(&P1609Tx_SendWSM_Mutex, NULL);

  // Create signal handler thread
  Stat = pthread_create(&Signal_ThreadId, NULL, SignalThread, NULL);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "Signal handler pthread_create failed [%d] %s\n",
             Stat, strerror(Stat));
    goto Exit;
  }

  /// Begin initialising and starting the stack library components
  printf("Initialising and starting stack components...\n");
  Stat = Stack_Init(pApp, ExtraConfigFilename, ConfAllow);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "Stack_Init failed [%d] %s\n", Stat, strerror(Stat));
    goto StackInitError;
  }

  // Start the application threads
  if(pApp->Cfg.SPATMAPLog) //Addedbyvaishnavi
  {
	  int LRes = SpatMapLog_Init(pApp->pStackConfigFilename);
	  if(LRes != 0)
	  {
		  d_printf(D_WARN,NULL,"SpatMapLog_Init(pApp->pStackConfigFilename) failed: %d\n",LRes);

	  }

  }
  if (pApp->Cfg.SDSMLog)
  {
    int LRes = SDSMLog_Init(pApp->pStackConfigFilename);
    if (LRes != 0)
      d_printf(D_WARN,NULL,"SDSMLog_Init failed: %d\n",LRes);
  }
  if (pApp->Cfg.BSMLog)
  {
    int LRes = BSMLog_Init(pApp->pStackConfigFilename);
    if (LRes != 0)
      d_printf(D_WARN,NULL,"BSMLog_Init failed: %d\n",LRes);
  }

  // TX latency logger — always-on; only writes when TX modules call TxLog_Record
  TxLog_Init();

  if (pApp->Cfg.RSATx)
  {
    d_printf(D_INFO, NULL, "Open RSATx...\n");
    ErrCode = RSATx_Open(&(pApp->Hdl.pRSATx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "RSATx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "RSATx Opened OK. (pRSATx=%p)\n",
               pApp->Hdl.pRSATx);
    }
  }

  if (pApp->Cfg.RSARx)
  {
    d_printf(D_INFO, NULL, "Open RSARx...\n");
    ErrCode = RSARx_Open(&(pApp->Hdl.pRSARx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "RSARx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "RSARx Opened OK. (pRSARx=%p)\n",
               pApp->Hdl.pRSARx);
    }
  }

  if (pApp->Cfg.CANVSC3)
  {
    d_printf(D_INFO, NULL, "Open CAN VSC3 Lib...\n");
    CANVSC3_LibraryInit ();
  }

  if (pApp->Cfg.RawTx)
  {
    d_printf(D_INFO, NULL, "Open RawTx...\n");
    ErrCode = RawTx_Open(&(pApp->Hdl.pRawTx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "RawTx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "RawTx Opened OK. (pRawTx=%p)\n",
               pApp->Hdl.pRawTx);
    }
  }

  if (pApp->Cfg.EVWTx)
  {
    d_printf(D_INFO, NULL, "Open EVWTx...\n");
    ErrCode = EVWTx_Open(&(pApp->Hdl.pEVWTx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "EVWTx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "EVWTx Opened OK. (pEVWTx=%p)\n",
               pApp->Hdl.pEVWTx);
    }
  }

  if (pApp->Cfg.SPATTx)
  {
    d_printf(D_INFO, NULL, "Open SPATTx...\n");
    ErrCode = SPATTx_Open(&(pApp->Hdl.pSPATTx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "SPATTx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "SPATTx Opened OK. (pSPATTx=%p)\n",
               pApp->Hdl.pSPATTx);
    }
  }

  if (pApp->Cfg.MAPTx)
  {
    d_printf(D_INFO, NULL, "Open MAPTx...\n");
    ErrCode = MAPTx_Open(&(pApp->Hdl.pMAPTx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "MAPTx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "MAPTx Opened OK. (pMAPTx=%p)\n",
               pApp->Hdl.pMAPTx);
    }
  }

  if (pApp->Cfg.SDSMTx)
  {
    d_printf(D_INFO, NULL, "Open SDSMTx...\n");
    ErrCode = SDSMTx_Open(&(pApp->Hdl.pSDSMTx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "SDSMTx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "SDSMTx Opened OK. (pSDSMTx=%p)\n",
               pApp->Hdl.pSDSMTx);
    }
  }

  if (pApp->Cfg.RSM)
  {
    d_printf(D_INFO, NULL, "Open RSM...\n");
    ErrCode = RSM_Open(&(pApp->Hdl.pRSM), &Attr,
                       pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "RSM Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "RSM Opened OK. (pRSM=%p)\n",
               pApp->Hdl.pRSM);
    }
  }

  if (pApp->Cfg.TIMGenericRx)
  {
    d_printf(D_INFO, NULL, "Open TIMGenericRx...\n");
    ErrCode = TIMGenericRx_Open(pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "TIMGenericRx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "TIMGenericRx Opened OK.\n");
      pApp->TIMGenericRxActive = true;
    }
  }

  if (pApp->Cfg.OBERx)
  {
    d_printf(D_INFO, NULL, "Open OBERx...\n");
    ErrCode = OBERx_Open(&(pApp->Hdl.pOBERx), &Attr,
                         pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "OBERx Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "OBERx Opened OK. (pOBERx=%p)\n",
               pApp->Hdl.pOBERx);
    }
  }

  if (pApp->Cfg.CANVState)
  {
    d_printf(D_INFO, NULL, "Open CANVState...\n");
    ErrCode = CANVState_Open(&(pApp->Hdl.pCANVState), &Attr,
                             pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "CANVState Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "CANVState Opened OK. (pCANVState=%p)\n",
               pApp->Hdl.pCANVState);
    }
  }

  if (pApp->Cfg.TCClient)
  {
    d_printf(D_INFO, NULL, "Open TCClient...\n");
    ErrCode = TCClient_Open(&(pApp->Hdl.pTCClient), &Attr,
                           pApp->pAppConfigFilename);
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "TCClient Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "TCClient Opened OK. (pTCClient=%p)\n",
               pApp->Hdl.pTCClient);
    }
  }

  if (pApp->Cfg.PosExt)
  {
    d_printf(D_INFO, NULL, "Open PosExt...\n");
    ErrCode = PosExt_Init();
    if (ErrCode != 0)
    {
      d_printf(D_ERR, NULL, "PosExt Open Error [%d] %s\n", ErrCode,
               strerror(ErrCode));
      goto AppInitError;
    }
    else
    {
      d_printf(D_INFO, NULL, "PosExt Opened OK. (pTCClient=%p)\n",
               pApp->Hdl.pTCClient);
    }
  }

  // Start watchdog thread
  if (pApp->Cfg.Watchdog)
  {
    Stat = pthread_create(&Watchdog_ThreadId, NULL, WatchdogThread, pApp);
    if (Stat)
    {
      d_printf(D_ERR, NULL, "Watchdog pthread_create failed [%d] %s\n", Stat,
               strerror(Stat));
      goto AppInitError;
    }
  }

  // Finally the plugins
  Stat = Plugin_Init();
  if (Stat != 0)
  {
    d_printf(D_CRIT, NULL, "Plugin_Init() = %d\n", Stat);
    goto PluginInitError;
  }

  // Running
  libPlat_Running();

  // Wait for signal handler to exit threads
  (void) pthread_join(Signal_ThreadId, NULL);

  // Tidy up mutex
  (void) pthread_mutex_destroy(&SigMutex);
  (void) pthread_mutex_destroy(&P1609Tx_SendWSM_Mutex);

  // Clean up initialised components
  Plugin_Exit();
  if (pApp->Cfg.SPATMAPLog)
      SpatMapLog_Exit();
  if (pApp->Cfg.SDSMLog)
  {
    SDSMLog_Exit();
  }
  if (pApp->Cfg.BSMLog)
    BSMLog_Exit();
  TxLog_Exit();
  App_Exit();
  Stack_Exit();

  goto Exit;

PluginInitError:
  App_Exit();
AppInitError:
  Stack_Exit();
StackInitError:

// need to change this to:

  // Close the signal thread
  SignalHandler(SIGTERM);
  (void) pthread_join(Signal_ThreadId, NULL);
  (void) pthread_mutex_destroy(&SigMutex);
  (void) pthread_mutex_destroy(&P1609Tx_SendWSM_Mutex);
Exit:

  return 0;
}

static int Attr_Init(pthread_attr_t *pAttr)
{
  int Res;

  // Init thread attribute data and start signal handler
  Res = pthread_attr_init(pAttr);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "pthread_attr_init failed [%d] %s\n",
             Res, strerror(Res));
    goto Exit;
  }

  #ifndef __aarch64__
  Res = pthread_attr_setinheritsched(pAttr, PTHREAD_EXPLICIT_SCHED);
  if (Res)
  {
    d_printf(D_ERR, NULL, "pthread_attr_setinheritsched failed [%d] %s\n",
             Res, strerror(Res));
    goto Exit;
  }
  #endif

  Res = pthread_attr_setstacksize(pAttr, APP_THREAD_STACK_SIZE);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "pthread_attr_setstacksize failed [%d,%d] %s\n",
             Res, APP_THREAD_STACK_SIZE, strerror(Res));
    goto Exit;
  }

  Res = pthread_attr_setschedpolicy(pAttr, SCHED_FIFO);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "pthread_attr_setschedpolicy failed [%d] %s\n",
             Res, strerror(Res));
    goto Exit;
  }

  {
    struct sched_param Param = { 0 };
    Param.sched_priority = sched_get_priority_min(SCHED_FIFO);
    Res = pthread_attr_setschedparam(pAttr, &Param);
    if (Res != 0)
    {
      d_printf(D_ERR, NULL, "pthread_attr_setschedparam failed [%d] %s\n",
               Res, strerror(Res));
      goto Exit;
    }
  }

  Res = pthread_attr_setdetachstate(pAttr, PTHREAD_CREATE_JOINABLE);
  if (Res != 0)
  {
    d_printf(D_ERR, NULL, "pthread_attr_setdetachstate failed [%d] %s\n",
             Res, strerror(Res));
    goto Exit;
  }

Exit:
  return Res;
}

/**
 * @brief Initialise stack components
 * @param pApp Pointer to application data
 * @param ExtraConfigFilename Get extra stack config filename from input params
 * @param ConfAllow  Whether to allow config values to exceed their limits
 * @return 0 on success, error code on error
 */
static int Stack_Init (tApp *pApp, bool ExtraConfigFilename, bool ConfAllow)
{
  int Stat = 0;

  // Initialise libPlat
  tlibPlat_InitData libPlatInitData;

  if (ExtraConfigFilename)
    libPlatInitData.pUserCfgFile = pApp->pExtraConfigFilename;
  else
    libPlatInitData.pUserCfgFile = NULL;

  libPlatInitData.pDefaultsCfgFile = pApp->pStackConfigFilename;
  libPlatInitData.AllowOutOfSpecValues = ConfAllow;
  libPlatInitData.LimitOutOfSpecValues = false;
  Stat = libPlat_Init(&libPlatInitData);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "libPlat_Init() failed, returned %d\n", Stat);
    goto Exit;
  }

  // re-read the application configuration file and stash it in the log directory
  {
    config_t Config;
    config_init(&Config);
    if (config_read_file(&Config, pApp->pAppConfigFilename) == CONFIG_TRUE)
    {
      char Path[LOG_PATH_MAX+1];
      Log_GetLogDir(Path);
      (void)strncat(Path, "/cfg", LOG_PATH_MAX);
      Path[LOG_PATH_MAX] = '\0';
      int Res = config_write_file(&Config, Path);
      d_printf(D_INFO, NULL, "config_write_file(%p,%s): %s\n",
               &Config, Path, (Res == CONFIG_TRUE) ? "OK" : "Fail");
    }
    else
    {
      d_printf(D_WARN, NULL, "config_read_file(%p,%s): Failed!\n",
               &Config, pApp->pAppConfigFilename);
    }
    config_destroy(&Config);
  }

  #ifdef POSIX_1609
  #if defined(V2XATK)
  {
    ll_init = NULL;
    ll_get = NULL;

    // (pIni->ProtocolMode == default_stack_ProtocolMode_1609)
    {
      void *pDLL = dlopen("lib1609net.so", RTLD_NOW | RTLD_LOCAL);
      if (pDLL != NULL)
      {
        ll_init = dlsym(pDLL, "dot4_ll_init");
        ll_get  = dlsym(pDLL, "dot4_ll_get");
        dlclose(pDLL);
      }
    }

    if ((ll_init == NULL) || (ll_get == NULL))
    {
      d_printf(D_ERR, NULL, "ll_init() or ll_get() unavailable\n");
      goto Exit;
    }
  }
  #endif // V2XATK

  IEEE1609Init();
  #endif // POSIX_1609

  GPSRX_LibraryInit();
  CANRx_LibraryInit();
  Stat = LPH_Init();
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "LPH_Init() failed, returned %d\n", Stat);
    goto FacInitError;
  }

  Stat = VState_LibraryInit();
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "VState_LibraryInit() failed, returned %d\n", Stat);
    goto FacInitError;
  }

  Stat = Ext_Init();
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "Ext_Init() failed, returned %d\n", Stat);
    goto FacInitError;
  }

  // Initialise lib1609Fac
  Stat = lib1609Fac_Init(NULL);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "lib1609Fac_Init() failed, returned %d\n", Stat);
    goto FacInitError;
  }

  // Start libPlat
  Stat = libPlat_Start();
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "libPlat_Start() failed, returned %d\n", Stat);
    goto PlatStartError;
  }

  // Start plat worker threads
  Stat = LPH_ThreadInit(SCHED_FIFO, THREAD_LOW_PRIORITY);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "LPH_ThreadInit() failed, returned %d\n", Stat);
    goto LPHThreadError;
  }
  Stat = VState_ThreadInit(SCHED_FIFO, THREAD_LOW_PRIORITY);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "VState_ThreadInit() failed, returned %d\n", Stat);
    goto VSThreadError;
  }
  Stat = Ext_ThreadInit(SCHED_FIFO, THREAD_LOW_PRIORITY);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "Ext_ThreadInit() failed, returned %d\n", Stat);
    goto ExtThreadError;
  }

  // Start all interface threads
  Stat = GPSRX_ThreadInit(SCHED_FIFO, THREAD_LOW_PRIORITY);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "GPSRX_ThreadInit() failed, returned %d\n", Stat);
    goto GPSThreadError;
  }
  Stat = CANRx_ThreadInit(SCHED_FIFO, THREAD_LOW_PRIORITY);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "CANRx_ThreadInit() failed, returned %d\n", Stat);
    goto CANThreadError;
  }

  // start the TC (no TA)
  Stat = TC_Init(0);
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "TC_Init() failed, returned %d\n", Stat);
    goto TCInitError;
  }

  // Start lib1609Fac
  Stat = lib1609Fac_Start();
  if (Stat != 0)
  {
    d_printf(D_ERR, NULL, "lib1609Fac_Start failed, returned %d\n", Stat);
    goto FacStartError;
  }

  // Successfully initialised everything
  goto Exit;

FacStartError:
  TC_Exit();
TCInitError:
  CANRx_ThreadDeinit();
CANThreadError:
  GPSRX_ThreadDeinit();
GPSThreadError:
  Ext_ThreadDeinit();
ExtThreadError:
  VState_ThreadDeinit();
VSThreadError:
  LPH_ThreadDeinit();
LPHThreadError:
  libPlat_Stop();
PlatStartError:
  (void)lib1609Fac_DeInit();
FacInitError:
#ifdef POSIX_1609
  IEEE1609DeInit();
#endif // POSIX_1609

Exit:
  return Stat;
}

/**
 * @brief Close down all stack components
 */
static void Stack_Exit()
{
  // Destruct in the reverse order
  // Close down the 1609 Library
  (void)lib1609Fac_Stop();

  // Library de-initialisations
  TC_Exit();

  // Shutdown all worker threads
  (void)CANRx_ThreadDeinit();
  (void)GPSRX_ThreadDeinit();

  (void)Ext_ThreadDeinit();
  (void)VState_ThreadDeinit();
  (void)LPH_ThreadDeinit();

  (void)libPlat_Stop();

  (void)lib1609Fac_DeInit();

  Ext_Exit();
  VState_LibraryDeinit();
  LPH_Exit();
  CANRx_LibraryDeinit();
  GPSRX_LibraryDeinit();

#ifdef POSIX_1609
  IEEE1609DeInit();
#endif // POSIX_1609

  (void)libPlat_DeInit();
}

/**
 * @brief Initialise the application (parse application configuration)
 * @param pApp Pointer to application data
 * @return 0 on success, error code on error
 */
static int App_Init (tApp *pApp)
{
  int Res; // return error code
  config_t Config; // configuration object read from config file
  config_t * pConfig;
  int ConfigVal = 0; // temporary variable to store config words
  config_setting_t *pSetting;

  d_fnstart(D_DEBUG, NULL, "(%p)\n", pApp);

  pConfig = &Config;

  // Set some default values (all disabled)
  pApp->Cfg.Watchdog = false;
  pApp->Cfg.RSATx = false;
  pApp->Cfg.RSARx = false;
  pApp->Cfg.RawTx = false;
  pApp->Cfg.EVWTx = false;
  pApp->Cfg.SPATTx = false;
  pApp->Cfg.MAPTx = false;
  pApp->Cfg.SDSMTx = false;
  pApp->Cfg.RSM = false;
  pApp->Cfg.TIMGenericRx = false;
  pApp->Cfg.OBERx = false;
  pApp->Cfg.CANVState = false;
  pApp->Cfg.TCClient = false;
  pApp->Cfg.PosExt = false;
  pApp->Cfg.CANVSC3 = false;
  pApp->Cfg.SPATMAPLog = false;
  pApp->Cfg.SDSMLog = false;
  pApp->Cfg.BSMLog = false;

  // Read settings from config file

  config_init(pConfig); // initialise the config object

  // Try to read the specified config file into the config object
  if (config_read_file(pConfig, pApp->pAppConfigFilename) != CONFIG_TRUE)
  {
    d_printf(D_ERR, NULL, "Could not open %s\n", pApp->pAppConfigFilename);
    d_printf(D_ERR, NULL, "libconfig: %s at line %d\n",
             config_error_text(pConfig), config_error_line(pConfig));
    Res = -EBADFD;
    goto Error;
  }

  // Get the application Setting
  pSetting = config_lookup(pConfig, "Example.APP");
  if (pSetting == NULL)
  {
    Res = -ESRCH;
    d_printf(D_WARN, 0, "Missing 'Example.APP' member in %s\n", pApp->pAppConfigFilename);
    goto Error;
  }

  // Config is open, now look for configuration entries

#define SET_BOOL(x)                     \
  do                                    \
  {                                     \
    if (config_setting_lookup_bool      \
        (pSetting, #x, &ConfigVal))     \
    {                                   \
      pApp->Cfg.x = (bool) ConfigVal;   \
      d_printf(D_INTERN, 0,             \
               #x ": %d\n", ConfigVal); \
    }                                   \
    else                                \
    {                                   \
      pApp->Cfg.x = false;              \
    }                                   \
  }                                     \
  while(0)

  SET_BOOL(Watchdog);
  SET_BOOL(RSATx);
  SET_BOOL(RSARx);
  SET_BOOL(RawTx);
  SET_BOOL(EVWTx);
  SET_BOOL(SPATTx);
  SET_BOOL(MAPTx);
  SET_BOOL(SDSMTx);
  SET_BOOL(RSM);
  SET_BOOL(TIMGenericRx);
  SET_BOOL(OBERx);
  SET_BOOL(CANVState);
  SET_BOOL(TCClient);
  SET_BOOL(PosExt);
  SET_BOOL(CANVSC3);
  SET_BOOL(SPATMAPLog);
  SET_BOOL(SDSMLog);
  SET_BOOL(BSMLog);

#undef SET_BOOL

  // Success!
  Res = 0;

Error:
  // Update complete - close the configuration
  config_destroy(pConfig);

  d_fnend(D_DEBUG, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief Periodically output statistics of various modules
 * @param pArg A pointer to the application data (tApp)
 */
static void *WatchdogThread (void *pArg)
{
  tApp *pApp = (tApp *) pArg;

  while (WatchdogThreadHalted == 0)
  {
    if (pApp->Hdl.pRSATx != NULL)
      RSATx_PrintStats(pApp->Hdl.pRSATx);
    if (pApp->Hdl.pRSARx != NULL)
      RSARx_PrintStats(pApp->Hdl.pRSARx);
    if (pApp->Hdl.pRawTx != NULL)
      RawTx_PrintStats(pApp->Hdl.pRawTx);
    if (pApp->Hdl.pEVWTx != NULL)
      EVWTx_PrintStats(pApp->Hdl.pEVWTx);
    if (pApp->Hdl.pSPATTx != NULL)
      SPATTx_PrintStats(pApp->Hdl.pSPATTx);
    if (pApp->Hdl.pMAPTx != NULL)
      MAPTx_PrintStats(pApp->Hdl.pMAPTx);
    if (pApp->Hdl.pSDSMTx != NULL)
      SDSMTx_PrintStats(pApp->Hdl.pSDSMTx);
    if (pApp->Hdl.pRSM != NULL)
      RSM_PrintStats(pApp->Hdl.pRSM);
    if (pApp->Hdl.pOBERx != NULL)
      OBERx_PrintStats(pApp->Hdl.pOBERx);
    if (pApp->Hdl.pCANVState != NULL)
      CANVState_PrintStats(pApp->Hdl.pCANVState);
    sleep(WATCHDOG_PERIOD_S);
  }

  (void) pthread_exit(NULL);
}

/**
 * @brief Print basic help type message for application usage
 */
static void PrintUsage (void)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  example %s <example_cfg_filename> %s <stack_cfg_filename>\n",
          OPTS_TAG_APP_CFG_FILENAME, OPTS_TAG_STACK_CFG_FILENAME);
  fprintf(stderr, "Where: \n"
          "  - example_cfg_filename - config file for app level parameters\n"
          "  - stack_cfg_filename   - config file for low level parameters\n");
  fprintf(stderr, "Note: \n"
          "  - must be run as root (sudo)\n"
          "  - example_cfg_filename must be positioned before stack params\n"
          "  - other stack params may follow stack_cfg_filename\n");
}

/**
 * @brief Stop threads, shutdown stack and free any memory
 */
static void App_Exit (void)
{
  tApp *pApp = &_App;

  d_fnstart(D_DEBUG, NULL, "()\n");

  // set the global shutdown flag
  Util_SetShutdownFlagBecause("App Exit");

  Plugin_Exit();

  if (pApp->Cfg.Watchdog)
  {
    // Close down Watchdog thread
    WatchdogThreadHalted = 1;
    // Wait for thread termination
    pthread_join(Watchdog_ThreadId, NULL);
  }

  // Final dump of stats before closing
  if (pApp->Hdl.pRSATx != NULL)
    RSATx_PrintStats(pApp->Hdl.pRSATx);
  if (pApp->Hdl.pRawTx != NULL)
    RawTx_PrintStats(pApp->Hdl.pRawTx);
  if (pApp->Hdl.pEVWTx != NULL)
    EVWTx_PrintStats(pApp->Hdl.pEVWTx);
  if (pApp->Hdl.pSPATTx != NULL)
    SPATTx_PrintStats(pApp->Hdl.pSPATTx);
  if (pApp->Hdl.pMAPTx != NULL)
    MAPTx_PrintStats(pApp->Hdl.pMAPTx);
  if (pApp->Hdl.pSDSMTx != NULL)
    SDSMTx_PrintStats(pApp->Hdl.pSDSMTx);
  if (pApp->Hdl.pRSM != NULL)
    RSM_PrintStats(pApp->Hdl.pRSM);
  if (pApp->Hdl.pRSARx != NULL)
    RSARx_PrintStats(pApp->Hdl.pRSARx);
  if (pApp->Hdl.pOBERx != NULL)
    OBERx_PrintStats(pApp->Hdl.pOBERx);
  if (pApp->Hdl.pCANVState != NULL)
    CANVState_PrintStats(pApp->Hdl.pCANVState);

  // Close RSATx / RSARx / RawTx / EVWTx / SPATTx / MAPTx / SDSMTx / TIMGenericRx /
  //       OBERx / CANVState / TCClient / CANVSC3 / PosExt (stops threads, free memory)
  if (pApp->Hdl.pRSATx != NULL)
  {
    RSATx_Close(pApp->Hdl.pRSATx);
    pApp->Hdl.pRSATx = NULL;
  }
  if (pApp->Hdl.pRSARx != NULL)
  {
    RSARx_Close(pApp->Hdl.pRSARx);
    pApp->Hdl.pRSARx = NULL;
  }
  if (pApp->Hdl.pRawTx != NULL)
  {
    RawTx_Close(pApp->Hdl.pRawTx);
    pApp->Hdl.pRawTx = NULL;
  }
  if (pApp->Hdl.pEVWTx != NULL)
  {
    EVWTx_Close(pApp->Hdl.pEVWTx);
    pApp->Hdl.pEVWTx = NULL;
  }
  if (pApp->Hdl.pSPATTx != NULL)
  {
    SPATTx_Close(pApp->Hdl.pSPATTx);
    pApp->Hdl.pSPATTx = NULL;
  }
  if (pApp->Hdl.pMAPTx != NULL)
  {
    MAPTx_Close(pApp->Hdl.pMAPTx);
    pApp->Hdl.pMAPTx = NULL;
  }
  if (pApp->Hdl.pSDSMTx != NULL)
  {
    SDSMTx_Close(pApp->Hdl.pSDSMTx);
    pApp->Hdl.pSDSMTx = NULL;
  }
  if (pApp->Hdl.pRSM != NULL)
  {
    RSM_Close(pApp->Hdl.pRSM);
    pApp->Hdl.pRSM = NULL;
  }
  if (pApp->Hdl.pOBERx != NULL)
  {
    OBERx_Close(pApp->Hdl.pOBERx);
    pApp->Hdl.pOBERx = NULL;
  }
  if (pApp->Hdl.pCANVState != NULL)
  {
    CANVState_Close(pApp->Hdl.pCANVState);
    pApp->Hdl.pCANVState = NULL;
  }
  if (pApp->Hdl.pTCClient != NULL)
  {
    TCClient_Close(pApp->Hdl.pTCClient);
    pApp->Hdl.pTCClient = NULL;
  }
  if (pApp->Cfg.CANVSC3)
  {
    CANVSC3_LibraryDeinit();
  }
  if (pApp->Cfg.PosExt)
  {
    PosExt_DeInit();
  }

  if (pApp->TIMGenericRxActive)
  {
    TIMGenericRx_Close();
    pApp->TIMGenericRxActive = false;
  }

  // Tidy up
  (void) pthread_attr_destroy(&Attr);

  d_fnend(D_DEBUG, NULL, "()\n");

  return;
}

/**
 * @brief General signal handler function for the application
 * @param Sig The signal which was received
 */
static void SignalHandler (int Sig)
{
  switch (Sig)
  {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
    case SIGHUP:
    {
      d_printf(D_INFO, NULL, "Caught signal %d - terminating\n", Sig);
      pthread_mutex_lock(&SigMutex);
      TerminateFlag = true;
      pthread_mutex_unlock(&SigMutex);
      break;
    }
    default:
    {
      d_printf(D_INFO, NULL, "Caught signal %d - no action\n", Sig);
      break;
    }
  }
}

/**
 * @brief Force exit the application - used after a graceful exit attempt
 * @param SigVal The signal which was received
 */
static void TimeoutHandler (sigval_t SigVal)
{
  (void) SigVal;
  // Force exit
  d_error(D_ERR, NULL, "Forced exit after termination timeout expired\n");
  exit(1);
}

/**
 * @brief Signal handler thread - set up signal handlers and exit app when needed
 * @param pArg Unused
 */
static void *SignalThread (void *pArg)
{
  (void)pArg;

  // Timer info
  timer_t ShutdownTimerId;
  struct itimerspec ShutdownT;
  struct sigevent ShutdownEvt;

  // Install signal handlers
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);
  signal(SIGQUIT, SignalHandler);
  signal(SIGHUP, SignalHandler);

  int SigThread = 1;
  int TerminateAttempts = 0;
  int TStat;
  bool HandleTerminate = false;

  while (SigThread == 1)
  {
    // Check for program termination once a second
    sleep(1);

    // Read Terminate flag (if set, clear and handle)
    pthread_mutex_lock(&SigMutex);
    if (TerminateFlag)
    {
      TerminateFlag = false;
      HandleTerminate = true;
    }
    pthread_mutex_unlock(&SigMutex);

    // Read HandleTerminate flag (if set, clear and handle)
    if (HandleTerminate)
    {
      HandleTerminate = false;
      if (TerminateAttempts < ATTEMPTS_BEFORE_FORCED_EXIT)
      {
        // Attempt graceful exit
        d_printf(D_NOTICE, NULL, "Exiting gracefully on signal\n");
        printf("Terminating Example Application...\n");

        // Create timer for shutdown limit time
        memset(&ShutdownEvt, 0, sizeof(ShutdownEvt));

        // Timer handler action
        ShutdownEvt.sigev_notify = SIGEV_THREAD;
        ShutdownEvt.sigev_notify_function = TimeoutHandler;

        TStat = timer_create(CLOCK_REALTIME, &ShutdownEvt, &ShutdownTimerId);
        if (TStat != 0)
        {
          d_printf(D_ERR, NULL, "Timer create failed [%d] %s\n", errno,
                   strerror(errno));
        }

        ShutdownT.it_interval.tv_sec = 0;
        ShutdownT.it_interval.tv_nsec = 0;
        ShutdownT.it_value.tv_sec = 30;
        ShutdownT.it_value.tv_nsec = 0;
        TStat = timer_settime(ShutdownTimerId, 0, &ShutdownT, NULL);
        if (TStat != 0)
        {
          d_printf(D_ERR, NULL, "Timer start failed [%d] %s\n", errno,
                   strerror(errno));
        }

        TerminateAttempts++;
        SigThread = 0;
      }
      else
      {
        // Forced exit
        d_error(D_ERR, NULL,
                "Forced exit after %d failed attempts\n", TerminateAttempts);
        exit(1);
      }
    }
  }
  pthread_exit(NULL);
}


// Close the doxygen group
/**
 * @}
 */

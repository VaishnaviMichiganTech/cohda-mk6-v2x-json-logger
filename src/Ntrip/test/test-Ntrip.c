//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "test-Ntrip-rx.h"

#include "Ntrip.h"
#include "Ntrip-rx.h"
#include "conf.h"
#include "debug-levels.h"
#include "default_stack_defn.h"
#include "v2xlib-plugin-if.h"

#include "TextUIRunner.h"
#include "TextOutputter.h"
#include "CompilerOutputter.h"
#include "XMLOutputter.h"
#include "EmbUnit.h"
#include "CohdaUtils.h"

#include <errno.h>
#include <string.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
extern TestRef Test_NtripRxTests(void);
extern tV2XLibPlugin V2XLIB_PLUGIN_SYMBOL_NAME;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
D_LEVEL_DECLARE();

//------------------------------------------------------------------------------
// Mocked API functions
//
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Test functions
//------------------------------------------------------------------------------

static void Test_SetUp(void)
{
}

static void Test_TearDown(void)
{
}

static void Test_Ntrip(void)
{
  ProtocolMode_1609;

  {
    tNtrip_Cust_App_Ntrip *pCfg = Ntrip_Clone();
    pCfg->DebugLevel = 255;
    pCfg->ENABLE = 1;
    free((void *)(pCfg->URI));
    pCfg->URI = strdup("ntrip://username:password^ntrip.example.com:80/mountpoint");
    Ntrip_Push(&pCfg);
  }

  int Init = -1;

  // In NA mode
  {
    ProtocolMode_1609;

    // Ntrip init
    tNtrip *pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NULL(pNtrip);
    Init = Ntrip_Init();
    TEST_ASSERT_EQ_INT_MSG(0, Init, "Error initialising NA Ntrip");
    pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NOT_NULL(pNtrip);

    // Ntrip teardown
    Ntrip_Exit();
    pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NULL(pNtrip);
  } // NA

  // In EU mode
  {
    ProtocolMode_ITS;

    // Ntrip init
    tNtrip *pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NULL(pNtrip);
    Init = Ntrip_Init();
    TEST_ASSERT_EQ_INT_MSG(0, Init, "Error initialising EU Ntrip");
    pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NOT_NULL(pNtrip);

    // Ntrip teardown
    Ntrip_Exit();
    pNtrip = Ntrip_Ptr();
    TEST_ASSERT_NULL(pNtrip);
  } // EU

  // Default parameters
  Init = Ntrip_Init();
  TEST_ASSERT_EQ_INT_MSG(0, Init, "Error initialising default Ntrip");
  Ntrip_Exit();

  // Plugin init/exit
  V2XLIB_PLUGIN_SYMBOL_NAME.Init();
  V2XLIB_PLUGIN_SYMBOL_NAME.Exit();
}

TestRef Test_NtripTests(void)
{
  EMB_UNIT_TESTFIXTURES(Test_Fixtures)
  {
#define FIXTURE(x) new_TestFixture(#x, x)
    FIXTURE(Test_Ntrip),
#undef FIXTURE
  };

  EMB_UNIT_TESTCALLER(Test_Caller,"Test_NtripTests",
                      Test_SetUp,
                      Test_TearDown,
                      Test_Fixtures);

  return (TestRef)&Test_Caller;
}


int main (int argc, const char* argv[])
{
  (void)argc;(void)argv;

  // Make sure the XML stuff is the first thing on stderr!
  // if other stuff is output first then the XML parser fails
  TextUIRunner_setOutputter(XMLOutputter_outputter());
  TextUIRunner_start();

  D_LEVEL_INIT();

  (void)ConfInitLib("test.conf", NULL, CONF_MODE_ENFORCING, NULL, NULL);

  ReConfig("Cohda_DebugLevel = 255;");
  Policy_SCHED_OTHER;

  TextUIRunner_runTest(Test_NtripTests());
  TextUIRunner_runTest(Test_NtripRx_tests());

  ConfDeInitLib();

  TextUIRunner_end();
  return 0;
}

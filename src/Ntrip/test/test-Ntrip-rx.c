//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "test-Ntrip-rx.h"

#include "Ntrip-rx.h"
#include "Ntrip.h"
#include "TC.h"
#include "conf.h"
#include "util.h"
#include "debug-levels.h"

#include "EmbUnit.h"

#include <stdio.h>
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

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Test functions
//------------------------------------------------------------------------------

static void Test_SetUp(void)
{
  ;
}

static void Test_TearDown(void)
{
  NtripRx_Exit();
}

static void Test_NtripRx(void)
{
  int Init = Ntrip_Init();
  TEST_ASSERT_EQ_INT_MSG(0, Init, "Error initialising default Ntrip");

  Init = NtripRx_Init();
  TEST_ASSERT_EQ_INT_MSG(0, Init, "Unable to initialise NtripRx");
  NtripRx_Exit();

  Ntrip_Exit();
}


TestRef Test_NtripRx_tests(void)
{
  EMB_UNIT_TESTFIXTURES(Test_Fixtures)
  {
#define FIXTURE(x) new_TestFixture(#x, x)
    FIXTURE(Test_NtripRx),
#undef FIXTURE
  };

  EMB_UNIT_TESTCALLER(Test_Caller,"NtripRx_Tests",
                      Test_SetUp,
                      Test_TearDown,
                      Test_Fixtures);

  return (TestRef)&Test_Caller;
  ;
}

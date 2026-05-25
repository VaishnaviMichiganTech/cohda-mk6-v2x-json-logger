/**
 * @addtogroup mod_tim_generic_rx TIM Generic ITIS Match Module
 * @{
 *
 * @section mod_tim_generic_rx What Does It Do
 * Demonstrates the usage of the @ref ta_api and @ref tm_api to process TIM
 * ITIS codes.
 *
 * @file tim-generic-rx.h TIM generic receiver definitions
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2017 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

#ifndef __TIM_GENERIC_RX_H__
#define __TIM_GENERIC_RX_H__

// ----------------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

int TIMGenericRx_Open(const char *pConfigFileName);
int TIMGenericRx_Close(void);

#endif //  __TIMGENERIC_RX_H__
/**
 * @}
 */

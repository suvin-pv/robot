// ----------------------------------------------------------------------------
// File Name    : idr_app_threadx.h
// Description  : Declares the C-linkage initialization entry prototype for
//                application-level ThreadX startup.
// ----------------------------------------------------------------------------
// Revision History (Version, Date, Name, Description)
// ----------------------------------------------------------------------------
// v01.00, 04/11/2026, Anusree, Initial creation
//
//
// <Add revision history above>
// ----------------------------------------------------------------------------
// Experion Technologies (P) Ltd. All rights reserved.
// ----------------------------------------------------------------------------

#ifndef IDR_APP_THREADX_H
#define IDR_APP_THREADX_H

// --------------------------------INCLUDES------------------------------------
#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------DECLARATIONS---------------------------------

UINT IdrAppThreadxInit(TX_BYTE_POOL *bytePool);

#ifdef __cplusplus
}
#endif

#endif // IDR_APP_THREADX_H

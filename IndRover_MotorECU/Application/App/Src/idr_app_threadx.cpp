// ----------------------------------------------------------------------------
// File Name    : idr_app_threadx.cpp
// Description  : Implements application ThreadX initialization, orchestrating
//                deterministic creation of RTOS objects and threads.
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

// --------------------------------INCLUDES------------------------------------
#include "idr_app_threadx.h"
#include "idr_tx_objects.hpp"
#include "idr_thread_cmd_mgr.hpp"

// ----------------------------PUBLIC FUNCTIONS--------------------------------

extern "C" UINT IdrAppThreadxInit(TX_BYTE_POOL *bytePool)
{
	UINT retVal = TX_SUCCESS;

	if (nullptr == bytePool)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		// 1. Create all system RTOS objects (queues)
		retVal = IdrCreateObjects(bytePool);

		// 2. Create and start tCmdMgr thread
		if (TX_SUCCESS == retVal)
		{
			retVal = IdrThreadCmdMgrCreate(bytePool);
		}
	}

	return retVal;
}

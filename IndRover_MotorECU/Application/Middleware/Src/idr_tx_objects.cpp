// ----------------------------------------------------------------------------
// File Name    : idr_tx_objects.cpp
// Description  : Defines and implements the IdrQueue class methods, allocates
//                queue memory from byte pool, and creates system RTOS objects.
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

// ---------------------------------INCLUDES-----------------------------------
#include "idr_tx_objects.hpp"

// ------------------------------GLOBAL OBJECTS--------------------------------

IdrQueue idr_q_cmd; //command queue between tCmdRx and tCmdMgr

// -----------------------PUBLIC MEMBER FUNCTIONS - IdrQueue-------------------

UINT IdrQueue::Create(TX_BYTE_POOL *bytePool, const char *szName, ULONG ulMessageWords, ULONG ulDepth)
{
	UINT retVal = TX_SUCCESS;

	// Check pointer parameters first
	if (nullptr == bytePool)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		const ULONG ulSizeBytes = ulMessageWords * sizeof(ULONG) * ulDepth;

		// Allocate dedicated queue storage buffer from byte pool
		retVal = tx_byte_allocate(bytePool, &m_pStorage, ulSizeBytes, TX_NO_WAIT);
		if (TX_SUCCESS == retVal)
		{
			retVal = tx_queue_create(
				&m_queue,
				const_cast<CHAR *>(szName),
				ulMessageWords,
				m_pStorage,
				ulSizeBytes
			);
		}
	}

	return retVal;
}

UINT IdrQueue::Receive(void *destPtr, ULONG ulWaitOption)
{
	UINT retVal = TX_SUCCESS;

	if (nullptr == destPtr)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		retVal = tx_queue_receive(&m_queue, destPtr, ulWaitOption);
	}

	return retVal;
}

UINT IdrQueue::Send(const void *sourcePtr, ULONG ulWaitOption)
{
	UINT retVal = TX_SUCCESS;

	if (nullptr == sourcePtr)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		retVal = tx_queue_send(&m_queue, const_cast<void *>(sourcePtr), ulWaitOption);
	}

	return retVal;
}

// ----------------------------PUBLIC FUNCTIONS--------------------------------

UINT IdrCreateObjects(TX_BYTE_POOL *bytePool)
{
	UINT retVal = TX_SUCCESS;

	if (nullptr == bytePool)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		// Create idr_q_cmd: 8 words (32 bytes), depth 8 (256 bytes)
		retVal = idr_q_cmd.Create(
			bytePool,
			"idr_q_cmd",
			IDR_CMD_Q_MSG_WORDS,
			IDR_CMD_Q_DEPTH
		);
	}

	return retVal;
}

// File Name    : idr_thread_cmd_mgr.hpp
// Description  : Implements stack allocation, thread registration, and the
//                main execution entry loop for the tCmdMgr thread.
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
#include "idr_thread_cmd_mgr.hpp"
#include "idr_tx_objects.hpp"

// -----------------------------GLOBAL OBJECTS---------------------------------

// Thread Control Block for tCmdMgr (preserved by RTOS scheduler)
TX_THREAD tCmdMgrTcb;

// -----------------LOCAL FILE-SCOPE VARIABLES (UNNAMED NAMESPACE--------------
namespace
{
	// Pointer to stack memory allocated from byte pool
	VOID *s_pCmdMgrStack = nullptr;
}

// -------------------------------PUBLIC FUNCTIONS----------------------------

UINT IdrThreadCmdMgrCreate(TX_BYTE_POOL *bytePool)
{
	UINT retVal = TX_SUCCESS;

	if (nullptr == bytePool)
	{
		retVal = TX_PTR_ERROR;
	}
	else
	{
		// Allocate 1024 bytes of stack space from the byte pool
		retVal = tx_byte_allocate(
			bytePool,
			&s_pCmdMgrStack,
			IDR_CMD_MGR_STACK_SIZE,
			TX_NO_WAIT
		);

		if (TX_SUCCESS == retVal)
		{
			retVal = tx_thread_create(
				&tCmdMgrTcb,
				const_cast<CHAR *>("tCmdMgr"),
				IdrThreadCmdMgrEntry,
				0U,
				s_pCmdMgrStack,
				IDR_CMD_MGR_STACK_SIZE,
				IDR_CMD_MGR_PRIORITY,
				IDR_CMD_MGR_PRIORITY,
				TX_NO_TIME_SLICE,
				TX_AUTO_START
			);
		}
	}

	return retVal;
}

VOID IdrThreadCmdMgrEntry(ULONG ulThreadInput)
{
	(void)ulThreadInput;

	// Local stack variable to receive the 32-byte idr_cmd_msg_t struct by value
	idr_cmd_msg_t receivedCmd{};

	for (;;)
	{
		// Block until a validated command struct is posted to idr_q_cmd
		UINT status = idr_q_cmd.Receive(&receivedCmd, TX_WAIT_FOREVER);
		if (TX_SUCCESS == status)
		{
			// Validated command struct received into receivedCmd by value:
			// - receivedCmd.rx_seq
			// - receivedCmd.cmd_id
			// - receivedCmd.kind (0=MOVE, 1=DIAG)
			// - receivedCmd.sub  (FRWD, REVSE, LEFT, RIGHT, ROTATE, STOP)
			// - receivedCmd.speed_mmps
			// - receivedCmd.time_ms
			// - receivedCmd.distance_mm
			// - receivedCmd.angle_ddeg
			//
			// In this feature branch, we strictly receive it.
			(void)receivedCmd;
		}
	}
}

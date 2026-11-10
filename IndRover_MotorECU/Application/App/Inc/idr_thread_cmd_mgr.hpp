// ----------------------------------------------------------------------------
// File Name    : idr_thread_cmd_mgr.hpp
// Description  : Declares thread configurations, constants, control blocks,
//                and function prototypes for the tCmdMgr thread.
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

#ifndef IDR_THREAD_CMD_MGR_HPP
#define IDR_THREAD_CMD_MGR_HPP

// --------------------------------INCLUDES------------------------------------
#include "idr_types.hpp"
#include "tx_api.h"

// ---------------------------MACROS & CONSTANTS-------------------------------
#define IDR_CMD_MGR_STACK_SIZE    (1024U)   // 1024 bytes (1 KB) stack
#define IDR_CMD_MGR_PRIORITY      (4U)      // Priority 4 in ThreadX

// -------------------------------DECLARATIONS---------------------------------

// Thread Control Block for tCmdMgr
extern TX_THREAD tCmdMgrTcb;

UINT IdrThreadCmdMgrCreate(TX_BYTE_POOL *bytePool);
VOID IdrThreadCmdMgrEntry(ULONG ulThreadInput);

#endif // IDR_THREAD_CMD_MGR_HPP

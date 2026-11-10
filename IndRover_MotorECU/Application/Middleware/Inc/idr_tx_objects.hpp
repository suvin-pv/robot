// ----------------------------------------------------------------------------
// File Name    : idr_tx_objects.hpp
// Description  : Declares the IdrQueue C++ wrapper class and the centralized
//                system RTOS object creation prototype.
// ----------------------------------------------------------------------------
// Revision History (Version, Date, Name, Description)
// ----------------------------------------------------------------------------
// v01.00, 05/11/2026, Anusree, Initial creation
//
//
// <Add revision history above>
// ----------------------------------------------------------------------------
// Experion Technologies (P) Ltd. All rights reserved.
// ----------------------------------------------------------------------------

#ifndef IDR_TX_OBJECTS_HPP
#define IDR_TX_OBJECTS_HPP

// --------------------------------INCLUDES------------------------------------
#include "idr_types.hpp"
#include "tx_api.h"

// ---------------------------------CLASSES------------------------------------
// ThreadX Queue C++ RAII/Object-Oriented wrapper class.
// Encapsulates buffer allocation and native TX_QUEUE operations.
class IdrQueue
{
public:
	IdrQueue(void) = default;
	~IdrQueue(void) = default;

	// Description : Allocates contiguous storage from bytePool and creates TX_QUEUE.
	// Parameters  :
	// 1. INOUT TX_BYTE_POOL* bytePool   - Pointer to system byte pool
	// 2. IN const char* szName          - Queue debug name string
	// 3. IN ULONG ulMessageWords        - Message size in 32-bit words (1, 2, 4, 8, 16)
	// 4. IN ULONG ulDepth               - Queue depth capacity (message count)
	// Return      :
	// 1. TX_SUCCESS on success
	// 2. TX_PTR_ERROR if pointer is NULL
	// 3. ThreadX error code on failure
	UINT Create(TX_BYTE_POOL *bytePool, const char *szName, ULONG ulMessageWords, ULONG ulDepth);

	// Description : Receives a message by value from the queue.
	// Parameters  :
	// 1. OUT void* destPtr              - Destination buffer (must hold message size)
	// 2. IN ULONG ulWaitOption          - TX_WAIT_FOREVER, TX_NO_WAIT, or ticks
	// Return      :
	// 1. TX_SUCCESS on success
	// 2. TX_QUEUE_EMPTY / TX_WAIT_ABORTED on timeout/failure
	UINT Receive(void *destPtr, ULONG ulWaitOption = TX_WAIT_FOREVER);

	// Description : Sends a message by value into the queue. Senders never block.
	// Parameters  :
	// 1. IN const void* sourcePtr       - Pointer to source message to copy
	// 2. IN ULONG ulWaitOption          - TX_NO_WAIT per communication principles
	// Return      :
	// 1. TX_SUCCESS on success
	// 2. TX_QUEUE_FULL if queue capacity is exceeded
	UINT Send(const void *sourcePtr, ULONG ulWaitOption = TX_NO_WAIT);

	// Description : Returns a pointer to the underlying native ThreadX TX_QUEUE control block.
	// Parameters  : (void)
	// Return      : Pointer to internal TX_QUEUE
	TX_QUEUE* GetHandle(void)
	{
		return &m_queue;
	}

private:
	TX_QUEUE m_queue{};
	VOID    *m_pStorage{nullptr};
};

// ---------------------------------DECLARATIONS--------------------------------

// Global queue instance for incoming validated commands
extern IdrQueue idr_q_cmd;

// Description : Creates all application RTOS objects and allocates queue storage.
// Parameters  :
// 1. INOUT TX_BYTE_POOL* bytePool - Pointer to master system byte pool
// Return      :
// 1. TX_SUCCESS on success
// 2. ThreadX error code if creation fails
UINT IdrCreateObjects(TX_BYTE_POOL *bytePool);

#endif // IDR_TX_OBJECTS_HPP

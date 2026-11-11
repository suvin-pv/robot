// ----------------------------------------------------------------------------
// File Name    : tx_shim.cpp  (HOST UNIT-TEST SHIM - not part of the firmware build)
// Description  : Host implementation of the ThreadX subset declared in tx_api.h.
//                See tx_api.h for scope and limitations.
// ----------------------------------------------------------------------------

#include "tx_api.h"

#include <string.h>

namespace
{
	constexpr ULONG kByteAlign     = 8U;   // host pointers are 8-byte aligned
	constexpr ULONG kMinPoolSize   = 32U;

	ULONG AlignUp(ULONG value)
	{
		return (value + (kByteAlign - 1U)) & ~(kByteAlign - 1U);
	}
}

// ---------------------------------- Byte pool -------------------------------

extern "C" UINT tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start, ULONG pool_size)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == pool_ptr) || (nullptr == pool_start))
	{
		status = TX_PTR_ERROR;
	}
	else if (pool_size < kMinPoolSize)
	{
		status = TX_SIZE_ERROR;
	}
	else if (TX_BYTE_POOL_ID == pool_ptr->tx_byte_pool_id)
	{
		status = TX_POOL_ERROR;   // already created
	}
	else
	{
		pool_ptr->tx_byte_pool_id          = TX_BYTE_POOL_ID;
		pool_ptr->tx_byte_pool_name        = name_ptr;
		pool_ptr->tx_byte_pool_start       = static_cast<UCHAR *>(pool_start);
		pool_ptr->tx_byte_pool_size        = pool_size;
		pool_ptr->tx_byte_pool_available   = pool_size;
		pool_ptr->tx_byte_pool_used_offset = 0U;
	}

	return status;
}

extern "C" UINT tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == pool_ptr) || (TX_BYTE_POOL_ID != pool_ptr->tx_byte_pool_id))
	{
		status = TX_POOL_ERROR;
	}
	else
	{
		memset(pool_ptr, 0, sizeof(*pool_ptr));
	}

	return status;
}

extern "C" UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size, ULONG wait_option)
{
	UINT status = TX_SUCCESS;
	(void)wait_option;   // the shim never blocks

	if ((nullptr == pool_ptr) || (TX_BYTE_POOL_ID != pool_ptr->tx_byte_pool_id))
	{
		status = TX_POOL_ERROR;
	}
	else if (nullptr == memory_ptr)
	{
		status = TX_PTR_ERROR;
	}
	else if ((0U == memory_size) || (memory_size > pool_ptr->tx_byte_pool_size))
	{
		status = TX_SIZE_ERROR;
	}
	else
	{
		const ULONG needed = AlignUp(memory_size);
		if (needed > pool_ptr->tx_byte_pool_available)
		{
			status = TX_NO_MEMORY;
		}
		else
		{
			*memory_ptr = pool_ptr->tx_byte_pool_start + pool_ptr->tx_byte_pool_used_offset;
			pool_ptr->tx_byte_pool_used_offset += needed;
			pool_ptr->tx_byte_pool_available   -= needed;
		}
	}

	return status;
}

// ------------------------------------ Queue ---------------------------------

extern "C" UINT tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, VOID *queue_start, ULONG queue_size)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == queue_ptr) || (TX_QUEUE_ID == queue_ptr->tx_queue_id))
	{
		status = TX_QUEUE_ERROR;       // null or already created
	}
	else if (nullptr == queue_start)
	{
		status = TX_PTR_ERROR;
	}
	else if ((message_size < 1U) || (message_size > TX_MAX_QUEUE_MSG_WORDS))
	{
		status = TX_SIZE_ERROR;
	}
	else if (queue_size < (message_size * static_cast<ULONG>(sizeof(ULONG))))
	{
		status = TX_SIZE_ERROR;
	}
	else
	{
		const ULONG capacity = queue_size / (message_size * static_cast<ULONG>(sizeof(ULONG)));

		queue_ptr->tx_queue_id                = TX_QUEUE_ID;
		queue_ptr->tx_queue_name              = name_ptr;
		queue_ptr->tx_queue_message_size      = message_size;
		queue_ptr->tx_queue_capacity          = capacity;
		queue_ptr->tx_queue_enqueued          = 0U;
		queue_ptr->tx_queue_available_storage = capacity;
		queue_ptr->tx_queue_start             = static_cast<ULONG *>(queue_start);
		queue_ptr->tx_queue_read              = 0U;
		queue_ptr->tx_queue_write             = 0U;
	}

	return status;
}

extern "C" UINT tx_queue_delete(TX_QUEUE *queue_ptr)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == queue_ptr) || (TX_QUEUE_ID != queue_ptr->tx_queue_id))
	{
		status = TX_QUEUE_ERROR;
	}
	else
	{
		memset(queue_ptr, 0, sizeof(*queue_ptr));
	}

	return status;
}

extern "C" UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
{
	UINT status = TX_SUCCESS;
	(void)wait_option;   // a full queue returns TX_QUEUE_FULL instead of blocking

	if ((nullptr == queue_ptr) || (TX_QUEUE_ID != queue_ptr->tx_queue_id))
	{
		status = TX_QUEUE_ERROR;
	}
	else if (nullptr == source_ptr)
	{
		status = TX_PTR_ERROR;
	}
	else if (0U == queue_ptr->tx_queue_available_storage)
	{
		status = TX_QUEUE_FULL;
	}
	else
	{
		ULONG *slot = queue_ptr->tx_queue_start + (queue_ptr->tx_queue_write * queue_ptr->tx_queue_message_size);
		memcpy(slot, source_ptr, queue_ptr->tx_queue_message_size * sizeof(ULONG));

		queue_ptr->tx_queue_write = (queue_ptr->tx_queue_write + 1U) % queue_ptr->tx_queue_capacity;
		queue_ptr->tx_queue_enqueued++;
		queue_ptr->tx_queue_available_storage--;
	}

	return status;
}

extern "C" UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
{
	UINT status = TX_SUCCESS;
	(void)wait_option;   // an empty queue returns TX_QUEUE_EMPTY instead of blocking

	if ((nullptr == queue_ptr) || (TX_QUEUE_ID != queue_ptr->tx_queue_id))
	{
		status = TX_QUEUE_ERROR;
	}
	else if (nullptr == destination_ptr)
	{
		status = TX_PTR_ERROR;
	}
	else if (0U == queue_ptr->tx_queue_enqueued)
	{
		status = TX_QUEUE_EMPTY;
	}
	else
	{
		const ULONG *slot = queue_ptr->tx_queue_start + (queue_ptr->tx_queue_read * queue_ptr->tx_queue_message_size);
		memcpy(destination_ptr, slot, queue_ptr->tx_queue_message_size * sizeof(ULONG));

		queue_ptr->tx_queue_read = (queue_ptr->tx_queue_read + 1U) % queue_ptr->tx_queue_capacity;
		queue_ptr->tx_queue_enqueued--;
		queue_ptr->tx_queue_available_storage++;
	}

	return status;
}

extern "C" UINT tx_queue_info_get(TX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage,
                                  TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == queue_ptr) || (TX_QUEUE_ID != queue_ptr->tx_queue_id))
	{
		status = TX_QUEUE_ERROR;
	}
	else
	{
		if (nullptr != name)              { *name = queue_ptr->tx_queue_name; }
		if (nullptr != enqueued)          { *enqueued = queue_ptr->tx_queue_enqueued; }
		if (nullptr != available_storage) { *available_storage = queue_ptr->tx_queue_available_storage; }
		if (nullptr != first_suspended)   { *first_suspended = nullptr; }
		if (nullptr != suspended_count)   { *suspended_count = 0U; }
		if (nullptr != next_queue)        { *next_queue = nullptr; }
	}

	return status;
}

// ----------------------------------- Thread ---------------------------------

extern "C" UINT tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG),
                                 ULONG entry_input, VOID *stack_start, ULONG stack_size, UINT priority,
                                 UINT preempt_threshold, ULONG time_slice, UINT auto_start)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == thread_ptr) || (TX_THREAD_ID == thread_ptr->tx_thread_id))
	{
		status = TX_THREAD_ERROR;      // null or already created
	}
	else if ((nullptr == stack_start) || (nullptr == entry_function))
	{
		status = TX_PTR_ERROR;
	}
	else if (stack_size < TX_MINIMUM_STACK)
	{
		status = TX_SIZE_ERROR;
	}
	else if (priority >= TX_MAX_PRIORITIES)
	{
		status = TX_PRIORITY_ERROR;
	}
	else if (preempt_threshold > priority)
	{
		status = TX_THRESH_ERROR;
	}
	else if (auto_start > TX_AUTO_START)
	{
		status = TX_START_ERROR;
	}
	else
	{
		thread_ptr->tx_thread_id                       = TX_THREAD_ID;
		thread_ptr->tx_thread_name                     = name_ptr;
		thread_ptr->tx_thread_entry                    = entry_function;   // never executed by the shim
		thread_ptr->tx_thread_entry_parameter          = entry_input;
		thread_ptr->tx_thread_stack_start              = stack_start;
		thread_ptr->tx_thread_stack_size               = stack_size;
		thread_ptr->tx_thread_priority                 = priority;
		thread_ptr->tx_thread_user_priority            = priority;
		thread_ptr->tx_thread_user_preempt_threshold   = preempt_threshold;
		thread_ptr->tx_thread_time_slice               = time_slice;
		thread_ptr->tx_thread_auto_start               = auto_start;
		thread_ptr->tx_thread_terminated               = 0U;
	}

	return status;
}

extern "C" UINT tx_thread_terminate(TX_THREAD *thread_ptr)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == thread_ptr) || (TX_THREAD_ID != thread_ptr->tx_thread_id))
	{
		status = TX_THREAD_ERROR;
	}
	else
	{
		thread_ptr->tx_thread_terminated = 1U;
	}

	return status;
}

extern "C" UINT tx_thread_delete(TX_THREAD *thread_ptr)
{
	UINT status = TX_SUCCESS;

	if ((nullptr == thread_ptr) || (TX_THREAD_ID != thread_ptr->tx_thread_id))
	{
		status = TX_THREAD_ERROR;
	}
	else if (0U == thread_ptr->tx_thread_terminated)
	{
		status = TX_DELETE_ERROR;      // real ThreadX: thread must be terminated first
	}
	else
	{
		memset(thread_ptr, 0, sizeof(*thread_ptr));
	}

	return status;
}

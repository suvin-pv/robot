// ----------------------------------------------------------------------------
// File Name    : tx_api.h  (HOST UNIT-TEST SHIM - not part of the firmware build)
// Description  : Minimal host (x86/x64) stand-in for the subset of the ThreadX
//                API used by the IndRover application code, so the application
//                sources can be compiled unchanged and unit tested on a PC.
//
//                What it models (matching real ThreadX behaviour / return codes):
//                  - byte pool : create / allocate / delete
//                  - queue     : create / send / receive / info_get / delete
//                  - thread    : create / terminate / delete  (never runs the thread)
//
//                What it does NOT do: scheduling, preemption, blocking. A call
//                that would block (TX_WAIT_FOREVER / ticks on an empty or full
//                queue) returns TX_QUEUE_EMPTY / TX_QUEUE_FULL immediately.
//
//                Types are 32-bit like the Cortex-M4 target (ULONG = 32 bit), so
//                message and storage sizes match the target.
// ----------------------------------------------------------------------------

#ifndef TX_API_H
#define TX_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------- Types -----------------------------------
typedef void          VOID;
typedef char          CHAR;
typedef unsigned char UCHAR;
typedef int           INT;
typedef unsigned int  UINT;
typedef uint32_t      ULONG;   // 32-bit on the target (unsigned long on ARM GCC)

// -------------------------------- Constants ---------------------------------
#define TX_SUCCESS              (0x00U)
#define TX_POOL_ERROR           (0x02U)
#define TX_PTR_ERROR            (0x03U)
#define TX_WAIT_ERROR           (0x04U)
#define TX_SIZE_ERROR           (0x05U)
#define TX_QUEUE_ERROR          (0x09U)
#define TX_QUEUE_EMPTY          (0x0AU)
#define TX_QUEUE_FULL           (0x0BU)
#define TX_THREAD_ERROR         (0x0EU)
#define TX_PRIORITY_ERROR       (0x0FU)
#define TX_NO_MEMORY            (0x10U)
#define TX_START_ERROR          (0x10U)
#define TX_DELETE_ERROR         (0x11U)
#define TX_THRESH_ERROR         (0x18U)
#define TX_WAIT_ABORTED         (0x1AU)

#define TX_NO_WAIT              (0UL)
#define TX_WAIT_FOREVER         (0xFFFFFFFFUL)

#define TX_NO_TIME_SLICE        (0UL)
#define TX_AUTO_START           (1U)
#define TX_DONT_START           (0U)

#define TX_MAX_PRIORITIES       (32U)
#define TX_MINIMUM_STACK        (200U)

#define TX_BYTE_POOL_ID         (0x42595445UL)
#define TX_QUEUE_ID             (0x51554555UL)
#define TX_THREAD_ID            (0x54485244UL)

#define TX_1_ULONG              (1U)
#define TX_2_ULONG              (2U)
#define TX_4_ULONG              (4U)
#define TX_8_ULONG              (8U)
#define TX_16_ULONG             (16U)

#define TX_MAX_QUEUE_MSG_WORDS  (16U)

// ----------------------------- Control blocks -------------------------------
// Field names follow the real ThreadX control blocks for the members used.

typedef struct TX_BYTE_POOL_STRUCT
{
	ULONG  tx_byte_pool_id;
	CHAR  *tx_byte_pool_name;
	UCHAR *tx_byte_pool_start;
	ULONG  tx_byte_pool_size;
	ULONG  tx_byte_pool_available;
	ULONG  tx_byte_pool_used_offset;   // shim bookkeeping (bump allocator)
} TX_BYTE_POOL;

typedef struct TX_THREAD_STRUCT TX_THREAD;
struct TX_THREAD_STRUCT
{
	ULONG  tx_thread_id;
	CHAR  *tx_thread_name;
	VOID (*tx_thread_entry)(ULONG);
	ULONG  tx_thread_entry_parameter;
	VOID  *tx_thread_stack_start;
	ULONG  tx_thread_stack_size;
	UINT   tx_thread_priority;
	UINT   tx_thread_user_priority;
	UINT   tx_thread_user_preempt_threshold;
	ULONG  tx_thread_time_slice;
	UINT   tx_thread_auto_start;
	UINT   tx_thread_terminated;
};

typedef struct TX_QUEUE_STRUCT TX_QUEUE;
struct TX_QUEUE_STRUCT
{
	ULONG  tx_queue_id;
	CHAR  *tx_queue_name;
	UINT   tx_queue_message_size;       // in ULONG words
	ULONG  tx_queue_capacity;           // messages
	ULONG  tx_queue_enqueued;           // messages currently stored
	ULONG  tx_queue_available_storage;  // free message slots
	ULONG *tx_queue_start;
	ULONG  tx_queue_read;               // read index (messages)
	ULONG  tx_queue_write;              // write index (messages)
};

// ------------------------------- Services -----------------------------------
UINT tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start, ULONG pool_size);
UINT tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr);
UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size, ULONG wait_option);

UINT tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, VOID *queue_start, ULONG queue_size);
UINT tx_queue_delete(TX_QUEUE *queue_ptr);
UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);
UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option);
UINT tx_queue_info_get(TX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage,
                       TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue);

UINT tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG),
                      ULONG entry_input, VOID *stack_start, ULONG stack_size, UINT priority,
                      UINT preempt_threshold, ULONG time_slice, UINT auto_start);
UINT tx_thread_terminate(TX_THREAD *thread_ptr);
UINT tx_thread_delete(TX_THREAD *thread_ptr);

#ifdef __cplusplus
}
#endif

#endif // TX_API_H

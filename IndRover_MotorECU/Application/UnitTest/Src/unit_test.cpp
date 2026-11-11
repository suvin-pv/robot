// ----------------------------------------------------------------------------
// File Name    : unit_test.cpp
// Description  : Implements unit tests for the command queue and command
//                manager modules. Runs on the host (GitHub Actions) with
//                GoogleTest against a ThreadX host shim - never on the STM32.
// ----------------------------------------------------------------------------
// Revision History (Version, Date, Name, Description)
// ----------------------------------------------------------------------------
// v01.00, 10/11/2026, Anusree, Initial creation of unit test suite
// v01.01, 11/11/2026, <Name>,  Aligned tests with idr_cmd_msg_t / IdrQueue API,
//                              added test fixtures, host-only GoogleTest, new
//                              negative tests
//
// <Add revision history above>
// ----------------------------------------------------------------------------
// Experion Technologies (P) Ltd. All rights reserved.
// ----------------------------------------------------------------------------

// --------------------------------INCLUDES------------------------------------
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "unit_test.h"
#include "idr_types.hpp"
#include "idr_tx_objects.hpp"
#include "idr_thread_cmd_mgr.hpp"

// ----------------------------LOCAL DEFINITIONS-------------------------------
namespace
{
	constexpr ULONG kPoolBytes = 4096U;   // plenty for the 256 B queue + 1 KB stack

	// Number of messages currently stored in idr_q_cmd (uses the ThreadX API,
	// so the same code works against the real kernel).
	ULONG PendingCount(void)
	{
		ULONG enqueued = 0U;
		(void)tx_queue_info_get(idr_q_cmd.GetHandle(), nullptr, &enqueued,
		                        nullptr, nullptr, nullptr, nullptr);
		return enqueued;
	}

	// Builds a fully initialised MOVE command; seq makes each message unique.
	idr_cmd_msg_t MakeMoveCmd(uint32_t seq)
	{
		idr_cmd_msg_t msg{};
		msg.rx_seq      = seq;
		msg.time_ms     = 2000U;
		msg.distance_mm = 500U;
		msg.cmd_id      = 42U;
		msg.speed_mmps  = 600U;
		msg.angle_ddeg  = 0U;
		msg.kind        = static_cast<uint8_t>(idr_cmd_kind_t::IDR_CMD_MOVE);
		msg.sub         = static_cast<uint8_t>(idr_mot_type_t::IDR_MOT_FRWD);
		msg.dir         = 1;
		return msg;
	}
}

// ---------------------------TEST FIXTURES------------------------------------

// Provides a fresh byte pool for every test and removes every RTOS object the
// tests may have created, so tests are independent of execution order.
class TxPoolTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		std::memset(&pool_, 0, sizeof(pool_));
		ASSERT_EQ(tx_byte_pool_create(&pool_, const_cast<CHAR *>("test_pool"),
		                              poolMem_, sizeof(poolMem_)),
		          TX_SUCCESS);
	}

	void TearDown() override
	{
		// Return codes ignored on purpose: the object may never have been created.
		(void)tx_queue_delete(idr_q_cmd.GetHandle());
		(void)tx_thread_terminate(&tCmdMgrTcb);
		(void)tx_thread_delete(&tCmdMgrTcb);
		(void)tx_byte_pool_delete(&pool_);
	}

	TX_BYTE_POOL pool_;
	alignas(8) unsigned char poolMem_[kPoolBytes];
};

// Same as TxPoolTest, with idr_q_cmd already created through the production path.
class CommandQueueTest : public TxPoolTest
{
protected:
	void SetUp() override
	{
		TxPoolTest::SetUp();
		ASSERT_EQ(IdrCreateObjects(&pool_), TX_SUCCESS);
	}
};

class CommandManagerTest : public TxPoolTest
{
};

// ---------------------------TEST SUITES--------------------------------------

// ----------------------------------------------------------------------------
// Test Suite : CommandQueueTest
// ----------------------------------------------------------------------------

// Test: Verify message layout matches the 32-byte (8 x 32-bit word) interface
TEST_F(CommandQueueTest, StructSizing)
{
	EXPECT_EQ(sizeof(idr_cmd_msg_t), 32U);
	EXPECT_EQ(IDR_CMD_Q_MSG_WORDS, 8U);
	EXPECT_EQ(IDR_CMD_Q_DEPTH, 8U);
	EXPECT_EQ(IDR_CMD_Q_STORAGE_BYTES, 256U);
	EXPECT_EQ(sizeof(idr_cmd_msg_t), IDR_CMD_Q_MSG_WORDS * sizeof(ULONG));

	// Byte offsets documented in idr_types.hpp
	EXPECT_EQ(offsetof(idr_cmd_msg_t, rx_seq),      0U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, time_ms),     4U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, distance_mm), 8U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, cmd_id),      12U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, speed_mmps),  14U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, angle_ddeg),  16U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, kind),        18U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, sub),         19U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, dir),         20U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, diag_param),  21U);
	EXPECT_EQ(offsetof(idr_cmd_msg_t, reserved),    28U);
}

// Test: Verify idr_q_cmd is created with the specified name, size and depth
TEST_F(TxPoolTest, QueueCreation)
{
	ASSERT_EQ(IdrCreateObjects(&pool_), TX_SUCCESS);

	CHAR *name      = nullptr;
	ULONG enqueued  = 99U;
	ULONG available = 0U;
	ASSERT_EQ(tx_queue_info_get(idr_q_cmd.GetHandle(), &name, &enqueued, &available,
	                            nullptr, nullptr, nullptr),
	          TX_SUCCESS);

	EXPECT_STREQ(name, "idr_q_cmd");
	EXPECT_EQ(enqueued, 0U);
	EXPECT_EQ(available, IDR_CMD_Q_DEPTH);
}

// Test: Verify creation is rejected when no byte pool is supplied
TEST_F(TxPoolTest, QueueCreationNullPool)
{
	EXPECT_EQ(IdrCreateObjects(nullptr), TX_PTR_ERROR);
	EXPECT_EQ(idr_q_cmd.Create(nullptr, "idr_q_cmd", IDR_CMD_Q_MSG_WORDS, IDR_CMD_Q_DEPTH),
	          TX_PTR_ERROR);
}

// Test: Verify creation reports the ThreadX error when the pool is too small
TEST_F(TxPoolTest, QueueCreationPoolExhausted)
{
	TX_BYTE_POOL smallPool;
	alignas(8) unsigned char smallMem[128];   // smaller than the 256 B queue storage
	std::memset(&smallPool, 0, sizeof(smallPool));
	ASSERT_EQ(tx_byte_pool_create(&smallPool, const_cast<CHAR *>("small"),
	                              smallMem, sizeof(smallMem)),
	          TX_SUCCESS);

	EXPECT_EQ(IdrCreateObjects(&smallPool), TX_SIZE_ERROR);   // larger than the whole pool

	(void)tx_byte_pool_delete(&smallPool);
}

// Test: Verify a second creation attempt on the same queue object is rejected
TEST_F(CommandQueueTest, QueueCreatedTwiceRejected)
{
	EXPECT_EQ(IdrCreateObjects(&pool_), TX_QUEUE_ERROR);
}

// Test: Verify payload fields are preserved accurately during transmission
TEST_F(CommandQueueTest, SendReceiveMoveCommand)
{
	const idr_cmd_msg_t txMsg = MakeMoveCmd(101U);

	// Enqueue message
	ASSERT_EQ(idr_q_cmd.Send(&txMsg, TX_NO_WAIT), TX_SUCCESS);

	// Dequeue message
	idr_cmd_msg_t rxMsg{};
	ASSERT_EQ(idr_q_cmd.Receive(&rxMsg, TX_NO_WAIT), TX_SUCCESS);

	// Verify all members
	EXPECT_EQ(rxMsg.rx_seq,      txMsg.rx_seq);
	EXPECT_EQ(rxMsg.time_ms,     txMsg.time_ms);
	EXPECT_EQ(rxMsg.distance_mm, txMsg.distance_mm);
	EXPECT_EQ(rxMsg.cmd_id,      txMsg.cmd_id);
	EXPECT_EQ(rxMsg.speed_mmps,  txMsg.speed_mmps);
	EXPECT_EQ(rxMsg.angle_ddeg,  txMsg.angle_ddeg);
	EXPECT_EQ(rxMsg.kind,        txMsg.kind);
	EXPECT_EQ(rxMsg.sub,         txMsg.sub);
	EXPECT_EQ(rxMsg.dir,         txMsg.dir);

	// Whole 32-byte message must be identical (no field lost or corrupted)
	EXPECT_EQ(std::memcmp(&rxMsg, &txMsg, sizeof(idr_cmd_msg_t)), 0);
}

// Test: Verify messages are delivered in the order they were sent
TEST_F(CommandQueueTest, FifoOrdering)
{
	for (uint32_t seq = 0U; seq < IDR_CMD_Q_DEPTH; ++seq)
	{
		const idr_cmd_msg_t msg = MakeMoveCmd(seq);
		ASSERT_EQ(idr_q_cmd.Send(&msg, TX_NO_WAIT), TX_SUCCESS);
	}

	for (uint32_t seq = 0U; seq < IDR_CMD_Q_DEPTH; ++seq)
	{
		idr_cmd_msg_t rxMsg{};
		ASSERT_EQ(idr_q_cmd.Receive(&rxMsg, TX_NO_WAIT), TX_SUCCESS);
		EXPECT_EQ(rxMsg.rx_seq, seq);
	}
}

// Test: Verify the queue wraps around correctly after repeated use
TEST_F(CommandQueueTest, WrapAround)
{
	for (uint32_t seq = 0U; seq < (3U * IDR_CMD_Q_DEPTH); ++seq)
	{
		const idr_cmd_msg_t txMsg = MakeMoveCmd(seq);
		ASSERT_EQ(idr_q_cmd.Send(&txMsg, TX_NO_WAIT), TX_SUCCESS);

		idr_cmd_msg_t rxMsg{};
		ASSERT_EQ(idr_q_cmd.Receive(&rxMsg, TX_NO_WAIT), TX_SUCCESS);
		EXPECT_EQ(rxMsg.rx_seq, seq);
	}
	EXPECT_EQ(PendingCount(), 0U);
}

// Test: Verify an empty queue reports TX_QUEUE_EMPTY without blocking
TEST_F(CommandQueueTest, ReceiveFromEmptyQueue)
{
	idr_cmd_msg_t rxMsg{};
	EXPECT_EQ(idr_q_cmd.Receive(&rxMsg, TX_NO_WAIT), TX_QUEUE_EMPTY);
}

// Test: Verify message counter increments and decrements as expected
TEST_F(CommandQueueTest, PendingCountTracking)
{
	EXPECT_EQ(PendingCount(), 0U);

	const idr_cmd_msg_t testMsg = MakeMoveCmd(1U);
	ASSERT_EQ(idr_q_cmd.Send(&testMsg, TX_NO_WAIT), TX_SUCCESS);
	EXPECT_EQ(PendingCount(), 1U);

	idr_cmd_msg_t rxMsg{};
	ASSERT_EQ(idr_q_cmd.Receive(&rxMsg, TX_NO_WAIT), TX_SUCCESS);
	EXPECT_EQ(PendingCount(), 0U);
}

// Test: Verify queue depth limit of 8 and rejection of overflow message
TEST_F(CommandQueueTest, QueueOverflowRejection)
{
	// Fill queue to maximum capacity
	for (uint32_t i = 0U; i < IDR_CMD_Q_DEPTH; ++i)
	{
		const idr_cmd_msg_t msg = MakeMoveCmd(i);
		ASSERT_EQ(idr_q_cmd.Send(&msg, TX_NO_WAIT), TX_SUCCESS);
	}
	EXPECT_EQ(PendingCount(), IDR_CMD_Q_DEPTH);

	// 9th send must fail with queue full and must not disturb queued messages
	const idr_cmd_msg_t overflowMsg = MakeMoveCmd(999U);
	EXPECT_EQ(idr_q_cmd.Send(&overflowMsg, TX_NO_WAIT), TX_QUEUE_FULL);
	EXPECT_EQ(PendingCount(), IDR_CMD_Q_DEPTH);

	// Drain queue and confirm the oldest message is still the first one sent
	idr_cmd_msg_t drainMsg{};
	ASSERT_EQ(idr_q_cmd.Receive(&drainMsg, TX_NO_WAIT), TX_SUCCESS);
	EXPECT_EQ(drainMsg.rx_seq, 0U);
	for (uint32_t i = 1U; i < IDR_CMD_Q_DEPTH; ++i)
	{
		(void)idr_q_cmd.Receive(&drainMsg, TX_NO_WAIT);
	}

	EXPECT_EQ(PendingCount(), 0U);
}

// Test: Verify NULL message pointers are rejected
TEST_F(CommandQueueTest, NullMessagePointer)
{
	EXPECT_EQ(idr_q_cmd.Send(nullptr, TX_NO_WAIT), TX_PTR_ERROR);
	EXPECT_EQ(idr_q_cmd.Receive(nullptr, TX_NO_WAIT), TX_PTR_ERROR);
	EXPECT_EQ(PendingCount(), 0U);
}

// ----------------------------------------------------------------------------
// Test Suite : CommandManagerTest
// ----------------------------------------------------------------------------

// Test: Verify defensive null check for stack memory pool pointer
TEST_F(CommandManagerTest, ThreadCreationNullCheck)
{
	EXPECT_EQ(IdrThreadCmdMgrCreate(nullptr), TX_PTR_ERROR);
}

// Test: Verify tCmdMgr is created with the configured name, stack and priority
TEST_F(CommandManagerTest, ThreadCreation)
{
	ASSERT_EQ(IdrThreadCmdMgrCreate(&pool_), TX_SUCCESS);

	EXPECT_STREQ(tCmdMgrTcb.tx_thread_name, "tCmdMgr");
	EXPECT_EQ(tCmdMgrTcb.tx_thread_stack_size, IDR_CMD_MGR_STACK_SIZE);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_priority, IDR_CMD_MGR_PRIORITY);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_entry, &IdrThreadCmdMgrEntry);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_auto_start, TX_AUTO_START);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_time_slice, TX_NO_TIME_SLICE);
}

// Test: Verify an exhausted byte pool is reported and no thread is created
TEST_F(CommandManagerTest, ThreadCreationPoolExhausted)
{
	TX_BYTE_POOL smallPool;
	alignas(8) unsigned char smallMem[512];   // smaller than the 1 KB stack
	std::memset(&smallPool, 0, sizeof(smallPool));
	ASSERT_EQ(tx_byte_pool_create(&smallPool, const_cast<CHAR *>("small"),
	                              smallMem, sizeof(smallMem)),
	          TX_SUCCESS);

	EXPECT_EQ(IdrThreadCmdMgrCreate(&smallPool), TX_SIZE_ERROR);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_id, 0U);   // thread was not created

	(void)tx_byte_pool_delete(&smallPool);
}

// Test: Verify TX_NO_MEMORY is reported when the pool cannot hold the stack
//       after the command queue storage has already been taken from it
TEST_F(CommandManagerTest, ThreadCreationNoMemory)
{
	TX_BYTE_POOL tightPool;
	alignas(8) unsigned char tightMem[1100];   // fits the 1 KB stack, but not stack + 256 B queue
	std::memset(&tightPool, 0, sizeof(tightPool));
	ASSERT_EQ(tx_byte_pool_create(&tightPool, const_cast<CHAR *>("tight"),
	                              tightMem, sizeof(tightMem)),
	          TX_SUCCESS);
	ASSERT_EQ(IdrCreateObjects(&tightPool), TX_SUCCESS);

	EXPECT_EQ(IdrThreadCmdMgrCreate(&tightPool), TX_NO_MEMORY);
	EXPECT_EQ(tCmdMgrTcb.tx_thread_id, 0U);   // thread was not created

	(void)tx_queue_delete(idr_q_cmd.GetHandle());
	(void)tx_byte_pool_delete(&tightPool);
}

// Test: Verify a second creation attempt for the same thread is rejected
TEST_F(CommandManagerTest, ThreadCreatedTwiceRejected)
{
	ASSERT_EQ(IdrThreadCmdMgrCreate(&pool_), TX_SUCCESS);
	EXPECT_EQ(IdrThreadCmdMgrCreate(&pool_), TX_THREAD_ERROR);
}

// ----------------------------PUBLIC FUNCTIONS--------------------------------

// Description : Executes all test cases.
// Parameters  : argc, argv - arguments passed to test runner
// Return      : 0 on success, non-zero on failure.
extern "C" int RunAllTests(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

// ----------------------------------------------------------------------------
// File Name    : idr_types.hpp
// Description  : Declares common enumeration types, structures, and sizing
//                constants for the IndRover Motor Control ECU.
// Revision     : 1.0 (Initial creation per Interface Design v0.2)
// Copyright    : (c) 2026 IndRover Project. All rights reserved.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// File Name    : idr_types.hpp
// Description  : Declares common enumeration types, structures, and sizing
//                constants for the IndRover Motor Control ECU.
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

#ifndef IDR_TYPES_HPP
#define IDR_TYPES_HPP

// -------------------------INCLUDES-------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// -------------------------MACROS & CONSTANTS---------------------------------
#define IDR_CMD_Q_MSG_WORDS     (8U)       // 8 32-bit words = 32 bytes
#define IDR_CMD_Q_DEPTH         (8U)       // Depth: 8 commands
#define IDR_CMD_Q_STORAGE_BYTES (IDR_CMD_Q_MSG_WORDS * sizeof(uint32_t) * IDR_CMD_Q_DEPTH) // 256 B

// -------------------------TYPES & ENUMERATIONS-------------------------------

// Command category (MOVE or DIAG)
enum class idr_cmd_kind_t : uint8_t
{
	IDR_CMD_MOVE = 0U,
	IDR_CMD_DIAG = 1U
};

// Motion sub-command type
enum class idr_mot_type_t : uint8_t
{
	IDR_MOT_FRWD   = 0U,
	IDR_MOT_REVSE  = 1U,
	IDR_MOT_LEFT   = 2U,
	IDR_MOT_RIGHT  = 3U,
	IDR_MOT_ROTATE = 4U,
	IDR_MOT_STOP   = 5U
};

// Motion reply handshake kind from tMotion
enum class idr_reply_kind_t : uint8_t
{
	IDR_REPLY_ACCEPTED = 0U,
	IDR_REPLY_DECLINED = 1U,
	IDR_REPLY_ENDED    = 2U
};

// Reason code for motion reply
enum class idr_end_reason_t : uint8_t
{
	IDR_REASON_NONE       = 0U,
	IDR_REASON_COMPLETE   = 1U,
	IDR_REASON_STOPPED    = 2U,
	IDR_REASON_SUPERSEDED = 3U,
	IDR_REASON_FAULT      = 4U
};

// Protocol status return codes
enum class idr_status_code_t : uint8_t
{
	IDR_ST_INITIATED   = 0U,
	IDR_ST_OK          = 1U,
	IDR_ST_INTERRUPTED = 2U,
	IDR_ST_WDT_STOP    = 3U,
	IDR_ST_BATT_STOP   = 4U,
	IDR_ST_ERR_CRC     = 5U,
	IDR_ST_ERR_FORMAT  = 6U,
	IDR_ST_ERR_PARAM   = 7U,
	IDR_ST_ERR_HW      = 8U
};

// Master ECU State Machine states
enum class idr_ecu_state_t : uint8_t
{
	IDR_STATE_INIT     = 0U,
	IDR_STATE_IDLE     = 1U,
	IDR_STATE_MOVING   = 2U,
	IDR_STATE_STOPPING = 3U,
	IDR_STATE_FAULT    = 4U
};

// Validated command message sent by tCmdRx to tCmdMgr via idr_q_cmd
struct idr_cmd_msg_t
{
	uint32_t rx_seq;        // Byte offset 0: Reception sequence number (IF-04)
	uint32_t time_ms;       // Byte offset 4: MOVE time in ms; 0 = none
	uint32_t distance_mm;   // Byte offset 8: MOVE distance; STOP stopping distance (0 = abrupt)
	uint16_t cmd_id;        // Byte offset 12: R1 message ID (e.g. 42 for RM42)
	uint16_t speed_mmps;    // Byte offset 14: Commanded speed (0-1300 mm/s)
	uint16_t angle_ddeg;    // Byte offset 16: Turn angle in tenths of degrees (0-3600)
	uint8_t  kind;          // Byte offset 18: Cast from idr_cmd_kind_t (0=MOVE, 1=DIAG)
	uint8_t  sub;           // Byte offset 19: Cast from idr_mot_type_t or DIAG code
	int8_t   dir;           // Byte offset 20: Direction (-1, 0, +1)
	uint8_t  diag_param[7]; // Byte offset 21-27: DIAG parameters; zero for MOVE
	uint8_t  reserved[4];   // Byte offset 28-31: Fills structure to 32 bytes (8 words)
};

// Compile-time verification of strict 32-byte layout requirement
static_assert(sizeof(idr_cmd_msg_t) == 32U, "idr_cmd_msg_t must be exactly 32 bytes (8 32-bit words)");

#endif // IDR_TYPES_HPP

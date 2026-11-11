// ----------------------------------------------------------------------------
// File Name    : unit_test.h
// Description  : Declares Google Test test runner interface for the command
//                queue and command manager modules
// ----------------------------------------------------------------------------
// Revision History (Version, Date, Name, Description)
// ----------------------------------------------------------------------------
// v01.00, 10/11/2026, Anusree, Initial creation of unit test suite
//
// <Add revision history above>
// ----------------------------------------------------------------------------
// Experion Technologies (P) Ltd. All rights reserved.
// ----------------------------------------------------------------------------

#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

// Runs all registered test cases and returns 0 on success, non-zero on failure
int RunAllTests(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // UNIT_TEST_H

// ----------------------------------------------------------------------------
// File Name    : main.cpp
// Description  : Host entry point for the unit test executable. Delegates to
//                RunAllTests() so GoogleTest command line options (such as
//                --gtest_output=xml:<file>) work as usual.
// ----------------------------------------------------------------------------

#include "unit_test.h"

int main(int argc, char **argv)
{
	return RunAllTests(argc, argv);
}

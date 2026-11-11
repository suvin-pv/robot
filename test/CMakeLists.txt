cmake_minimum_required(VERSION 3.22)
project(idr_unit_tests C CXX)

# ---------------------------------------------------------------------------
# Host (x86/x64) unit tests for the IndRover Motor ECU application code.
#   - Application sources are compiled UNCHANGED.
#   - ThreadX is replaced by a small host shim (test/threadx_shim/tx_api.h);
#     the real ThreadX headers are deliberately NOT on the include path.
#   - GoogleTest comes from the OS package (apt install libgtest-dev).
#
# To test a new module: add its .cpp to APP_UNDER_TEST below.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FW_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../IndRover_MotorECU)
set(APP_DIR ${FW_DIR}/Application)

find_package(GTest REQUIRED)

# --- Application code under test (explicit list: no firmware-only modules) ---
set(APP_UNDER_TEST
  ${APP_DIR}/Middleware/Src/idr_tx_objects.cpp
  ${APP_DIR}/App/Src/idr_thread_cmd_mgr.cpp
)

# Include directories: every folder under Application/ that holds headers,
# except the unit test folder itself.
file(GLOB_RECURSE _app_headers CONFIGURE_DEPENDS ${APP_DIR}/*.h ${APP_DIR}/*.hpp)
set(APP_INCS "")
foreach(h ${_app_headers})
  if(NOT h MATCHES "/Application/UnitTest/")
    get_filename_component(d ${h} DIRECTORY)
    list(APPEND APP_INCS ${d})
  endif()
endforeach()
list(REMOVE_DUPLICATES APP_INCS)

# --- ThreadX host shim ---
add_library(tx_shim STATIC threadx_shim/tx_shim.cpp)
target_include_directories(tx_shim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/threadx_shim)

# --- Application under test ---
add_library(app_under_test STATIC ${APP_UNDER_TEST})
target_include_directories(app_under_test PUBLIC ${APP_INCS})
target_link_libraries(app_under_test PUBLIC tx_shim)
target_compile_options(app_under_test PRIVATE -Wall -Wextra)

# --- Test executable ---
add_executable(idr_unit_tests
  main.cpp
  ${APP_DIR}/UnitTest/Src/unit_test.cpp
)
target_include_directories(idr_unit_tests PRIVATE ${APP_DIR}/UnitTest/Inc)
target_link_libraries(idr_unit_tests PRIVATE app_under_test GTest::gtest)
target_compile_options(idr_unit_tests PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME idr_unit_tests COMMAND idr_unit_tests)

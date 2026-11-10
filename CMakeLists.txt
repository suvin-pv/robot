cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# CI build for an STM32CubeIDE project (CubeMX-generated layout:
# Core/, Drivers/, optionally Middlewares/, *_FLASH.ld in the root).
# >>> EDIT THE THREE LINES MARKED "EDIT" TO MATCH YOUR CHIP <<<
# (all values are in STM32CubeIDE: Project > Properties > C/C++ Build > Settings)
# ---------------------------------------------------------------------------
set(MCU_FLAGS  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)   # EDIT: core/FPU
set(MCU_DEFINES USE_HAL_DRIVER STM32F407xx TX_INCLUDE_USER_DEFINE_FILE)            # matches .cproject (Debug adds DEBUG)
set(PROJECT_NAME_FW motor_ecu)                                               # EDIT: any name
set(FW_DIR IndRover_MotorECU)   # CubeIDE project folder inside the repo ("." if it is the repo root)

if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
endif()

project(${PROJECT_NAME_FW} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS_RELEASE   "-Os -g0")   # CubeIDE Release uses -Os
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")
set(CMAKE_ASM_FLAGS_RELEASE "-g0")

# Sources: everything under these folders that exists in the repo
foreach(dir Core AZURE_RTOS App Application Drivers Middlewares)
  if(EXISTS ${CMAKE_SOURCE_DIR}/${FW_DIR}/${dir})
    file(GLOB_RECURSE _src CONFIGURE_DEPENDS
         ${FW_DIR}/${dir}/*.c ${FW_DIR}/${dir}/*.cpp ${FW_DIR}/${dir}/*.cc)
    list(APPEND SRCS ${_src})
    file(GLOB_RECURSE _hdr CONFIGURE_DEPENDS ${FW_DIR}/${dir}/*.h ${FW_DIR}/${dir}/*.hpp)
    foreach(h ${_hdr})
      get_filename_component(d ${h} DIRECTORY)
      list(APPEND INCS ${d})
    endforeach()
  endif()
endforeach()
list(REMOVE_DUPLICATES INCS)

# ThreadX: only the Cortex-M4 GNU port (the folder also ships ports for IAR/AC6 etc.)
set(THREADX_PORT "/ports/cortex_m4/gnu/")
foreach(var SRCS INCS)
  set(_keep "")
  foreach(item ${${var}})
    if(item MATCHES "/threadx/ports/" AND NOT item MATCHES "${THREADX_PORT}")
      continue()
    endif()
    if(item MATCHES "/example_build/")
      continue()
    endif()
    list(APPEND _keep ${item})
  endforeach()
  set(${var} ${_keep})
endforeach()

# Assembly sources (ThreadX port: scheduler, context switch, low-level init)
file(GLOB_RECURSE ASM_SRCS CONFIGURE_DEPENDS
     ${FW_DIR}/Core/*.S ${FW_DIR}/Core/*.s
     ${FW_DIR}/Middlewares/*.S ${FW_DIR}/Middlewares/*.s
     ${FW_DIR}/Application/*.S ${FW_DIR}/Application/*.s)
list(FILTER ASM_SRCS EXCLUDE REGEX "startup|/example_build/")
foreach(item ${ASM_SRCS})
  if(item MATCHES "/threadx/ports/" AND NOT item MATCHES "${THREADX_PORT}")
    continue()
  endif()
  list(APPEND SRCS ${item})
endforeach()

# Drop example/template files that CubeMX ships but are not part of the build
list(FILTER SRCS EXCLUDE REGEX "_template\\.c$|/Templates/|/Examples/")

# Startup file (startup_stm32f407xx.s) and linker script (*.ld): search the whole repo
file(GLOB_RECURSE STARTUP CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${FW_DIR}/*startup*.s ${CMAKE_SOURCE_DIR}/${FW_DIR}/*startup*.S)
list(FILTER STARTUP EXCLUDE REGEX "/build/")
file(GLOB_RECURSE LD_ALL CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${FW_DIR}/*.ld)
list(FILTER LD_ALL EXCLUDE REGEX "/build/")
set(LD_FLASH ${LD_ALL})
list(FILTER LD_FLASH INCLUDE REGEX "FLASH")
if(LD_FLASH)
  list(GET LD_FLASH 0 LDSCRIPT)
elseif(LD_ALL)
  list(GET LD_ALL 0 LDSCRIPT)
endif()
if(NOT LDSCRIPT)
  message(FATAL_ERROR "No linker script (*.ld) found in the repo. Commit your STM32F407xx_FLASH.ld "
                      "(check .gitignore is not hiding *.ld files).")
endif()
if(NOT STARTUP)
  message(FATAL_ERROR "No startup file (startup_*.s) found. Commit Core/Startup/startup_stm32f407xx.s "
                      "(check .gitignore is not hiding *.s files).")
endif()
message(STATUS "Linker script: ${LDSCRIPT}")
message(STATUS "Startup file : ${STARTUP}")

add_executable(${PROJECT_NAME_FW}.elf ${SRCS} ${STARTUP})
target_include_directories(${PROJECT_NAME_FW}.elf PRIVATE ${INCS})
target_compile_definitions(${PROJECT_NAME_FW}.elf PRIVATE ${MCU_DEFINES})
target_compile_options(${PROJECT_NAME_FW}.elf PRIVATE
  ${MCU_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>)
target_link_options(${PROJECT_NAME_FW}.elf PRIVATE
  ${MCU_FLAGS} -T${LDSCRIPT} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections
  -Wl,-Map=${PROJECT_NAME_FW}.map)

# CI WORKAROUND: Core/Src/tx_initialize_low_level.s uses Arm-Compiler/Keil symbol names that the
# GCC linker script does not define. Map them to the GCC equivalents so the CI link succeeds.
# (Proper fix: regenerate that file for the STM32CubeIDE toolchain in CubeMX, then set this OFF.)
option(CI_KEIL_SYMBOL_SHIM "Map Keil ThreadX symbols to GCC ones" ON)
if(CI_KEIL_SYMBOL_SHIM)
  target_link_options(${PROJECT_NAME_FW}.elf PRIVATE
    "LINKER:--defsym=Image$$RW_IRAM1$$ZI$$Limit=_end"
    "LINKER:--defsym=__Vectors=g_pfnVectors")
endif()

add_custom_command(TARGET ${PROJECT_NAME_FW}.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME_FW}.elf> ${PROJECT_NAME_FW}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME_FW}.elf>)

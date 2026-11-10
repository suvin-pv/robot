cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# CI build for an STM32CubeIDE project (CubeMX-generated layout:
# Core/, Drivers/, optionally Middlewares/, *_FLASH.ld in the root).
# >>> EDIT THE THREE LINES MARKED "EDIT" TO MATCH YOUR CHIP <<<
# (all values are in STM32CubeIDE: Project > Properties > C/C++ Build > Settings)
# ---------------------------------------------------------------------------
set(MCU_FLAGS  -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)   # EDIT: core/FPU
set(MCU_DEFINES USE_HAL_DRIVER STM32F407xx)                                  # EDIT: chip define
set(PROJECT_NAME_FW motor_ecu)                                               # EDIT: any name

if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
endif()

project(${PROJECT_NAME_FW} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sources: everything under these folders that exists in the repo
foreach(dir Core App Application Drivers Middlewares)
  if(EXISTS ${CMAKE_SOURCE_DIR}/${dir})
    file(GLOB_RECURSE _src CONFIGURE_DEPENDS
         ${dir}/*.c ${dir}/*.cpp ${dir}/*.cc)
    list(APPEND SRCS ${_src})
    file(GLOB_RECURSE _hdr CONFIGURE_DEPENDS ${dir}/*.h ${dir}/*.hpp)
    foreach(h ${_hdr})
      get_filename_component(d ${h} DIRECTORY)
      list(APPEND INCS ${d})
    endforeach()
  endif()
endforeach()
list(REMOVE_DUPLICATES INCS)

# Drop example/template files that CubeMX ships but are not part of the build
list(FILTER SRCS EXCLUDE REGEX "_template\\.c$|/Templates/|/Examples/")

file(GLOB STARTUP CONFIGURE_DEPENDS Core/Startup/*.s Core/Startup/*.S)
file(GLOB LDSCRIPT CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/*_FLASH.ld)
list(GET LDSCRIPT 0 LDSCRIPT)

add_executable(${PROJECT_NAME_FW}.elf ${SRCS} ${STARTUP})
target_include_directories(${PROJECT_NAME_FW}.elf PRIVATE ${INCS})
target_compile_definitions(${PROJECT_NAME_FW}.elf PRIVATE ${MCU_DEFINES})
target_compile_options(${PROJECT_NAME_FW}.elf PRIVATE
  ${MCU_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>)
target_link_options(${PROJECT_NAME_FW}.elf PRIVATE
  ${MCU_FLAGS} -T${LDSCRIPT} --specs=nano.specs -Wl,--gc-sections
  -Wl,-Map=${PROJECT_NAME_FW}.map)

add_custom_command(TARGET ${PROJECT_NAME_FW}.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME_FW}.elf> ${PROJECT_NAME_FW}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME_FW}.elf>)

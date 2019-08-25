cmake_minimum_required(VERSION 3.0)

set(PROJ fmsx)

if (NOT TOOL_CHAIN)
    set(TOOLCHAIN "/usr/local/opt/kendryte-toolchain/bin")
endif()

get_filename_component(SDK_ROOT "third_party/kendryte-standalone-sdk" ABSOLUTE)
set(FMSX_ROOT "third_party/fMSX54")

set(BUILDING_SDK "yes" CACHE INTERNAL "")

include(${SDK_ROOT}/cmake/common.cmake)
project(${PROJ} C CXX ASM)

include(${SDK_ROOT}/cmake/macros.internal.cmake)
header_directories(${SDK_ROOT}/lib)

add_subdirectory(${SDK_ROOT}/lib)

include_directories(
    .
    ${FMSX_ROOT}/Z80
    ${FMSX_ROOT}/EMULib
    ${FMSX_ROOT}/fMSX
    )
    
add_definitions(
    -DBPS16 -DBPP16 -DUNIX -DLSB_FIRST
    )

add_subdirectory(sd_card)
add_subdirectory(${FMSX_ROOT})

add_source_files(
    main.cpp
    fmsx_interface.c
    std_file.cpp
    lcd.cpp
    audio.cpp
    )

include(${SDK_ROOT}/cmake/executable.cmake)

target_link_libraries(${PROJ} sd_card fmsxlib)

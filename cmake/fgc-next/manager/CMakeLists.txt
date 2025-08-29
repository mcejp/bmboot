cmake_minimum_required(VERSION 3.17)

# bmboot manager (C++ Linux library for interacting with baremetal cores)
project(bmboot_manager C CXX)
set(BMBOOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

set(GENERATED_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/generated_include)
file(MAKE_DIRECTORY ${GENERATED_INCLUDE})

include(ExternalProject)

# add executor code as ExternalProject
ExternalProject_Add(monitor_zynqmp
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../monitor
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../Aarch64BareMetal.cmake
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ""
        BUILD_ALWAYS ON
        BUILD_BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu1.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu2.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu3.bin"
        )

# We need to convert all monitor binaries to a C++ header
include(${BMBOOT_ROOT}/cmake/FileEmbed.cmake)
set(MONITOR_ZYNQMP_HPP_ALL)
ExternalProject_Get_Property(monitor_zynqmp BINARY_DIR)

foreach(CPU 1 2 3)
    set(MONITOR_ZYNQMP_HPP ${GENERATED_INCLUDE}/monitor_zynqmp_cpu${CPU}.hpp)
    FileEmbed_Add(${BINARY_DIR}/monitor_zynqmp_cpu${CPU}.bin ${MONITOR_ZYNQMP_HPP} monitor_zynqmp_cpu${CPU}_payload)
    list(APPEND MONITOR_ZYNQMP_HPP_ALL ${MONITOR_ZYNQMP_HPP})
endforeach()

add_subdirectory(${BMBOOT_ROOT}/elfload ${CMAKE_CURRENT_BINARY_DIR}/elfload)

add_library(bmboot_manager STATIC
        ${BMBOOT_ROOT}/include/bmboot.hpp
        ${BMBOOT_ROOT}/include/bmboot/domain.hpp
        ${BMBOOT_ROOT}/src/bmboot_internal.hpp
        ${BMBOOT_ROOT}/src/manager/configuration.cpp
        ${BMBOOT_ROOT}/src/manager/coredump_linux.cpp
        ${BMBOOT_ROOT}/src/manager/domain.cpp
        ${BMBOOT_ROOT}/src/manager/domain_helpers.cpp
        ${BMBOOT_ROOT}/src/platform/zynqmp/manager/zynqmp_manager.cpp
        ${BMBOOT_ROOT}/src/utility/crc32.c
        ${BMBOOT_ROOT}/src/utility/to_string.cpp

        ${MONITOR_ZYNQMP_HPP_ALL}
        )

add_dependencies(bmboot_manager monitor_zynqmp)

target_include_directories(bmboot_manager PUBLIC ${BMBOOT_ROOT}/include)
target_include_directories(bmboot_manager PRIVATE
        ${BMBOOT_ROOT}/src
        ${BMBOOT_ROOT}/src/platform/zynqmp
        ${BMBOOT_ROOT}/src/platform/zynqmp/manager

        ${GENERATED_INCLUDE}
        )

#    target_compile_features(bmboot_manager cpp20)
#    set_property(TARGET bmboot_manager PROPERTY CXX_STANDARD 20)
target_compile_features(bmboot_manager PUBLIC cxx_std_20)
target_link_libraries(bmboot_manager PUBLIC elfload)
add_library(bmboot::manager ALIAS bmboot_manager)

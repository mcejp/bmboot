cmake_minimum_required(VERSION 3.17)

# bmboot monitor (Baremetal "hypervisor" capable of running a payload)
# To be precise, what this configuration builds is the set of everything that bmboot_manager requires:
#   - monitor_zynqmp_cpuN
project(bmboot_monitor C CXX ASM)
set(BMBOOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# These flags will be added to all targets defined in this file
# No nicer way to do --gc-sections: https://gitlab.kitware.com/cmake/cmake/-/issues/23235
add_compile_options(-Wall -ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections -specs=nosys.specs)

# Monitor target (common)
add_library(monitor_zynqmp OBJECT
    ${BMBOOT_ROOT}/include/bmboot.hpp
    ${BMBOOT_ROOT}/src/executor/executor.cpp
    ${BMBOOT_ROOT}/src/executor/executor_asm.S
    ${BMBOOT_ROOT}/src/executor/monitor/monitor_asm.S
    ${BMBOOT_ROOT}/src/executor/monitor/monitor.cpp
    ${BMBOOT_ROOT}/src/executor/monitor/smc_handlers.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/asm_vectors.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/boot.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/monitor/interrupt_controller.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/monitor/vectors_el3.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/sleep.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/translation_table.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/xil-crt0.S
    ${BMBOOT_ROOT}/src/utility/crc32.c
    )

target_include_directories(monitor_zynqmp PRIVATE
    ${BMBOOT_ROOT}/include
    ${BMBOOT_ROOT}/src
    ${BMBOOT_ROOT}/src/executor
    ${BMBOOT_ROOT}/src/executor/monitor
    ${BMBOOT_ROOT}/src/platform/zynqmp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/monitor
    )
target_compile_features(monitor_zynqmp PUBLIC cxx_std_20)
target_link_libraries(monitor_zynqmp PRIVATE m)

# Instantiate monitor target for each CPU core

foreach(CPU 1 2 3)
    set(TARGET monitor_zynqmp_cpu${CPU})

    add_executable("${TARGET}" $<TARGET_OBJECTS:monitor_zynqmp>)
    # TODO: according to https://cmake.org/cmake/help/v3.4/manual/cmake-buildsystem.7.html#object-libraries,
    #       we should not link against monitor_zynqmp. For the time being, it seems to work, anyway.
    target_link_libraries("${TARGET}" PRIVATE monitor_zynqmp)

    set(LINKER_SCRIPT "${TOP_LEVEL_SOURCE_DIR}/bmboot_monitor_cpu${CPU}.ld")

    if(EXISTS "${LINKER_SCRIPT}")
        target_link_options(${TARGET} PRIVATE "-Wl,-T,${LINKER_SCRIPT}")
    else()
        message(WARNING "Linker script not found: ${LINKER_SCRIPT}")
    endif()

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}> ${CMAKE_BINARY_DIR}/${TARGET}.bin
        COMMAND ${CMAKE_OBJDUMP} -dt $<TARGET_FILE:${TARGET}> > ${TARGET}.txt
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
        COMMENT "Building ${CMAKE_BINARY_DIR}/${TARGET}.bin"
        )

    add_executable(bmboot::monitor_cpu${CPU} ALIAS ${TARGET})
endforeach()

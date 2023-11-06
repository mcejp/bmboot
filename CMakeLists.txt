cmake_minimum_required(VERSION 3.17)

if (BUILD_MONITOR OR BMBOOT_BUILD_PAYLOAD)
    if (CMAKE_SIZE STREQUAL "")
        message(FATAL_ERROR "You must provide CMAKE_SIZE option")
    endif()

    set(CMAKE_SYSTEM_NAME Generic)
    # SET(CMAKE_C_COMPILER_WORKS 1)
    # SET(CMAKE_CXX_COMPILER_WORKS 1)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    add_compile_options(-O2 -Wall -mtune=cortex-a53 -ffunction-sections -fdata-sections)
    add_compile_definitions(_DIOT=1)

    add_link_options(
            -Wall
#            -ffreestanding                  # no reason to use this since we do have a libc
            -specs=nosys.specs              # without this we don't get _exit, _sbrk and I/O functions
            -Wl,--gc-sections
    )
endif()

project(bmboot C CXX ASM)

include(cmake/Bmboot.cmake)

function(add_bmboot_payload_library BMBOOT_BSP_EL1_HOME BMBOOT_BSP_EL1_INCLUDE_DIR BMBOOT_BSP_EL1_LIBRARIES)
    set(TARGET bmboot_payload_runtime)
    add_library(${TARGET} STATIC
            src/executor/executor.cpp
            src/executor/executor_asm.S
            src/executor/payload/payload_runtime.cpp
            src/executor/payload/syscalls.cpp
            src/executor/payload/syscalls.h
            src/platform/zynqmp/executor/asm_vectors.S
            src/platform/zynqmp/executor/payload/vectors_el1.cpp
            )

    target_compile_features(${TARGET} PUBLIC cxx_std_20)

    target_include_directories(${TARGET} PUBLIC
            include
            )

    target_include_directories(${TARGET} PRIVATE
            src
            src/executor
            src/executor/payload
            src/platform/zynqmp
            src/platform/zynqmp/executor
            src/platform/zynqmp/executor/payload

            ${BMBOOT_BSP_EL1_INCLUDE_DIR}       # Note that this is PRIVATE, so it won't leak into user application
                                                # Q: Is that a feature or a bug?
            )

    target_link_libraries(${TARGET} PUBLIC
            ${BMBOOT_BSP_EL1_LIBRARIES}
            )

    target_link_options(${TARGET} PUBLIC
            # These are necessary when syscalls.cpp is in a static library: https://stackoverflow.com/q/34986536
            -Wl,--undefined=_close
            -Wl,--undefined=_fstat
            -Wl,--undefined=_isatty
            -Wl,--undefined=_lseek
            -Wl,--undefined=_read
            -Wl,--undefined=_write
            )
endfunction()

if (BMBOOT_BUILD_PAYLOAD)
    # note that in this case we expect BSP paths to be provided from outside

    if(NOT DEFINED BMBOOT_BSP_EL1_HOME)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_HOME must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL1_INCLUDE_DIR)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_INCLUDE_DIR must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL1_LIBRARIES)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_LIBRARIES must be set")
    endif()

    # -----------------------------------------------------------------------------------------------------------

    add_bmboot_payload_library("${BMBOOT_BSP_EL1_HOME}" "${BMBOOT_BSP_EL1_INCLUDE_DIR}" "${BMBOOT_BSP_EL1_LIBRARIES}")

    add_bmboot_payload(bmboot_example_payload src/payloads/hello_world.cpp)

    # -----------------------------------------------------------------------------------------------------------
elseif (BUILD_MONITOR)
    # to be precise, what this configuration builds is the set of everything that bmboot_manager requires:
    #   - monitor_zynqmp_cpuN
    #   - test payloads

    # not what we _should_ be doing, but something that works
    include(../bsp/bsp.cmake)
    MakeBsp(3)

    if(NOT DEFINED BMBOOT_BSP_EL3_INCLUDE_DIR)
        message(FATAL_ERROR "BMBOOT_BSP_EL3_INCLUDE_DIR must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL3_LIBRARIES)
        message(FATAL_ERROR "BMBOOT_BSP_EL3_LIBRARIES must be set")
    endif()

    foreach(CPU ${BMBOOT_ALL_CPUS})
        set(TARGET monitor_zynqmp_cpu${CPU})

        add_executable(${TARGET}
                include/bmboot.hpp
                src/executor/executor.cpp
                src/executor/executor_asm.S
                src/executor/monitor/monitor_asm.S
                src/executor/monitor/monitor.cpp
                src/executor/monitor/smc_handlers.cpp
                src/platform/zynqmp/executor/asm_vectors.S
                src/platform/zynqmp/executor/monitor/interrupt_controller.cpp
                src/platform/zynqmp/executor/monitor/vectors_el3.cpp
                src/utility/crc32.c
                )

        # -----------------------------------------------------------------------------------------------------------

        target_include_directories(${TARGET} PRIVATE
                include
                src
                src/executor
                src/executor/monitor
                src/platform/zynqmp
                src/platform/zynqmp/executor
                src/platform/zynqmp/executor/monitor

                ${BMBOOT_BSP_EL3_INCLUDE_DIR}
                )
        target_compile_features(${TARGET} PUBLIC cxx_std_20)
        target_link_libraries(${TARGET} PRIVATE
                m
                ${BMBOOT_BSP_EL3_LIBRARIES}
                )
        target_link_options(${TARGET} PRIVATE
                -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/src/executor/monitor/monitor_cpu${CPU}.ld
                )

        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}> ${CMAKE_BINARY_DIR}/${TARGET}.bin
                COMMAND ${CMAKE_OBJDUMP} -dt $<TARGET_FILE:${TARGET}> > ${TARGET}.txt
                COMMENT "Building ${CMAKE_BINARY_DIR}/${TARGET}.bin")

        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
            )

    endforeach()

    # -----------------------------------------------------------------------------------------------------------

    # use this opportunity to also build test payloads
    # is it ok to trigger also EL1 BSP here? or is this all wrong? THIS NEEDS A LONG, HARD LOOK

    MakeBsp(1)

    if(NOT DEFINED BMBOOT_BSP_EL1_HOME)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_HOME must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL1_INCLUDE_DIR)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_INCLUDE_DIR must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL1_LIBRARIES)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_LIBRARIES must be set")
    endif()

    add_bmboot_payload_library("${BMBOOT_BSP_EL1_HOME}" "${BMBOOT_BSP_EL1_INCLUDE_DIR}" "${BMBOOT_BSP_EL1_LIBRARIES}")

    foreach(PAYLOAD access_violation adrian_irq_demo hello_world timer_demo exception_caught_demo)
        add_bmboot_payload(payload_${PAYLOAD} src/payloads/${PAYLOAD}.cpp)
    endforeach()

    # -----------------------------------------------------------------------------------------------------------
else()
    set(GENERATED_INCLUDE ${CMAKE_BINARY_DIR}/generated_include)
    file(MAKE_DIRECTORY ${GENERATED_INCLUDE})

    include(ExternalProject)

    if(NOT DEFINED CMAKE_C_COMPILER_AARCH64_NONE_ELF)
        message(FATAL_ERROR "CMAKE_C_COMPILER_AARCH64_NONE_ELF must be set")
    endif()

    if(NOT DEFINED CMAKE_CXX_COMPILER_AARCH64_NONE_ELF)
        message(FATAL_ERROR "CMAKE_CXX_COMPILER_AARCH64_NONE_ELF must be set")
    endif()

    if(NOT DEFINED CMAKE_OBJCOPY_AARCH64_NONE_ELF)
        message(FATAL_ERROR "CMAKE_OBJCOPY_AARCH64_NONE_ELF must be set")
    endif()

    if(NOT DEFINED CMAKE_OBJDUMP_AARCH64_NONE_ELF)
        message(FATAL_ERROR "CMAKE_OBJDUMP_AARCH64_NONE_ELF must be set")
    endif()

    if(NOT DEFINED CMAKE_SIZE_AARCH64_NONE_ELF)
        message(FATAL_ERROR "CMAKE_SIZE_AARCH64_NONE_ELF must be set")
    endif()

    # add executor code as ExternalProject
    ExternalProject_Add(monitor_zynqmp
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
            CMAKE_ARGS
                # TODO: can we do better than having to enumerate these one by one?
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER_AARCH64_NONE_ELF}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER_AARCH64_NONE_ELF}
                -DCMAKE_OBJCOPY=${CMAKE_OBJCOPY_AARCH64_NONE_ELF}
                -DCMAKE_OBJDUMP=${CMAKE_OBJDUMP_AARCH64_NONE_ELF}
                -DCMAKE_SIZE=${CMAKE_SIZE_AARCH64_NONE_ELF}
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=RelWithDebInfo
                -DBUILD_MONITOR=ON
                -DBMBOOT_BSP_EL3_INCLUDE_DIR=${BMBOOT_BSP_EL3_INCLUDE_DIR}
                -DBMBOOT_BSP_EL3_LIBRARIES=${BMBOOT_BSP_EL3_LIBRARIES}
            INSTALL_COMMAND ""
            BUILD_ALWAYS ON
            BUILD_BYPRODUCTS
                "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu1.bin"
                "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu2.bin"
                "${CMAKE_CURRENT_BINARY_DIR}/monitor_zynqmp-prefix/src/monitor_zynqmp-build/monitor_zynqmp_cpu3.bin"
            )

    # We need to convert monitor binary to a C++ header
    include(cmake/FileEmbed.cmake)
    ExternalProject_Get_Property(monitor_zynqmp BINARY_DIR)
    set(MONITOR_ZYNQMP_HPP_ALL)

    foreach(CPU ${BMBOOT_ALL_CPUS})
        set(MONITOR_ZYNQMP_HPP ${GENERATED_INCLUDE}/monitor_zynqmp_cpu${CPU}.hpp)
        FileEmbed_Add(${BINARY_DIR}/monitor_zynqmp_cpu${CPU}.bin ${MONITOR_ZYNQMP_HPP} monitor_zynqmp_cpu${CPU}_payload)
        list(APPEND MONITOR_ZYNQMP_HPP_ALL ${MONITOR_ZYNQMP_HPP})
    endforeach()

    add_library(bmboot_manager STATIC
            include/bmboot.hpp
            include/bmboot/domain.hpp
            src/bmboot_internal.hpp
            src/manager/coredump_linux.cpp
            src/manager/domain.cpp
            src/manager/domain_helpers.cpp
            src/platform/zynqmp/manager/zynqmp_manager.cpp
            src/utility/crc32.c
            src/utility/to_string.cpp

            ${MONITOR_ZYNQMP_HPP_ALL}
            )

    add_dependencies(bmboot_manager monitor_zynqmp)

    target_include_directories(bmboot_manager PUBLIC include)
    target_include_directories(bmboot_manager PRIVATE
            src
            src/platform/zynqmp
            src/platform/zynqmp/manager

            ${GENERATED_INCLUDE}
            )

#    target_compile_features(bmboot_manager cpp20)
#    set_property(TARGET bmboot_manager PROPERTY CXX_STANDARD 20)
    target_compile_features(bmboot_manager PUBLIC cxx_std_20)

    add_executable(bmctl
            src/tools/bmctl.cpp
            )
    target_link_libraries(bmctl PUBLIC bmboot_manager)

    # Testing
    add_subdirectory("${LIBRARIES_HOME}/googletest-1.11.0" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

    add_executable(bmtest
            src/tests/tests.cpp
            src/tools/bmtest.cpp
            )
    target_link_libraries(bmtest PUBLIC bmboot_manager GTest::gtest pthread dl)
    target_link_options(bmtest PUBLIC  -Wl,-dynamic-linker,/my-lib/ld-linux-aarch64.so.1 -Wl,-rpath,/my-lib)

    add_executable(console src/tools/console.cpp)
    target_link_libraries(console PUBLIC bmboot_manager)

    foreach(TOOL bmctl console)
        # Make sure bmctl is linked fully statically
        # TODO: the clean solution would be to use a toolchain that matches the target OS
        target_link_options(${TOOL} PRIVATE -static -static-libgcc -static-libstdc++)
    endforeach()

endif()

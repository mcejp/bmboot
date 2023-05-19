cmake_minimum_required(VERSION 3.17)

if (BUILD_SLAVE_MONITOR OR BUILD_SLAVE_PAYLOAD)
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
            -ffreestanding
            -specs=nosys.specs
            -Wl,--gc-sections
            -L${CMAKE_CURRENT_SOURCE_DIR}/mainBinary
    )
endif()

project(bmboot C CXX ASM)

if (BUILD_SLAVE_PAYLOAD)
    if(NOT DEFINED BMBOOT_BSP_EL1_INCLUDE_DIR)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_INCLUDE_DIR must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL1_LIBRARIES)
        message(FATAL_ERROR "BMBOOT_BSP_EL1_LIBRARIES must be set")
    endif()

    # -----------------------------------------------------------------------------------------------------------

    add_library(bmboot_payload STATIC
            slave/zynqmp/vectors.cpp
            src/bmboot_slave.cpp

            ${BMBOOT_BSP_EL1_HOME}/libsrc/standalone_v7_1/src/asm_vectors.S
            )

    target_compile_features(bmboot_payload PUBLIC cxx_std_20)

    target_include_directories(bmboot_payload PUBLIC
            include
            ${BMBOOT_BSP_EL1_INCLUDE_DIR}
            )

    target_link_libraries(bmboot_payload PUBLIC
            ${BMBOOT_BSP_EL1_LIBRARIES}
            )

    target_link_options(bmboot_payload PUBLIC
            -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/slave/zynqmp/cpu1/payload.ld
            )

    include(cmake/Bmboot.cmake)

    add_executable(bmboot_example_payload.elf
            src/example_payload/main.cpp
            )

    target_link_libraries(bmboot_example_payload.elf PRIVATE bmboot_payload)

    Bmboot_PayloadPostBuild(bmboot_example_payload.elf)

    # -----------------------------------------------------------------------------------------------------------
elseif (BUILD_SLAVE_MONITOR)
    # not what we _should_ be doing, but something that works
    include(../bsp/bsp.cmake)
    MakeBsp(3)

    if(NOT DEFINED BMBOOT_BSP_EL3_INCLUDE_DIR)
        message(FATAL_ERROR "BMBOOT_BSP_EL3_INCLUDE_DIR must be set")
    endif()

    if(NOT DEFINED BMBOOT_BSP_EL3_LIBRARIES)
        message(FATAL_ERROR "BMBOOT_BSP_EL3_LIBRARIES must be set")
    endif()

    add_executable(bmboot_slave_zynqmp_cpu1.elf
            include/bmboot.hpp
            include/bmboot_slave.hpp
            src/mach/zynqmp_baremetal.cpp
            src/slave_lowlevel.S
            src/slave_main.cpp
            src/bmboot_internal.hpp
            src/bmboot_slave.cpp

            # slightly wrong as this will indicate crashedPayload even if the fault is in the monitor
            slave/zynqmp/vectors.cpp
            slave/zynqmp/el3/asm_vectors.S
            )

    # -----------------------------------------------------------------------------------------------------------

    target_include_directories(bmboot_slave_zynqmp_cpu1.elf PRIVATE
            include
            ${BMBOOT_BSP_EL3_INCLUDE_DIR}
            )
    target_compile_features(bmboot_slave_zynqmp_cpu1.elf PUBLIC cxx_std_20)
    target_link_libraries(bmboot_slave_zynqmp_cpu1.elf PRIVATE
            m
            ${BMBOOT_BSP_EL3_LIBRARIES}
            )
    target_link_options(bmboot_slave_zynqmp_cpu1.elf PRIVATE
            -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/slave/zynqmp/cpu1/monitor.ld
            )

    add_custom_command(TARGET bmboot_slave_zynqmp_cpu1.elf POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:bmboot_slave_zynqmp_cpu1.elf> ${CMAKE_BINARY_DIR}/bmboot_slave_zynqmp_cpu1.bin
            COMMAND ${CMAKE_OBJDUMP} -dt $<TARGET_FILE:bmboot_slave_zynqmp_cpu1.elf> > bmboot_slave_zynqmp_cpu1.txt
            COMMENT "Building ${CMAKE_BINARY_DIR}/bmboot_slave_zynqmp_cpu1.bin")

    add_custom_command(TARGET bmboot_slave_zynqmp_cpu1.elf POST_BUILD
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:bmboot_slave_zynqmp_cpu1.elf>
            )

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

    # add slave code as ExternalProject
    ExternalProject_Add(bmboot_slave_zynqmp
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
            CMAKE_ARGS
                # TODO: can we do better than having to enumerate these one by one?
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER_AARCH64_NONE_ELF}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER_AARCH64_NONE_ELF}
                -DCMAKE_OBJCOPY=${CMAKE_OBJCOPY_AARCH64_NONE_ELF}
                -DCMAKE_OBJDUMP=${CMAKE_OBJDUMP_AARCH64_NONE_ELF}
                -DCMAKE_SIZE=${CMAKE_SIZE_AARCH64_NONE_ELF}
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_BUILD_TYPE=Release
                -DBUILD_SLAVE_MONITOR=ON
                -DBMBOOT_BSP_EL3_INCLUDE_DIR=${BMBOOT_BSP_EL3_INCLUDE_DIR}
                -DBMBOOT_BSP_EL3_LIBRARIES=${BMBOOT_BSP_EL3_LIBRARIES}
            INSTALL_COMMAND ""
            BUILD_ALWAYS ON
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/bmboot_slave_zynqmp-prefix/src/bmboot_slave_zynqmp-build/bmboot_slave_zynqmp_cpu1.bin"
            )

    # We need to convert slave binary to a C++ header
    include(cmake/FileEmbed.cmake)
    set(SLAVE_ZYNQMP_CPU1_HPP ${GENERATED_INCLUDE}/bmboot_slave_zynqmp_cpu1.hpp)
    ExternalProject_Get_Property(bmboot_slave_zynqmp BINARY_DIR)
    FileEmbed_Add(${BINARY_DIR}/bmboot_slave_zynqmp_cpu1.bin ${SLAVE_ZYNQMP_CPU1_HPP} slave_zynqmp_cpu1_payload)

    add_library(bmboot_master STATIC
            include/bmboot.hpp
            include/bmboot_master.hpp
            src/mach/zynqmp_linux.cpp
            src/bmboot_internal.hpp
            src/bmboot_master.cpp
            src/utility/to_string.cpp

            ${SLAVE_ZYNQMP_CPU1_HPP}
            )

    add_dependencies(bmboot_master bmboot_slave_zynqmp)

    target_include_directories(bmboot_master PUBLIC include)
    target_include_directories(bmboot_master PRIVATE ${GENERATED_INCLUDE})

#    target_compile_features(bmboot_master cpp20)
#    set_property(TARGET bmboot_master PROPERTY CXX_STANDARD 20)
    target_compile_features(bmboot_master PUBLIC cxx_std_20)

    add_executable(bmctl src/tools/bmctl.cpp)
    target_link_libraries(bmctl PUBLIC bmboot_master)

    add_executable(console src/tools/console.cpp)
    target_link_libraries(console PUBLIC bmboot_master)

    foreach(TOOL bmctl console)
        # Make sure bmctl is linked fully statically
        # TODO: the clean solution would be to use a toolchain that matches the target OS
        target_link_options(${TOOL} PRIVATE -static -static-libgcc -static-libstdc++)
    endforeach()

endif()

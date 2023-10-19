***************
How to build it
***************

The build system is driven by CMake. A tricky aspect of it is the requirement to use two separate toolchains:

- ``aarch64-none-linux-gnu-gcc`` 12.3 (Linux)
- ``aarch64-none-elf-gcc`` 12.3 (bare metal)

The build system operates in one of 3 modes:

- :term:`manager`
- :term:`monitor`
- :term:`payload`

When building the manager, CMake will recurse into itself to build the monitor, which ends up being embedded in
the manager library. Due to this, a number of variables are mandatory, whether building the project stand-alone or via
``add_subdirectory``.

- CMAKE_C_COMPILER_AARCH64_NONE_ELF
- CMAKE_CXX_COMPILER_AARCH64_NONE_ELF
- CMAKE_OBJCOPY_AARCH64_NONE_ELF
- CMAKE_OBJDUMP_AARCH64_NONE_ELF
- CMAKE_SIZE_AARCH64_NONE_ELF
- BMBOOT_BSP_EL3_INCLUDE_DIR
- BMBOOT_BSP_EL3_LIBRARIES

To build the payload runtime support library, bmboot must be configured with the ``BMBOOT_BUILD_PAYLOAD`` CMake variable set.
Additionally, these are mandatory:

- BMBOOT_BSP_EL1_HOME
- BMBOOT_BSP_EL1_INCLUDE_DIR
- BMBOOT_BSP_EL1_LIBRARIES


User payloads
=============

The following CMake function should be used to declare payloads:

.. code-block:: cmake

  add_bmboot_payload(<name> [source1] [source2 ...])

The ``<name>`` argument will be used as a basis for naming the instantiated targets, which can be several,
in order to support multiple executor CPUs. All remaining arguments will be passed on to the underlying call(s) to
`add_executable`_.

.. _add_executable: https://cmake.org/cmake/help/latest/command/add_executable.html

The complete list of targets created will be saved into a variable called ``<name>_TARGETS``.

Voici un exemple:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.17)

    project(hello_world C CXX ASM)

    set(BMBOOT_BUILD_PAYLOAD 1)
    add_subdirectory(../bmboot ${CMAKE_CURRENT_BINARY_DIR}/bmboot)

    include(../bmboot/cmake/Bmboot.cmake)

    add_bmboot_payload(hello_world src/main.cpp)

    foreach (TARGET ${hello_world_TARGETS})
        target_compile_definitions(${TARGET} PRIVATE HELLO=world)
        target_include_directories(${TARGET} PRIVATE include)
    endforeach()



.. TODO: BSP concerns

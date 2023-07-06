***************
How to build it
***************

The build system is driven by CMake. A tricky aspect of it is the requirement to use two separate toolchains:

- ``aarch64-none-linux-gnu-gcc`` 10.3+ (Linux)
- ``aarch64-none-elf-gcc`` 10.3+ (bare metal)

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


.. TODO: BSP concerns

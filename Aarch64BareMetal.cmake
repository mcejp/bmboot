set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(PREFIX "aarch64-none-elf-")

set(CMAKE_LINKER aarch64-none-elf-ld CACHE FILEPATH "Linker Binary")
#set(CMAKE_ASM_COMPILER aarch64-none-elf-as CACHE FILEPATH "ASM Compiler")      # TODO: this breaks Bmboot right now
set(CMAKE_C_COMPILER aarch64-none-elf-gcc CACHE FILEPATH "C Compiler")
set(CMAKE_CXX_COMPILER aarch64-none-elf-g++ CACHE FILEPATH "C Compiler")
set(CMAKE_OBJCOPY aarch64-none-elf-objcopy CACHE FILEPATH "Objcopy Binary")
set(CMAKE_OBJDUMP aarch64-none-elf-objdump CACHE FILEPATH "Objdump Binary")

# This one is not known to CMake, but we need it
find_program(CMAKE_SIZE aarch64-none-elf-size)

set(CMAKE_C_FLAGS_INIT -mtune=cortex-a53)

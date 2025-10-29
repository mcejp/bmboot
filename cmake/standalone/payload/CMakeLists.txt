cmake_minimum_required(VERSION 3.17)

project(bmboot_payload C CXX ASM)

set(BMBOOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_compile_options(-Wall -ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections -specs=nosys.specs)

#######################################################
# Set up the payload library
#######################################################
set(BMBOOT_PAYLOAD_LIB bmboot_payload_runtime)
add_library(${BMBOOT_PAYLOAD_LIB} STATIC
    ${BMBOOT_ROOT}/src/executor/executor.cpp
    ${BMBOOT_ROOT}/src/executor/executor_asm.S
    ${BMBOOT_ROOT}/src/executor/payload/payload_runtime.cpp
    ${BMBOOT_ROOT}/src/executor/payload/syscalls.cpp
    ${BMBOOT_ROOT}/src/executor/payload/syscalls.h
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/asm_vectors.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/boot.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/payload/vectors_el1.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/sleep.cpp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/translation_table.S
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/xil-crt0.S
)
add_library(bmboot::payload_runtime ALIAS ${BMBOOT_PAYLOAD_LIB})

target_compile_definitions(${BMBOOT_PAYLOAD_LIB} PUBLIC __bmboot__=1)
target_compile_features   (${BMBOOT_PAYLOAD_LIB} PUBLIC cxx_std_20)
target_compile_options    (${BMBOOT_PAYLOAD_LIB} PRIVATE -Wall)

target_include_directories(${BMBOOT_PAYLOAD_LIB} PUBLIC ${BMBOOT_ROOT}/include)

target_include_directories(${BMBOOT_PAYLOAD_LIB} PRIVATE
    ${BMBOOT_ROOT}/src
    ${BMBOOT_ROOT}/src/executor
    ${BMBOOT_ROOT}/src/executor/payload
    ${BMBOOT_ROOT}/src/platform/zynqmp
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor
    ${BMBOOT_ROOT}/src/platform/zynqmp/executor/payload
)

target_link_options(${BMBOOT_PAYLOAD_LIB} PUBLIC
        # These are necessary when syscalls.cpp is in a static library: https://stackoverflow.com/q/34986536
        -Wl,--undefined=_close
        -Wl,--undefined=_fstat
        -Wl,--undefined=_isatty
        -Wl,--undefined=_lseek
)

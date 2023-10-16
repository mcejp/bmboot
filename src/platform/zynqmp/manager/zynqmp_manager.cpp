//! @file
//! @brief  Machine-specific functions, Linux
//! @author Martin Cejp

#include "utility/mmap.hpp"
#include "zynqmp_manager.hpp"

#include <cstring>

#define IPI_BUF_BASE_APU            0xFF990400
#define IPI_BUF_APU_TO_APU_REQ      IPI_BUF_BASE_APU
#define IPI_BUF_APU_TO_APU_RESP     (IPI_BUF_BASE_APU + 0x20)
#define IPI_BUF_SIZE                0x20

using namespace bmboot;
using namespace bmboot::mach;

// ************************************************************

bool mach::isZynqCpu1InReset(int devmem_fd) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);

    if (!base_0xFD1A0000) {
        return false;       // WRONG
    }

    return (base_0xFD1A0000.read32(0x0104) & (1 << 1)) != 0;
}

// ************************************************************

std::optional<ErrorCode> mach::bootZynqCpu1(int devmem_fd, uintptr_t reset_address) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);
    Mmap base_0xFD5C0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD5C0000);

    if (!base_0xFD1A0000 || !base_0xFD5C0000) {
        return ErrorCode::mmap_failed;
    }

    int cpu_index = 1;

    // De-assert core reset through the RST_FPD_APU control register
    // https://docs.xilinx.com/r/en-US/ug1087-zynq-ultrascale-registers/RST_FPD_APU-CRF_APB-Register
    //
    // Essentially, this can only be done once per SoC reset. See note in UG1085 Chapter 38:
    // > IMPORTANT: The system can hang when software reset control is asserted during a pending AXI/APB
    // > transfer.
    //
    // And thus, Bmboot was born.
    auto init_val = base_0xFD1A0000.read32(0x0104);

    // acpuN_reset or acpuN_pwron_reset must be set, otherwise the core is already running
    if ((init_val & (0x401 << cpu_index)) == 0)
    {
        return ErrorCode::hw_resource_unavailable;
    }

    base_0xFD5C0000.write32(0x0048, reset_address);             // set initial address: RVBARADDR1L
    base_0xFD5C0000.write32(0x004C, reset_address >> 32);       //                      RVBARADDR1H
    base_0xFD1A0000.write32(0x0104, init_val & ~(0x401 << cpu_index));      // de-assert POR + reset on CPU1

    return {};
}

// ************************************************************

void mach::sendIpiMessage(int devmem_fd, std::span<const uint8_t> message) {
    Mmap base_0xFF990000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFF990000);
    Mmap base_0xFF300000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFF300000);

    uint32_t message_mirror[IPI_BUF_SIZE / 4];
    memcpy(message_mirror, message.data(), std::min<size_t>(message.size(), IPI_BUF_SIZE));

    // must be done with 32-bit access; memcpy will crash
    for (int i = 0; i < std::size(message_mirror); i++) {
        base_0xFF990000.write32(IPI_BUF_APU_TO_APU_REQ - 0xFF990000 + i * 4, message_mirror[i]);
    }

    // trigger interrupt
    base_0xFF300000.write32(0, 1);
}

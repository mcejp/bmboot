//! @file
//! @brief  Machine-specific functions, Linux
//! @author Martin Cejp

#include "utility/mmap.hpp"
#include "zynqmp.hpp"
#include "zynqmp_manager.hpp"

#include <cstring>

#define IPI_BUF_BASE_APU            0xFF990400
#define IPI_BUF_APU_TO_APU_REQ      IPI_BUF_BASE_APU
#define IPI_BUF_APU_TO_APU_RESP     (IPI_BUF_BASE_APU + 0x20)
#define IPI_BUF_SIZE                0x20

using namespace bmboot;
using namespace bmboot::mach;

static int getCpuIndex(DomainIndex domain_index)
{
    switch (domain_index)
    {
        case DomainIndex::cpu1: return 1;
        case DomainIndex::cpu2: return 2;
        case DomainIndex::cpu3: return 3;

        default:
            std::terminate();
    }
}

// ************************************************************

bool mach::isCoreInReset(int devmem_fd, DomainIndex domain_index) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);

    if (!base_0xFD1A0000) {
        return false;       // WRONG
    }

    auto cpu_index = getCpuIndex(domain_index);

    return (base_0xFD1A0000.read32(0x0104) & (0x401 << cpu_index)) != 0;
}

// ************************************************************

std::optional<ErrorCode> mach::bootCore(int devmem_fd, DomainIndex domain_index, uintptr_t reset_address) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);
    Mmap base_0xFD5C0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD5C0000);

    if (!base_0xFD1A0000 || !base_0xFD5C0000) {
        return ErrorCode::mmap_failed;
    }

    auto cpu_index = getCpuIndex(domain_index);

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

    base_0xFD5C0000.write32(0x0040 + cpu_index * 8, reset_address);         // set initial address: RVBARADDR1L
    base_0xFD5C0000.write32(0x0044 + cpu_index * 8, reset_address >> 32);   //                      RVBARADDR1H
    base_0xFD1A0000.write32(0x0104, init_val & ~(0x401 << cpu_index));      // de-assert POR + reset on CPU1

    return {};
}

// ************************************************************

std::optional<ErrorCode> mach::sendIpiMessage(int devmem_fd, DomainIndex domain_index, std::span<const uint8_t> message) {
    off_t message_buffer_base;

    // Mapping of IPI channels to base addresses can be found in UG1085, Table 13-3: IPI Channel and Message Buffer Default Associations

    switch (getIpiChannelForCpu(getCpuIndex(domain_index)))
    {
        case IpiChannel::ch0:
            message_buffer_base = 0xFF99'0400;
            break;

        case IpiChannel::ch1:
            message_buffer_base = 0xFF99'0000;
            break;

        case IpiChannel::ch2:
            message_buffer_base = 0xFF99'0200;
            break;

        case IpiChannel::ch7:
            message_buffer_base = 0xFF99'0600;
            break;

        case IpiChannel::ch8:
            message_buffer_base = 0xFF99'0800;
            break;

        case IpiChannel::ch9:
            message_buffer_base = 0xFF99'0A00;
            break;

        case IpiChannel::ch10:
            message_buffer_base = 0xFF99'0C00;
            break;

        default:
            return ErrorCode::hw_resource_unavailable;
    }

    off_t irq_base = getIpiBaseAddress(IPI_SRC_BMBOOT_MANAGER);

    Mmap base_0xFF990000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFF990000);
    Mmap irq_mmap(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, irq_base);

    uint32_t message_mirror[IPI_BUF_SIZE / 4];
    memcpy(message_mirror, message.data(), std::min<size_t>(message.size(), IPI_BUF_SIZE));

    // must be done with 32-bit access; memcpy will crash
    for (int i = 0; i < std::size(message_mirror); i++) {
        base_0xFF990000.write32(message_buffer_base - 0xFF990000 + i * 4, message_mirror[i]);
    }

    // trigger interrupt
    auto receiver_channel = getIpiChannelForCpu(getCpuIndex(domain_index));
    irq_mmap.write32(0, getIpiPeerMask(receiver_channel));

    return {};
}

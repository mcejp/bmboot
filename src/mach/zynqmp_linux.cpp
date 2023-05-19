#include "mach_linux.hpp"
#include "../bmboot_internal.hpp"
#include "../utility/mmap.hpp"

#include <cstring>


namespace bmboot::mach {

bool is_zynq_cpu1_in_reset(int devmem_fd) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);

    if (!base_0xFD1A0000) {
        return false;       // WRONG
    }

    return (base_0xFD1A0000.read32(0x0104) & (1 << 1)) != 0;
}

std::optional<ErrorCode> boot_zynq_cpu1(int devmem_fd, uintptr_t reset_address) {
    Mmap base_0xFD1A0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD1A0000);
    Mmap base_0xFD5C0000(nullptr, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, 0xFD5C0000);

    if (!base_0xFD1A0000 || !base_0xFD5C0000) {
        return ErrorCode::mmapFailed;
    }

    base_0xFD1A0000.write32(0x0104, 0x0000380E);           // assert reset on CPU1
    base_0xFD5C0000.write32(0x0048, reset_address);        // set initial address
    base_0xFD5C0000.write32(0x004C, reset_address >> 32);
    base_0xFD1A0000.write32(0x0104, 0x0000300C);           // de-assert reset on CPU1

    return {};
}

#define IPI_BUF_BASE_APU            0xFF990400
#define IPI_BUF_APU_TO_APU_REQ      IPI_BUF_BASE_APU
#define IPI_BUF_APU_TO_APU_RESP     (IPI_BUF_BASE_APU + 0x20)
#define IPI_BUF_SIZE                0x20

void send_ipi_message(int devmem_fd, std::span<const uint8_t> message) {
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

}

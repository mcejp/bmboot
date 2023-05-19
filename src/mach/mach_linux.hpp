#pragma once

#include "bmboot.hpp"

#include <cstdint>
#include <optional>
#include <span>

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

namespace bmboot::mach {

using namespace bmboot;

bool is_zynq_cpu1_in_reset(int devmem_fd);
std::optional<ErrorCode> boot_zynq_cpu1(int devmem_fd, uintptr_t reset_address);

void send_ipi_message(int devmem_fd, std::span<const uint8_t> message);

}

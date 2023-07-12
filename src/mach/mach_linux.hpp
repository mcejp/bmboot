//! @file
//! @brief  Machine-specific functions, Linux
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"

#include <cstdint>
#include <optional>
#include <span>

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

namespace bmboot::mach
{

bool isZynqCpu1InReset(int devmem_fd);
std::optional<ErrorCode> bootZynqCpu1(int devmem_fd, uintptr_t reset_address);

void sendIpiMessage(int devmem_fd, std::span<const uint8_t> message);

}

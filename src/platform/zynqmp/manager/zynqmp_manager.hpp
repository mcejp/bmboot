//! @file
//! @brief  Machine-specific functions, Linux
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"

#include <cstdint>
#include <optional>
#include <span>

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

namespace zynqmp
{

bool isCoreInReset(int devmem_fd, bmboot::DomainIndex domain_index);
std::optional<bmboot::ErrorCode> bootCore(int devmem_fd, bmboot::DomainIndex domain_index, uintptr_t reset_address);

std::optional<bmboot::ErrorCode> sendIpiMessage(int devmem_fd, bmboot::DomainIndex domain_index, std::span<const uint8_t> message);

}

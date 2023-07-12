//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"
#include "mach_baremetal_defs.hpp"

#include <span>

namespace bmboot::mach
{

void flushICache();

void sendIpiMessage(std::span<const std::byte> message);

void enableCpuInterrupts();
void enableIpiReception(int src_channel);
void setupInterrupt(int ch, int target_cpu);

}

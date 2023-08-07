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

void disablePrivatePeripheralInterrupt(int ch);
void enableCpuInterrupts();
void enableIpiReception(int src_channel);

//! Unmask a given Private Peripheral Interrupt (PPI) for the current CPU.
//!
//! See ARM IHI 0048B.b for more details
//! \param ch
void enablePrivatePeripheralInterrupt(int ch);

//! Route a given Shared Peripheral Interrupt (SPI) to the given CPU and make sure it is not masked.
//!
//! See ARM IHI 0048B.b for more details
//! \param ch
//! \param target_cpu
void enableSharedPeripheralInterruptAndRouteToCpu(int ch, int target_cpu);

void handleTimerIrq();

}

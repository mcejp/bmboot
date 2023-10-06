//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"
#include "mach_baremetal_defs.hpp"

#include <span>

namespace bmboot::mach
{

enum class InterruptGroup
{
    group0_fiq_el3,
    group1_irq_el1,
};

// higher value = lower priority
// the resolution is implementation-defined, on the Zynq the upper 4 bits of the byte should be taken into consideration
enum class InterruptPriority
{
    high = 0x20,
    medium = 0x80,
};

void flushICache();

void sendIpiMessage(std::span<const std::byte> message);

void disablePrivatePeripheralInterrupt(int ch);
void enableCpuInterrupts();
void enableIpiReception(int src_channel);

//! Unmask a given Private Peripheral Interrupt (PPI) for the current CPU.
//!
//! See ARM IHI 0048B.b for more details
//! \param ch
//! \param group
//! \param priority
void enablePrivatePeripheralInterrupt(int ch, InterruptGroup group, InterruptPriority priority);

//! Route a given Shared Peripheral Interrupt (SPI) to the given CPU and make sure it is not masked.
//!
//! See ARM IHI 0048B.b for more details
//! \param ch
//! \param target_cpu
//! \param group
//! \param priority
void enableSharedPeripheralInterruptAndRouteToCpu(int ch, int target_cpu, InterruptGroup group, InterruptPriority priority);

void handleTimerIrq();

}

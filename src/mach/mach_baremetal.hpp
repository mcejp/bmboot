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
enum class MonitorInterruptPriority
{
    m7_max = 0x00,
    m6 = 0x10,
    m5 = 0x20,
    m4 = 0x30,
    m3 = 0x40,
    m2 = 0x50,
    m1 = 0x60,
    m0_min = 0x70,
    payloadMinPriorityValue = 0x80,
    payloadMaxPriorityValue = 0xF0,
};

enum class InterruptTrigger
{
    unchanged,
    level,
    edge,
};

void flushICache();

//! Disable all interrupts that have been routed to EL1
//! This is necessary when the monitor restarts, since the IRQ/FIQ routing options will be reset and the interrupts
//! would be delivered to EL3 (and we crash pretty hard on any spurious interrupt. that's by design.)
void teardownEl1Interrupts();

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
void enablePrivatePeripheralInterrupt(int ch, InterruptGroup group, MonitorInterruptPriority priority);

//! Route a given Shared Peripheral Interrupt (SPI) to the given CPU and make sure it is not masked.
//!
//! See ARM IHI 0048B.b for more details
//! \param ch
//! \param target_cpu
//! \param group
//! \param priority
void enableSharedPeripheralInterruptAndRouteToCpu(int ch,
                                                  InterruptTrigger trigger,
                                                  int target_cpu,
                                                  InterruptGroup group,
                                                  MonitorInterruptPriority priority);

void handleTimerIrq();

}

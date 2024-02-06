//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#include "armv8a.hpp"
#include "bmboot_internal.hpp"
#include "executor.hpp"
#include "platform_interrupt_controller.hpp"
#include "zynqmp.hpp"

// ************************************************************

using namespace arm;
using arm::armv8a::DAIF_F_MASK;
using arm::armv8a::DAIF_I_MASK;
using namespace bmboot::internal;
using namespace bmboot::platform;
using zynqmp::ipipsu::IpiChannel;
using zynqmp::scugic::getInterruptIdForIpi;
using zynqmp::scugic::GICC;
using zynqmp::scugic::GICD;

static bool interrupt_routed_to_el1[(GIC_MAX_USER_INTERRUPT_ID + 1) - GIC_MIN_USER_INTERRUPT_ID];

// ************************************************************

int zynqmp::scugic::getInterruptIdForIpi(IpiChannel ipi_channel)
{
    // Mapping of IPI channels to interrupt IDs can be found in UG1085, Table 13-1: System Interrupts
    switch (ipi_channel)
    {
        case IpiChannel::ch0: return 67;
        case IpiChannel::ch1: return 65;
        case IpiChannel::ch2: return 66;
        case IpiChannel::ch7: return 61;
        case IpiChannel::ch8: return 62;
        case IpiChannel::ch9: return 63;
        case IpiChannel::ch10: return 64;
    }
}

// ************************************************************

static void enableCpuInterrupts() {
    // Upper 4 bits of priority value determine priority for preemption purposes
    GICC->BPR = 0x03;
    // Priority Mask Register: maximum value
    GICC->PMR = 0xFF;

    // Note that the meaning of the bits in this register changes based on S/NS access
    // We execute this in EL3 aka Secure
    GICC->CTRL = (gicv2::GICC::CTRL_FIQEn |
                  gicv2::GICC::CTRL_AckCtl |
                  gicv2::GICC::CTRL_EnableGrp1 |
                  gicv2::GICC::CTRL_EnableGrp0);

    // Enable FIQ & IRQ.
    // By design, in the monitor we only receive FIQ, but since all IRQ handler code is already in place, just in case
    // we mess up and get one, we might as well report it, instead of falling into some kind of stub handler.
    writeSysReg(DAIF, readSysReg(DAIF) & ~DAIF_I_MASK & ~DAIF_F_MASK);
}

// ************************************************************

static void enableIpiReception(IpiChannel src_channel, IpiChannel dst_channel)
{
    auto ipi = getIpi(dst_channel);

    // clear any pending request
    ipi->ISR = getIpiPeerMask(src_channel);

    // enable channel
    ipi->IER = getIpiPeerMask(src_channel);
}

// ************************************************************

void bmboot::platform::flushICache() {
    // Adapted from Xil_ICacheInvalidate in BSP lib/bsp/standalone/src/arm/ARMv8/64bit/xil_cache.c
    // This code seems rather weird -- compare with __asm_invalidate_icache_all in U-Boot arch/arm/cpu/armv8/cache.S

    // Copyright (C) 2014 - 2022 Xilinx, Inc.  All rights reserved.
    // Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All Rights Reserved.

    unsigned int currmask;
    currmask = readSysReg(DAIF);
    writeSysReg(DAIF, currmask | DAIF_F_MASK | DAIF_I_MASK);
    writeSysReg(CSSELR_EL1,0x1);
    __asm__ __volatile__("dsb sy");
    /* invalidate the instruction cache */
    __asm__ __volatile__("ic iallu");
    /* Wait for invalidate to complete */
    __asm__ __volatile__("dsb sy");
    writeSysReg(DAIF, currmask);
}

// ************************************************************

static void setGroupForInterruptChannel(int int_id, InterruptGroup group)
{
    if (group == InterruptGroup::group0_fiq_el3)
    {
        GICD->setGroup(int_id, 0);
    }
    else
    {
        GICD->setGroup(int_id, 1);
    }

    // Update interrupt_routed_to_el1 if the interrupt ID falls in its range
    if (int_id >= GIC_MIN_USER_INTERRUPT_ID && int_id <= GIC_MAX_USER_INTERRUPT_ID)
    {
        interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID] = (group == InterruptGroup::group1_irq_el1);
    }
}

void bmboot::platform::configurePrivatePeripheralInterrupt(int ch, InterruptGroup group, MonitorInterruptPriority priority) {
    // TODO: should maybe just use Xilinx SDK functions

    setGroupForInterruptChannel(ch, group);

    GICD->IPRIORITYRn[ch] = (int)priority;
}

// ************************************************************

void bmboot::platform::configureSharedPeripheralInterruptAndRouteToCpu(int ch,
                                                                       InterruptTrigger trigger,
                                                                       int target_cpu,
                                                                       InterruptGroup group,
                                                                       MonitorInterruptPriority priority) {
    setGroupForInterruptChannel(ch, group);

    GICD->IPRIORITYRn[ch] = (int)priority;
    GICD->ITARGETSRn[ch] = (1 << target_cpu);

    if (trigger == InterruptTrigger::level)
    {
        GICD->setTriggerLevel(ch);
    }
    else if (trigger == InterruptTrigger::edge)
    {
        GICD->setTriggerEdge(ch);
    }
}

// ************************************************************

void bmboot::platform::disableInterrupt(int interrupt_id)
{
    GICD->clearEnable(interrupt_id);
}

// ************************************************************

void bmboot::platform::enableInterrupt(int interrupt_id)
{
    // Clear pending and active statuses.
    // We don't know what happened in the past, a previous payload might have been terminated during the handling this
    // interrupt, in which case the interrupt would remain in an Active state in the GIC.

    GICD->clearPending(interrupt_id);
    GICD->clearActive(interrupt_id);
    GICD->setEnable(interrupt_id);
}

// ************************************************************

void bmboot::platform::teardownEl1Interrupts()
{
    platform::disableInterrupt(zynqmp::scugic::CNTPNS_INTERRUPT_ID);

    for (int int_id = GIC_MIN_USER_INTERRUPT_ID; int_id <= GIC_MAX_USER_INTERRUPT_ID; int_id++)
    {
        if (interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID])
        {
            disableInterrupt(int_id);

            interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID] = false;
        }
    }
}

// ************************************************************

void bmboot::platform::setupInterrupts()
{
    enableCpuInterrupts();

    // Enable IPI reception
    const auto my_ipi_channel = getIpiChannelForCpu(getCpuIndex());
    const auto ipi_interrupt_id = getInterruptIdForIpi(my_ipi_channel);

    platform::configureSharedPeripheralInterruptAndRouteToCpu(ipi_interrupt_id,
                                                              platform::InterruptTrigger::unchanged,
                                                              getCpuIndex(),
                                                              platform::InterruptGroup::group0_fiq_el3,
                                                              platform::MonitorInterruptPriority::m7_max);

    platform::enableInterrupt(ipi_interrupt_id);

    enableIpiReception(IPI_SRC_BMBOOT_MANAGER, my_ipi_channel);
}

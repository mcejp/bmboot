//! @file
//! @brief  Machine-specific definitions
//! @author Martin Cejp

// TODO: this file still has a purpose for being separate?

#pragma once

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

namespace bmboot::mach {

constexpr inline auto IPI_SRC_APU = 0;
constexpr inline auto IPI_CH0_GIC_CHANNEL = 67;

constexpr inline auto CNTPNS_IRQ_CHANNEL = 30;

// these are true for now, but will need to be redone for multiple executor CPUs
const int SELF_CPU_INDEX = 1;
constexpr inline auto IPI_CURRENT_CPU_GIC_CHANNEL = IPI_CH0_GIC_CHANNEL;

}

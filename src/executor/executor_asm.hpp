//! @file
//! @brief  Executor assembly functions
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

// Note that this syntax is a GCC extension
#define readSysReg(reg)         ({ uint64_t value; __asm__ __volatile__("mrs %0, " #reg : "=r" (value)); value; })
#define writeSysReg(reg, value)   __asm__ __volatile__("msr " #reg ",%0"  : : "r" (value))

namespace bmboot::internal
{

extern "C" void saveFpuState(Aarch64_FpRegs&);

extern "C" intptr_t smc(int function_id, ...);

}

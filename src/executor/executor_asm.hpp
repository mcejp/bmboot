//! @file
//! @brief  Executor assembly functions
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

#define memory_write_reorder_barrier() __asm volatile ("dmb ishst" : : : "memory")

namespace bmboot::internal
{

extern "C" void saveFpuState(Aarch64_FpRegs&);

extern "C" intptr_t smc(int function_id, ...);

}

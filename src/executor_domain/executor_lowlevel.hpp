//! @file
//! @brief  Low-level executor functions (implemented in assembly).
//! @author Martin Cejp

#pragma once

#include "../bmboot_internal.hpp"

namespace bmboot::internal
{

extern "C" void _boot();
extern "C" void enterEL1Payload(uintptr_t address);
extern "C" void saveFpuState(Aarch64_FpRegs&);

extern "C" intptr_t smc(int function_id, ...);

}

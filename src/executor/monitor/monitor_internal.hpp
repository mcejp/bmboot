//! @file
//! @brief  Monitor internal functions
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

namespace bmboot::internal
{

enum class CrashingEntity
{
    monitor,
    payload,
};

void reportCrash(CrashingEntity who, const char* desc, uintptr_t address);
void handleSmc(Aarch64_Regs& saved_regs);

// Assembly functions
extern "C" void _boot();
extern "C" void enterEL1Payload(uintptr_t address);

}

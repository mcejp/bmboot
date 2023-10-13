//! @file
//! @brief  Monitor assembly functions
//! @author Martin Cejp

#pragma once

#include <stdint.h>

#define get_ELR() mfcp(ELR_EL3)

namespace bmboot::internal
{

extern "C" void _boot();
extern "C" void enterEL1Payload(uintptr_t address);

}

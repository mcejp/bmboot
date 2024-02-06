//! @file
//! @brief  ARMv8-A definitions
//! @author Martin Cejp

#pragma once

#include <stdint.h>

// Note that this syntax is a GCC extension
#define readSysReg(reg)         ({ uint64_t value; __asm__ __volatile__("mrs %0, " #reg : "=r" (value)); value; })
#define writeSysReg(reg, value)   __asm__ __volatile__("msr " #reg ",%0"  : : "r" (value))

namespace arm::armv8a
{
    static constexpr inline uint32_t DAIF_F_MASK =  (1<<6);
    static constexpr inline uint32_t DAIF_I_MASK =  (1<<7);
    static constexpr inline uint32_t DAIF_A_MASK =  (1<<8);
    static constexpr inline uint32_t DAIF_D_MASK =  (1<<9);
}

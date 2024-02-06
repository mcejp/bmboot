//! @file
//! @brief  ARM GICv2 definitions
//! @author Martin Cejp

#pragma once

#include <stdint.h>

namespace arm::gicv2
{

// ARM IHI 0048B.b, Table 4-2 CPU interface register map
struct GICC
{
    volatile uint32_t CTRL;     // CPU Interface Control Register
    volatile uint32_t PMR;      // Interrupt Priority Mask Register
    volatile uint32_t BPR;      // Binary Point Register
    volatile uint32_t IAR;      // Interrupt Acknowledge Register
    volatile uint32_t EOIR;     // End of Interrupt Register
    volatile uint32_t RPR;      // Running Priority Register
    volatile uint32_t HPPIR;    // Highest Priority Pending Interrupt Register

    // per Table 4-31 GICC_CTLR bit assignments, GICv2 without Security Extensions or Secure
    static constexpr inline uint32_t CTRL_EnableGrp0 =  (1<<0);
    static constexpr inline uint32_t CTRL_EnableGrp1 =  (1<<1);
    static constexpr inline uint32_t CTRL_AckCtl =      (1<<2);
    static constexpr inline uint32_t CTRL_FIQEn =       (1<<3);

    static constexpr inline uint32_t IAR_CPUID_MASK = 0x00000C00U;
    static constexpr inline uint32_t IAR_INTERRUPT_ID_MASK = 0x000003FFU;
};

}

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

// ARM IHI 0048B.b, Table 4-1 Distributor register map
// "In addition, the GICD_IPRIORITYRn, GICD_ITARGETSRn, GICD_CPENDSGIRn, and GICD_SPENDSGIRn
// registers support byte accesses."
struct GICD
{
    volatile uint32_t CTLR;                 // Distributor Control Register
    volatile uint32_t TYPER;                // Interrupt Controller Type Register
    volatile uint32_t IIDR;                 // Distributor Implementer Identification Register
    volatile uint32_t reserved_00c[5];
    volatile uint32_t impl_def_020[8];
    volatile uint32_t reserved_040[16];

    volatile uint32_t IGROUPRn[1];          // Interrupt Group Registers
    volatile uint32_t reserved_084[31];

    volatile uint32_t ISENABLERn[32];       // Interrupt Set-Enable Registers
    volatile uint32_t ICENABLERn[32];       // Interrupt Clear-Enable Registers

    volatile uint32_t ISPENDRn[32];         // Interrupt Set-Pending Registers
    volatile uint32_t ICPENDRn[32];         // Interrupt Clear-Pending Registers
    volatile uint32_t ISACTIVERn[32];       // GICv2 Interrupt Set-Active Registers
    volatile uint32_t ICACTIVERn[32];       // Interrupt Clear-Active Registers
    volatile uint8_t  IPRIORITYRn[1020];    // Interrupt Priority Registers
    volatile uint32_t reserved_7fc;

    volatile uint8_t  ITARGETSRn[1020];     // Interrupt Processor Targets Registers
    volatile uint32_t reserved_bfc;

    volatile uint32_t ICFGRn[64];           // Interrupt Configuration Registers

    inline void clearActive(int interrupt_id)
    {
        ICACTIVERn[interrupt_id / 32] = (1 << (interrupt_id % 32));
    }

    inline void clearEnable(int interrupt_id)
    {
        ICENABLERn[interrupt_id / 32] = (1 << (interrupt_id % 32));
    }

    inline void clearPending(int interrupt_id)
    {
        ICPENDRn[interrupt_id / 32] = (1 << (interrupt_id % 32));
    }

    inline void setEnable(int interrupt_id)
    {
        ISENABLERn[interrupt_id / 32] = (1 << (interrupt_id % 32));
    }

    inline void setGroup(int interrupt_id, int group)
    {
        auto mask = (1 << (interrupt_id % 32));
        IGROUPRn[interrupt_id / 32] = (IGROUPRn[interrupt_id / 32] & ~mask) | (group ? mask : 0);
    }

    inline void setTriggerEdge(int interrupt_id)
    {
        auto mask = (0b10 << ((interrupt_id % 16) * 2));
        ICFGRn[interrupt_id / 16] |= mask;
    }

    inline void setTriggerLevel(int interrupt_id)
    {
        auto mask = (0b10 << ((interrupt_id % 16) * 2));
        ICFGRn[interrupt_id / 16] &= mask;
    }
};

static_assert(sizeof(GICD) == 0xD00);

}

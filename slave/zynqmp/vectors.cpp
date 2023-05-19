#include "bmboot_slave.hpp"
#include "../src/mach/mach_baremetal_defs.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

#if EL3
#define get_ELR() mfcp(ELR_EL3)
#elif EL1_NONSECURE
#define get_ELR() mfcp(ELR_EL1)
#endif

extern "C" {

void FIQInterrupt(void)
{
    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("FIQInterrupt", fault_address);
    for (;;) {}
}

extern "C" void _boot();

void IRQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

    if ((iar & XSCUGIC_ACK_INTID_MASK) == bmboot::mach::IPI_CH0_GIC_CHANNEL) {
        // acknowledge IPI
        auto source_mask = XIpiPsu_ReadReg(XPAR_PSU_IPI_0_S_AXI_BASEADDR, XIPIPSU_ISR_OFFSET);
        XIpiPsu_WriteReg(XPAR_PSU_IPI_0_S_AXI_BASEADDR, XIPIPSU_ISR_OFFSET, source_mask);

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);

        // IRQ triggered by APU?
        if (source_mask & (1 << bmboot::mach::IPI_SRC_APU)) {
            // reset monitor -- jump to entry
            _boot();
        }
    }

    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("IRQInterrupt", fault_address);

    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

void SynchronousInterrupt(void)
{
    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("SynchronousInterrupt", fault_address);
    for (;;) {}
}

void SErrorInterrupt(void)
{
    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("SErrorInterrupt", fault_address);
    for (;;) {}
}

}

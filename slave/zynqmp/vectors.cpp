#include "bmboot_slave.hpp"
#include "../src/bmboot_internal.hpp"
#include "../src/mach/mach_baremetal_defs.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

#if EL3
#define get_ELR() mfcp(ELR_EL3)
#define get_SPSR() mfcp(SPSR_EL3)
#define EL_STRING "EL3"
#elif EL1_NONSECURE
#define get_ELR() mfcp(ELR_EL1)
#define get_SPSR() mfcp(SPSR_EL1)
#define EL_STRING "EL1"
#endif

static auto& ipc_block = *(bmboot_internal::IpcBlock*) bmboot_internal::MONITOR_IPC_START;

extern "C" void save_FPU_state(Aarch64_FpRegs&);

static void fill_crash_info(uintptr_t pc, uintptr_t sp, uint64_t* regs) {
    ipc_block.dom_regs.pc = pc;
    ipc_block.dom_regs.sp = sp;
    ipc_block.dom_regs.regs[29] = *regs++;
    ipc_block.dom_regs.regs[30] = *regs++;

    for (int i = 18; i >= 0; i -= 2) {
        ipc_block.dom_regs.regs[i] = *regs++;
        ipc_block.dom_regs.regs[i + 1] = *regs++;
    }
}

extern "C" {

void FIQInterrupt(void)
{
    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("FIQInterrupt " EL_STRING, fault_address);
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
    bmboot_s::notify_payload_crashed("IRQInterrupt " EL_STRING, fault_address);

    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

// rethink: currently this gets compiled separately for EL1 and EL3. but do we want that?
// shouldn't all EL1 errors trigger a switch into EL3 and be handled there?
//
// also because we can end up here EITHER due to an EL3 issue or an EL1 issue that trapped to EL3
void SynchronousInterrupt(uint64_t* the_sp)
{
    auto fault_address = get_ELR();

    save_FPU_state(ipc_block.dom_fpregs);

    ipc_block.dom_regs.pstate = get_SPSR();

    // TODO: specify the layout of saved registers
    // BUG: this does not save all GPRs!
    fill_crash_info(fault_address, (uintptr_t)(the_sp + 32), the_sp);

    bmboot_s::notify_payload_crashed("SynchronousInterrupt " EL_STRING, fault_address);
    for (;;) {}
}

void SErrorInterrupt(void)
{
    auto fault_address = get_ELR();
    bmboot_s::notify_payload_crashed("SErrorInterrupt " EL_STRING, fault_address);
    for (;;) {}
}

}

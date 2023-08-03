//! @file
//! @brief  Low-level executor functions (implemented in assembly).
//! @author Martin Cejp

.global enterEL1Payload

// Reference: ARM DAI 0527A Bare-metal Boot Code for ARMv8-A Processors

enterEL1Payload:
		// Initialize SCTLR_EL2 and HCR_EL2 to save values before entering EL2.
		MSR SCTLR_EL2, XZR
		MSR HCR_EL2, XZR

		// Determine the EL2 Execution state.
        // IRQ & FIQ must be routed to EL3 even when executing EL1 code (payload)
		mov x1, xzr
		orr x1, x1, #(1<<22) // EIS Exception Entry is Context Synchronizing.
		ORR X1, X1, #(1<<10) // RW EL2 Execution state is AArch64.
		orr x1, x1, #(1<<3) // EA Take External Abort and SError to EL3
		orr x1, x1, #(1<<2) // FIQ Take FIQs to EL3
		// IRQs are taken to EL1
		ORR X1, X1, #(1<<0) // NS EL1 is Non-secure world.
		MSR SCR_EL3, x1
		MOV X1, #0b01001 // DAIF=0000
		MSR SPSR_EL3, X1 // M[4:0]=01001 EL2h must match SCR_EL3.RW

		// Determine EL2 entry.
		ADR X1, el2_entry // el2_entry points to the first instruction of
		MSR ELR_EL3, X1 // EL2 code.
		ERET

el2_entry:
		// Initialize the SCTLR_EL1 register before entering EL1.
		MSR SCTLR_EL1, XZR

		// Determine the EL1 Execution state.
		mov x1, #0
		ORR X1, X1, #(1<<31) // RW=1 EL1 Execution state is AArch64.
        MSR HCR_EL2, X1

		MOV X1, #0b00101 // DAIF=0000
		MSR SPSR_EL2, X1 // M[4:0]=00101 EL1h must match HCR_EL2.RW.

		MSR ELR_EL2, X0 // EL1 code.
		ERET


.global saveFpuState

// Adapted from asm_vectors.S. Layout corresponds to Aarch64_FpRegs from cpu_state.hpp
saveFpuState:
	stp	q0,q1, [x0], #0x20
	stp	q2,q3, [x0], #0x20
	stp	q4,q5, [x0], #0x20
	stp	q6,q7, [x0], #0x20
	stp	q8,q9, [x0], #0x20
	stp	q10,q11, [x0], #0x20
	stp	q12,q13, [x0], #0x20
	stp	q14,q15, [x0], #0x20
	stp	q16,q17, [x0], #0x20
	stp	q18,q19, [x0], #0x20
	stp	q20,q21, [x0], #0x20
	stp	q22,q23, [x0], #0x20
	stp	q24,q25, [x0], #0x20
	stp	q26,q27, [x0], #0x20
	stp	q28,q29, [x0], #0x20
	stp	q30,q31, [x0], #0x20
	mrs	x2, FPSR
	mrs	x3, FPCR
	stp	x2, x3, [x0], #0x10

    ret


.global smc

// The SMC calling convention is similar enough to the normal procedure call standard (AAPCS) for <=7 arguments
// that we can literally just invoke the instruction and then return back to caller (assuming the SMC returns)
smc:
   smc #0
   ret

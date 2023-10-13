//! @file
//! @brief  Monitor assembly functions
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
		// Reset values as per https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/System-Control-Register--EL1:
		// 0b0011 0000 1101 0101 0000 1000 0011 1000, or 0x30C50838
		// Note that we just need to provide some sane defaults, since the EL1 startup code from BSP will reconfigure
		// the register again
		ldr x1, =0x30C50838
		msr SCTLR_EL1, x1

		// Determine the EL1 Execution state.
		mov x1, #0
		ORR X1, X1, #(1<<31) // RW=1 EL1 Execution state is AArch64.
        MSR HCR_EL2, X1

		MOV X1, #0b00101 // DAIF=0000
		MSR SPSR_EL2, X1 // M[4:0]=00101 EL1h must match HCR_EL2.RW.

		MSR ELR_EL2, X0 // EL1 code.
		ERET

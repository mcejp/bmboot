//! @file
//! @brief  Monitor assembly functions
//! @author Martin Cejp

.global enterEL1Payload

// Reference: ARM DAI 0527A Bare-metal Boot Code for ARMv8-A Processors

enterEL1Payload:
     // IRQ & FIQ must be routed to EL3 even when executing EL1 code (payload)
     mov x1, #0
     orr x1, x1, #(1<<10)  // RW EL2 Execution state is AArch64.
     orr x1, x1, #(1<<3)   // EA Take External Abort and SError to EL3
     orr x1, x1, #(1<<2)   // FIQ Take FIQs to EL3
                           // (IRQs are taken to EL1)
     orr x1, x1, #(1<<0)   // NS EL1 is Non-secure world.
     msr SCR_EL3, x1

     mov x1, #0
     orr x1, x1, #(1<<31)  // RW=1 EL1 Execution state is AArch64.
     msr HCR_EL2, x1

     // Initialize the SCTLR_EL1 register before entering EL1.
     // Reset values as per https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/System-Control-Register--EL1:
     // 0b0011 0000 1101 0101 0000 1000 0011 1000, or 0x30C50838
     // Note that we just need to provide some sane defaults, since the EL1 startup code from BSP will reconfigure
     // the register again
     ldr x1, =0x30C50838
     msr SCTLR_EL1, x1

     mov x1, #0b00101      // M[4:0]=00101 EL1h
     msr SPSR_EL3, x1

     msr ELR_EL3, x0       // Address of EL1 code
     eret

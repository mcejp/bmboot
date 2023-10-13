//! @file
//! @brief  Executor assembly functions
//! @author Martin Cejp

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

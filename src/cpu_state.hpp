//! @file
//! @brief  CPU state structures
//! @author Martin Cejp

#pragma once

namespace bmboot::internal
{

// this must correspond exactly to user_regs_struct from sys/user.h
// not sure if we can just include that file directly and be sure it will match between Linux & bare metal
struct Aarch64_Regs
{
    unsigned long long regs[31];        // note that there's only 31 general-purpose registers. x30 also serves as LR
    unsigned long long sp;
    unsigned long long pc;
    unsigned long long pstate;
};

// this must correspond exactly to user_fpsimd_struct from sys/user.h
struct Aarch64_FpRegs
{
    __uint128_t  vregs[32];
    unsigned int fpsr;
    unsigned int fpcr;
};

}

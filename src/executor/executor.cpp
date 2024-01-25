//! @file
//! @brief  Executor internal functions
//! @author Martin Cejp

#include "executor.hpp"
#include "executor_asm.hpp"

using namespace bmboot;
using namespace bmboot::internal;

int internal::getCpuIndex()
{
    // Read Aff0 field of MPIDR_EL1
    return (readSysReg(MPIDR_EL1) & 0xff);
}

IpcBlock& internal::getIpcBlock()
{
    switch (getCpuIndex())
    {
        case 1: return *(IpcBlock*) bmboot_cpu1_monitor_ipc_ADDRESS;
        case 2: return *(IpcBlock*) bmboot_cpu2_monitor_ipc_ADDRESS;
        case 3: return *(IpcBlock*) bmboot_cpu3_monitor_ipc_ADDRESS;
        default: abort();
    }
}

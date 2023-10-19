//! @file
//! @brief  Machine-specific declarations and functions
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>

namespace bmboot::mach
{

enum class IpiChannel
{
    ch0,
    ch1,
    ch2,
    // channels 3 thru 6 are hard-wired to the PMU and therefore of no interest to us
    ch7,
    ch8,
    ch9,
    ch10,
};

constexpr inline auto IPI_SRC_BMBOOT_MANAGER = IpiChannel::ch0;

inline IpiChannel getIpiChannelForCpu(int cpu_index)
{
    // Mapping of cpu_index to IPI channels is arbitrary, but must be consistent across all Bmboot components
    // (if desired, it could be communicated to monitor at start-up, but that is not currently done)
    switch (cpu_index)
    {
        case 1: return IpiChannel::ch0;
        case 2: return IpiChannel::ch7;
        case 3: return IpiChannel::ch8;
        default: abort();
    }
}

inline uintptr_t getIpiBaseAddress(IpiChannel ipi_channel)
{
    // Mapping of IPI channels to base addresses can be found in UG1085, Table 13-3: IPI Channel and Message Buffer Default Associations
    switch (ipi_channel)
    {
        case IpiChannel::ch0:   return 0xFF30'0000;
        case IpiChannel::ch1:   return 0xFF31'0000;
        case IpiChannel::ch2:   return 0xFF32'0000;
        case IpiChannel::ch7:   return 0xFF34'0000;
        case IpiChannel::ch8:   return 0xFF35'0000;
        case IpiChannel::ch9:   return 0xFF36'0000;
        case IpiChannel::ch10:  return 0xFF37'0000;
    }
}

inline uint32_t getIpiPeerMask(IpiChannel ipi_channel)
{
    // Bit assignments for receiving IPI channels can be found in UG1087, APU_TRIG (IPI) Register
    switch (ipi_channel)
    {
        case IpiChannel::ch0:   return 0x00000001;
        case IpiChannel::ch1:   return 0x00000100;
        case IpiChannel::ch2:   return 0x00000200;
        case IpiChannel::ch7:   return 0x01000000;
        case IpiChannel::ch8:   return 0x02000000;
        case IpiChannel::ch9:   return 0x04000000;
        case IpiChannel::ch10:  return 0x08000000;
    }
}

}

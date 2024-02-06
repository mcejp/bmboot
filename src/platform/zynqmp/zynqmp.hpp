//! @file
//! @brief  Machine-specific declarations and functions
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>

#include "../../executor/gicv2.hpp"

namespace zynqmp
{
    // XIpiPsu (XilinxProcessorIPLib/drivers/ipipsu/src/xipipsu_hw.h)
    namespace ipipsu
    {
        constexpr inline uintptr_t BUF_BASE_APU  =          0xFF990400;
        constexpr inline uintptr_t BUF_APU_TO_APU_REQ =     BUF_BASE_APU;
        constexpr inline uintptr_t BUF_APU_TO_APU_RESP =    (BUF_BASE_APU + 0x20);
        constexpr inline uintptr_t BUF_SIZE =               0x20;

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

        struct IPIPSU
        {
            volatile uint32_t TRIG;         // Trigger register
            volatile uint32_t OBS;          // Observation register
            volatile uint32_t reserved[2];  //
            volatile uint32_t ISR;          // ISR register
            volatile uint32_t IMR;          // Interrupt Mask Register
            volatile uint32_t IER;          // Interrupt Enable Register
            volatile uint32_t IDR;          // Interrupt Disable Register
        };

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

        inline auto getIpi(IpiChannel ipi_channel)
        {
            return (zynqmp::ipipsu::IPIPSU*) getIpiBaseAddress(ipi_channel);
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

    // XScuGic
    namespace scugic
    {
        // Not sure why this is defined in xparameters.h, seems very much hardwired in silicon...
        constexpr inline uintptr_t DIST_BASEADDR = 0xF9010000U;
        constexpr inline uintptr_t CPU_BASEADDR = 0xF9020000U;

        // UG1085, Table 13-4: APU Private Peripheral Interrupts
        constexpr inline int CNTPNS_INTERRUPT_ID = 30;

        inline auto GICD = (arm::gicv2::GICD*) DIST_BASEADDR;

        inline auto GICC = (arm::gicv2::GICC*) CPU_BASEADDR;
        inline bool THICC = true;

        // implemented in interrupt_controller.cpp
        int getInterruptIdForIpi(zynqmp::ipipsu::IpiChannel ipi_channel);
    }
}

namespace bmboot::internal
{

constexpr inline auto IPI_SRC_BMBOOT_MANAGER = zynqmp::ipipsu::IpiChannel::ch0;

inline zynqmp::ipipsu::IpiChannel getIpiChannelForCpu(int cpu_index)
{
    using zynqmp::ipipsu::IpiChannel;

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

}

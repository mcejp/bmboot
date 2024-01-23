//! @file
//! @brief  Manager configuration
//! @author Martin Cejp

#include "bmboot/manager_configuration.hpp"

#include <iostream>
#include <fstream>

namespace bmboot
{

bool loadConfigurationFromDefaultFile(ManagerConfiguration& config_out)
{
    // TODO: This is a temporary hot-fix. A structured configuration format should be used (example: https://github.com/jtilly/inih)

    std::fstream f("/etc/bmboot.conf", std::ios_base::in);

    if ((f >> config_out.cntfrq))
    {
        return true;
    }
    else
    {
        return false;
    }
}

}

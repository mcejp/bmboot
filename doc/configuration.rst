*************
Configuration
*************

There are some aspects of the execution environment that are difficult or impossible to determine automatically.
These include:

- the set of CPU cores available for bare metal execution
- the frequency of the CPU Generic Timer (CNTFRQ)

A configuration is required to provide some of this information.

The file name must be ``/etc/bmboot.conf`` and currently must contain a single numeric value, specifying the Generic
Timer frequency in Hz. This frequency is set in the Vivado project and can be found inside the exported XSA file.
(as ``XPAR_PSU_CORTEXA53_0_TIMESTAMP_CLK_FREQ``)

Observed values of this frequency are:

- ZCU102: 99990005
- DIOT: 50000000

The provided example ``payload_timer_demo`` can be used to approximately check the correctness of the setting.

In the future the format of this file will be changed to a key-value format.

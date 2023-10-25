*************************
Exceptions and interrupts
*************************

AArch64 has 4 types of exceptions:

- synchronous exception
- interrupt request (IRQ)
- fast interrupt request (FIQ)
- system error (SError)

Generally speaking, a synchronous exception is triggered when the program tries to perform either an invalid,
or somehow "special" operation.
In the context of Bmboot, the former case would be an error in the payload, such as invalid memory access,
and the latter would be an :term:`SMC`.

IRQ and FIQ are triggered by external sources -- mostly hardware peripherals.
One important exception is the inter-processor interrupt (IPI), which is sent from one CPU core to another.
In bmboot this is used to abort the running :term:`payload` and return control to the :term:`monitor`.
This would be normally done if the payload crashes or hangs, or if the application needs to load another payload.

Exception handling is implemented in :src_file:`src/platform/zynqmp/executor/monitor/vectors_el3.cpp`.

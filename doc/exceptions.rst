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
In the context of bmboot, the former case would be an error in the payload, such as invalid memory access,
and the latter would be an :term:`SMC`.

IRQ and FIQ are triggered by external sources -- mostly hardware peripherals.
One important exception is the inter-processor interrupt (IPI), which is sent from one CPU core to another.
In bmboot this is used to abort the running :term:`payload` and return control to the :term:`monitor`.
This would be normally done if the payload crashes or hangs, or if the application needs to load another payload.

Exception handling is implemented in :src_file:`src/executor_domain/zynqmp/vectors.cpp`.


.. needtable:: Exceptions normally encountered in bmboot and payload
   :filter: type == "exceptn"
   :columns: id;handler;source;symptom;title
   :style: table


.. raw:: html

   <hr>


.. exceptn:: Application error
   :id: EXC_11
   :handler: EL1 Syn
   :source: EL1

   An error inside the application. Requires a snapshot of all registers to produce a core dump.


.. exceptn:: Process timer
   :id: EXC_12
   :handler: EL1 IRQ
   :symptom: GIC.IAR == 31

   An IPI triggered by another CPU core


.. exceptn:: Monitor fault
   :id: EXC_31
   :handler: EL3 Syn
   :source: EL3

   An unexpected error inside the monitor


.. exceptn:: SMC
   :id: EXC_32
   :handler: EL3 Syn
   :source: EL1
   :symptom: ESR.EC == SMC

   Secure Monitor Call by EL1 payload to EL3. Need to extract call arguments from saved registers.


.. exceptn:: Unexpected exception from EL1
   :id: EXC_33
   :handler: EL3 Syn
   :source: EL1
   :symptom: ESR.EC != SMC

   TBC if this can happen. See :need:`Q_EL1EXC`.


.. exceptn:: Inter-processor interrupt
   :id: EXC_34
   :handler: EL3 FIQ
   :symptom: GIC.IAR == 67

   An IPI triggered by another CPU core. No state needs to be preserved, since we do not return to the payload.

**********************************
Key questions and design decisions
**********************************

.. needtable:: Unanswered
   :filter: type == "question" and (not status or status.upper() != "RESOLVED")
   :columns: id;title
   :style: table


.. raw:: html

   <hr>


.. question:: Which features do we need?
   :status: RESOLVED

   - Stage 2 translation / emulation of physical memory -> no
   - Trapping of exceptions -> would be nice
   - Trapping of system register access -> no
   - Some degree of memory protection -> would be nice
   - Mechanism to interrupt execution externally -> yes

      - virtual interrupts?
      - Inter-Processor Interrupts?


.. question:: Can an existing project fulfill the needs?
   :status: RESOLVED

   We have looked at KVM, Xen, Jailhouse. None of them is a good match.

   There are some other interesting projects but more as motivation than production-ready tools:

   - https://github.com/ktemkin-archive/bfstub
   - https://github.com/m8/armvisor


.. question:: ABI compatibility
   :status: OPEN
   :links: Q_PLD_MON_ABI

   Question: Since FGCD and v_loop will be deployed independently, how to ensure ABI compatibility between bmboot
   manager and payload library?


.. question:: Can EL0 have its own VBAR?
   :status: RESOLVED

   Answer: Per ARM ARM, section *Taking synchronous exceptions from EL0*, it would seem that synchronous exceptions
   from EL0 are always taken to EL1.


.. question:: Can EL1 exceptions be taken to the monitor's exception handler?
   :status: RESOLVED

   Answer: Not in the same way as EL0 exceptions go to EL1. It seems the only way would be for the monitor to set the
   EL1 VBAR (from EL3) and disallow EL1 from changing it.


.. question:: Can we trap EL0 WFI & WFE instructions?
   :status: RESOLVED

   Answer: Yes; see `SCTLR_EL1` bits `nTWI`, `nTWE`


.. question:: Can we trap into monitor from another core?
   :status: RESOLVED

   Can we do it without a kernel driver? Maybe a built-in one exists that exposes /proc or /sys?
   And what is ``/sys/firmware/devicetree/base/zynqmp_ipi/interrupts``?

   Answer: We can trigger an IPI by writting to IPI Channel 0 TRIG register (0xFF30_0000).
   See UG1085, Table 13-3: IPI Channel and Message Buffer Default Associations.


.. question:: Do we want to share the stack pointer? (PSTATE.SP)
   :status: RESOLVED

   User SP might be corrupted or point into EL3 private memory.

   Answer: No, EL3 should have its own protected stack.


.. question:: How does PSCI affect us?


.. qquestion:: How to have certain IRQs handled in EL3 and others in EL1?


.. question:: How to ensure that our IRQ will be taken even if EL1 crashes to SyncErr?
   :status: RESOLVED

   Answer: if EL3.IRQ is set, IRQs cannot be masked by EL1


.. question:: How to manage memory map so that there is a single source of truth?


.. question:: How to trace down original executable given a core dump?
   :id: Q_CORE_ID


.. question:: How to validate that payload being loaded targets the correct CPU core, memory address, ABI version...


.. question:: Is there value in having a separate ``starting_payload`` state?

   It can help in recognizing that an invalid file was loaded as a payload.


.. question:: Payload executable format
   :id: Q_PLD_FMT
   :status: RESOLVED

   Question: What executable format should be used for the payload?

   ELF would have some benefits:

   - opens the door to better memory protection (read-only code)
   - can inform core dumping process to only save relevant parts
   - opens the possibility of relocatable payloads
   - can embed metadata

   Decision: for now we stick to **flat binary** to minimize complexity, but the question should be revisited later


.. question:: Should all payload->monitor calls be via the SMC instruction?
   :id: Q_PLD_MON_ABI
   :status: RESOLVED

   It would be cleaner design, but for now we will not bother and we will access IPC memory directly.


.. question:: Virtual address of shared memory mapped into Linux process
   :status: OPEN

   Question: CClibs needs to be mapped at a specific virtual address equal to its physical address. How to ensure that
   the required virtual memory range will be free in the FGCD process?

   It should be possible by patching the ELF file.


.. question:: Will separate monitor binaries be required for different domains?
   :status: RESOLVED

   Answer: **yes**, at least as long as we use the Xilinx SDKs. There are multiple places where the CPU index must be
   hard-coded:

   - BIF (Boot Image Format) points to different cores in one line.
   - in xparameters.h from bspinclude sets different ``XPAR_CPU_ID``, ``XPAR_CPU_CORTEXA53_{0, 1}_CPU_CLK_FREQ_HZ``,
     and ``XPAR_CPU_CORTEXA53_{0, 1}_TIMESTAMP_CLK_FREQ``.
   - in system.mss, ``PARAMETER PROC_INSTANCE`` and ``PARAMETER HW_INSTANCE`` are set to different core IDs:
     ``psu_cortexa53_{0, 1}``


.. question:: Will separate payload builds be required for different domains?
   :status: RESOLVED
   :links: Q_PLD_FMT

   Answer: **yes**, unless decision on use of flat binaries for payloads is reversed.


.. question:: What are the trade-offs of different exception levels and which should we use?
   :status: RESOLVED

   Advantages of payload @ EL0:

   - synchronous exceptions go directly to monitor at higher EL

   Disadvantages of payload @ EL0:

   - locked out of many CPU features

      - can't make SMC calls (=> monitor needs to have at least some code at EL1)
      - can't set up its own page table

   - not supported by Xilinx SDK

   Disadvantage of monitor @ EL2:

   - cannot trap general EL1 exceptions?
   - not supported by Xilinx SDK

   Disadvantage of monitor @ EL3:

   - it is not exactly what EL3 was meant for

   Decision: **use EL1 and EL3**, because these are supported by the SDK.
   EL0 and EL1 might make more sense in theory, but would be more work to implement.

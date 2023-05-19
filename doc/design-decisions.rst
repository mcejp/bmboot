Design decisions
================

Which features do we need?
--------------------------

- Stage 2 translation / emulation of physical memory -> no
- Trapping of exceptions -> would be nice
- Trapping of system register access -> no
- Mechanism to interrupt execution externally -> **yes**
	- virtual interrupts?
	- Inter-Processor Interrupts?


EL3+1 vs EL2+1 vs EL2+0 vs EL1+0?
---------------------------------

Disadvantage of payload @ EL0:
- locked out of many CPU features (can't make SMC calls, for example)
- don't have out-of-the box startup code for this setup

Disadvantage of monitor @ EL2:
- cannot trap general EL1 exceptions?
- Xilinx SDK in principle supports only EL3 & EL1


Do we want to share the stack pointer? (PSTATE.SP)
--------------------------------------------------

Probably not -> user SP might be corrupted or point into EL3 private parts


Do we want to use SMC for all payload->monitor communication?
-------------------------------------------------------------

It would be cleaner design, but for now we will not bother.

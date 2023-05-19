Miscellaneous questions and answers
===================================

Q: why is this needed in the first place?
A: we need to load the cclibs code *somehow*. we also want to be able to reload it at runtime,
   and to recover from a hang or crash

Q: why not use an off-the-shelf hypervisor?
A: complexity

Q: where is it described how the ELs work _in general_?
A: ARM ARM primarily
- "Armv8-A virtualization" Doc ID 102142
- https://krinkinmu.github.io/2021/01/04/aarch64-exception-levels.html
- https://krinkinmu.github.io/2021/01/10/aarch64-interrupt-handling.html useful?

Q: can EL0 have its own VBAR? (not that we want it to)
Q: how does linux do this (see aarch64 entry.S)
A: yes, from reading linux code it even seems mandatory (?)
ARM ARM: "Taking synchronous exceptions from EL0" -> makes it seem that synchronous exceptions from EL0 are always taken to EL1 or EL2

Q: can we trap EL0 WFI, WFE?
A: yes, see SCTLR_EL1.nTWI, .nTWE

Q: can we trap into monitor from another core? + can we do it without a kernel driver?
   (maybe a built-in one exists that exposes /proc or /sys)
Q: /sys/firmware/devicetree/base/zynqmp_ipi/interrupts ?

Q: how to make sure that our IRQ will be taken even if EL1 crashes to SyncErr?
A: if EL3.IRQ is set, IRQs cannot be masked by EL1

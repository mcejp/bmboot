*********************
External requirements
*********************

Given that the Linux kernel is not aware of bmboot's resource usage, it is necessary to adjust the device tree to
reserve the needed resources:

- memory range used (see also :doc:`memory-map`)
- CPU cores dedicated to bare-metal code

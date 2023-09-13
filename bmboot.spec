[generate]
path = src/bmboot_memmap.hpp

regions =
  bmboot.*

[substitute1]
in = src/executor_domain/zynqmp/cpu1/monitor.ld.in
out = src/executor_domain/zynqmp/cpu1/monitor.ld

[substitute2]
in = src/executor_domain/zynqmp/cpu1/payload.ld.in
out = src/executor_domain/zynqmp/cpu1/payload.ld

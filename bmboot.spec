[generate]
path = src/bmboot_memmap.hpp

regions =
  bmboot.*

[substitute:monitor-cpu1]
in = src/executor/monitor/monitor.ld.in
out = src/executor/monitor/monitor_cpu1.ld
aliases =
  bmboot.cpuN_monitor bmboot.cpu1_monitor

[substitute:monitor-cpu2]
in = src/executor/monitor/monitor.ld.in
out = src/executor/monitor/monitor_cpu2.ld
aliases =
  bmboot.cpuN_monitor bmboot.cpu2_monitor

[substitute:monitor-cpu3]
in = src/executor/monitor/monitor.ld.in
out = src/executor/monitor/monitor_cpu3.ld
aliases =
  bmboot.cpuN_monitor bmboot.cpu3_monitor

[substitute:payload-cpu1]
in = src/executor/payload/payload.ld.in
out = src/executor/payload/payload_cpu1.ld
aliases =
  bmboot.cpuN_payload bmboot.cpu1_payload

[substitute:payload-cpu2]
in = src/executor/payload/payload.ld.in
out = src/executor/payload/payload_cpu2.ld
aliases =
  bmboot.cpuN_payload bmboot.cpu2_payload

[substitute:payload-cpu3]
in = src/executor/payload/payload.ld.in
out = src/executor/payload/payload_cpu3.ld
aliases =
  bmboot.cpuN_payload bmboot.cpu3_payload

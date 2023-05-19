# 🐝 bumbleboot: a minimalist loader & monitor for bare-metal aarch64 code

### Overall design

- one master + a number of slaves (_domains_ as per Xen terminology)
- master must be a Linux userspace process
  - bmboot library statically linked
- slave must be zynqmp CPU1..n
- payload must be cooperative (`notify_payload_started`)
- cache-coherent memory is assumed
- monitor at EL3, payload at EL1
- code is C++20 but with appropriate constraints for the low-level portions

### Terminology

- master
- slave
- domain
- monitor
- payload

### Memory map

```
0x7800'0000 .. 0x7800'ffff  64k   Slave1 monitor code + data + stack
0x7801'0000 .. 0x7801'ffff  64k   Slave1 IPC region
0x7802'0000 .. ?                  Slave1 user code + data + stack + heap ...
```

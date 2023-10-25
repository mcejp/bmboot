# 🐝 bumbleboot: a minimalist loader & monitor for bare-metal aarch64 code

### Overall design

- one manager + a number of executors (_domains_ as per Xen terminology)
- manager must be a Linux userspace process
  - bmboot library statically linked
- exectutor must be zynqmp CPU1..n
- payload must be cooperative (`notifyPayloadStarted`)
- cache-coherent memory is assumed
- monitor at EL3, payload at EL1
- code is C++20 but with appropriate constraints for the low-level portions

### Building docs

```
doxygen Doxyfile; and python3 -m sphinx_autobuild doc doc/_build
```
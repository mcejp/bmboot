**************************************************
Error recovery and inter-processor interupts (IPI)
**************************************************

An :term:`IPI` may be sent to abort the running :term:`payload` (if any) and return control to the :term:`monitor`.
This would be normally done if the payload crashes or hangs, or if the application needs to load another payload.

Implementation: :src_file:`src/executor_domain/zynqmp/vectors.cpp`, inside function ``IRQInterrupt``.

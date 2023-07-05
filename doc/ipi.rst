**************************************************
Error recovery and inter-processor interupts (IPI)
**************************************************

An IPI may be sent to abort the running payload (if any) and return control to the monitor.
This would be done normally done if the payload crashes or hangs, or if the application needs to load another payload.

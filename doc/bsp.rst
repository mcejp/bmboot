On the one hand, the BSP lets us _not_ rewrite the world from scratch.

It provides convenient things like:
- startup code
- linker script
- peripheral drivers (relevant for us: IpiPsu, ScuGic)
- assembly intrinsics

On the other hand, it is a thorn in the side and we would like to remove this dependency.

Known usage:

interrupt_controller.cpp ->
   XScuGic
   mtcpsr

vectors_el1.cpp ->
   XScuGic

vectors_el3 ->
   XScuGic

implicit ->
   boot.S
   page table
   usleep
   ?
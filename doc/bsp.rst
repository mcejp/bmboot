The BSP is a thorn in the side and we would like to remove this dependency.

Known usage:

interrupt_controller.cpp ->
   XScuGic
   mtcpsr

vectors_el1.cpp ->
   XScuGic

vectors_el3 ->
   XScuGic
   XIpiPsu

implicit ->
   boot.S
   page table
   usleep
   ?
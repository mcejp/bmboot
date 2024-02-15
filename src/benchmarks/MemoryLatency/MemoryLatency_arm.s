.text

.global latencytest
.global preplatencyarr
.global stlftest
.global stlftest32
.global matchedstlftest

/* x0 = Start of the array
   x1 = Array length
   preplatencyarr: convert index values in the array to pointers */
preplatencyarr:
  sub sp, sp, #0x20                       /* allocate 32 bytes on stack for context saving*/
  stp x14, x15, [sp, #0x10]               /* save x14 and x15 */
  mov x15, 0                              /* loop counter to 0*/
preplatencyarr_loop:
  /* in this context 
    - x14 serves the purpose of a temporary register, used to store the value of the array element 
    - x15 serves the purpose of a loop counter */
  ldr x14, [x0, w15, uxtw #3]             /* load into x14 the value pointed to by x0 + w15 * 8 */
  /* each element in the array is a 64-bit integer, so we multiply the index by 8 to get the offset */
  lsl x14, x14, 3                         /* multiply x14 by 8 by shifting*/
  add x14, x14, x0                        /* x14 = x14 + x0 */
  str x14, [x0, w15, uxtw #3]             /* store the value of x14 into the array at x0 + w15 * 8 */
  add w15, w15, 1                         /* increment the loop counter */
  cmp x15, x1                             /* compare x15 with x1 (counter vs array length) */
  b.ne preplatencyarr_loop                /* if x15 is not equal to x1, jump to preplatencyarr_loop */
  ldp x14, x15, [sp, #0x10]               /* restore x14 and x15 */
  add sp, sp, #0x20                       /* deallocate 32 bytes from the stack */
  ret                                     /* return */

/* x0 = Iteration count, returns the result of the pointer chasing 
   x1 = Pointer to the first element of the array
   do pointer chasing for specified iteration count. 
   Pointer values are accumulated in x14 to ensure no optimization occurs. 
   The value is then returned, and checked for being != 0 by the caller */
latencytest:
  sub sp, sp, #0x20                       /* allocate 32 bytes on stack for context saving*/
  stp x14, x15, [sp, #0x10]               /* save x14 and x15 */
  mov x14, 0                              /* x14 = 0 */
  ldr x15, [x1]                           /* set x15 to the value pointed to by x1 */
latencytest_loop:
  ldr x15, [x15]                          /* set x15 to the value pointed to by x15 */
  add x14, x14, x15                       /* add x15 to x14 and store the result in x14 */
  sub x0, x0, 1                           /* decrement the loop counter by 1 */
  cbnz x0, latencytest_loop               /* if x0 is not 0, jump to latencytest_loop */
  mov x0, x14                             /* set x0 to x14 */
  ldp x14, x15, [sp, #0x10]               /* restore x14 and x15 */
  add sp, sp, #0x20                       /* deallocate 32 bytes from the stack */
  ret                                     /* return */

/* x0 = iteration count
   x1 = ptr to arr. first 32-bit int = store offset, second = load offset */
stlftest:
  sub sp, sp, #0x40                       /* allocate 64 bytes on stack for context saving*/
  stp x14, x15, [sp, #0x10]               /* save x14 and x15 */
  stp x12, x13, [sp, #0x20]               /* x12 = store ptr, x13 = load ptr */
  ldr x15, [x1]                           /* load the value pointed by x1 into x15 -- PURPOSE? */
  /* subsequent instructions are targenting the 32-bit lower halves of x## registers which can be referred to as w## */
  ldr w12, [x1]                           /* load the store offset: the first 32-bit of the value pointed by x1 into w12 */
  ldr w13, [x1, 4]                        /* load the load offset: the second 32-bit of the value pointed by x1 (that is x1+4) into w13 */
  add x12, x12, x1                        /* x12 = x12 + x1 */
  add x13, x13, x1                        /* x13 = x13 + x1 */
stlftest_loop:
  str x15, [x12]                          /* store the value of x15 into the address pointed to by x12 */
  ldr w15, [x13]                          /* load the value pointed to by x13 into w15 */
  str x15, [x12]                          /* store the value of x15 into the address pointed to by x12 */
  ldr w15, [x13]                          /* load the value pointed to by x13 into w15 */
  str x15, [x12]                          /* store the value of x15 into the address pointed to by x12 */
  ldr w15, [x13]                          /* load the value pointed to by x13 into w15 */
  str x15, [x12]                          /* store the value of x15 into the address pointed to by x12 */
  ldr w15, [x13]                          /* load the value pointed to by x13 into w15 */
  str x15, [x12]                          /* store the value of x15 into the address pointed to by x12 */
  ldr w15, [x13]                          /* load the value pointed to by x13 into w15 */
  sub x0, x0, 5                           /* decrement the loop counter by 5 */
  cmp x0, 0                               /* compare x0 with 0 */
  b.gt stlftest_loop                      /* if x0 is greater than 0, jump to stlftest_loop */
  ldp x12, x13, [sp, #0x10]               /* restore x12 and x13 */
  ldp x14, x15, [sp, #0x10]               /* restore x14 and x15 */
  add sp, sp, #0x40                       /* deallocate 64 bytes from the stack */
  ret                                     /* return */

  
stlftest32:
  sub sp, sp, #0x40                       /* Allocate 64 bytes on stack for context saving */
  stp x14, x15, [sp, #0x10]               /* Save x14 and x15 at the beginning of allocated space */
  stp x12, x13, [sp, #0x20]               /* Save x12 and x13, 32 bytes into the allocated space */
  ldr x15, [x1]                           /* Load a 64-bit value from the address pointed by x1 into x15 (potentially unused or setup value) */
  ldr w12, [x1]                           /* Load the first 32-bit value from the address x1 points to into the lower 32 bits of x12 */
  ldr w13, [x1, 4]                        /* Load the next 32-bit value from address x1+4 into the lower 32 bits of x13 */
  add x12, x12, x1                        /* Calculate the store address by adding x1 to the offset in x12 */
  add x13, x13, x1                        /* Calculate the load address by adding x1 to the offset in x13 */
stlftest32_loop:
  str w15, [x12]                          /* Store the lower 32 bits of x15 to the address pointed by x12 */
  ldrh w15, [x13]                         /* Load a 16-bit value (halfword) from the address pointed by x13 into the lower bits of x15 */
  str w15, [x12]                          /* Repeat the store of w15 to [x12], demonstrating store-to-load forwarding with 32-bit values */
  ldrh w15, [x13]                         /* Repeat loading a 16-bit value into w15 */
  str w15, [x12]                          /* Repeat store */
  ldrh w15, [x13]                         /* Repeat load */
  str w15, [x12]                          /* Repeat store */
  ldrh w15, [x13]                         /* Repeat load */
  str w15, [x12]                          /* Repeat store */
  ldrh w15, [x13]                         /* Final load in the loop */
  sub x0, x0, 5                           /* Decrement loop counter by 5, accounting for 5 pairs of store/load operations */
  cmp x0, 0                               /* Compare loop counter with 0 to determine if loop should continue */
  b.gt stlftest32_loop                    /* If counter is greater than 0, continue looping */
  ldp x12, x13, [sp, #0x20]               /* Restore x12 and x13 from stack, correct offset for restoration */
  ldp x14, x15, [sp, #0x10]               /* Restore x14 and x15 from stack */
  add sp, sp, #0x40                       /* Deallocate 64 bytes from the stack */
  ret                                     /* Return from function */


matchedstlftest:
  sub sp, sp, #0x40                       /* Allocate 64 bytes on stack for context saving */
  stp x14, x15, [sp, #0x10]               /* Save x14 and x15 at the beginning of allocated space */
  stp x12, x13, [sp, #0x20]               /* Save x12 and x13, 32 bytes into the allocated space */
  ldr x15, [x1]                           /* Load a 64-bit value from the address pointed by x1 into x15 (potentially unused or setup value) */
  ldr w12, [x1]                           /* Load the first 32-bit value from the address x1 points to into the lower 32 bits of x12 */
  ldr w13, [x1, 4]                        /* Load the next 32-bit value from address x1+4 into the lower 32 bits of x13 */
  add x12, x12, x1                        /* Calculate the store address by adding x1 to the offset in x12 */
  add x13, x13, x1                        /* Calculate the load address by adding x1 to the offset in x13 */
matchedstlftest_loop:
  str x15, [x12]                          /* Store the 64-bit value of x15 into the address pointed to by x12 */
  ldr x15, [x13]                          /* Load a 64-bit value from the address pointed by x13 into x15 */
  str x15, [x12]                          /* Repeat the store of x15 to [x12] */
  ldr x15, [x13]                          /* Repeat loading a 64-bit value into x15 */
  str x15, [x12]                          /* Repeat store */
  ldr x15, [x13]                          /* Repeat load */
  str x15, [x12]                          /* Repeat store */
  ldr x15, [x13]                          /* Repeat load */
  str x15, [x12]                          /* Repeat store */
  ldr x15, [x13]                          /* Final load in the loop */
  sub x0, x0, 5                           /* Decrement loop counter by 5, accounting for 5 pairs of store/load operations */
  cmp x0, 0                               /* Compare loop counter with 0 to determine if loop should continue */
  b.gt matchedstlftest_loop               /* If counter is greater than 0, continue looping */
  ldp x12, x13, [sp, #0x20]               /* Restore x12 and x13 from stack, using correct offset for restoration */
  ldp x14, x15, [sp, #0x10]               /* Restore x14 and x15 from stack */
  add sp, sp, #0x40                       /* Deallocate 64 bytes from the stack */
  ret                                     /* Return from function */

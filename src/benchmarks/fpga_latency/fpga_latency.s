.text

.global ld32_fixed_address
.global ld32_dsb_fixed_address
.global st32_fixed_address
.global st32_dsb_fixed_address

/*
 * x0 = addr
 * x1 = iteration count
 */
ld32_fixed_address:
  ldr wzr, [x0]
  sub x1, x1, 1
  cbnz x1, ld32_fixed_address
  ret

/*
 * x0 = addr
 * x1 = iteration count
 */
ld32_dsb_fixed_address:
  ldr wzr, [x0]
  dsb oshld
  sub x1, x1, 1
  cbnz x1, ld32_dsb_fixed_address
  ret

/*
 * x0 = addr
 * x1 = iteration count
 */
st32_fixed_address:
  str wzr, [x0]
  sub x1, x1, 1
  cbnz x1, st32_fixed_address
  ret

/*
 * x0 = addr
 * x1 = iteration count
 */
st32_dsb_fixed_address:
  str wzr, [x0]
  dsb oshst
  sub x1, x1, 1
  cbnz x1, st32_dsb_fixed_address
  ret

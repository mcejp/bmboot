#pragma once

/*
 Whenever there is a backwards-incompatible change made to the monitor, increment ABI_MAJOR and reset ABI_MINOR to 0.
 Whenever the ABI changes in a backward-compatible way (new SMC calls), increment ABI_MINOR
*/
#define ABI_MAGIC_NUMBER    0x6f626d42
#define ABI_MAJOR           0x01
#define ABI_MINOR           0x01

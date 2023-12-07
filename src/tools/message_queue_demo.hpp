//! @file
//! @brief  Message queue demo
//! @author Martin Cejp

#include <stddef.h>
#include <stdint.h>

struct MyHeader
{
    int foo;
    float bar;
};

const inline uintptr_t physical_address_of_queue = 0x806100000;
const inline size_t map_size = 64*1024;
const inline size_t queue_size = 1024;

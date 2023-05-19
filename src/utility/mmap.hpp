#pragma once

#include <cstdint>
#include <sys/mman.h>

namespace bmboot
{
    class Mmap
    {
    public:
        Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
            this->base = mmap(addr, len, prot, flags, fd, offset);
            this->len = len;

            if (this->base == MAP_FAILED) {
                this->base = nullptr;
                this->len = 0;
            }
        }

        ~Mmap() {
            this->unmap();
        }

        void unmap() {
            if (this->base) {
                ::munmap(this->base, len);
                this->base = nullptr;
            }
        }

        uint32_t read32(size_t offset) {
            return *(uint32_t volatile*)((uint8_t*)base + offset);
        }

        void write32(size_t offset, uint32_t value) {
            *(uint32_t volatile*)((uint8_t*)base + offset) = value;
        }

        explicit operator bool() {
            return base != nullptr;
        }

        void* operator+(size_t offset) {
            return (uint8_t*)base + offset;
        }

    private:
        void* base;
        size_t len;
    };
}

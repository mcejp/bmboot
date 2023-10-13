//! @file
//! @brief  Retargeting functions for standard I/O
//! @author Martin Cejp

#pragma once

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

int _isatty(int fd);
int _write(int fd, char* ptr, int len);
int _close(int fd);
int _lseek(int fd, int ptr, int dir);
int _read(int fd, char* ptr, int len);
int _fstat(int fd, struct stat* st);

#ifdef __cplusplus
}
#endif

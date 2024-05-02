#pragma once

#include_next <fcntl.h>
#include <sys/stat.h>

#define open FreeBSDPort_open
int FreeBSDPort_open(const char *path, int flags, ...);

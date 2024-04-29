#pragma once

#include_next <sys/mman.h>

/*
 * We don't have hugetlb support. But it doesn't matter: it's all automatic. So
 * just stub it out: it will work.
 */
#define MAP_HUGETLB 0
#define MADV_HUGEPAGE MADV_NORMAL

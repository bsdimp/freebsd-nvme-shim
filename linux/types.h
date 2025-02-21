#pragma once

#include <asm/types.h>

typedef __s8	s8;
typedef __s16	s16;
typedef __s32	s32;
typedef __s64	s64;

typedef __u8	u8;
typedef __u16	u16;
typedef __u32	u32;
typedef __u64	u64;

#undef bool
typedef _Bool bool;
#define false   0
#define true    1

/* compiler_types.h */
#define __kernel
#define __user

/* compiler_attributes.h */
#define __must_check __attribute__((__warn_unused_result__))

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

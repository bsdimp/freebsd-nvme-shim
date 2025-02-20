#pragma once

#include_next <sys/elf64.h>

/* FreeBSD's versions of these lack proper casts */
#undef ELF64_R_SYM
#undef ELF64_R_TYPE
#undef ELF64_R_INFO

#define	ELF64_R_SYM(info)	((Elf64_Xword)(info) >> 32)
#define	ELF64_R_TYPE(info)	((Elf64_Xword)(info) & 0xffffffffL)

/* Macro for constructing r_info from field values. */
#define	ELF64_R_INFO(sym, type)	\
	((Elf64_Xword)(sym) << 32) + ((Elf64_Xword)(type) & 0xffffffffL)

#pragma once

#include_next <sys/elf_common.h>

/* Extras for Linux, or variant spellings */
#define R_X86_64_JUMP_SLOT R_X86_64_JMP_SLOT

/* Not yet in FreeBSD's elf_common.h */
#define R_ARM_CALL		28
#define R_ARM_JUMP24		29
#define R_ARM_THM_JUMP24	30
#define R_ARM_MOVW_ABS_NC	43
#define R_ARM_MOVT_ABS		44
#define R_ARM_THM_MOVW_ABS_NC	47
#define R_ARM_THM_MOVT_ABS	48
#define R_ARM_THM_JUMP19	51

#pragma once

#include_next <elf.h>

/*
 * FreeBSD defines these, but Linux does not... And expects to define them, so
 * undef them here.
 */
#undef ElfW
#undef ELF_CLASS
#undef ELF_R_SYM
#undef ELF_R_TYPE
#undef ELF_ST_TYPE
#undef ELF_ST_BIND
#undef ELF_ST_VISIBILITY

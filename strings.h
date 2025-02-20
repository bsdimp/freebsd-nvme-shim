#pragma once

/* work around fls issues */

#undef fls

#include_next <strings.h>

#define fls(x) generic_fls(x)

#pragma once

#include_next <errno.h>

/* Extra defines for Linux errnos */

#define ENOKEY		(ELAST + 1)
#define ERESTART	(ELAST + 2)
#define EREMOTEIO	(ELAST + 3)
#define EBADSLT		(ELAST + 4)

#pragma once

/*
 * This exists for the scaleflux capacity change. This requires allocation of
 * kernel mmeory for buffers in ways I've not yet puzzled out. So, we lie and
 * say we have no free memory, which hits the sanity warning.
 */
struct sysinfo 
{
	long freeram;
};

static inline int sysinfo(struct sysinfo *s) { s->freeram = 0; return 0; }


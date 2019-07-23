



































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "seccomon.h"

static size_t rng_systemFromNoise(unsigned char *dest, size_t maxLen);

#if defined(XP_UNIX) || defined(XP_BEOS)
#include "unix_rand.c"
#endif
#ifdef XP_WIN
#include "win_rand.c"
#endif
#ifdef XP_OS2
#include "os2_rand.c"
#endif





static size_t 
rng_systemFromNoise(unsigned char *dest, size_t maxLen) 
{
   size_t retBytes = maxLen;

   while (maxLen) {
	size_t nbytes = RNG_GetNoise(dest, maxLen);

	PORT_Assert(nbytes != 0);

	dest += nbytes;
	maxLen -= nbytes;

	

	rng_systemJitter();
   }
   return retBytes;
}


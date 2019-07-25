#ifndef strings_h
#define strings_h



#include <intrin.h>
#pragma intrinsic(_BitScanForward)
static __forceinline int ffsl(long x)
{
	unsigned long i;

	if (_BitScanForward(&i, x))
		return (i + 1);
	return (0);
}

static __forceinline int ffs(int x)
{

	return (ffsl(x));
}

#endif

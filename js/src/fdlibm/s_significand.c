
























































#include "fdlibm.h"

#ifdef __STDC__
	double fd_significand(double x)
#else
	double fd_significand(x)
	double x;
#endif
{
	return __ieee754_scalb(x,(double) -fd_ilogb(x));
}

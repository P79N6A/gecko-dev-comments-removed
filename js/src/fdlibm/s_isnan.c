























































#include "fdlibm.h"

#ifdef __STDC__
	int fd_isnan(double x)
#else
	int fd_isnan(x)
	double x;
#endif
{
        fd_twoints u;
	int hx,lx;
        u.d = x;
	hx = (__HI(u)&0x7fffffff);
	lx = __LO(u);
	hx |= (unsigned)(lx|(-lx))>>31;	
	hx = 0x7ff00000 - hx;
	return ((unsigned)(hx))>>31;
}

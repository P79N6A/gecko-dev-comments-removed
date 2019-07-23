























































#include "fdlibm.h"

#ifdef __STDC__
	int fd_finite(double x)
#else
	int fd_finite(x)
	double x;
#endif
{
        fd_twoints u;
	int hx; 
        u.d = x;
	hx = __HI(u);
	return  (unsigned)((hx&0x7fffffff)-0x7ff00000)>>31;
}

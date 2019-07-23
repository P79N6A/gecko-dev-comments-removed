






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_fabs(double x)
#else
	double fd_fabs(x)
	double x;
#endif
{
        fd_twoints u;
        u.d = x;
	__HI(u) &= 0x7fffffff;
        x = u.d;
        return x;
}

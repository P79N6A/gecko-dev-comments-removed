
























































#include "fdlibm.h"

#ifdef __STDC__
	double fd_copysign(double x, double y)
#else
	double fd_copysign(x,y)
	double x,y;
#endif
{
        fd_twoints ux, uy;
        ux.d = x; uy.d = y;
	__HI(ux) = (__HI(ux)&0x7fffffff)|(__HI(uy)&0x80000000);
        x = ux.d;
        return x;
}






























































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
two54 =  1.80143985094819840000e+16; 

#ifdef __STDC__
	double fd_frexp(double x, int *eptr)
#else
	double fd_frexp(x, eptr)
	double x; int *eptr;
#endif
{
	int  hx, ix, lx;
        fd_twoints u;
        u.d = x;
	hx = __HI(u);
	ix = 0x7fffffff&hx;
	lx = __LO(u);
	*eptr = 0;
	if(ix>=0x7ff00000||((ix|lx)==0)) return x;	
	if (ix<0x00100000) {		
	    x *= two54;
            u.d = x;
	    hx = __HI(u);
	    ix = hx&0x7fffffff;
	    *eptr = -54;
	}
	*eptr += (ix>>20)-1022;
	hx = (hx&0x800fffff)|0x3fe00000;
        u.d = x;
	__HI(u) = hx;
        x = u.d;
	return x;
}

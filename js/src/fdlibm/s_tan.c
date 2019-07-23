
















































































#include "fdlibm.h"

#ifdef __STDC__
	double fd_tan(double x)
#else
	double fd_tan(x)
	double x;
#endif
{
        fd_twoints u;
	double y[2],z=0.0;
	int n, ix;

    
        u.d = x;
	ix = __HI(u);

    
	ix &= 0x7fffffff;
	if(ix <= 0x3fe921fb) return __kernel_tan(x,z,1);

    
	else if (ix>=0x7ff00000) return x-x;		

    
	else {
	    n = __ieee754_rem_pio2(x,y);
	    return __kernel_tan(y[0],y[1],1-((n&1)<<1)); 

	}
}

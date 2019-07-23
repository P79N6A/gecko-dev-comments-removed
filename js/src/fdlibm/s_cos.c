

















































































#include "fdlibm.h"

#ifdef __STDC__
	double fd_cos(double x)
#else
	double fd_cos(x)
	double x;
#endif
{
        fd_twoints u;
	double y[2],z=0.0;
	int n, ix;

    
        u.d = x;
	ix = __HI(u);

    
	ix &= 0x7fffffff;
	if(ix <= 0x3fe921fb) return __kernel_cos(x,z);

    
	else if (ix>=0x7ff00000) return x-x;

    
	else {
	    n = __ieee754_rem_pio2(x,y);
	    switch(n&3) {
		case 0: return  __kernel_cos(y[0],y[1]);
		case 1: return -__kernel_sin(y[0],y[1],1);
		case 2: return -__kernel_cos(y[0],y[1]);
		default:
		        return  __kernel_sin(y[0],y[1],1);
	    }
	}
}

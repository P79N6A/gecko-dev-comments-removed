






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_hypot(double x, double y)
#else
	double fd_hypot(x,y)		
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_hypot(x,y);
#else
	double z;
	z = __ieee754_hypot(x,y);
	if(_LIB_VERSION == _IEEE_) return z;
	if((!fd_finite(z))&&fd_finite(x)&&fd_finite(y)) { 
        int err;
	    return __kernel_standard(x,y,4,&err); 
    } else
	    return z;
#endif
}

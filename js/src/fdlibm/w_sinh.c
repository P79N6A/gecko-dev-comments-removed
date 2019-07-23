






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_sinh(double x)		
#else
	double fd_sinh(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sinh(x);
#else
	double z; 
	z = __ieee754_sinh(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!fd_finite(z)&&fd_finite(x)) {
        int err;
	    return __kernel_standard(x,x,25,&err); 
	} else
	    return z;
#endif
}

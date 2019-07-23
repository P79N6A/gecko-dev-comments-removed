






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_cosh(double x)		
#else
	double fd_cosh(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_cosh(x);
#else
	double z;
	z = __ieee754_cosh(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(fd_fabs(x)>7.10475860073943863426e+02) {	
        int err;
        return __kernel_standard(x,x,5,&err); 
	} else
	    return z;
#endif
}

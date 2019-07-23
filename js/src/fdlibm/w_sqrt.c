






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_sqrt(double x)		
#else
	double fd_sqrt(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sqrt(x);
#else
	double z;
	z = __ieee754_sqrt(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(x<0.0) {
	    int err;
	    return __kernel_standard(x,x,26,&err); 
	} else
	    return z;
#endif
}

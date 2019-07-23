























































#include "fdlibm.h"

#ifdef __STDC__
	double fd_acosh(double x)		
#else
	double fd_acosh(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acosh(x);
#else
	double z;
	z = __ieee754_acosh(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(x<1.0) {
        int err;
        return __kernel_standard(x,x,29,&err); 
	} else
	    return z;
#endif
}

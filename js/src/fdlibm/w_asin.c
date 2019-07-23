
























































#include "fdlibm.h"


#ifdef __STDC__
	double fd_asin(double x)		
#else
	double fd_asin(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_asin(x);
#else
	double z;
	z = __ieee754_asin(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(fd_fabs(x)>1.0) {
        int err;
        return __kernel_standard(x,x,2,&err); 
	} else
	    return z;
#endif
}

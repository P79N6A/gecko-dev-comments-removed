





















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_atanh(double x)		
#else
	double fd_atanh(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atanh(x);
#else
	double z,y;
	z = __ieee754_atanh(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	y = fd_fabs(x);
	if(y>=1.0) {
        int err;
	    if(y>1.0)
	        return __kernel_standard(x,x,30,&err); 
	    else 
	        return __kernel_standard(x,x,31,&err); 
	} else
	    return z;
#endif
}

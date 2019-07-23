






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_log(double x)		
#else
	double fd_log(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log(x);
#else
	double z;
    int err;
	z = __ieee754_log(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x) || x > 0.0) return z;
	if(x==0.0)
	    return __kernel_standard(x,x,16,&err); 
	else 
	    return __kernel_standard(x,x,17,&err); 
#endif
}

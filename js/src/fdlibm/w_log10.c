






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_log10(double x)		
#else
	double fd_log10(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log10(x);
#else
	double z;
	z = __ieee754_log10(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(x<=0.0) {
        int err;
	    if(x==0.0)
	        return __kernel_standard(x,x,18,&err); 
	    else 
	        return __kernel_standard(x,x,19,&err); 
	} else
	    return z;
#endif
}

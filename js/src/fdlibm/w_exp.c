






















































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
o_threshold=  7.09782712893383973096e+02,  
u_threshold= -7.45133219101941108420e+02;  

#ifdef __STDC__
	double fd_exp(double x)		
#else
	double fd_exp(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp(x);
#else
	double z;
	z = __ieee754_exp(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(fd_finite(x)) {
        int err;
	    if(x>o_threshold)
	        return __kernel_standard(x,x,6,&err); 
	    else if(x<u_threshold)
	        return __kernel_standard(x,x,7,&err); 
	} 
	return z;
#endif
}

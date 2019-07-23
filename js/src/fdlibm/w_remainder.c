






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_remainder(double x, double y)	
#else
	double fd_remainder(x,y)			
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_remainder(x,y);
#else
	double z;
	z = __ieee754_remainder(x,y);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(y)) return z;
	if(y==0.0) {
	    int err;
	    return __kernel_standard(x,y,28,&err); 
    } else
	    return z;
#endif
}

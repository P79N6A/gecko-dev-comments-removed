























































#include "fdlibm.h"


#ifdef __STDC__
	double fd_atan2(double y, double x)	
#else
	double fd_atan2(y,x)			
	double y,x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_atan2(y,x);
#else
	double z;
	z = __ieee754_atan2(y,x);
	if(_LIB_VERSION == _IEEE_||fd_isnan(x)||fd_isnan(y)) return z;
	if(x==0.0&&y==0.0) {
        int err;
        return __kernel_standard(y,x,3,&err); 
	} else
	    return z;
#endif
}

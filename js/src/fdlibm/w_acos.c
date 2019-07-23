






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_acos(double x)		
#else
	double fd_acos(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_acos(x);
#else
	double z;
	z = __ieee754_acos(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x)) return z;
	if(fd_fabs(x)>1.0) {
        int err;
        return __kernel_standard(x,x,1,&err); 
	} else
	    return z;
#endif
}

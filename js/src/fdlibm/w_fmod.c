






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_fmod(double x, double y)	
#else
	double fd_fmod(x,y)		
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_fmod(x,y);
#else
	double z;
	z = __ieee754_fmod(x,y);
	if(_LIB_VERSION == _IEEE_ ||fd_isnan(y)||fd_isnan(x)) return z;
	if(y==0.0) {
        int err;
        return __kernel_standard(x,y,27,&err); 
	} else
	    return z;
#endif
}

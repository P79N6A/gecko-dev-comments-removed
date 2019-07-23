






















































#include "fdlibm.h"


#ifdef __STDC__
	double fd_lgamma_r(double x, int *signgamp) 
#else
	double fd_lgamma_r(x,signgamp)              
        double x; int *signgamp;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_lgamma_r(x,signgamp);
#else
        double y;
        y = __ieee754_lgamma_r(x,signgamp);
        if(_LIB_VERSION == _IEEE_) return y;
        if(!fd_finite(y)&&fd_finite(x)) {
            int err;
            if(fd_floor(x)==x&&x<=0.0)
                return __kernel_standard(x,x,15,&err); 
            else
                return __kernel_standard(x,x,14,&err); 
        } else
            return y;
#endif
}             

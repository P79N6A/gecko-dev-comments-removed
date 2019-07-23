

























































#include "fdlibm.h"

extern int signgam;

#ifdef __STDC__
	double fd_gamma(double x)
#else
	double fd_gamma(x)
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_gamma_r(x,&signgam);
#else
        double y;
        y = __ieee754_gamma_r(x,&signgam);
        if(_LIB_VERSION == _IEEE_) return y;
        if(!fd_finite(y)&&fd_finite(x)) {
            int err;
            if(fd_floor(x)==x&&x<=0.0)
                return __kernel_standard(x,x,41,&err); 
            else
                return __kernel_standard(x,x,40,&err); 
        } else
            return y;
#endif
}             

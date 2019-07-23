
























































#include "fdlibm.h"

#include <errno.h>

#ifdef __STDC__
#ifdef _SCALB_INT
	double fd_scalb(double x, int fn)		
#else
	double fd_scalb(double x, double fn)	
#endif
#else
	double fd_scalb(x,fn)			
#ifdef _SCALB_INT
	double x; int fn;
#else
	double x,fn;
#endif
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_scalb(x,fn);
#else
	double z;
    int err;
	z = __ieee754_scalb(x,fn);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!(fd_finite(z)||fd_isnan(z))&&fd_finite(x)) {
	    return __kernel_standard(x,(double)fn,32,&err); 
	}
	if(z==0.0&&z!=x) {
	    return __kernel_standard(x,(double)fn,33,&err); 
	} 
#ifndef _SCALB_INT
	if(!fd_finite(fn)) errno = ERANGE;
#endif
	return z;
#endif 
}

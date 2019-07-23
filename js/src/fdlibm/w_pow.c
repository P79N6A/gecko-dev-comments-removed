























































#include "fdlibm.h"


#ifdef __STDC__
	double fd_pow(double x, double y)	
#else
	double fd_pow(x,y)			
	double x,y;
#endif
{
#ifdef _IEEE_LIBM
	return  __ieee754_pow(x,y);
#else
	double z;
    int err;
	z=__ieee754_pow(x,y);
	if(_LIB_VERSION == _IEEE_|| fd_isnan(y)) return z;
	if(fd_isnan(x)) {
	    if(y==0.0) 
	        return __kernel_standard(x,y,42,&err); 
	    else 
		return z;
	}
	if(x==0.0){ 
	    if(y==0.0)
	        return __kernel_standard(x,y,20,&err); 
	    if(fd_finite(y)&&y<0.0)
	        return __kernel_standard(x,y,23,&err); 
	    return z;
	}
	if(!fd_finite(z)) {
	    if(fd_finite(x)&&fd_finite(y)) {
	        if(fd_isnan(z))
	            return __kernel_standard(x,y,24,&err); 
	        else 
	            return __kernel_standard(x,y,21,&err); 
	    }
	} 
	if(z==0.0&&fd_finite(x)&&fd_finite(y))
	    return __kernel_standard(x,y,22,&err); 
	return z;
#endif
}

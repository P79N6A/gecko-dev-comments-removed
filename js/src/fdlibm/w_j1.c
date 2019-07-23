






















































#include "fdlibm.h"

#ifdef __STDC__
	double fd_j1(double x)		
#else
	double fd_j1(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_j1(x);
#else
	double z;
	z = __ieee754_j1(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x) ) return z;
	if(fd_fabs(x)>X_TLOSS) {
        int err;
        return __kernel_standard(x,x,36,&err); 
	} else
	    return z;
#endif
}

#ifdef __STDC__
	double y1(double x)		
#else
	double y1(x)			
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_y1(x);
#else
	double z;
    int err;
	z = __ieee754_y1(x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    
                    return __kernel_standard(x,x,10,&err);
                else
                    
                    return __kernel_standard(x,x,11,&err);
        }
	if(x>X_TLOSS) {
	        return __kernel_standard(x,x,37,&err); 
	} else
	    return z;
#endif
}

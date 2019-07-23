












































































#include "fdlibm.h"

#ifdef __STDC__
	double fd_jn(int n, double x)	
#else
	double fd_jn(n,x)			
	double x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_jn(n,x);
#else
	double z;
	z = __ieee754_jn(n,x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x) ) return z;
	if(fd_fabs(x)>X_TLOSS) {
        int err;
	    return __kernel_standard((double)n,x,38,&err); 
	} else
	    return z;
#endif
}

#ifdef __STDC__
	double yn(int n, double x)	
#else
	double yn(n,x)			
	double x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_yn(n,x);
#else
	double z;
    int err;
	z = __ieee754_yn(n,x);
	if(_LIB_VERSION == _IEEE_ || fd_isnan(x) ) return z;
        if(x <= 0.0){
                if(x==0.0)
                    
                    return __kernel_standard((double)n,x,12,&err);
                else
                    
                    return __kernel_standard((double)n,x,13,&err);
        }
	if(x>X_TLOSS) {
	    return __kernel_standard((double)n,x,39,&err); 
	} else
	    return z;
#endif
}

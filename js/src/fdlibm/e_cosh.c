







































































#include "fdlibm.h"

#ifdef _WIN32
#define huge myhuge
#endif

#ifdef __STDC__
static const double one = 1.0, half=0.5, really_big = 1.0e300;
#else
static double one = 1.0, half=0.5, really_big = 1.0e300;
#endif

#ifdef __STDC__
	double __ieee754_cosh(double x)
#else
	double __ieee754_cosh(x)
	double x;
#endif
{	
        fd_twoints u;
	double t,w;
	int ix;
	unsigned lx;
        
    
        u.d = x;
	ix = __HI(u);
	ix &= 0x7fffffff;

    
	if(ix>=0x7ff00000) return x*x;	

    
	if(ix<0x3fd62e43) {
	    t = fd_expm1(fd_fabs(x));
	    w = one+t;
	    if (ix<0x3c800000) return w;	
	    return one+(t*t)/(w+w);
	}

    
	if (ix < 0x40360000) {
		t = __ieee754_exp(fd_fabs(x));
		return half*t+half/t;
	}

    
	if (ix < 0x40862E42)  return half*__ieee754_exp(fd_fabs(x));

    
	lx = *( (((*(unsigned*)&one)>>29)) + (unsigned*)&x);
	if (ix<0x408633CE || 
	      (ix==0x408633ce)&&(lx<=(unsigned)0x8fb9f87d)) {
	    w = __ieee754_exp(half*fd_fabs(x));
	    t = half*w;
	    return t*w;
	}

    
	return really_big*really_big;
}

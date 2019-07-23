





































































#include "fdlibm.h"

#ifdef __STDC__
static const double one = 1.0, really_big = 1e300;
#else
static double one = 1.0, really_big = 1e300;
#endif

static double zero = 0.0;

#ifdef __STDC__
	double __ieee754_atanh(double x)
#else
	double __ieee754_atanh(x)
	double x;
#endif
{
	double t;
	int hx,ix;
	unsigned lx;
        fd_twoints u;
        u.d = x;
	hx = __HI(u);		
	lx = __LO(u);		
	ix = hx&0x7fffffff;
	if ((ix|((lx|(-(int)lx))>>31))>0x3ff00000) 
	    return (x-x)/(x-x);
	if(ix==0x3ff00000) 
	    return x/zero;
	if(ix<0x3e300000&&(really_big+x)>zero) return x;	
        u.d = x;
	__HI(u) = ix;		
        x = u.d;
	if(ix<0x3fe00000) {		
	    t = x+x;
	    t = 0.5*fd_log1p(t+t*x/(one-x));
	} else 
	    t = 0.5*fd_log1p((x+x)/(one-x));
	if(hx>=0) return t; else return -t;
}

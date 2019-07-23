





























































#include "fdlibm.h"

#ifdef __STDC__
static const double 
#else
static double 
#endif
one =  1.00000000000000000000e+00, 
ln2 =  6.93147180559945286227e-01, 
really_big=  1.00000000000000000000e+300; 

#ifdef __STDC__
	double fd_asinh(double x)
#else
	double fd_asinh(x)
	double x;
#endif
{	
        fd_twoints u;
	double t,w;
	int hx,ix;
        u.d = x;
	hx = __HI(u);
	ix = hx&0x7fffffff;
	if(ix>=0x7ff00000) return x+x;	
	if(ix< 0x3e300000) {	
	    if(really_big+x>one) return x;	
	} 
	if(ix>0x41b00000) {	
	    w = __ieee754_log(fd_fabs(x))+ln2;
	} else if (ix>0x40000000) {	
	    t = fd_fabs(x);
	    w = __ieee754_log(2.0*t+one/(fd_sqrt(x*x+one)+t));
	} else {		
	    t = x*x;
	    w =fd_log1p(fd_fabs(x)+t/(one+fd_sqrt(one+t)));
	}
	if(hx>0) return w; else return -w;
}

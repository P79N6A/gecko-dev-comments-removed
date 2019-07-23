
























































#include "fdlibm.h"

#ifdef _SCALB_INT
#ifdef __STDC__
	double __ieee754_scalb(double x, int fn)
#else
	double __ieee754_scalb(x,fn)
	double x; int fn;
#endif
#else
#ifdef __STDC__
	double __ieee754_scalb(double x, double fn)
#else
	double __ieee754_scalb(x,fn)
	double x, fn;
#endif
#endif
{
#ifdef _SCALB_INT
	return fd_scalbn(x,fn);
#else
	if (fd_isnan(x)||fd_isnan(fn)) return x*fn;
	if (!fd_finite(fn)) {
	    if(fn>0.0) return x*fn;
	    else       return x/(-fn);
	}
	if (fd_rint(fn)!=fn) return (fn-fn)/(fn-fn);
	if ( fn > 65000.0) return fd_scalbn(x, 65000);
	if (-fn > 65000.0) return fd_scalbn(x,-65000);
	return fd_scalbn(x,(int)fn);
#endif
}



























































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
two54   =  1.80143985094819840000e+16, 
twom54  =  5.55111512312578270212e-17, 
really_big   = 1.0e+300,
tiny   = 1.0e-300;

#ifdef __STDC__
	double fd_scalbn (double x, int n)
#else
	double fd_scalbn (x,n)
	double x; int n;
#endif
{
        fd_twoints u;
	int  k,hx,lx;
        u.d = x;
	hx = __HI(u);
	lx = __LO(u);
        k = (hx&0x7ff00000)>>20;		
        if (k==0) {				
            if ((lx|(hx&0x7fffffff))==0) return x; 
	    x *= two54;
            u.d = x;
	    hx = __HI(u);
	    k = ((hx&0x7ff00000)>>20) - 54; 
            if (n< -50000) return tiny*x; 	
	    }
        if (k==0x7ff) return x+x;		
        k = k+n; 
        if (k >  0x7fe) return really_big*fd_copysign(really_big,x); 
        if (k > 0) 				
	    {u.d = x; __HI(u) = (hx&0x800fffff)|(k<<20); x = u.d; return x;}
        if (k <= -54) {
            if (n > 50000) 	
		return really_big*fd_copysign(really_big,x);	
	    else return tiny*fd_copysign(tiny,x); 	
        }
        k += 54;				
        u.d = x;
        __HI(u) = (hx&0x800fffff)|(k<<20);
        x = u.d;
        return x*twom54;
}






























































#include "fdlibm.h"

#ifdef __STDC__
static const double one = 1.0;
#else
static double one = 1.0;
#endif

#ifdef __STDC__
	double fd_modf(double x, double *iptr)
#else
	double fd_modf(x, iptr)
	double x,*iptr;
#endif
{
	int i0,i1,j0;
	unsigned i;
        fd_twoints u;
        u.d = x;
	i0 =  __HI(u);		
	i1 =  __LO(u);		
	j0 = ((i0>>20)&0x7ff)-0x3ff;	
	if(j0<20) {			
	    if(j0<0) {			
                u.d = *iptr;
		__HI(u) = i0&0x80000000;
		__LO(u) = 0;		
                *iptr = u.d;
		return x;
	    } else {
		i = (0x000fffff)>>j0;
		if(((i0&i)|i1)==0) {		
		    *iptr = x;
                    u.d = x;
		    __HI(u) &= 0x80000000;
		    __LO(u)  = 0;	
                    x = u.d;
		    return x;
		} else {
                    u.d = *iptr;
		    __HI(u) = i0&(~i);
		    __LO(u) = 0;
                    *iptr = u.d;
		    return x - *iptr;
		}
	    }
	} else if (j0>51) {		
	    *iptr = x*one;
            u.d = x;
	    __HI(u) &= 0x80000000;
	    __LO(u)  = 0;	
            x = u.d;
	    return x;
	} else {			
	    i = ((unsigned)(0xffffffff))>>(j0-20);
	    if((i1&i)==0) { 		
		*iptr = x;
                u.d = x;
		__HI(u) &= 0x80000000;
		__LO(u)  = 0;	
                x = u.d;
		return x;
	    } else {
                u.d = *iptr;
		__HI(u) = i0;
		__LO(u) = i1&(~i);
                *iptr = u.d;
		return x - *iptr;
	    }
	}
}

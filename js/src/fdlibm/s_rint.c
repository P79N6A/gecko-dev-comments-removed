




























































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double 
#endif
TWO52[2]={
  4.50359962737049600000e+15, 
 -4.50359962737049600000e+15, 
};

#ifdef __STDC__
	double fd_rint(double x)
#else
	double fd_rint(x)
	double x;
#endif
{
	int i0,j0,sx;
	unsigned i,i1;
	double w,t;
        fd_twoints u;

        u.d = x;
	i0 =  __HI(u);
	sx = (i0>>31)&1;
	i1 =  __LO(u);
	j0 = ((i0>>20)&0x7ff)-0x3ff;
	if(j0<20) {
	    if(j0<0) { 	
		if(((i0&0x7fffffff)|i1)==0) return x;
		i1 |= (i0&0x0fffff);
		i0 &= 0xfffe0000;
		i0 |= ((i1|-(int)i1)>>12)&0x80000;
                u.d = x;
		__HI(u)=i0;
                x = u.d;
	        w = TWO52[sx]+x;
	        t =  w-TWO52[sx];
                u.d = t;
	        i0 = __HI(u);
	        __HI(u) = (i0&0x7fffffff)|(sx<<31);
                t = u.d;
	        return t;
	    } else {
		i = (0x000fffff)>>j0;
		if(((i0&i)|i1)==0) return x; 
		i>>=1;
		if(((i0&i)|i1)!=0) {
		    if(j0==19) i1 = 0x40000000; else
		    i0 = (i0&(~i))|((0x20000)>>j0);
		}
	    }
	} else if (j0>51) {
	    if(j0==0x400) return x+x;	
	    else return x;		
	} else {
	    i = ((unsigned)(0xffffffff))>>(j0-20);
	    if((i1&i)==0) return x;	
	    i>>=1;
	    if((i1&i)!=0) i1 = (i1&(~i))|((0x40000000)>>(j0-20));
	}
        u.d = x;
	__HI(u) = i0;
	__LO(u) = i1;
        x = u.d;
	w = TWO52[sx]+x;
	return w-TWO52[sx];
}

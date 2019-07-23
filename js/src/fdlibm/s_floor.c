



























































#include "fdlibm.h"

#ifdef __STDC__
static const double really_big = 1.0e300;
#else
static double really_big = 1.0e300;
#endif

#ifdef __STDC__
	double fd_floor(double x)
#else
	double fd_floor(x)
	double x;
#endif
{
        fd_twoints u;
	int i0,i1,j0;
	unsigned i,j;
        u.d = x;
	i0 =  __HI(u);
	i1 =  __LO(u);
	j0 = ((i0>>20)&0x7ff)-0x3ff;
	if(j0<20) {
	    if(j0<0) { 	
		if(really_big+x>0.0) {
		    if(i0>=0) {i0=i1=0;} 
		    else if(((i0&0x7fffffff)|i1)!=0)
			{ i0=0xbff00000;i1=0;}
		}
	    } else {
		i = (0x000fffff)>>j0;
		if(((i0&i)|i1)==0) return x; 
		if(really_big+x>0.0) {	
		    if(i0<0) i0 += (0x00100000)>>j0;
		    i0 &= (~i); i1=0;
		}
	    }
	} else if (j0>51) {
	    if(j0==0x400) return x+x;	
	    else return x;		
	} else {
	    i = ((unsigned)(0xffffffff))>>(j0-20);
	    if((i1&i)==0) return x;	
	    if(really_big+x>0.0) { 		
		if(i0<0) {
		    if(j0==20) i0+=1; 
		    else {
			j = i1+(1<<(52-j0));
			if((int)j<i1) i0 +=1 ; 	
			i1=j;
		    }
		}
		i1 &= (~i);
	    }
	}
        u.d = x;
	__HI(u) = i0;
	__LO(u) = i1;
        x = u.d;
	return x;
}

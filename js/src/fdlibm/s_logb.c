
























































#include "fdlibm.h"

#ifdef __STDC__
	double fd_logb(double x)
#else
	double fd_logb(x)
	double x;
#endif
{
	int lx,ix;
        fd_twoints u;

        u.d = x;
	ix = (__HI(u))&0x7fffffff;	
	lx = __LO(u);			
	if((ix|lx)==0) return -1.0/fd_fabs(x);
	if(ix>=0x7ff00000) return x*x;
	if((ix>>=20)==0) 			
		return -1022.0; 
	else
		return (double) (ix-1023); 
}

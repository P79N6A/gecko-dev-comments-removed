
























































#include "fdlibm.h"

#ifdef __STDC__
	int fd_ilogb(double x)
#else
	int fd_ilogb(x)
	double x;
#endif
{
	int hx,lx,ix;
        fd_twoints u;
        u.d = x;
	hx  = (__HI(u))&0x7fffffff;	
	if(hx<0x00100000) {
	    lx = __LO(u);
	    if((hx|lx)==0) 
		return 0x80000001;	
	    else			
		if(hx==0) {
		    for (ix = -1043; lx>0; lx<<=1) ix -=1;
		} else {
		    for (ix = -1022,hx<<=11; hx>0; hx<<=1) ix -=1;
		}
	    return ix;
	}
	else if (hx<0x7ff00000) return (hx>>20)-1023;
	else return 0x7fffffff;
}

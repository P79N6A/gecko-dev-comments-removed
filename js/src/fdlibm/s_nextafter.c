

























































#include "fdlibm.h"

#ifdef __STDC__
	double fd_nextafter(double x, double y)
#else
	double fd_nextafter(x,y)
	double x,y;
#endif
{
	int	hx,hy,ix,iy;
	unsigned lx,ly;
        fd_twoints ux, uy;

        ux.d = x; uy.d = y;
	hx = __HI(ux);		
	lx = __LO(ux);		
	hy = __HI(uy);		
	ly = __LO(uy);		
	ix = hx&0x7fffffff;		
	iy = hy&0x7fffffff;		

	if(((ix>=0x7ff00000)&&((ix-0x7ff00000)|lx)!=0) ||    
	   ((iy>=0x7ff00000)&&((iy-0x7ff00000)|ly)!=0))      
	   return x+y;				
	if(x==y) return x;		
	if((ix|lx)==0) {			
            ux.d = x;
	    __HI(ux) = hy&0x80000000;	
	    __LO(ux) = 1;
            x = ux.d;
	    y = x*x;
	    if(y==x) return y; else return x;	
	} 
	if(hx>=0) {				
	    if(hx>hy||((hx==hy)&&(lx>ly))) {	
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				
		lx += 1;
		if(lx==0) hx += 1;
	    }
	} else {				
	    if(hy>=0||hx>hy||((hx==hy)&&(lx>ly))){
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				
		lx += 1;
		if(lx==0) hx += 1;
	    }
	}
	hy = hx&0x7ff00000;
	if(hy>=0x7ff00000) return x+x;	
	if(hy<0x00100000) {		
	    y = x*x;
	    if(y!=x) {		
                uy.d = y;
		__HI(uy) = hx; __LO(uy) = lx;
                y = uy.d;
		return y;
	    }
	}
        ux.d = x;
	__HI(ux) = hx; __LO(ux) = lx;
        x = ux.d;
	return x;
}

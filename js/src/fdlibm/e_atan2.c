














































































#include "fdlibm.h"

#ifdef __STDC__
static const double 
#else
static double 
#endif
tiny  = 1.0e-300,
zero  = 0.0,
pi_o_4  = 7.8539816339744827900E-01, 
pi_o_2  = 1.5707963267948965580E+00, 
pi      = 3.1415926535897931160E+00, 
pi_lo   = 1.2246467991473531772E-16; 

#ifdef __STDC__
	double __ieee754_atan2(double y, double x)
#else
	double __ieee754_atan2(y,x)
	double  y,x;
#endif
{  
        fd_twoints ux, uy, uz;
	double z;
	int k,m,hx,hy,ix,iy;
	unsigned lx,ly;

        ux.d = x; uy.d = y;
	hx = __HI(ux); ix = hx&0x7fffffff;
	lx = __LO(ux);
	hy = __HI(uy); iy = hy&0x7fffffff;
	ly = __LO(uy);
	if(((ix|((lx|-(int)lx)>>31))>0x7ff00000)||
	   ((iy|((ly|-(int)ly)>>31))>0x7ff00000))	
	   return x+y;
	if(((hx-0x3ff00000)|lx)==0) return fd_atan(y);   
	m = ((hy>>31)&1)|((hx>>30)&2);	

    
	if((iy|ly)==0) {
	    switch(m) {
		case 0: 
		case 1: return y; 	
		case 2: return  pi+tiny;
		case 3: return -pi-tiny;
	    }
	}
    
	if((ix|lx)==0) return (hy<0)?  -pi_o_2-tiny: pi_o_2+tiny;
	    
    
	if(ix==0x7ff00000) {
	    if(iy==0x7ff00000) {
		switch(m) {
		    case 0: return  pi_o_4+tiny;
		    case 1: return -pi_o_4-tiny;
		    case 2: return  3.0*pi_o_4+tiny;
		    case 3: return -3.0*pi_o_4-tiny;
		}
	    } else {
		switch(m) {
		    case 0: return  zero  ;	
		    case 1: return -zero  ;	
		    case 2: return  pi+tiny  ;	
		    case 3: return -pi-tiny  ;	
		}
	    }
	}
    
	if(iy==0x7ff00000) return (hy<0)? -pi_o_2-tiny: pi_o_2+tiny;

    
	k = (iy-ix)>>20;
	if(k > 60) z=pi_o_2+0.5*pi_lo; 	
	else if(hx<0&&k<-60) z=0.0; 	
	else z=fd_atan(fd_fabs(y/x));		
	switch (m) {
	    case 0: return       z  ;	
	    case 1: uz.d = z;
                    __HI(uz) ^= 0x80000000;
                    z = uz.d;
		    return       z  ;	
	    case 2: return  pi-(z-pi_lo);
	    default: 
	    	    return  (z-pi_lo)-pi;
	}
}

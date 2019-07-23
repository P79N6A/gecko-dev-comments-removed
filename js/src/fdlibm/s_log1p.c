



















































































































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
ln2_hi  =  6.93147180369123816490e-01,	
ln2_lo  =  1.90821492927058770002e-10,	
two54   =  1.80143985094819840000e+16,  
Lp1 = 6.666666666666735130e-01,  
Lp2 = 3.999999999940941908e-01,  
Lp3 = 2.857142874366239149e-01,  
Lp4 = 2.222219843214978396e-01,  
Lp5 = 1.818357216161805012e-01,  
Lp6 = 1.531383769920937332e-01,  
Lp7 = 1.479819860511658591e-01;  

static double zero = 0.0;

#ifdef __STDC__
	double fd_log1p(double x)
#else
	double fd_log1p(x)
	double x;
#endif
{
	double hfsq,f,c,s,z,R,u;
	int k,hx,hu,ax;
        fd_twoints un;

        un.d = x;
	hx = __HI(un);		
	ax = hx&0x7fffffff;

	k = 1;
	if (hx < 0x3FDA827A) {			
	    if(ax>=0x3ff00000) {		
		if(x==-1.0) return -two54/zero; 
		else return (x-x)/(x-x);	
	    }
	    if(ax<0x3e200000) {			
		if(two54+x>zero			
	            &&ax<0x3c900000) 		
		    return x;
		else
		    return x - x*x*0.5;
	    }
	    if(hx>0||hx<=((int)0xbfd2bec3)) {
		k=0;f=x;hu=1;}	
	} 
	if (hx >= 0x7ff00000) return x+x;
	if(k!=0) {
	    if(hx<0x43400000) {
		u  = 1.0+x; 
                un.d = u;
	        hu = __HI(un);		
	        k  = (hu>>20)-1023;
	        c  = (k>0)? 1.0-(u-x):x-(u-1.0);
		c /= u;
	    } else {
		u  = x;
                un.d = u;
	        hu = __HI(un);		
	        k  = (hu>>20)-1023;
		c  = 0;
	    }
	    hu &= 0x000fffff;
	    if(hu<0x6a09e) {
                un.d = u;
	        __HI(un) = hu|0x3ff00000;	
                u = un.d;
	    } else {
	        k += 1; 
                un.d = u;
	        __HI(un) = hu|0x3fe00000;	
                u = un.d;
	        hu = (0x00100000-hu)>>2;
	    }
	    f = u-1.0;
	}
	hfsq=0.5*f*f;
	if(hu==0) {	
	    if(f==zero) if(k==0) return zero;  
			else {c += k*ln2_lo; return k*ln2_hi+c;}
	    R = hfsq*(1.0-0.66666666666666666*f);
	    if(k==0) return f-R; else
	    	     return k*ln2_hi-((R-(k*ln2_lo+c))-f);
	}
 	s = f/(2.0+f); 
	z = s*s;
	R = z*(Lp1+z*(Lp2+z*(Lp3+z*(Lp4+z*(Lp5+z*(Lp6+z*Lp7))))));
	if(k==0) return f-(hfsq-s*(hfsq+R)); else
		 return k*ln2_hi-((hfsq-(s*(hfsq+R)+(k*ln2_lo+c)))-f);
}

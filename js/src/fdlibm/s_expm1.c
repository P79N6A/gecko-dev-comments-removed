

















































































































































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
one		= 1.0,
really_big		= 1.0e+300,
tiny		= 1.0e-300,
o_threshold	= 7.09782712893383973096e+02,
ln2_hi		= 6.93147180369123816490e-01,
ln2_lo		= 1.90821492927058770002e-10,
invln2		= 1.44269504088896338700e+00,
	
Q1  =  -3.33333333333331316428e-02, 
Q2  =   1.58730158725481460165e-03, 
Q3  =  -7.93650757867487942473e-05, 
Q4  =   4.00821782732936239552e-06, 
Q5  =  -2.01099218183624371326e-07; 

#ifdef __STDC__
	double fd_expm1(double x)
#else
	double fd_expm1(x)
	double x;
#endif
{
        fd_twoints u;
	double y,hi,lo,c,t,e,hxs,hfx,r1;
	int k,xsb;
	unsigned hx;

        u.d = x;
	hx  = __HI(u);	
	xsb = hx&0x80000000;		
	if(xsb==0) y=x; else y= -x;	
	hx &= 0x7fffffff;		

    
	if(hx >= 0x4043687A) {			
	    if(hx >= 0x40862E42) {		
                if(hx>=0x7ff00000) {
                    u.d = x;
		    if(((hx&0xfffff)|__LO(u))!=0) 
		         return x+x; 	 
		    else return (xsb==0)? x:-1.0;
	        }
	        if(x > o_threshold) return really_big*really_big; 
	    }
	    if(xsb!=0) { 
		if(x+tiny<0.0)		
		return tiny-one;	
	    }
	}

    
	if(hx > 0x3fd62e42) {		 
	    if(hx < 0x3FF0A2B2) {	
		if(xsb==0)
		    {hi = x - ln2_hi; lo =  ln2_lo;  k =  1;}
		else
		    {hi = x + ln2_hi; lo = -ln2_lo;  k = -1;}
	    } else {
		k  = (int)(invln2*x+((xsb==0)?0.5:-0.5));
		t  = k;
		hi = x - t*ln2_hi;	
		lo = t*ln2_lo;
	    }
	    x  = hi - lo;
	    c  = (hi-x)-lo;
	} 
	else if(hx < 0x3c900000) {  	
	    t = really_big+x;	
	    return x - (t-(really_big+x));	
	}
	else k = 0;

    
	hfx = 0.5*x;
	hxs = x*hfx;
	r1 = one+hxs*(Q1+hxs*(Q2+hxs*(Q3+hxs*(Q4+hxs*Q5))));
	t  = 3.0-r1*hfx;
	e  = hxs*((r1-t)/(6.0 - x*t));
	if(k==0) return x - (x*e-hxs);		
	else {
	    e  = (x*(e-c)-c);
	    e -= hxs;
	    if(k== -1) return 0.5*(x-e)-0.5;
	    if(k==1) 
	       	if(x < -0.25) return -2.0*(e-(x+0.5));
	       	else 	      return  one+2.0*(x-e);
	    if (k <= -2 || k>56) {   
	        y = one-(e-x);
                u.d = y;
	        __HI(u) += (k<<20);	
                y = u.d;
	        return y-one;
	    }
	    t = one;
	    if(k<20) {
                u.d = t;
	       	__HI(u) = 0x3ff00000 - (0x200000>>k);  
                t = u.d;
	       	y = t-(e-x);
                u.d = y;
	       	__HI(u) += (k<<20);	
                y = u.d;
	   } else {
               u.d = t;
	       	__HI(u)  = ((0x3ff-k)<<20);	
                t = u.d;
	       	y = x-(e+t);
	       	y += one;
                u.d = y;
	       	__HI(u) += (k<<20);	
                y = u.d;
	    }
	}
	return y;
}

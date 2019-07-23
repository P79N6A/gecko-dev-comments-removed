



















































#include "fdlibm.h"




#ifdef __STDC__
static const unsigned 
#else
static unsigned 
#endif
	B1 = 715094163, 
	B2 = 696219795; 

#ifdef __STDC__
static const double
#else
static double
#endif
C =  5.42857142857142815906e-01, 
D = -7.05306122448979611050e-01, 
E =  1.41428571428571436819e+00, 
F =  1.60714285714285720630e+00, 
G =  3.57142857142857150787e-01; 

#ifdef __STDC__
	double fd_cbrt(double x) 
#else
	double fd_cbrt(x) 
	double x;
#endif
{
        fd_twoints u;
	int	hx;
	double r,s,t=0.0,w;
	unsigned sign;

        u.d = x;
	hx = __HI(u);		
	sign=hx&0x80000000; 		
	hx  ^=sign;
	if(hx>=0x7ff00000) return(x+x); 
	if((hx|__LO(u))==0) {
            x = u.d;
	    return(x);		
        }
        u.d = x;
	__HI(u) = hx;	
        x = u.d;
    
	if(hx<0x00100000) 		
	  {u.d = t; __HI(u)=0x43500000; t=u.d;		
	   t*=x; __HI(u)=__HI(u)/3+B2;
	  }
	else {
	  u.d = t; __HI(u)=hx/3+B1; t = u.d;
        }


    
	r=t*t/x;
	s=C+r*t;
	t*=G+F/(s+E+D/s);	

     
        u.d = t;
	__LO(u)=0; __HI(u)+=0x00000001;
        t = u.d;

    
	s=t*t;		
	r=x/s;
	w=t+t;
	r=(r-t)/(w+r);	
	t=t+t*r;

    
        u.d = t;
	__HI(u) |= sign;
        t = u.d;
	return(t);
}





























































#include "fdlibm.h"

#ifdef __STDC__
static const double zero = 0.0;
#else
static double zero = 0.0;
#endif


#ifdef __STDC__
	double __ieee754_remainder(double x, double p)
#else
	double __ieee754_remainder(x,p)
	double x,p;
#endif
{
        fd_twoints u;
	int hx,hp;
	unsigned sx,lx,lp;
	double p_half;

        u.d = x;
	hx = __HI(u);		
	lx = __LO(u);		
        u.d = p;
	hp = __HI(u);		
	lp = __LO(u);		
	sx = hx&0x80000000;
	hp &= 0x7fffffff;
	hx &= 0x7fffffff;

    
	if((hp|lp)==0) return (x*p)/(x*p); 	
	if((hx>=0x7ff00000)||			
	  ((hp>=0x7ff00000)&&			
	  (((hp-0x7ff00000)|lp)!=0)))
	    return (x*p)/(x*p);


	if (hp<=0x7fdfffff) x = __ieee754_fmod(x,p+p);	
	if (((hx-hp)|(lx-lp))==0) return zero*x;
	x  = fd_fabs(x);
	p  = fd_fabs(p);
	if (hp<0x00200000) {
	    if(x+x>p) {
		x-=p;
		if(x+x>=p) x -= p;
	    }
	} else {
	    p_half = 0.5*p;
	    if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	    }
	}
        u.d = x;
	__HI(u) ^= sx;
        x = u.d;
	return x;
}

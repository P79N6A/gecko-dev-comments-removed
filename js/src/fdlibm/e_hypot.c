


















































































#include "fdlibm.h"

#ifdef __STDC__
	double __ieee754_hypot(double x, double y)
#else
	double __ieee754_hypot(x,y)
	double x, y;
#endif
{
        fd_twoints ux, uy;
	double a=x,b=y,t1,t2,y1,y2,w;
	int j,k,ha,hb;
        
        ux.d = x; uy.d = y;
	ha = __HI(ux)&0x7fffffff;	
	hb = __HI(uy)&0x7fffffff;	
	if(hb > ha) {a=y;b=x;j=ha; ha=hb;hb=j;} else {a=x;b=y;}
        ux.d = a; uy.d = b;
	__HI(ux) = ha;	
	__HI(uy) = hb;	
        a = ux.d; b = uy.d;
	if((ha-hb)>0x3c00000) {return a+b;} 
	k=0;
	if(ha > 0x5f300000) {	
	   if(ha >= 0x7ff00000) {	
	       w = a+b;			
               ux.d = a; uy.d = b;
	       if(((ha&0xfffff)|__LO(ux))==0) w = a;
	       if(((hb^0x7ff00000)|__LO(uy))==0) w = b;
	       return w;
	   }
	   
	   ha -= 0x25800000; hb -= 0x25800000;	k += 600;
           ux.d = a; uy.d = b;
	   __HI(ux) = ha;
	   __HI(uy) = hb;
           a = ux.d; b = uy.d;
	}
	if(hb < 0x20b00000) {	
	    if(hb <= 0x000fffff) {		
                uy.d = b;
		if((hb|(__LO(uy)))==0) return a;
		t1=0;
                ux.d = t1;
		__HI(ux) = 0x7fd00000;	
                t1 = ux.d;
		b *= t1;
		a *= t1;
		k -= 1022;
	    } else {		
	        ha += 0x25800000; 	
		hb += 0x25800000;	
		k -= 600;
                ux.d = a; uy.d = b;
	   	__HI(ux) = ha;
	   	__HI(uy) = hb;
                a = ux.d; b = uy.d;
	    }
	}
    
	w = a-b;
	if (w>b) {
	    t1 = 0;
            ux.d = t1;
	    __HI(ux) = ha;
            t1 = ux.d;
	    t2 = a-t1;
	    w  = fd_sqrt(t1*t1-(b*(-b)-t2*(a+t1)));
	} else {
	    a  = a+a;
	    y1 = 0;
            ux.d = y1;
	    __HI(ux) = hb;
            y1 = ux.d;
	    y2 = b - y1;
	    t1 = 0;
            ux.d = t1;
	    __HI(ux) = ha+0x00100000;
            t1 = ux.d;
	    t2 = a - t1;
	    w  = fd_sqrt(t1*y1-(w*(-w)-(t1*y2+t2*b)));
	}
	if(k!=0) {
	    t1 = 1.0;
            ux.d = t1;
	    __HI(ux) += (k<<20);
            t1 = ux.d;
	    return t1*w;
	} else return w;
}

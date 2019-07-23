






















































































































#include "fdlibm.h"

#ifdef __STDC__
static const double 
#else
static double 
#endif
two52=  4.50359962737049600000e+15, 
half=  5.00000000000000000000e-01, 
one =  1.00000000000000000000e+00, 
pi  =  3.14159265358979311600e+00, 
a0  =  7.72156649015328655494e-02, 
a1  =  3.22467033424113591611e-01, 
a2  =  6.73523010531292681824e-02, 
a3  =  2.05808084325167332806e-02, 
a4  =  7.38555086081402883957e-03, 
a5  =  2.89051383673415629091e-03, 
a6  =  1.19270763183362067845e-03, 
a7  =  5.10069792153511336608e-04, 
a8  =  2.20862790713908385557e-04, 
a9  =  1.08011567247583939954e-04, 
a10 =  2.52144565451257326939e-05, 
a11 =  4.48640949618915160150e-05, 
tc  =  1.46163214496836224576e+00, 
tf  = -1.21486290535849611461e-01, 

tt  = -3.63867699703950536541e-18, 
t0  =  4.83836122723810047042e-01, 
t1  = -1.47587722994593911752e-01, 
t2  =  6.46249402391333854778e-02, 
t3  = -3.27885410759859649565e-02, 
t4  =  1.79706750811820387126e-02, 
t5  = -1.03142241298341437450e-02, 
t6  =  6.10053870246291332635e-03, 
t7  = -3.68452016781138256760e-03, 
t8  =  2.25964780900612472250e-03, 
t9  = -1.40346469989232843813e-03, 
t10 =  8.81081882437654011382e-04, 
t11 = -5.38595305356740546715e-04, 
t12 =  3.15632070903625950361e-04, 
t13 = -3.12754168375120860518e-04, 
t14 =  3.35529192635519073543e-04, 
u0  = -7.72156649015328655494e-02, 
u1  =  6.32827064025093366517e-01, 
u2  =  1.45492250137234768737e+00, 
u3  =  9.77717527963372745603e-01, 
u4  =  2.28963728064692451092e-01, 
u5  =  1.33810918536787660377e-02, 
v1  =  2.45597793713041134822e+00, 
v2  =  2.12848976379893395361e+00, 
v3  =  7.69285150456672783825e-01, 
v4  =  1.04222645593369134254e-01, 
v5  =  3.21709242282423911810e-03, 
s0  = -7.72156649015328655494e-02, 
s1  =  2.14982415960608852501e-01, 
s2  =  3.25778796408930981787e-01, 
s3  =  1.46350472652464452805e-01, 
s4  =  2.66422703033638609560e-02, 
s5  =  1.84028451407337715652e-03, 
s6  =  3.19475326584100867617e-05, 
r1  =  1.39200533467621045958e+00, 
r2  =  7.21935547567138069525e-01, 
r3  =  1.71933865632803078993e-01, 
r4  =  1.86459191715652901344e-02, 
r5  =  7.77942496381893596434e-04, 
r6  =  7.32668430744625636189e-06, 
w0  =  4.18938533204672725052e-01, 
w1  =  8.33333333333329678849e-02, 
w2  = -2.77777777728775536470e-03, 
w3  =  7.93650558643019558500e-04, 
w4  = -5.95187557450339963135e-04, 
w5  =  8.36339918996282139126e-04, 
w6  = -1.63092934096575273989e-03; 

static double zero=  0.00000000000000000000e+00;

#ifdef __STDC__
	static double sin_pi(double x)
#else
	static double sin_pi(x)
	double x;
#endif
{
        fd_twoints u;
	double y,z;
	int n,ix;

        u.d = x;
	ix = 0x7fffffff&__HI(u);

	if(ix<0x3fd00000) return __kernel_sin(pi*x,zero,0);
	y = -x;		

    



	z = fd_floor(y);
	if(z!=y) {				
	    y  *= 0.5;
	    y   = 2.0*(y - fd_floor(y));		
	    n   = (int) (y*4.0);
	} else {
            if(ix>=0x43400000) {
                y = zero; n = 0;                 
            } else {
                if(ix<0x43300000) z = y+two52;	
                u.d = z;
                n   = __LO(u)&1;        
                y  = n;
                n<<= 2;
            }
        }
	switch (n) {
	    case 0:   y =  __kernel_sin(pi*y,zero,0); break;
	    case 1:   
	    case 2:   y =  __kernel_cos(pi*(0.5-y),zero); break;
	    case 3:  
	    case 4:   y =  __kernel_sin(pi*(one-y),zero,0); break;
	    case 5:
	    case 6:   y = -__kernel_cos(pi*(y-1.5),zero); break;
	    default:  y =  __kernel_sin(pi*(y-2.0),zero,0); break;
	    }
	return -y;
}


#ifdef __STDC__
	double __ieee754_lgamma_r(double x, int *signgamp)
#else
	double __ieee754_lgamma_r(x,signgamp)
	double x; int *signgamp;
#endif
{
        fd_twoints u;
	double t,y,z,nadj,p,p1,p2,p3,q,r,w;
	int i,hx,lx,ix;

        u.d = x;
	hx = __HI(u);
	lx = __LO(u);

    
	*signgamp = 1;
	ix = hx&0x7fffffff;
	if(ix>=0x7ff00000) return x*x;
	if((ix|lx)==0) return one/zero;
	if(ix<0x3b900000) {	
	    if(hx<0) {
	        *signgamp = -1;
	        return -__ieee754_log(-x);
	    } else return -__ieee754_log(x);
	}
	if(hx<0) {
	    if(ix>=0x43300000) 	
		return one/zero;
	    t = sin_pi(x);
	    if(t==zero) return one/zero; 
	    nadj = __ieee754_log(pi/fd_fabs(t*x));
	    if(t<zero) *signgamp = -1;
	    x = -x;
	}

    
	if((((ix-0x3ff00000)|lx)==0)||(((ix-0x40000000)|lx)==0)) r = 0;
    
	else if(ix<0x40000000) {
	    if(ix<=0x3feccccc) { 	
		r = -__ieee754_log(x);
		if(ix>=0x3FE76944) {y = one-x; i= 0;}
		else if(ix>=0x3FCDA661) {y= x-(tc-one); i=1;}
	  	else {y = x; i=2;}
	    } else {
	  	r = zero;
	        if(ix>=0x3FFBB4C3) {y=2.0-x;i=0;} 
	        else if(ix>=0x3FF3B4C4) {y=x-tc;i=1;} 
		else {y=x-one;i=2;}
	    }
	    switch(i) {
	      case 0:
		z = y*y;
		p1 = a0+z*(a2+z*(a4+z*(a6+z*(a8+z*a10))));
		p2 = z*(a1+z*(a3+z*(a5+z*(a7+z*(a9+z*a11)))));
		p  = y*p1+p2;
		r  += (p-0.5*y); break;
	      case 1:
		z = y*y;
		w = z*y;
		p1 = t0+w*(t3+w*(t6+w*(t9 +w*t12)));	
		p2 = t1+w*(t4+w*(t7+w*(t10+w*t13)));
		p3 = t2+w*(t5+w*(t8+w*(t11+w*t14)));
		p  = z*p1-(tt-w*(p2+y*p3));
		r += (tf + p); break;
	      case 2:	
		p1 = y*(u0+y*(u1+y*(u2+y*(u3+y*(u4+y*u5)))));
		p2 = one+y*(v1+y*(v2+y*(v3+y*(v4+y*v5))));
		r += (-0.5*y + p1/p2);
	    }
	}
	else if(ix<0x40200000) { 			
	    i = (int)x;
	    t = zero;
	    y = x-(double)i;
	    p = y*(s0+y*(s1+y*(s2+y*(s3+y*(s4+y*(s5+y*s6))))));
	    q = one+y*(r1+y*(r2+y*(r3+y*(r4+y*(r5+y*r6)))));
	    r = half*y+p/q;
	    z = one;	
	    switch(i) {
	    case 7: z *= (y+6.0);	
	    case 6: z *= (y+5.0);	
	    case 5: z *= (y+4.0);	
	    case 4: z *= (y+3.0);	
	    case 3: z *= (y+2.0);	
		    r += __ieee754_log(z); break;
	    }
    
	} else if (ix < 0x43900000) {
	    t = __ieee754_log(x);
	    z = one/x;
	    y = z*z;
	    w = w0+z*(w1+y*(w2+y*(w3+y*(w4+y*(w5+y*w6)))));
	    r = (x-half)*(t-one)+w;
	} else 
    
	    r =  x*(__ieee754_log(x)-one);
	if(hx<0) r = nadj - r;
	return r;
}

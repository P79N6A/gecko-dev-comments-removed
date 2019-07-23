












































































#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
invsqrtpi=  5.64189583547756279280e-01, 
two   =  2.00000000000000000000e+00, 
one   =  1.00000000000000000000e+00; 

static double zero  =  0.00000000000000000000e+00;

#ifdef __STDC__
	double __ieee754_jn(int n, double x)
#else
	double __ieee754_jn(n,x)
	int n; double x;
#endif
{
        fd_twoints u;
	int i,hx,ix,lx, sgn;
	double a, b, temp, di;
	double z, w;

    


        u.d = x;
	hx = __HI(u);
	ix = 0x7fffffff&hx;
	lx = __LO(u);
    
	if((ix|((unsigned)(lx|-lx))>>31)>0x7ff00000) return x+x;
	if(n<0){		
		n = -n;
		x = -x;
		hx ^= 0x80000000;
	}
	if(n==0) return(__ieee754_j0(x));
	if(n==1) return(__ieee754_j1(x));
	sgn = (n&1)&(hx>>31);	
	x = fd_fabs(x);
	if((ix|lx)==0||ix>=0x7ff00000) 	
	    b = zero;
	else if((double)n<=x) {   
		
	    if(ix>=0x52D00000) { 
    












		switch(n&3) {
		    case 0: temp =  fd_cos(x)+fd_sin(x); break;
		    case 1: temp = -fd_cos(x)+fd_sin(x); break;
		    case 2: temp = -fd_cos(x)-fd_sin(x); break;
		    case 3: temp =  fd_cos(x)-fd_sin(x); break;
		}
		b = invsqrtpi*temp/fd_sqrt(x);
	    } else {	
	        a = __ieee754_j0(x);
	        b = __ieee754_j1(x);
	        for(i=1;i<n;i++){
		    temp = b;
		    b = b*((double)(i+i)/x) - a; 
		    a = temp;
	        }
	    }
	} else {
	    if(ix<0x3e100000) {	
    


		if(n>33)	
		    b = zero;
		else {
		    temp = x*0.5; b = temp;
		    for (a=one,i=2;i<=n;i++) {
			a *= (double)i;		
			b *= temp;		
		    }
		    b = b/a;
		}
	    } else {
		
		


























	    
		double t,v;
		double q0,q1,h,tmp; int k,m;
		w  = (n+n)/(double)x; h = 2.0/(double)x;
		q0 = w;  z = w+h; q1 = w*z - 1.0; k=1;
		while(q1<1.0e9) {
			k += 1; z += h;
			tmp = z*q1 - q0;
			q0 = q1;
			q1 = tmp;
		}
		m = n+n;
		for(t=zero, i = 2*(n+k); i>=m; i -= 2) t = one/(i/x-t);
		a = t;
		b = one;
		







		tmp = n;
		v = two/x;
		tmp = tmp*__ieee754_log(fd_fabs(v*tmp));
		if(tmp<7.09782712893383973096e+02) {
	    	    for(i=n-1,di=(double)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
	     	    }
		} else {
	    	    for(i=n-1,di=(double)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
		    
			if(b>1e100) {
			    a /= b;
			    t /= b;
			    b  = one;
			}
	     	    }
		}
	    	b = (t*__ieee754_j0(x)/b);
	    }
	}
	if(sgn==1) return -b; else return b;
}

#ifdef __STDC__
	double __ieee754_yn(int n, double x) 
#else
	double __ieee754_yn(n,x) 
	int n; double x;
#endif
{
        fd_twoints u;
	int i,hx,ix,lx;
	int sign;
	double a, b, temp;

        u.d = x;
	hx = __HI(u);
	ix = 0x7fffffff&hx;
	lx = __LO(u);
    
	if((ix|((unsigned)(lx|-lx))>>31)>0x7ff00000) return x+x;
	if((ix|lx)==0) return -one/zero;
	if(hx<0) return zero/zero;
	sign = 1;
	if(n<0){
		n = -n;
		sign = 1 - ((n&1)<<1);
	}
	if(n==0) return(__ieee754_y0(x));
	if(n==1) return(sign*__ieee754_y1(x));
	if(ix==0x7ff00000) return zero;
	if(ix>=0x52D00000) { 
    












		switch(n&3) {
		    case 0: temp =  fd_sin(x)-fd_cos(x); break;
		    case 1: temp = -fd_sin(x)-fd_cos(x); break;
		    case 2: temp = -fd_sin(x)+fd_cos(x); break;
		    case 3: temp =  fd_sin(x)+fd_cos(x); break;
		}
		b = invsqrtpi*temp/fd_sqrt(x);
	} else {
	    a = __ieee754_y0(x);
	    b = __ieee754_y1(x);
	
            u.d = b;
	    for(i=1;i<n&&(__HI(u) != 0xfff00000);i++){ 
		temp = b;
		b = ((double)(i+i)/x)*b - a;
		a = temp;
	    }
	}
	if(sign>0) return b; else return -b;
}

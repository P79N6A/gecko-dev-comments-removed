































































































#include "fdlibm.h"

#if defined(_MSC_VER)

#pragma warning( disable : 4723 ) /* disables potential divide by 0 warning */
#endif

#ifdef __STDC__
static const double 
#else
static double 
#endif
bp[] = {1.0, 1.5,},
dp_h[] = { 0.0, 5.84962487220764160156e-01,}, 
dp_l[] = { 0.0, 1.35003920212974897128e-08,}, 
zero    =  0.0,
one	=  1.0,
two	=  2.0,
two53	=  9007199254740992.0,	
really_big	=  1.0e300,
tiny    =  1.0e-300,
	
L1  =  5.99999999999994648725e-01, 
L2  =  4.28571428578550184252e-01, 
L3  =  3.33333329818377432918e-01, 
L4  =  2.72728123808534006489e-01, 
L5  =  2.30660745775561754067e-01, 
L6  =  2.06975017800338417784e-01, 
P1   =  1.66666666666666019037e-01, 
P2   = -2.77777777770155933842e-03, 
P3   =  6.61375632143793436117e-05, 
P4   = -1.65339022054652515390e-06, 
P5   =  4.13813679705723846039e-08, 
lg2  =  6.93147180559945286227e-01, 
lg2_h  =  6.93147182464599609375e-01, 
lg2_l  = -1.90465429995776804525e-09, 
ovt =  8.0085662595372944372e-0017, 
cp    =  9.61796693925975554329e-01, 
cp_h  =  9.61796700954437255859e-01, 
cp_l  = -7.02846165095275826516e-09, 
ivln2    =  1.44269504088896338700e+00, 
ivln2_h  =  1.44269502162933349609e+00, 
ivln2_l  =  1.92596299112661746887e-08; 

#ifdef __STDC__
	double __ieee754_pow(double x, double y)
#else
	double __ieee754_pow(x,y)
	double x, y;
#endif
{
        fd_twoints ux, uy, uz;
        double y1,t1,p_h,t,z,ax;
	double z_h,z_l,p_l;
	double t2,r,s,u,v,w;
	int i,j,k,yisint,n;
	int hx,hy,ix,iy;
	unsigned lx,ly;

        ux.d = x; uy.d = y;
	hx = __HI(ux); lx = __LO(ux);
	hy = __HI(uy); ly = __LO(uy);
	ix = hx&0x7fffffff;  iy = hy&0x7fffffff;

    
	if((iy|ly)==0) return one; 	

    
	if(ix > 0x7ff00000 || ((ix==0x7ff00000)&&(lx!=0)) ||
	   iy > 0x7ff00000 || ((iy==0x7ff00000)&&(ly!=0))) 
		return x+y;	

    




	yisint  = 0;
	if(hx<0) {	
	    if(iy>=0x43400000) yisint = 2; 
	    else if(iy>=0x3ff00000) {
		k = (iy>>20)-0x3ff;	   
		if(k>20) {
		    j = ly>>(52-k);
		    if((j<<(52-k))==(int)ly) yisint = 2-(j&1);
		} else if(ly==0) {
		    j = iy>>(20-k);
		    if((j<<(20-k))==iy) yisint = 2-(j&1);
		}
	    }		
	} 

    
	if(ly==0) { 	
	    if (iy==0x7ff00000) {	
	        if(((ix-0x3ff00000)|lx)==0)
#ifdef _WIN32
 
                    return y / y;
#else
		    return  y - y;	
#endif
	        else if (ix >= 0x3ff00000)
		    return (hy>=0)? y: zero;
	        else			
		    return (hy<0)?-y: zero;
	    } 
	    if(iy==0x3ff00000) {	
		if(hy<0) return one/x; else return x;
	    }
	    if(hy==0x40000000) return x*x; 
	    if(hy==0x3fe00000) {	
		if(hx>=0)	
		return fd_sqrt(x);	
	    }
	}

	ax   = fd_fabs(x);
    
	if(lx==0) {
	    if(ix==0x7ff00000||ix==0||ix==0x3ff00000){
		z = ax;			
		if(hy<0) z = one/z;	
		if(hx<0) {
		    if(((ix-0x3ff00000)|yisint)==0) {
			z = (z-z)/(z-z); 
		    } else if(yisint==1) {
#ifdef HPUX
                        uz.d = z;
			__HI(uz) ^= 1<<31; 
                        z = uz.d;
#else
			z = -z;		
#endif
			}
		}
		return z;
	    }
	}
    
    
	if((((hx>>31)+1)|yisint)==0) return (x-x)/(x-x);

    
	if(iy>0x41e00000) { 
	    if(iy>0x43f00000){	
		if(ix<=0x3fefffff) return (hy<0)? really_big*really_big:tiny*tiny;
		if(ix>=0x3ff00000) return (hy>0)? really_big*really_big:tiny*tiny;
	    }
	
	    if(ix<0x3fefffff) return (hy<0)? really_big*really_big:tiny*tiny;
	    if(ix>0x3ff00000) return (hy>0)? really_big*really_big:tiny*tiny;
	

	    t = x-1;		
	    w = (t*t)*(0.5-t*(0.3333333333333333333333-t*0.25));
	    u = ivln2_h*t;	
	    v = t*ivln2_l-w*ivln2;
	    t1 = u+v;
            uz.d = t1;
	    __LO(uz) = 0;
            t1 = uz.d;
	    t2 = v-(t1-u);
	} else {
	    double s_h,t_h;
	    double s2,s_l,t_l;
	    n = 0;
	
	    if(ix<0x00100000) 
		{ax *= two53; n -= 53; uz.d = ax; ix = __HI(uz); }
	    n  += ((ix)>>20)-0x3ff;
	    j  = ix&0x000fffff;
	
	    ix = j|0x3ff00000;		
	    if(j<=0x3988E) k=0;		
	    else if(j<0xBB67A) k=1;	
	    else {k=0;n+=1;ix -= 0x00100000;}
            uz.d = ax;
	    __HI(uz) = ix;
            ax = uz.d;

	
	    u = ax-bp[k];		
	    v = one/(ax+bp[k]);
	    s = u*v;
	    s_h = s;
            uz.d = s_h;
	    __LO(uz) = 0;
            s_h = uz.d;
	
	    t_h = zero;
            uz.d = t_h;
	    __HI(uz)=((ix>>1)|0x20000000)+0x00080000+(k<<18); 
            t_h = uz.d;
	    t_l = ax - (t_h-bp[k]);
	    s_l = v*((u-s_h*t_h)-s_h*t_l);
	
	    s2 = s*s;
	    r = s2*s2*(L1+s2*(L2+s2*(L3+s2*(L4+s2*(L5+s2*L6)))));
	    r += s_l*(s_h+s);
	    s2  = s_h*s_h;
	    t_h = 3.0+s2+r;
            uz.d = t_h;
	    __LO(uz) = 0;
            t_h = uz.d;
	    t_l = r-((t_h-3.0)-s2);
	
	    u = s_h*t_h;
	    v = s_l*t_h+t_l*s;
	
	    p_h = u+v;
            uz.d = p_h;
	    __LO(uz) = 0;
            p_h = uz.d;
	    p_l = v-(p_h-u);
	    z_h = cp_h*p_h;		
	    z_l = cp_l*p_h+p_l*cp+dp_l[k];
	
	    t = (double)n;
	    t1 = (((z_h+z_l)+dp_h[k])+t);
            uz.d = t1;
	    __LO(uz) = 0;
            t1 = uz.d;
	    t2 = z_l-(((t1-t)-dp_h[k])-z_h);
	}

	s = one; 
	if((((hx>>31)+1)|(yisint-1))==0) s = -one;

    
	y1  = y;
        uy.d = y1;
	__LO(uy) = 0;
        y1 = uy.d;
	p_l = (y-y1)*t1+y*t2;
	p_h = y1*t1;
	z = p_l+p_h;
        uz.d = z;
	j = __HI(uz);
	i = __LO(uz);

	if (j>=0x40900000) {				
	    if(((j-0x40900000)|i)!=0)			
		return s*really_big*really_big;			
	    else {
		if(p_l+ovt>z-p_h) return s*really_big*really_big;	
	    }
	} else if((j&0x7fffffff)>=0x4090cc00 ) {	
	    if(((j-0xc090cc00)|i)!=0) 		
		return s*tiny*tiny;		
	    else {
		if(p_l<=z-p_h) return s*tiny*tiny;	
	    }
	}
    


	i = j&0x7fffffff;
	k = (i>>20)-0x3ff;
	n = 0;
	if(i>0x3fe00000) {		
	    n = j+(0x00100000>>(k+1));
	    k = ((n&0x7fffffff)>>20)-0x3ff;	
	    t = zero;
            uz.d = t;
	    __HI(uz) = (n&~(0x000fffff>>k));
            t = uz.d;
	    n = ((n&0x000fffff)|0x00100000)>>(20-k);
	    if(j<0) n = -n;
	    p_h -= t;
	} 
	t = p_l+p_h;
        uz.d = t;
	__LO(uz) = 0;
        t = uz.d;
	u = t*lg2_h;
	v = (p_l-(t-p_h))*lg2+t*lg2_l;
	z = u+v;
	w = v-(z-u);
	t  = z*z;
	t1  = z - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
	r  = (z*t1)/(t1-two)-(w+z*w);
	z  = one-(r-z);
        uz.d = z;
	j  = __HI(uz);
	j += (n<<20);
	if((j>>20)<=0) z = fd_scalbn(z,n);	
	else { uz.d = z; __HI(uz) += (n<<20); z = uz.d; }
	return s*z;
}

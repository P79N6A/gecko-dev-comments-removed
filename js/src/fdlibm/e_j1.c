































































































#include "fdlibm.h"

#ifdef __STDC__
static double pone(double), qone(double);
#else
static double pone(), qone();
#endif

#ifdef __STDC__
static const double 
#else
static double 
#endif
really_big    = 1e300,
one	= 1.0,
invsqrtpi=  5.64189583547756279280e-01, 
tpi      =  6.36619772367581382433e-01, 
	
r00  = -6.25000000000000000000e-02, 
r01  =  1.40705666955189706048e-03, 
r02  = -1.59955631084035597520e-05, 
r03  =  4.96727999609584448412e-08, 
s01  =  1.91537599538363460805e-02, 
s02  =  1.85946785588630915560e-04, 
s03  =  1.17718464042623683263e-06, 
s04  =  5.04636257076217042715e-09, 
s05  =  1.23542274426137913908e-11; 

static double zero    = 0.0;

#ifdef __STDC__
	double __ieee754_j1(double x) 
#else
	double __ieee754_j1(x) 
	double x;
#endif
{
        fd_twoints un;
	double z, s,c,ss,cc,r,u,v,y;
	int hx,ix;

        un.d = x;
	hx = __HI(un);
	ix = hx&0x7fffffff;
	if(ix>=0x7ff00000) return one/x;
	y = fd_fabs(x);
	if(ix >= 0x40000000) {	
		s = fd_sin(y);
		c = fd_cos(y);
		ss = -s-c;
		cc = s-c;
		if(ix<0x7fe00000) {  
		    z = fd_cos(y+y);
		    if ((s*c)>zero) cc = z/ss;
		    else 	    ss = z/cc;
		}
	



		if(ix>0x48000000) z = (invsqrtpi*cc)/fd_sqrt(y);
		else {
		    u = pone(y); v = qone(y);
		    z = invsqrtpi*(u*cc-v*ss)/fd_sqrt(y);
		}
		if(hx<0) return -z;
		else  	 return  z;
	}
	if(ix<0x3e400000) {	
	    if(really_big+x>one) return 0.5*x;
	}
	z = x*x;
	r =  z*(r00+z*(r01+z*(r02+z*r03)));
	s =  one+z*(s01+z*(s02+z*(s03+z*(s04+z*s05))));
	r *= x;
	return(x*0.5+r/s);
}

#ifdef __STDC__
static const double U0[5] = {
#else
static double U0[5] = {
#endif
 -1.96057090646238940668e-01, 
  5.04438716639811282616e-02, 
 -1.91256895875763547298e-03, 
  2.35252600561610495928e-05, 
 -9.19099158039878874504e-08, 
};
#ifdef __STDC__
static const double V0[5] = {
#else
static double V0[5] = {
#endif
  1.99167318236649903973e-02, 
  2.02552581025135171496e-04, 
  1.35608801097516229404e-06, 
  6.22741452364621501295e-09, 
  1.66559246207992079114e-11, 
};

#ifdef __STDC__
	double __ieee754_y1(double x) 
#else
	double __ieee754_y1(x) 
	double x;
#endif
{
        fd_twoints un;
	double z, s,c,ss,cc,u,v;
	int hx,ix,lx;

        un.d = x;
        hx = __HI(un);
        ix = 0x7fffffff&hx;
        lx = __LO(un);
    
	if(ix>=0x7ff00000) return  one/(x+x*x); 
        if((ix|lx)==0) return -one/zero;
        if(hx<0) return zero/zero;
        if(ix >= 0x40000000) {  
                s = fd_sin(x);
                c = fd_cos(x);
                ss = -s-c;
                cc = s-c;
                if(ix<0x7fe00000) {  
                    z = fd_cos(x+x);
                    if ((s*c)>zero) cc = z/ss;
                    else            ss = z/cc;
                }
        










                if(ix>0x48000000) z = (invsqrtpi*ss)/fd_sqrt(x);
                else {
                    u = pone(x); v = qone(x);
                    z = invsqrtpi*(u*ss+v*cc)/fd_sqrt(x);
                }
                return z;
        } 
        if(ix<=0x3c900000) {    
            return(-tpi/x);
        } 
        z = x*x;
        u = U0[0]+z*(U0[1]+z*(U0[2]+z*(U0[3]+z*U0[4])));
        v = one+z*(V0[0]+z*(V0[1]+z*(V0[2]+z*(V0[3]+z*V0[4]))));
        return(x*(u/v) + tpi*(__ieee754_j1(x)*__ieee754_log(x)-one/x));
}











#ifdef __STDC__
static const double pr8[6] = { 
#else
static double pr8[6] = { 
#endif
  0.00000000000000000000e+00, 
  1.17187499999988647970e-01, 
  1.32394806593073575129e+01, 
  4.12051854307378562225e+02, 
  3.87474538913960532227e+03, 
  7.91447954031891731574e+03, 
};
#ifdef __STDC__
static const double ps8[5] = {
#else
static double ps8[5] = {
#endif
  1.14207370375678408436e+02, 
  3.65093083420853463394e+03, 
  3.69562060269033463555e+04, 
  9.76027935934950801311e+04, 
  3.08042720627888811578e+04, 
};

#ifdef __STDC__
static const double pr5[6] = { 
#else
static double pr5[6] = { 
#endif
  1.31990519556243522749e-11, 
  1.17187493190614097638e-01, 
  6.80275127868432871736e+00, 
  1.08308182990189109773e+02, 
  5.17636139533199752805e+02, 
  5.28715201363337541807e+02, 
};
#ifdef __STDC__
static const double ps5[5] = {
#else
static double ps5[5] = {
#endif
  5.92805987221131331921e+01, 
  9.91401418733614377743e+02, 
  5.35326695291487976647e+03, 
  7.84469031749551231769e+03, 
  1.50404688810361062679e+03, 
};

#ifdef __STDC__
static const double pr3[6] = {
#else
static double pr3[6] = {
#endif
  3.02503916137373618024e-09, 
  1.17186865567253592491e-01, 
  3.93297750033315640650e+00, 
  3.51194035591636932736e+01, 
  9.10550110750781271918e+01, 
  4.85590685197364919645e+01, 
};
#ifdef __STDC__
static const double ps3[5] = {
#else
static double ps3[5] = {
#endif
  3.47913095001251519989e+01, 
  3.36762458747825746741e+02, 
  1.04687139975775130551e+03, 
  8.90811346398256432622e+02, 
  1.03787932439639277504e+02, 
};

#ifdef __STDC__
static const double pr2[6] = {
#else
static double pr2[6] = {
#endif
  1.07710830106873743082e-07, 
  1.17176219462683348094e-01, 
  2.36851496667608785174e+00, 
  1.22426109148261232917e+01, 
  1.76939711271687727390e+01, 
  5.07352312588818499250e+00, 
};
#ifdef __STDC__
static const double ps2[5] = {
#else
static double ps2[5] = {
#endif
  2.14364859363821409488e+01, 
  1.25290227168402751090e+02, 
  2.32276469057162813669e+02, 
  1.17679373287147100768e+02, 
  8.36463893371618283368e+00, 
};

#ifdef __STDC__
	static double pone(double x)
#else
	static double pone(x)
	double x;
#endif
{
#ifdef __STDC__
	const double *p,*q;
#else
	double *p,*q;
#endif
        fd_twoints un;
	double z,r,s;
        int ix;
        un.d = x;
        ix = 0x7fffffff&__HI(un);
        if(ix>=0x40200000)     {p = pr8; q= ps8;}
        else if(ix>=0x40122E8B){p = pr5; q= ps5;}
        else if(ix>=0x4006DB6D){p = pr3; q= ps3;}
        else if(ix>=0x40000000){p = pr2; q= ps2;}
        z = one/(x*x);
        r = p[0]+z*(p[1]+z*(p[2]+z*(p[3]+z*(p[4]+z*p[5]))));
        s = one+z*(q[0]+z*(q[1]+z*(q[2]+z*(q[3]+z*q[4]))));
        return one+ r/s;
}
		











#ifdef __STDC__
static const double qr8[6] = { 
#else
static double qr8[6] = { 
#endif
  0.00000000000000000000e+00, 
 -1.02539062499992714161e-01, 
 -1.62717534544589987888e+01, 
 -7.59601722513950107896e+02, 
 -1.18498066702429587167e+04, 
 -4.84385124285750353010e+04, 
};
#ifdef __STDC__
static const double qs8[6] = {
#else
static double qs8[6] = {
#endif
  1.61395369700722909556e+02, 
  7.82538599923348465381e+03, 
  1.33875336287249578163e+05, 
  7.19657723683240939863e+05, 
  6.66601232617776375264e+05, 
 -2.94490264303834643215e+05, 
};

#ifdef __STDC__
static const double qr5[6] = { 
#else
static double qr5[6] = { 
#endif
 -2.08979931141764104297e-11, 
 -1.02539050241375426231e-01, 
 -8.05644828123936029840e+00, 
 -1.83669607474888380239e+02, 
 -1.37319376065508163265e+03, 
 -2.61244440453215656817e+03, 
};
#ifdef __STDC__
static const double qs5[6] = {
#else
static double qs5[6] = {
#endif
  8.12765501384335777857e+01, 
  1.99179873460485964642e+03, 
  1.74684851924908907677e+04, 
  4.98514270910352279316e+04, 
  2.79480751638918118260e+04, 
 -4.71918354795128470869e+03, 
};

#ifdef __STDC__
static const double qr3[6] = {
#else
static double qr3[6] = {
#endif
 -5.07831226461766561369e-09, 
 -1.02537829820837089745e-01, 
 -4.61011581139473403113e+00, 
 -5.78472216562783643212e+01, 
 -2.28244540737631695038e+02, 
 -2.19210128478909325622e+02, 
};
#ifdef __STDC__
static const double qs3[6] = {
#else
static double qs3[6] = {
#endif
  4.76651550323729509273e+01, 
  6.73865112676699709482e+02, 
  3.38015286679526343505e+03, 
  5.54772909720722782367e+03, 
  1.90311919338810798763e+03, 
 -1.35201191444307340817e+02, 
};

#ifdef __STDC__
static const double qr2[6] = {
#else
static double qr2[6] = {
#endif
 -1.78381727510958865572e-07, 
 -1.02517042607985553460e-01, 
 -2.75220568278187460720e+00, 
 -1.96636162643703720221e+01, 
 -4.23253133372830490089e+01, 
 -2.13719211703704061733e+01, 
};
#ifdef __STDC__
static const double qs2[6] = {
#else
static double qs2[6] = {
#endif
  2.95333629060523854548e+01, 
  2.52981549982190529136e+02, 
  7.57502834868645436472e+02, 
  7.39393205320467245656e+02, 
  1.55949003336666123687e+02, 
 -4.95949898822628210127e+00, 
};

#ifdef __STDC__
	static double qone(double x)
#else
	static double qone(x)
	double x;
#endif
{
#ifdef __STDC__
	const double *p,*q;
#else
	double *p,*q;
#endif
        fd_twoints un;
	double  s,r,z;
	int ix;
        un.d = x;
	ix = 0x7fffffff&__HI(un);
	if(ix>=0x40200000)     {p = qr8; q= qs8;}
	else if(ix>=0x40122E8B){p = qr5; q= qs5;}
	else if(ix>=0x4006DB6D){p = qr3; q= qs3;}
	else if(ix>=0x40000000){p = qr2; q= qs2;}
	z = one/(x*x);
	r = p[0]+z*(p[1]+z*(p[2]+z*(p[3]+z*(p[4]+z*p[5]))));
	s = one+z*(q[0]+z*(q[1]+z*(q[2]+z*(q[3]+z*(q[4]+z*q[5])))));
	return (.375 + r/s)/x;
}
















































































#include "fdlibm.h"

#ifdef __STDC__
static const double 
#else
static double 
#endif
half =  5.00000000000000000000e-01, 
S1  = -1.66666666666666324348e-01, 
S2  =  8.33333333332248946124e-03, 
S3  = -1.98412698298579493134e-04, 
S4  =  2.75573137070700676789e-06, 
S5  = -2.50507602534068634195e-08, 
S6  =  1.58969099521155010221e-10; 

#ifdef __STDC__
	double __kernel_sin(double x, double y, int iy)
#else
	double __kernel_sin(x, y, iy)
	double x,y; int iy;		
#endif
{
        fd_twoints u;
	double z,r,v;
	int ix;
        u.d = x;
	ix = __HI(u)&0x7fffffff;	
	if(ix<0x3e400000)			
	   {if((int)x==0) return x;}		
	z	=  x*x;
	v	=  z*x;
	r	=  S2+z*(S3+z*(S4+z*(S5+z*S6)));
	if(iy==0) return x+v*(S1+z*r);
	else      return x-((z*(half*y-v*r)-y)-v*S1);
}

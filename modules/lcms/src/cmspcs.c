























#include "lcms.h"









































































static
float CubeRoot(float x)
{
       float fr, r;
       int ex, shx;

       
       fr = (float) frexp(x, &ex); 
       shx = ex % 3;

       if (shx > 0)
              shx -= 3; 

       ex = (ex - shx) / 3;        
       fr = (float) ldexp(fr, shx);

       

#ifdef ITERATE
       

       fr = (-0.46946116F * fr + 1.072302F) * fr + 0.3812513F;
       r = ldexp(fr, ex);          

       

       r = (float)(2.0/3.0) * r + (float)(1.0/3.0) * x / (r * r); 
       r = (float)(2.0/3.0) * r + (float)(1.0/3.0) * x / (r * r); 
#else 

       

       fr = (float) (((((45.2548339756803022511987494 * fr +
       192.2798368355061050458134625) * fr +
       119.1654824285581628956914143) * fr +
       13.43250139086239872172837314) * fr +
       0.1636161226585754240958355063)
       /
       ((((14.80884093219134573786480845 * fr +
       151.9714051044435648658557668) * fr +
       168.5254414101568283957668343) * fr +
       33.9905941350215598754191872) * fr +
       1.0));
       r = (float) ldexp(fr, ex); 
#endif
       return r;
}

static
double f(double t)
{

      const double Limit = (24.0/116.0) * (24.0/116.0) * (24.0/116.0);

       if (t <= Limit)
              return (841.0/108.0) * t + (16.0/116.0);
       else
              return CubeRoot((float) t); 
}


static
double f_1(double t)
{
       const double Limit = (24.0/116.0);

       if (t <= Limit)
       {
              double tmp;

              tmp = (108.0/841.0) * (t - (16.0/116.0));
              if (tmp <= 0.0) return 0.0;
              else return tmp;
       }

       return t * t * t;
}



void LCMSEXPORT cmsXYZ2Lab(LPcmsCIEXYZ WhitePoint, LPcmsCIELab Lab, const cmsCIEXYZ* xyz)
{
       double fx, fy, fz;

       if (xyz -> X == 0 && xyz -> Y == 0 && xyz -> Z == 0)
       {
        Lab -> L = 0;
        Lab -> a = 0;
        Lab -> b = 0;
        return;
       }

       if (WhitePoint == NULL) 
            WhitePoint = cmsD50_XYZ();

       fx = f(xyz->X / WhitePoint->X);
       fy = f(xyz->Y / WhitePoint->Y);
       fz = f(xyz->Z / WhitePoint->Z);

       Lab->L = 116.0* fy - 16.;

       Lab->a = 500.0*(fx - fy);
       Lab->b = 200.0*(fy - fz);
}



void cmsXYZ2LabEncoded(WORD XYZ[3], WORD Lab[3])
{
       Fixed32 X, Y, Z;
       double x, y, z, L, a, b;
       double fx, fy, fz;
       Fixed32 wL, wa, wb;

       X = (Fixed32) XYZ[0] << 1;
       Y = (Fixed32) XYZ[1] << 1;
       Z = (Fixed32) XYZ[2] << 1;


       if (X==0 && Y==0 && Z==0) {

                     Lab[0] = 0;
                     Lab[1] = Lab[2] =  0x8000; 
                     return;
       }

       


       x = FIXED_TO_DOUBLE(X) / D50X;
       y = FIXED_TO_DOUBLE(Y) / D50Y;
       z = FIXED_TO_DOUBLE(Z) / D50Z;


       fx = f(x);
       fy = f(y);
       fz = f(z);

       L = 116.* fy - 16.;

       a = 500.*(fx - fy);
       b = 200.*(fy - fz);

       a += 128.;
       b += 128.;

       wL = (int) (L * 652.800 + .5);
       wa = (int) (a * 256.0   + .5);
       wb = (int) (b * 256.0   + .5);


       Lab[0] = Clamp_L(wL);
       Lab[1] = Clamp_ab(wa);
       Lab[2] = Clamp_ab(wb);


}






void LCMSEXPORT cmsLab2XYZ(LPcmsCIEXYZ WhitePoint, LPcmsCIEXYZ xyz,  const cmsCIELab* Lab)
{
        double x, y, z;

        if (Lab -> L <= 0) {
               xyz -> X = 0;
               xyz -> Y = 0;
               xyz -> Z = 0;
               return;
        }


       if (WhitePoint == NULL) 
            WhitePoint = cmsD50_XYZ();

       y = (Lab-> L + 16.0) / 116.0;
       x = y + 0.002 * Lab -> a;
       z = y - 0.005 * Lab -> b;

       xyz -> X = f_1(x) * WhitePoint -> X;
       xyz -> Y = f_1(y) * WhitePoint -> Y;
       xyz -> Z = f_1(z) * WhitePoint -> Z;

}



void cmsLab2XYZEncoded(WORD Lab[3], WORD XYZ[3])
{
       double L, a, b;
       double X, Y, Z, x, y, z;


       L = ((double) Lab[0] * 100.0) / 65280.0;
       if (L==0.0) {

       XYZ[0] = 0; XYZ[1] = 0; XYZ[2] = 0;
       return;
       }

       a = ((double) Lab[1] / 256.0) - 128.0;
       b = ((double) Lab[2] / 256.0) - 128.0;

       y = (L + 16.) / 116.0;
       x = y + 0.002 * a;
       z = y - 0.005 * b;

       X = f_1(x) * D50X;
       Y = f_1(y) * D50Y;
       Z = f_1(z) * D50Z;

       

       
       XYZ[0] = _cmsClampWord((int) floor(X * 32768.0 + 0.5));
       XYZ[1] = _cmsClampWord((int) floor(Y * 32768.0 + 0.5));
       XYZ[2] = _cmsClampWord((int) floor(Z * 32768.0 + 0.5));
       

}

static
double L2float3(WORD v)
{
       Fixed32 fix32;

       fix32 = (Fixed32) v;
       return (double) fix32 / 652.800;
}




static
double ab2float3(WORD v)
{
       Fixed32 fix32;

       fix32 = (Fixed32) v;
       return ((double) fix32/256.0)-128.0;
}

static
WORD L2Fix3(double L)
{
        return (WORD) (L *  652.800 + 0.5);
}

static
WORD ab2Fix3(double ab)
{
        return (WORD) ((ab + 128.0) * 256.0 + 0.5);
}




static 
WORD L2Fix4(double L)
{
     return (WORD) (L *  655.35 + 0.5);
}

static
WORD ab2Fix4(double ab)
{
        return (WORD) ((ab + 128.0) * 257.0 + 0.5);
}

static
double L2float4(WORD v)
{
       Fixed32 fix32;

       fix32 = (Fixed32) v;
       return (double) fix32 / 655.35;
}




static
double ab2float4(WORD v)
{
       Fixed32 fix32;

       fix32 = (Fixed32) v;
       return ((double) fix32/257.0)-128.0;
}


void LCMSEXPORT cmsLabEncoded2Float(LPcmsCIELab Lab, const WORD wLab[3])
{
        Lab->L = L2float3(wLab[0]);
        Lab->a = ab2float3(wLab[1]);
        Lab->b = ab2float3(wLab[2]);
}


void LCMSEXPORT cmsLabEncoded2Float4(LPcmsCIELab Lab, const WORD wLab[3])
{
        Lab->L = L2float4(wLab[0]);
        Lab->a = ab2float4(wLab[1]);
        Lab->b = ab2float4(wLab[2]);
}

static
double Clamp_L_double(double L)
{
    if (L < 0) L = 0;
    if (L > 100) L = 100;

    return L;
}


static
double Clamp_ab_double(double ab)
{
    if (ab < -128) ab = -128.0;
    if (ab > +127.9961) ab = +127.9961;

    return ab;
}

void LCMSEXPORT cmsFloat2LabEncoded(WORD wLab[3], const cmsCIELab* fLab)
{
    cmsCIELab Lab;

    
    Lab.L = Clamp_L_double(fLab ->L);
    Lab.a = Clamp_ab_double(fLab ->a);
    Lab.b = Clamp_ab_double(fLab ->b);
                                                
    wLab[0] = L2Fix3(Lab.L);
    wLab[1] = ab2Fix3(Lab.a);
    wLab[2] = ab2Fix3(Lab.b);
}


void LCMSEXPORT cmsFloat2LabEncoded4(WORD wLab[3], const cmsCIELab* fLab)
{
    cmsCIELab Lab;

    
    Lab.L = fLab ->L;
    Lab.a = fLab ->a;
    Lab.b = fLab ->b;
                                

    if (Lab.L < 0) Lab.L = 0;
    if (Lab.L > 100.) Lab.L = 100.;

    if (Lab.a < -128.) Lab.a = -128.;
    if (Lab.a > 127.) Lab.a = 127.;
    if (Lab.b < -128.) Lab.b = -128.;
    if (Lab.b > 127.) Lab.b = 127.;
                

    wLab[0] = L2Fix4(Lab.L);
    wLab[1] = ab2Fix4(Lab.a);
    wLab[2] = ab2Fix4(Lab.b);
}




void LCMSEXPORT cmsLab2LCh(LPcmsCIELCh LCh, const cmsCIELab* Lab)
{
    double a, b;

    LCh -> L = Clamp_L_double(Lab -> L);

    a = Clamp_ab_double(Lab -> a);
    b = Clamp_ab_double(Lab -> b);

    LCh -> C = pow(a * a + b * b, 0.5);

    if (a == 0 && b == 0)
            LCh -> h   = 0;
    else
            LCh -> h = atan2(b, a);
    

    LCh -> h *= (180. / M_PI);

    
    while (LCh -> h >= 360.)         
                LCh -> h -= 360.;

    while (LCh -> h < 0)
                LCh -> h += 360.;    

}




void LCMSEXPORT cmsLCh2Lab(LPcmsCIELab Lab, const cmsCIELCh* LCh)
{
        
    double h = (LCh -> h * M_PI) / 180.0;
    
    Lab -> L = Clamp_L_double(LCh -> L);
    Lab -> a = Clamp_ab_double(LCh -> C * cos(h));
    Lab -> b = Clamp_ab_double(LCh -> C * sin(h));          
    
}







static
WORD XYZ2Fix(double d)
{     
    return (WORD) floor(d * 32768.0 + 0.5);
}


void LCMSEXPORT cmsFloat2XYZEncoded(WORD XYZ[3], const cmsCIEXYZ* fXYZ)
{
    cmsCIEXYZ xyz;
    
    xyz.X = fXYZ -> X;
    xyz.Y = fXYZ -> Y;
    xyz.Z = fXYZ -> Z;


    
    

    
    if (xyz.Y <= 0) {

                xyz.X = 0;
                xyz.Y = 0;
                xyz.Z = 0;
    }
    
    
    if (xyz.X > 1.99996)            
           xyz.X = 1.99996;
    
    if (xyz.X < 0)
           xyz.X = 0;

    if (xyz.Y > 1.99996)            
                xyz.Y = 1.99996;
    
    if (xyz.Y < 0)
           xyz.Y = 0;


    if (xyz.Z > 1.99996)            
                xyz.Z = 1.99996;
    
    if (xyz.Z < 0)
           xyz.Z = 0;

        

    XYZ[0] = XYZ2Fix(xyz.X);
    XYZ[1] = XYZ2Fix(xyz.Y);
    XYZ[2] = XYZ2Fix(xyz.Z);        
    
}




static
double XYZ2float(WORD v)
{
       Fixed32 fix32;

       

       fix32 = v << 1;

       

       return FIXED_TO_DOUBLE(fix32);
}


void LCMSEXPORT cmsXYZEncoded2Float(LPcmsCIEXYZ fXYZ, const WORD XYZ[3])
{

    fXYZ -> X = XYZ2float(XYZ[0]);
    fXYZ -> Y = XYZ2float(XYZ[1]);
    fXYZ -> Z = XYZ2float(XYZ[2]);

}       





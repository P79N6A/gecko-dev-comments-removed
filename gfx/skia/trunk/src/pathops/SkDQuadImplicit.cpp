





#include "SkDQuadImplicit.h"












































static bool straight_forward = false;

SkDQuadImplicit::SkDQuadImplicit(const SkDQuad& q) {
    double a, b, c;
    SkDQuad::SetABC(&q[0].fX, &a, &b, &c);
    double d, e, f;
    SkDQuad::SetABC(&q[0].fY, &d, &e, &f);
    
    if (straight_forward) {  
        fP[kXx_Coeff] = d * d;
        fP[kXy_Coeff] = -2 * a * d;
        fP[kYy_Coeff] = a * a;
        fP[kX_Coeff] = -2*c*d*d + b*e*d - a*e*e + 2*a*f*d;
        fP[kY_Coeff] = -2*f*a*a + e*b*a - d*b*b + 2*d*c*a;
        fP[kC_Coeff] = a*(a*f*f + c*e*e - c*f*d - b*e*f)
                   + d*(b*b*f + c*c*d - c*a*f - c*e*b);
    } else {  
        double aa = a * a;
        double ad = a * d;
        double dd = d * d;
        fP[kXx_Coeff] = dd;
        fP[kXy_Coeff] = -2 * ad;
        fP[kYy_Coeff] = aa;
        double be = b * e;
        double bde = be * d;
        double cdd = c * dd;
        double ee = e * e;
        fP[kX_Coeff] =  -2*cdd + bde - a*ee + 2*ad*f;
        double aaf = aa * f;
        double abe = a * be;
        double ac = a * c;
        double bb_2ac = b*b - 2*ac;
        fP[kY_Coeff] = -2*aaf + abe - d*bb_2ac;
        fP[kC_Coeff] = aaf*f + ac*ee + d*f*bb_2ac - abe*f + c*cdd - c*bde;
    }
}

 






bool SkDQuadImplicit::match(const SkDQuadImplicit& p2) const {
    int first = 0;
    for (int index = 0; index <= kC_Coeff; ++index) {
        if (approximately_zero(fP[index]) && approximately_zero(p2.fP[index])) {
            first += first == index;
            continue;
        }
        if (first == index) {
            continue;
        }
        if (!AlmostDequalUlps(fP[index] * p2.fP[first], fP[first] * p2.fP[index])) {
            return false;
        }
    }
    return true;
}

bool SkDQuadImplicit::Match(const SkDQuad& quad1, const SkDQuad& quad2) {
    SkDQuadImplicit i1(quad1);  
    SkDQuadImplicit i2(quad2);
    return i1.match(i2);
}

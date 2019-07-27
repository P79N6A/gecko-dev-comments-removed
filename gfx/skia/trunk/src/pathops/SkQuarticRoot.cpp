


























#include "SkPathOpsCubic.h"
#include "SkPathOpsQuad.h"
#include "SkQuarticRoot.h"

int SkReducedQuarticRoots(const double t4, const double t3, const double t2, const double t1,
        const double t0, const bool oneHint, double roots[4]) {
#ifdef SK_DEBUG
    
    
    
    char str[1024];
    sk_bzero(str, sizeof(str));
    SK_SNPRINTF(str, sizeof(str),
            "Solve[%1.19g x^4 + %1.19g x^3 + %1.19g x^2 + %1.19g x + %1.19g == 0, x]",
            t4, t3, t2, t1, t0);
    SkPathOpsDebug::MathematicaIze(str, sizeof(str));
#if ONE_OFF_DEBUG && ONE_OFF_DEBUG_MATHEMATICA
    SkDebugf("%s\n", str);
#endif
#endif
    if (approximately_zero_when_compared_to(t4, t0)  
            && approximately_zero_when_compared_to(t4, t1)
            && approximately_zero_when_compared_to(t4, t2)) {
        if (approximately_zero_when_compared_to(t3, t0)
            && approximately_zero_when_compared_to(t3, t1)
            && approximately_zero_when_compared_to(t3, t2)) {
            return SkDQuad::RootsReal(t2, t1, t0, roots);
        }
        if (approximately_zero_when_compared_to(t4, t3)) {
            return SkDCubic::RootsReal(t3, t2, t1, t0, roots);
        }
    }
    if ((approximately_zero_when_compared_to(t0, t1) || approximately_zero(t1))  
      
            && approximately_zero_when_compared_to(t0, t3)
            && approximately_zero_when_compared_to(t0, t4)) {
        int num = SkDCubic::RootsReal(t4, t3, t2, t1, roots);
        for (int i = 0; i < num; ++i) {
            if (approximately_zero(roots[i])) {
                return num;
            }
        }
        roots[num++] = 0;
        return num;
    }
    if (oneHint) {
        SkASSERT(approximately_zero_double(t4 + t3 + t2 + t1 + t0) ||
                approximately_zero_when_compared_to(t4 + t3 + t2 + t1 + t0,  
                SkTMax(fabs(t4), SkTMax(fabs(t3), SkTMax(fabs(t2), SkTMax(fabs(t1), fabs(t0)))))));
        
        int num = SkDCubic::RootsReal(t4, t4 + t3, -(t1 + t0), -t0, roots);
        for (int i = 0; i < num; ++i) {
            if (approximately_equal(roots[i], 1)) {
                return num;
            }
        }
        roots[num++] = 1;
        return num;
    }
    return -1;
}

int SkQuarticRootsReal(int firstCubicRoot, const double A, const double B, const double C,
        const double D, const double E, double s[4]) {
    double  u, v;
    
    const double invA = 1 / A;
    const double a = B * invA;
    const double b = C * invA;
    const double c = D * invA;
    const double d = E * invA;
    

    const double a2 = a * a;
    const double p = -3 * a2 / 8 + b;
    const double q = a2 * a / 8 - a * b / 2 + c;
    const double r = -3 * a2 * a2 / 256 + a2 * b / 16 - a * c / 4 + d;
    int num;
    double largest = SkTMax(fabs(p), fabs(q));
    if (approximately_zero_when_compared_to(r, largest)) {
    
        num = SkDCubic::RootsReal(1, 0, p, q, s);
        s[num++] = 0;
    } else {
        
        double cubicRoots[3];
        int roots = SkDCubic::RootsReal(1, -p / 2, -r, r * p / 2 - q * q / 8, cubicRoots);
        int index;
        
        double z;
        num = 0;
        int num2 = 0;
        for (index = firstCubicRoot; index < roots; ++index) {
            z = cubicRoots[index];
            
            u = z * z - r;
            v = 2 * z - p;
            if (approximately_zero_squared(u)) {
                u = 0;
            } else if (u > 0) {
                u = sqrt(u);
            } else {
                continue;
            }
            if (approximately_zero_squared(v)) {
                v = 0;
            } else if (v > 0) {
                v = sqrt(v);
            } else {
                continue;
            }
            num = SkDQuad::RootsReal(1, q < 0 ? -v : v, z - u, s);
            num2 = SkDQuad::RootsReal(1, q < 0 ? v : -v, z + u, s + num);
            if (!((num | num2) & 1)) {
                break;  
            }
        }
        num += num2;
        if (!num) {
            return 0;  
        }
    }
    
    const double sub = a / 4;
    for (int i = 0; i < num; ++i) {
        s[i] -= sub;
    }
    
    for (int i = 0; i < num - 1; ++i) {
        for (int j = i + 1; j < num; ) {
            if (AlmostDequalUlps(s[i], s[j])) {
                if (j < --num) {
                    s[j] = s[num];
                }
            } else {
                ++j;
            }
        }
    }
    return num;
}

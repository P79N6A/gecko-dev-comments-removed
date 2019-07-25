








#include "SkGeometry.h"
#include "Sk64.h"
#include "SkMatrix.h"

bool SkXRayCrossesLine(const SkXRay& pt, const SkPoint pts[2], bool* ambiguous) {
    if (ambiguous) {
        *ambiguous = false;
    }
    
    
    
    if (pt.fY == pts[0].fY) {
        if (ambiguous) {
            *ambiguous = true;
        }
        return false;
    }
    if (pt.fY < pts[0].fY && pt.fY < pts[1].fY)
        return false;
    if (pt.fY > pts[0].fY && pt.fY > pts[1].fY)
        return false;
    if (pt.fX > pts[0].fX && pt.fX > pts[1].fX)
        return false;
    
    if (SkScalarNearlyZero(pts[0].fY - pts[1].fY))
        return false;
    if (SkScalarNearlyZero(pts[0].fX - pts[1].fX)) {
        
        
        if (pt.fX <= pts[0].fX) {
            if (ambiguous) {
                *ambiguous = (pt.fY == pts[1].fY);
            }
            return true;
        }
        return false;
    }
    
    if (pt.fY == pts[1].fY) {
        if (pt.fX <= pts[1].fX) {
            if (ambiguous) {
                *ambiguous = true;
            }
            return true;
        }
        return false;
    }
    
    SkScalar delta_y = pts[1].fY - pts[0].fY;
    SkScalar delta_x = pts[1].fX - pts[0].fX;
    SkScalar slope = SkScalarDiv(delta_y, delta_x);
    SkScalar b = pts[0].fY - SkScalarMul(slope, pts[0].fX);
    
    SkScalar x = SkScalarDiv(pt.fY - b, slope);
    return pt.fX <= x;
}





#ifdef SK_SCALAR_IS_FIXED
    #define DIRECT_EVAL_OF_POLYNOMIALS
#endif



#ifdef SK_SCALAR_IS_FIXED
    static int is_not_monotonic(int a, int b, int c, int d)
    {
        return (((a - b) | (b - c) | (c - d)) & ((b - a) | (c - b) | (d - c))) >> 31;
    }

    static int is_not_monotonic(int a, int b, int c)
    {
        return (((a - b) | (b - c)) & ((b - a) | (c - b))) >> 31;
    }
#else
    static int is_not_monotonic(float a, float b, float c)
    {
        float ab = a - b;
        float bc = b - c;
        if (ab < 0)
            bc = -bc;
        return ab == 0 || bc < 0;
    }
#endif



static bool is_unit_interval(SkScalar x)
{
    return x > 0 && x < SK_Scalar1;
}

static int valid_unit_divide(SkScalar numer, SkScalar denom, SkScalar* ratio)
{
    SkASSERT(ratio);

    if (numer < 0)
    {
        numer = -numer;
        denom = -denom;
    }

    if (denom == 0 || numer == 0 || numer >= denom)
        return 0;

    SkScalar r = SkScalarDiv(numer, denom);
    if (SkScalarIsNaN(r)) {
        return 0;
    }
    SkASSERT(r >= 0 && r < SK_Scalar1);
    if (r == 0) 
        return 0;
    *ratio = r;
    return 1;
}







int SkFindUnitQuadRoots(SkScalar A, SkScalar B, SkScalar C, SkScalar roots[2])
{
    SkASSERT(roots);

    if (A == 0)
        return valid_unit_divide(-C, B, roots);

    SkScalar* r = roots;

#ifdef SK_SCALAR_IS_FLOAT
    float R = B*B - 4*A*C;
    if (R < 0 || SkScalarIsNaN(R)) {  
        return 0;
    }
    R = sk_float_sqrt(R);
#else
    Sk64    RR, tmp;

    RR.setMul(B,B);
    tmp.setMul(A,C);
    tmp.shiftLeft(2);
    RR.sub(tmp);
    if (RR.isNeg())
        return 0;
    SkFixed R = RR.getSqrt();
#endif

    SkScalar Q = (B < 0) ? -(B-R)/2 : -(B+R)/2;
    r += valid_unit_divide(Q, A, r);
    r += valid_unit_divide(C, Q, r);
    if (r - roots == 2)
    {
        if (roots[0] > roots[1])
            SkTSwap<SkScalar>(roots[0], roots[1]);
        else if (roots[0] == roots[1])  
            r -= 1; 
    }
    return (int)(r - roots);
}

#ifdef SK_SCALAR_IS_FIXED



static int Sk64FindFixedQuadRoots(const Sk64& A, const Sk64& B, const Sk64& C, SkFixed roots[2])
{
    int na = A.shiftToMake32();
    int nb = B.shiftToMake32();
    int nc = C.shiftToMake32();

    int shift = SkMax32(na, SkMax32(nb, nc));
    SkASSERT(shift >= 0);

    return SkFindUnitQuadRoots(A.getShiftRight(shift), B.getShiftRight(shift), C.getShiftRight(shift), roots);
}
#endif




static SkScalar eval_quad(const SkScalar src[], SkScalar t)
{
    SkASSERT(src);
    SkASSERT(t >= 0 && t <= SK_Scalar1);

#ifdef DIRECT_EVAL_OF_POLYNOMIALS
    SkScalar    C = src[0];
    SkScalar    A = src[4] - 2 * src[2] + C;
    SkScalar    B = 2 * (src[2] - C);
    return SkScalarMulAdd(SkScalarMulAdd(A, t, B), t, C);
#else
    SkScalar    ab = SkScalarInterp(src[0], src[2], t);
    SkScalar    bc = SkScalarInterp(src[2], src[4], t); 
    return SkScalarInterp(ab, bc, t);
#endif
}

static SkScalar eval_quad_derivative(const SkScalar src[], SkScalar t)
{
    SkScalar A = src[4] - 2 * src[2] + src[0];
    SkScalar B = src[2] - src[0];

    return 2 * SkScalarMulAdd(A, t, B);
}

static SkScalar eval_quad_derivative_at_half(const SkScalar src[])
{
    SkScalar A = src[4] - 2 * src[2] + src[0];
    SkScalar B = src[2] - src[0];
    return A + 2 * B;
}

void SkEvalQuadAt(const SkPoint src[3], SkScalar t, SkPoint* pt, SkVector* tangent)
{
    SkASSERT(src);
    SkASSERT(t >= 0 && t <= SK_Scalar1);

    if (pt)
        pt->set(eval_quad(&src[0].fX, t), eval_quad(&src[0].fY, t));
    if (tangent)
        tangent->set(eval_quad_derivative(&src[0].fX, t),
                     eval_quad_derivative(&src[0].fY, t));
}

void SkEvalQuadAtHalf(const SkPoint src[3], SkPoint* pt, SkVector* tangent)
{
    SkASSERT(src);

    if (pt)
    {
        SkScalar x01 = SkScalarAve(src[0].fX, src[1].fX);
        SkScalar y01 = SkScalarAve(src[0].fY, src[1].fY);
        SkScalar x12 = SkScalarAve(src[1].fX, src[2].fX);
        SkScalar y12 = SkScalarAve(src[1].fY, src[2].fY);
        pt->set(SkScalarAve(x01, x12), SkScalarAve(y01, y12));
    }
    if (tangent)
        tangent->set(eval_quad_derivative_at_half(&src[0].fX),
                     eval_quad_derivative_at_half(&src[0].fY));
}

static void interp_quad_coords(const SkScalar* src, SkScalar* dst, SkScalar t)
{
    SkScalar    ab = SkScalarInterp(src[0], src[2], t);
    SkScalar    bc = SkScalarInterp(src[2], src[4], t);

    dst[0] = src[0];
    dst[2] = ab;
    dst[4] = SkScalarInterp(ab, bc, t);
    dst[6] = bc;
    dst[8] = src[4];
}

void SkChopQuadAt(const SkPoint src[3], SkPoint dst[5], SkScalar t)
{
    SkASSERT(t > 0 && t < SK_Scalar1);

    interp_quad_coords(&src[0].fX, &dst[0].fX, t);
    interp_quad_coords(&src[0].fY, &dst[0].fY, t);
}

void SkChopQuadAtHalf(const SkPoint src[3], SkPoint dst[5])
{
    SkScalar x01 = SkScalarAve(src[0].fX, src[1].fX);
    SkScalar y01 = SkScalarAve(src[0].fY, src[1].fY);
    SkScalar x12 = SkScalarAve(src[1].fX, src[2].fX);
    SkScalar y12 = SkScalarAve(src[1].fY, src[2].fY);

    dst[0] = src[0];
    dst[1].set(x01, y01);
    dst[2].set(SkScalarAve(x01, x12), SkScalarAve(y01, y12));
    dst[3].set(x12, y12);
    dst[4] = src[2];
}






int SkFindQuadExtrema(SkScalar a, SkScalar b, SkScalar c, SkScalar tValue[1])
{
    


#ifdef SK_SCALAR_IS_FIXED
    return is_not_monotonic(a, b, c) && valid_unit_divide(a - b, a - b - b + c, tValue);
#else
    return valid_unit_divide(a - b, a - b - b + c, tValue);
#endif
}

static inline void flatten_double_quad_extrema(SkScalar coords[14])
{
    coords[2] = coords[6] = coords[4];
}




int SkChopQuadAtYExtrema(const SkPoint src[3], SkPoint dst[5])
{
    SkASSERT(src);
    SkASSERT(dst);
    
#if 0
    static bool once = true;
    if (once)
    {
        once = false;
        SkPoint s[3] = { 0, 26398, 0, 26331, 0, 20621428 };
        SkPoint d[6];
        
        int n = SkChopQuadAtYExtrema(s, d);
        SkDebugf("chop=%d, Y=[%x %x %x %x %x %x]\n", n, d[0].fY, d[1].fY, d[2].fY, d[3].fY, d[4].fY, d[5].fY);
    }
#endif
    
    SkScalar a = src[0].fY;
    SkScalar b = src[1].fY;
    SkScalar c = src[2].fY;
    
    if (is_not_monotonic(a, b, c))
    {
        SkScalar    tValue;
        if (valid_unit_divide(a - b, a - b - b + c, &tValue))
        {
            SkChopQuadAt(src, dst, tValue);
            flatten_double_quad_extrema(&dst[0].fY);
            return 1;
        }
        
        
        b = SkScalarAbs(a - b) < SkScalarAbs(b - c) ? a : c;
    }
    dst[0].set(src[0].fX, a);
    dst[1].set(src[1].fX, b);
    dst[2].set(src[2].fX, c);
    return 0;
}




int SkChopQuadAtXExtrema(const SkPoint src[3], SkPoint dst[5])
{
    SkASSERT(src);
    SkASSERT(dst);
    
    SkScalar a = src[0].fX;
    SkScalar b = src[1].fX;
    SkScalar c = src[2].fX;
    
    if (is_not_monotonic(a, b, c)) {
        SkScalar tValue;
        if (valid_unit_divide(a - b, a - b - b + c, &tValue)) {
            SkChopQuadAt(src, dst, tValue);
            flatten_double_quad_extrema(&dst[0].fX);
            return 1;
        }
        
        
        b = SkScalarAbs(a - b) < SkScalarAbs(b - c) ? a : c;
    }
    dst[0].set(a, src[0].fY);
    dst[1].set(b, src[1].fY);
    dst[2].set(c, src[2].fY);
    return 0;
}













int SkChopQuadAtMaxCurvature(const SkPoint src[3], SkPoint dst[5])
{
    SkScalar    Ax = src[1].fX - src[0].fX;
    SkScalar    Ay = src[1].fY - src[0].fY;
    SkScalar    Bx = src[0].fX - src[1].fX - src[1].fX + src[2].fX;
    SkScalar    By = src[0].fY - src[1].fY - src[1].fY + src[2].fY;
    SkScalar    t = 0;  

#ifdef SK_SCALAR_IS_FLOAT
    (void)valid_unit_divide(-(Ax * Bx + Ay * By), Bx * Bx + By * By, &t);
#else
    
    Sk64    numer, denom, tmp;

    numer.setMul(Ax, -Bx);
    tmp.setMul(Ay, -By);
    numer.add(tmp);

    if (numer.isPos())  
    {
        denom.setMul(Bx, Bx);
        tmp.setMul(By, By);
        denom.add(tmp);
        SkASSERT(!denom.isNeg());
        if (numer < denom)
        {
            t = numer.getFixedDiv(denom);
            SkASSERT(t >= 0 && t <= SK_Fixed1);     
            if ((unsigned)t >= SK_Fixed1)           
                t = 0;  
        }
    }
#endif

    if (t == 0)
    {
        memcpy(dst, src, 3 * sizeof(SkPoint));
        return 1;
    }
    else
    {
        SkChopQuadAt(src, dst, t);
        return 2;
    }
}

#ifdef SK_SCALAR_IS_FLOAT
    #define SK_ScalarTwoThirds  (0.666666666f)
#else
    #define SK_ScalarTwoThirds  ((SkFixed)(43691))
#endif

void SkConvertQuadToCubic(const SkPoint src[3], SkPoint dst[4]) {
    const SkScalar scale = SK_ScalarTwoThirds;
    dst[0] = src[0];
    dst[1].set(src[0].fX + SkScalarMul(src[1].fX - src[0].fX, scale),
               src[0].fY + SkScalarMul(src[1].fY - src[0].fY, scale));
    dst[2].set(src[2].fX + SkScalarMul(src[1].fX - src[2].fX, scale),
               src[2].fY + SkScalarMul(src[1].fY - src[2].fY, scale));
    dst[3] = src[2];
}





static void get_cubic_coeff(const SkScalar pt[], SkScalar coeff[4])
{
    coeff[0] = pt[6] + 3*(pt[2] - pt[4]) - pt[0];
    coeff[1] = 3*(pt[4] - pt[2] - pt[2] + pt[0]);
    coeff[2] = 3*(pt[2] - pt[0]);
    coeff[3] = pt[0];
}

void SkGetCubicCoeff(const SkPoint pts[4], SkScalar cx[4], SkScalar cy[4])
{
    SkASSERT(pts);

    if (cx)
        get_cubic_coeff(&pts[0].fX, cx);
    if (cy)
        get_cubic_coeff(&pts[0].fY, cy);
}

static SkScalar eval_cubic(const SkScalar src[], SkScalar t)
{
    SkASSERT(src);
    SkASSERT(t >= 0 && t <= SK_Scalar1);

    if (t == 0)
        return src[0];

#ifdef DIRECT_EVAL_OF_POLYNOMIALS
    SkScalar D = src[0];
    SkScalar A = src[6] + 3*(src[2] - src[4]) - D;
    SkScalar B = 3*(src[4] - src[2] - src[2] + D);
    SkScalar C = 3*(src[2] - D);

    return SkScalarMulAdd(SkScalarMulAdd(SkScalarMulAdd(A, t, B), t, C), t, D);
#else
    SkScalar    ab = SkScalarInterp(src[0], src[2], t);
    SkScalar    bc = SkScalarInterp(src[2], src[4], t);
    SkScalar    cd = SkScalarInterp(src[4], src[6], t);
    SkScalar    abc = SkScalarInterp(ab, bc, t);
    SkScalar    bcd = SkScalarInterp(bc, cd, t);
    return SkScalarInterp(abc, bcd, t);
#endif
}



static SkScalar eval_quadratic(SkScalar A, SkScalar B, SkScalar C, SkScalar t)
{
    SkASSERT(t >= 0 && t <= SK_Scalar1);

    return SkScalarMulAdd(SkScalarMulAdd(A, t, B), t, C);
}

static SkScalar eval_cubic_derivative(const SkScalar src[], SkScalar t)
{
    SkScalar A = src[6] + 3*(src[2] - src[4]) - src[0];
    SkScalar B = 2*(src[4] - 2 * src[2] + src[0]);
    SkScalar C = src[2] - src[0];

    return eval_quadratic(A, B, C, t);
}

static SkScalar eval_cubic_2ndDerivative(const SkScalar src[], SkScalar t)
{
    SkScalar A = src[6] + 3*(src[2] - src[4]) - src[0];
    SkScalar B = src[4] - 2 * src[2] + src[0];

    return SkScalarMulAdd(A, t, B);
}

void SkEvalCubicAt(const SkPoint src[4], SkScalar t, SkPoint* loc, SkVector* tangent, SkVector* curvature)
{
    SkASSERT(src);
    SkASSERT(t >= 0 && t <= SK_Scalar1);

    if (loc)
        loc->set(eval_cubic(&src[0].fX, t), eval_cubic(&src[0].fY, t));
    if (tangent)
        tangent->set(eval_cubic_derivative(&src[0].fX, t),
                     eval_cubic_derivative(&src[0].fY, t));
    if (curvature)
        curvature->set(eval_cubic_2ndDerivative(&src[0].fX, t),
                       eval_cubic_2ndDerivative(&src[0].fY, t));
}







int SkFindCubicExtrema(SkScalar a, SkScalar b, SkScalar c, SkScalar d, SkScalar tValues[2])
{
#ifdef SK_SCALAR_IS_FIXED
    if (!is_not_monotonic(a, b, c, d))
        return 0;
#endif

    
    SkScalar A = d - a + 3*(b - c);
    SkScalar B = 2*(a - b - b + c);
    SkScalar C = b - a;

    return SkFindUnitQuadRoots(A, B, C, tValues);
}

static void interp_cubic_coords(const SkScalar* src, SkScalar* dst, SkScalar t)
{
    SkScalar    ab = SkScalarInterp(src[0], src[2], t);
    SkScalar    bc = SkScalarInterp(src[2], src[4], t);
    SkScalar    cd = SkScalarInterp(src[4], src[6], t);
    SkScalar    abc = SkScalarInterp(ab, bc, t);
    SkScalar    bcd = SkScalarInterp(bc, cd, t);
    SkScalar    abcd = SkScalarInterp(abc, bcd, t);

    dst[0] = src[0];
    dst[2] = ab;
    dst[4] = abc;
    dst[6] = abcd;
    dst[8] = bcd;
    dst[10] = cd;
    dst[12] = src[6];
}

void SkChopCubicAt(const SkPoint src[4], SkPoint dst[7], SkScalar t)
{
    SkASSERT(t > 0 && t < SK_Scalar1);

    interp_cubic_coords(&src[0].fX, &dst[0].fX, t);
    interp_cubic_coords(&src[0].fY, &dst[0].fY, t);
}
























void SkChopCubicAt(const SkPoint src[4], SkPoint dst[], const SkScalar tValues[], int roots)
{
#ifdef SK_DEBUG
    {
        for (int i = 0; i < roots - 1; i++)
        {
            SkASSERT(is_unit_interval(tValues[i]));
            SkASSERT(is_unit_interval(tValues[i+1]));
            SkASSERT(tValues[i] < tValues[i+1]);
        }
    }
#endif

    if (dst)
    {
        if (roots == 0) 
            memcpy(dst, src, 4*sizeof(SkPoint));
        else
        {
            SkScalar    t = tValues[0];
            SkPoint     tmp[4];

            for (int i = 0; i < roots; i++)
            {
                SkChopCubicAt(src, dst, t);
                if (i == roots - 1)
                    break;

                dst += 3;
                
                memcpy(tmp, dst, 4 * sizeof(SkPoint));
                src = tmp;

                
                if (!valid_unit_divide(tValues[i+1] - tValues[i],
                                       SK_Scalar1 - tValues[i], &t)) {
                    
                    dst[4] = dst[5] = dst[6] = src[3];
                    break;
                }
            }
        }
    }
}

void SkChopCubicAtHalf(const SkPoint src[4], SkPoint dst[7])
{
    SkScalar x01 = SkScalarAve(src[0].fX, src[1].fX);
    SkScalar y01 = SkScalarAve(src[0].fY, src[1].fY);
    SkScalar x12 = SkScalarAve(src[1].fX, src[2].fX);
    SkScalar y12 = SkScalarAve(src[1].fY, src[2].fY);
    SkScalar x23 = SkScalarAve(src[2].fX, src[3].fX);
    SkScalar y23 = SkScalarAve(src[2].fY, src[3].fY);

    SkScalar x012 = SkScalarAve(x01, x12);
    SkScalar y012 = SkScalarAve(y01, y12);
    SkScalar x123 = SkScalarAve(x12, x23);
    SkScalar y123 = SkScalarAve(y12, y23);

    dst[0] = src[0];
    dst[1].set(x01, y01);
    dst[2].set(x012, y012);
    dst[3].set(SkScalarAve(x012, x123), SkScalarAve(y012, y123));
    dst[4].set(x123, y123);
    dst[5].set(x23, y23);
    dst[6] = src[3];
}

static void flatten_double_cubic_extrema(SkScalar coords[14])
{
    coords[4] = coords[8] = coords[6];
}









int SkChopCubicAtYExtrema(const SkPoint src[4], SkPoint dst[10]) {
    SkScalar    tValues[2];
    int         roots = SkFindCubicExtrema(src[0].fY, src[1].fY, src[2].fY,
                                           src[3].fY, tValues);
    
    SkChopCubicAt(src, dst, tValues, roots);
    if (dst && roots > 0) {
        
        flatten_double_cubic_extrema(&dst[0].fY);
        if (roots == 2) {
            flatten_double_cubic_extrema(&dst[3].fY);
        }
    }
    return roots;
}

int SkChopCubicAtXExtrema(const SkPoint src[4], SkPoint dst[10]) {
    SkScalar    tValues[2];
    int         roots = SkFindCubicExtrema(src[0].fX, src[1].fX, src[2].fX,
                                           src[3].fX, tValues);
    
    SkChopCubicAt(src, dst, tValues, roots);
    if (dst && roots > 0) {
        
        flatten_double_cubic_extrema(&dst[0].fX);
        if (roots == 2) {
            flatten_double_cubic_extrema(&dst[3].fX);
        }
    }
    return roots;
}












int SkFindCubicInflections(const SkPoint src[4], SkScalar tValues[])
{
    SkScalar    Ax = src[1].fX - src[0].fX;
    SkScalar    Ay = src[1].fY - src[0].fY;
    SkScalar    Bx = src[2].fX - 2 * src[1].fX + src[0].fX;
    SkScalar    By = src[2].fY - 2 * src[1].fY + src[0].fY;
    SkScalar    Cx = src[3].fX + 3 * (src[1].fX - src[2].fX) - src[0].fX;
    SkScalar    Cy = src[3].fY + 3 * (src[1].fY - src[2].fY) - src[0].fY;
    int         count;

#ifdef SK_SCALAR_IS_FLOAT
    count = SkFindUnitQuadRoots(Bx*Cy - By*Cx, Ax*Cy - Ay*Cx, Ax*By - Ay*Bx, tValues);
#else
    Sk64    A, B, C, tmp;

    A.setMul(Bx, Cy);
    tmp.setMul(By, Cx);
    A.sub(tmp);

    B.setMul(Ax, Cy);
    tmp.setMul(Ay, Cx);
    B.sub(tmp);

    C.setMul(Ax, By);
    tmp.setMul(Ay, Bx);
    C.sub(tmp);

    count = Sk64FindFixedQuadRoots(A, B, C, tValues);
#endif

    return count;
}

int SkChopCubicAtInflections(const SkPoint src[], SkPoint dst[10])
{
    SkScalar    tValues[2];
    int         count = SkFindCubicInflections(src, tValues);

    if (dst)
    {
        if (count == 0)
            memcpy(dst, src, 4 * sizeof(SkPoint));
        else
            SkChopCubicAt(src, dst, tValues, count);
    }
    return count + 1;
}

template <typename T> void bubble_sort(T array[], int count)
{
    for (int i = count - 1; i > 0; --i)
        for (int j = i; j > 0; --j)
            if (array[j] < array[j-1])
            {
                T   tmp(array[j]);
                array[j] = array[j-1];
                array[j-1] = tmp;
            }
}

#include "SkFP.h"


#if 0
static SkScalar refine_cubic_root(const SkFP coeff[4], SkScalar root)
{
    

    SkFP    T = SkScalarToFloat(root);
    SkFP    N, D;

    
    D = SkFPMul(SkFPMul(coeff[0], SkFPMul(T,T)), 3);
    D = SkFPAdd(D, SkFPMulInt(SkFPMul(coeff[1], T), 2));
    D = SkFPAdd(D, coeff[2]);

    if (D == 0)
        return root;

    
    N = SkFPMul(SkFPMul(SkFPMul(T, T), T), coeff[0]);
    N = SkFPAdd(N, SkFPMul(SkFPMul(T, T), coeff[1]));
    N = SkFPAdd(N, SkFPMul(T, coeff[2]));
    N = SkFPAdd(N, coeff[3]);

    if (N)
    {
        SkScalar delta = SkFPToScalar(SkFPDiv(N, D));

        if (delta)
            root -= delta;
    }
    return root;
}
#endif

#if defined _WIN32 && _MSC_VER >= 1300  && defined SK_SCALAR_IS_FIXED 
#pragma warning ( disable : 4702 )
#endif





static int solve_cubic_polynomial(const SkFP coeff[4], SkScalar tValues[3])
{
#ifndef SK_SCALAR_IS_FLOAT
    return 0;   
#endif

    if (SkScalarNearlyZero(coeff[0]))   
    {
        return SkFindUnitQuadRoots(coeff[1], coeff[2], coeff[3], tValues);
    }

    SkFP    a, b, c, Q, R;

    {
        SkASSERT(coeff[0] != 0);

        SkFP inva = SkFPInvert(coeff[0]);
        a = SkFPMul(coeff[1], inva);
        b = SkFPMul(coeff[2], inva);
        c = SkFPMul(coeff[3], inva);
    }
    Q = SkFPDivInt(SkFPSub(SkFPMul(a,a), SkFPMulInt(b, 3)), 9);

    R = SkFPMulInt(SkFPMul(SkFPMul(a, a), a), 2);
    R = SkFPSub(R, SkFPMulInt(SkFPMul(a, b), 9));
    R = SkFPAdd(R, SkFPMulInt(c, 27));
    R = SkFPDivInt(R, 54);

    SkFP Q3 = SkFPMul(SkFPMul(Q, Q), Q);
    SkFP R2MinusQ3 = SkFPSub(SkFPMul(R,R), Q3);
    SkFP adiv3 = SkFPDivInt(a, 3);

    SkScalar*   roots = tValues;
    SkScalar    r;

    if (SkFPLT(R2MinusQ3, 0))   
    {
#ifdef SK_SCALAR_IS_FLOAT
        float theta = sk_float_acos(R / sk_float_sqrt(Q3));
        float neg2RootQ = -2 * sk_float_sqrt(Q);

        r = neg2RootQ * sk_float_cos(theta/3) - adiv3;
        if (is_unit_interval(r))
            *roots++ = r;

        r = neg2RootQ * sk_float_cos((theta + 2*SK_ScalarPI)/3) - adiv3;
        if (is_unit_interval(r))
            *roots++ = r;

        r = neg2RootQ * sk_float_cos((theta - 2*SK_ScalarPI)/3) - adiv3;
        if (is_unit_interval(r))
            *roots++ = r;

        
        bubble_sort(tValues, (int)(roots - tValues));
#endif
    }
    else                
    {
        SkFP A = SkFPAdd(SkFPAbs(R), SkFPSqrt(R2MinusQ3));
        A = SkFPCubeRoot(A);
        if (SkFPGT(R, 0))
            A = SkFPNeg(A);

        if (A != 0)
            A = SkFPAdd(A, SkFPDiv(Q, A));
        r = SkFPToScalar(SkFPSub(A, adiv3));
        if (is_unit_interval(r))
            *roots++ = r;
    }

    return (int)(roots - tValues);
}












static void formulate_F1DotF2(const SkScalar src[], SkFP coeff[4])
{
    SkScalar    a = src[2] - src[0];
    SkScalar    b = src[4] - 2 * src[2] + src[0];
    SkScalar    c = src[6] + 3 * (src[2] - src[4]) - src[0];

    SkFP    A = SkScalarToFP(a);
    SkFP    B = SkScalarToFP(b);
    SkFP    C = SkScalarToFP(c);

    coeff[0] = SkFPMul(C, C);
    coeff[1] = SkFPMulInt(SkFPMul(B, C), 3);
    coeff[2] = SkFPMulInt(SkFPMul(B, B), 2);
    coeff[2] = SkFPAdd(coeff[2], SkFPMul(C, A));
    coeff[3] = SkFPMul(A, B);
}



#define kMinTValueForChopping   0












int SkFindCubicMaxCurvature(const SkPoint src[4], SkScalar tValues[3])
{
    SkFP    coeffX[4], coeffY[4];
    int     i;

    formulate_F1DotF2(&src[0].fX, coeffX);
    formulate_F1DotF2(&src[0].fY, coeffY);

    for (i = 0; i < 4; i++)
        coeffX[i] = SkFPAdd(coeffX[i],coeffY[i]);

    SkScalar    t[3];
    int         count = solve_cubic_polynomial(coeffX, t);
    int         maxCount = 0;

    
    
    for (i = 0; i < count; i++)
    {
        
        if (t[i] > kMinTValueForChopping && t[i] < SK_Scalar1 - kMinTValueForChopping)
            tValues[maxCount++] = t[i];
    }
    return maxCount;
}

int SkChopCubicAtMaxCurvature(const SkPoint src[4], SkPoint dst[13], SkScalar tValues[3])
{
    SkScalar    t_storage[3];

    if (tValues == NULL)
        tValues = t_storage;

    int count = SkFindCubicMaxCurvature(src, tValues);

    if (dst)
    {
        if (count == 0)
            memcpy(dst, src, 4 * sizeof(SkPoint));
        else
            SkChopCubicAt(src, dst, tValues, count);
    }
    return count + 1;
}

bool SkXRayCrossesMonotonicCubic(const SkXRay& pt, const SkPoint cubic[4], bool* ambiguous) {
    if (ambiguous) {
        *ambiguous = false;
    }

    
    
    SkScalar min_y = SkMinScalar(cubic[0].fY, cubic[3].fY);
    SkScalar max_y = SkMaxScalar(cubic[0].fY, cubic[3].fY);

    if (pt.fY == cubic[0].fY
        || pt.fY < min_y
        || pt.fY > max_y) {
        
        if (ambiguous) {
            *ambiguous = (pt.fY == cubic[0].fY);
        }
        return false;
    }

    bool pt_at_extremum = (pt.fY == cubic[3].fY);

    SkScalar min_x =
        SkMinScalar(
            SkMinScalar(
                SkMinScalar(cubic[0].fX, cubic[1].fX),
                cubic[2].fX),
            cubic[3].fX);
    if (pt.fX < min_x) {
        
        if (ambiguous) {
            *ambiguous = pt_at_extremum;
        }
        return true;
    }

    SkScalar max_x =
        SkMaxScalar(
            SkMaxScalar(
                SkMaxScalar(cubic[0].fX, cubic[1].fX),
                cubic[2].fX),
            cubic[3].fX);
    if (pt.fX > max_x) {
        
        return false;
    }

    
    
    

    
    
    
    const int kMaxIter = 23;
    SkPoint eval;
    int iter = 0;
    SkScalar upper_t;
    SkScalar lower_t;
    
    
    if (cubic[3].fY > cubic[0].fY) {
        upper_t = SK_Scalar1;
        lower_t = SkFloatToScalar(0);
    } else {
        upper_t = SkFloatToScalar(0);
        lower_t = SK_Scalar1;
    }
    do {
        SkScalar t = SkScalarAve(upper_t, lower_t);
        SkEvalCubicAt(cubic, t, &eval, NULL, NULL);
        if (pt.fY > eval.fY) {
            lower_t = t;
        } else {
            upper_t = t;
        }
    } while (++iter < kMaxIter
             && !SkScalarNearlyZero(eval.fY - pt.fY));
    if (pt.fX <= eval.fX) {
        if (ambiguous) {
            *ambiguous = pt_at_extremum;
        }
        return true;
    }
    return false;
}

int SkNumXRayCrossingsForCubic(const SkXRay& pt, const SkPoint cubic[4], bool* ambiguous) {
    int num_crossings = 0;
    SkPoint monotonic_cubics[10];
    int num_monotonic_cubics = SkChopCubicAtYExtrema(cubic, monotonic_cubics);
    if (ambiguous) {
        *ambiguous = false;
    }
    bool locally_ambiguous;
    if (SkXRayCrossesMonotonicCubic(pt, &monotonic_cubics[0], &locally_ambiguous))
        ++num_crossings;
    if (ambiguous) {
        *ambiguous |= locally_ambiguous;
    }
    if (num_monotonic_cubics > 0)
        if (SkXRayCrossesMonotonicCubic(pt, &monotonic_cubics[3], &locally_ambiguous))
            ++num_crossings;
    if (ambiguous) {
        *ambiguous |= locally_ambiguous;
    }
    if (num_monotonic_cubics > 1)
        if (SkXRayCrossesMonotonicCubic(pt, &monotonic_cubics[6], &locally_ambiguous))
            ++num_crossings;
    if (ambiguous) {
        *ambiguous |= locally_ambiguous;
    }
    return num_crossings;
}






static SkScalar quad_solve(SkScalar a, SkScalar b, SkScalar c, SkScalar d)
{
    
    SkScalar A = a - 2 * b + c;
    SkScalar B = 2 * (b - a);
    SkScalar C = a - d;

    SkScalar    roots[2];
    int         count = SkFindUnitQuadRoots(A, B, C, roots);

    SkASSERT(count <= 1);
    return count == 1 ? roots[0] : 0;
}





static bool quad_pt2OffCurve(const SkPoint quad[3], SkScalar x, SkScalar y, SkPoint* offCurve)
{
    const SkScalar* base;
    SkScalar        value;

    if (SkScalarAbs(x) < SkScalarAbs(y)) {
        base = &quad[0].fX;
        value = x;
    } else {
        base = &quad[0].fY;
        value = y;
    }

    
    
    SkScalar t = quad_solve(base[0], base[2], base[4], value);

    if (t > 0)
    {
        SkPoint tmp[5];
        SkChopQuadAt(quad, tmp, t);
        *offCurve = tmp[1];
        return true;
    } else {
        










        if ((base[0] < base[4] && value > base[2]) ||
            (base[0] > base[4] && value < base[2]))   
        {
            *offCurve = quad[1];
            return true;
        }
    }
    return false;
}

static const SkPoint gQuadCirclePts[kSkBuildQuadArcStorage] = {
    { SK_Scalar1,           0               },
    { SK_Scalar1,           SK_ScalarTanPIOver8 },
    { SK_ScalarRoot2Over2,  SK_ScalarRoot2Over2 },
    { SK_ScalarTanPIOver8,  SK_Scalar1          },

    { 0,                    SK_Scalar1      },
    { -SK_ScalarTanPIOver8, SK_Scalar1  },
    { -SK_ScalarRoot2Over2, SK_ScalarRoot2Over2 },
    { -SK_Scalar1,          SK_ScalarTanPIOver8 },

    { -SK_Scalar1,          0               },
    { -SK_Scalar1,          -SK_ScalarTanPIOver8    },
    { -SK_ScalarRoot2Over2, -SK_ScalarRoot2Over2    },
    { -SK_ScalarTanPIOver8, -SK_Scalar1     },

    { 0,                    -SK_Scalar1     },
    { SK_ScalarTanPIOver8,  -SK_Scalar1     },
    { SK_ScalarRoot2Over2,  -SK_ScalarRoot2Over2    },
    { SK_Scalar1,           -SK_ScalarTanPIOver8    },

    { SK_Scalar1,           0   }
};

int SkBuildQuadArc(const SkVector& uStart, const SkVector& uStop,
                   SkRotationDirection dir, const SkMatrix* userMatrix,
                   SkPoint quadPoints[])
{
    
    SkScalar x = SkPoint::DotProduct(uStart, uStop);
    SkScalar y = SkPoint::CrossProduct(uStart, uStop);

    SkScalar absX = SkScalarAbs(x);
    SkScalar absY = SkScalarAbs(y);

    int pointCount;

    
    
    
    if (absY <= SK_ScalarNearlyZero && x > 0 &&
        ((y >= 0 && kCW_SkRotationDirection == dir) ||
         (y <= 0 && kCCW_SkRotationDirection == dir))) {
            
        
        quadPoints[0].set(SK_Scalar1, 0);
        pointCount = 1;
    } else {
        if (dir == kCCW_SkRotationDirection)
            y = -y;

        
        int oct = 0;
        bool sameSign = true;

        if (0 == y)
        {
            oct = 4;        
            SkASSERT(SkScalarAbs(x + SK_Scalar1) <= SK_ScalarNearlyZero);
        }
        else if (0 == x)
        {
            SkASSERT(absY - SK_Scalar1 <= SK_ScalarNearlyZero);
            if (y > 0)
                oct = 2;    
            else
                oct = 6;    
        }
        else
        {
            if (y < 0)
                oct += 4;
            if ((x < 0) != (y < 0))
            {
                oct += 2;
                sameSign = false;
            }
            if ((absX < absY) == sameSign)
                oct += 1;
        }

        int wholeCount = oct << 1;
        memcpy(quadPoints, gQuadCirclePts, (wholeCount + 1) * sizeof(SkPoint));

        const SkPoint* arc = &gQuadCirclePts[wholeCount];
        if (quad_pt2OffCurve(arc, x, y, &quadPoints[wholeCount + 1]))
        {
            quadPoints[wholeCount + 2].set(x, y);
            wholeCount += 2;
        }
        pointCount = wholeCount + 1;
    }

    
    SkMatrix    matrix;
    matrix.setSinCos(uStart.fY, uStart.fX);
    if (dir == kCCW_SkRotationDirection) {
        matrix.preScale(SK_Scalar1, -SK_Scalar1);
    }
    if (userMatrix) {
        matrix.postConcat(*userMatrix);
    }
    matrix.mapPoints(quadPoints, pointCount);
    return pointCount;
}


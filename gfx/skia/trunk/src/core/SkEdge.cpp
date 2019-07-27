








#include "SkEdge.h"
#include "SkFDot6.h"
#include "SkMath.h"













static inline SkFixed SkFDot6ToFixedDiv2(SkFDot6 value) {
    
    
    return value << (16 - 6 - 1);
}



int SkEdge::setLine(const SkPoint& p0, const SkPoint& p1, const SkIRect* clip,
                    int shift) {
    SkFDot6 x0, y0, x1, y1;

    {
#ifdef SK_RASTERIZE_EVEN_ROUNDING
        x0 = SkScalarRoundToFDot6(p0.fX, shift);
        y0 = SkScalarRoundToFDot6(p0.fY, shift);
        x1 = SkScalarRoundToFDot6(p1.fX, shift);
        y1 = SkScalarRoundToFDot6(p1.fY, shift);
#else
        float scale = float(1 << (shift + 6));
        x0 = int(p0.fX * scale);
        y0 = int(p0.fY * scale);
        x1 = int(p1.fX * scale);
        y1 = int(p1.fY * scale);
#endif
    }

    int winding = 1;

    if (y0 > y1) {
        SkTSwap(x0, x1);
        SkTSwap(y0, y1);
        winding = -1;
    }

    int top = SkFDot6Round(y0);
    int bot = SkFDot6Round(y1);

    
    if (top == bot) {
        return 0;
    }
    
    if (NULL != clip && (top >= clip->fBottom || bot <= clip->fTop)) {
        return 0;
    }

    SkFixed slope = SkFDot6Div(x1 - x0, y1 - y0);
    const int dy  = SkEdge_Compute_DY(top, y0);

    fX          = SkFDot6ToFixed(x0 + SkFixedMul(slope, dy));   
    fDX         = slope;
    fFirstY     = top;
    fLastY      = bot - 1;
    fCurveCount = 0;
    fWinding    = SkToS8(winding);
    fCurveShift = 0;

    if (clip) {
        this->chopLineWithClip(*clip);
    }
    return 1;
}


int SkEdge::updateLine(SkFixed x0, SkFixed y0, SkFixed x1, SkFixed y1)
{
    SkASSERT(fWinding == 1 || fWinding == -1);
    SkASSERT(fCurveCount != 0);


    y0 >>= 10;
    y1 >>= 10;

    SkASSERT(y0 <= y1);

    int top = SkFDot6Round(y0);
    int bot = SkFDot6Round(y1);



    
    if (top == bot)
        return 0;

    x0 >>= 10;
    x1 >>= 10;

    SkFixed slope = SkFDot6Div(x1 - x0, y1 - y0);
    const int dy  = SkEdge_Compute_DY(top, y0);

    fX          = SkFDot6ToFixed(x0 + SkFixedMul(slope, dy));   
    fDX         = slope;
    fFirstY     = top;
    fLastY      = bot - 1;

    return 1;
}

void SkEdge::chopLineWithClip(const SkIRect& clip)
{
    int top = fFirstY;

    SkASSERT(top < clip.fBottom);

    
    if (top < clip.fTop)
    {
        SkASSERT(fLastY >= clip.fTop);
        fX += fDX * (clip.fTop - top);
        fFirstY = clip.fTop;
    }
}








#define MAX_COEFF_SHIFT     6

static inline SkFDot6 cheap_distance(SkFDot6 dx, SkFDot6 dy)
{
    dx = SkAbs32(dx);
    dy = SkAbs32(dy);
    
    if (dx > dy)
        dx += dy >> 1;
    else
        dx = dy + (dx >> 1);
    return dx;
}

static inline int diff_to_shift(SkFDot6 dx, SkFDot6 dy)
{
    
    SkFDot6 dist = cheap_distance(dx, dy);

    
    
    
    
    dist = (dist + (1 << 4)) >> 5;

    
    return (32 - SkCLZ(dist)) >> 1;
}

int SkQuadraticEdge::setQuadratic(const SkPoint pts[3], int shift)
{
    SkFDot6 x0, y0, x1, y1, x2, y2;

    {
#ifdef SK_RASTERIZE_EVEN_ROUNDING
        x0 = SkScalarRoundToFDot6(pts[0].fX, shift);
        y0 = SkScalarRoundToFDot6(pts[0].fY, shift);
        x1 = SkScalarRoundToFDot6(pts[1].fX, shift);
        y1 = SkScalarRoundToFDot6(pts[1].fY, shift);
        x2 = SkScalarRoundToFDot6(pts[2].fX, shift);
        y2 = SkScalarRoundToFDot6(pts[2].fY, shift);
#else
        float scale = float(1 << (shift + 6));
        x0 = int(pts[0].fX * scale);
        y0 = int(pts[0].fY * scale);
        x1 = int(pts[1].fX * scale);
        y1 = int(pts[1].fY * scale);
        x2 = int(pts[2].fX * scale);
        y2 = int(pts[2].fY * scale);
#endif
    }

    int winding = 1;
    if (y0 > y2)
    {
        SkTSwap(x0, x2);
        SkTSwap(y0, y2);
        winding = -1;
    }
    SkASSERT(y0 <= y1 && y1 <= y2);

    int top = SkFDot6Round(y0);
    int bot = SkFDot6Round(y2);

    
    if (top == bot)
        return 0;

    
    {
        SkFDot6 dx = ((x1 << 1) - x0 - x2) >> 2;
        SkFDot6 dy = ((y1 << 1) - y0 - y2) >> 2;
        shift = diff_to_shift(dx, dy);
        SkASSERT(shift >= 0);
    }
    
    if (shift == 0) {
        shift = 1;
    } else if (shift > MAX_COEFF_SHIFT) {
        shift = MAX_COEFF_SHIFT;
    }

    fWinding    = SkToS8(winding);
    
    fCurveCount = SkToS8(1 << shift);

    

















    fCurveShift = SkToU8(shift - 1);

    SkFixed A = SkFDot6ToFixedDiv2(x0 - x1 - x1 + x2);  
    SkFixed B = SkFDot6ToFixed(x1 - x0);                

    fQx     = SkFDot6ToFixed(x0);
    fQDx    = B + (A >> shift);     
    fQDDx   = A >> (shift - 1);     

    A = SkFDot6ToFixedDiv2(y0 - y1 - y1 + y2);  
    B = SkFDot6ToFixed(y1 - y0);                

    fQy     = SkFDot6ToFixed(y0);
    fQDy    = B + (A >> shift);     
    fQDDy   = A >> (shift - 1);     

    fQLastX = SkFDot6ToFixed(x2);
    fQLastY = SkFDot6ToFixed(y2);

    return this->updateQuadratic();
}

int SkQuadraticEdge::updateQuadratic()
{
    int     success;
    int     count = fCurveCount;
    SkFixed oldx = fQx;
    SkFixed oldy = fQy;
    SkFixed dx = fQDx;
    SkFixed dy = fQDy;
    SkFixed newx, newy;
    int     shift = fCurveShift;

    SkASSERT(count > 0);

    do {
        if (--count > 0)
        {
            newx    = oldx + (dx >> shift);
            dx    += fQDDx;
            newy    = oldy + (dy >> shift);
            dy    += fQDDy;
        }
        else    
        {
            newx    = fQLastX;
            newy    = fQLastY;
        }
        success = this->updateLine(oldx, oldy, newx, newy);
        oldx = newx;
        oldy = newy;
    } while (count > 0 && !success);

    fQx         = newx;
    fQy         = newy;
    fQDx        = dx;
    fQDy        = dy;
    fCurveCount = SkToS8(count);
    return success;
}



static inline int SkFDot6UpShift(SkFDot6 x, int upShift) {
    SkASSERT((x << upShift >> upShift) == x);
    return x << upShift;
}









static SkFDot6 cubic_delta_from_line(SkFDot6 a, SkFDot6 b, SkFDot6 c, SkFDot6 d)
{
    SkFDot6 oneThird = ((a << 3) - ((b << 4) - b) + 6*c + d) * 19 >> 9;
    SkFDot6 twoThird = (a + 6*b - ((c << 4) - c) + (d << 3)) * 19 >> 9;

    return SkMax32(SkAbs32(oneThird), SkAbs32(twoThird));
}

int SkCubicEdge::setCubic(const SkPoint pts[4], const SkIRect* clip, int shift)
{
    SkFDot6 x0, y0, x1, y1, x2, y2, x3, y3;

    {
#ifdef SK_RASTERIZE_EVEN_ROUNDING
        x0 = SkScalarRoundToFDot6(pts[0].fX, shift);
        y0 = SkScalarRoundToFDot6(pts[0].fY, shift);
        x1 = SkScalarRoundToFDot6(pts[1].fX, shift);
        y1 = SkScalarRoundToFDot6(pts[1].fY, shift);
        x2 = SkScalarRoundToFDot6(pts[2].fX, shift);
        y2 = SkScalarRoundToFDot6(pts[2].fY, shift);
        x3 = SkScalarRoundToFDot6(pts[3].fX, shift);
        y3 = SkScalarRoundToFDot6(pts[3].fY, shift);
#else
        float scale = float(1 << (shift + 6));
        x0 = int(pts[0].fX * scale);
        y0 = int(pts[0].fY * scale);
        x1 = int(pts[1].fX * scale);
        y1 = int(pts[1].fY * scale);
        x2 = int(pts[2].fX * scale);
        y2 = int(pts[2].fY * scale);
        x3 = int(pts[3].fX * scale);
        y3 = int(pts[3].fY * scale);
#endif
    }

    int winding = 1;
    if (y0 > y3)
    {
        SkTSwap(x0, x3);
        SkTSwap(x1, x2);
        SkTSwap(y0, y3);
        SkTSwap(y1, y2);
        winding = -1;
    }

    int top = SkFDot6Round(y0);
    int bot = SkFDot6Round(y3);

    
    if (top == bot)
        return 0;

    
    if (clip && (top >= clip->fBottom || bot <= clip->fTop))
        return 0;

    
    {
        
        
        
        SkFDot6 dx = cubic_delta_from_line(x0, x1, x2, x3);
        SkFDot6 dy = cubic_delta_from_line(y0, y1, y2, y3);
        
        shift = diff_to_shift(dx, dy) + 1;
    }
    
    SkASSERT(shift > 0);
    if (shift > MAX_COEFF_SHIFT) {
        shift = MAX_COEFF_SHIFT;
    }

    



    int upShift = 6;    
    int downShift = shift + upShift - 10;
    if (downShift < 0) {
        downShift = 0;
        upShift = 10 - shift;
    }

    fWinding    = SkToS8(winding);
    fCurveCount = SkToS8(-1 << shift);
    fCurveShift = SkToU8(shift);
    fCubicDShift = SkToU8(downShift);

    SkFixed B = SkFDot6UpShift(3 * (x1 - x0), upShift);
    SkFixed C = SkFDot6UpShift(3 * (x0 - x1 - x1 + x2), upShift);
    SkFixed D = SkFDot6UpShift(x3 + 3 * (x1 - x2) - x0, upShift);

    fCx     = SkFDot6ToFixed(x0);
    fCDx    = B + (C >> shift) + (D >> 2*shift);    
    fCDDx   = 2*C + (3*D >> (shift - 1));           
    fCDDDx  = 3*D >> (shift - 1);                   

    B = SkFDot6UpShift(3 * (y1 - y0), upShift);
    C = SkFDot6UpShift(3 * (y0 - y1 - y1 + y2), upShift);
    D = SkFDot6UpShift(y3 + 3 * (y1 - y2) - y0, upShift);

    fCy     = SkFDot6ToFixed(y0);
    fCDy    = B + (C >> shift) + (D >> 2*shift);    
    fCDDy   = 2*C + (3*D >> (shift - 1));           
    fCDDDy  = 3*D >> (shift - 1);                   

    fCLastX = SkFDot6ToFixed(x3);
    fCLastY = SkFDot6ToFixed(y3);

    if (clip)
    {
        do {
            if (!this->updateCubic()) {
                return 0;
            }
        } while (!this->intersectsClip(*clip));
        this->chopLineWithClip(*clip);
        return 1;
    }
    return this->updateCubic();
}

int SkCubicEdge::updateCubic()
{
    int     success;
    int     count = fCurveCount;
    SkFixed oldx = fCx;
    SkFixed oldy = fCy;
    SkFixed newx, newy;
    const int ddshift = fCurveShift;
    const int dshift = fCubicDShift;

    SkASSERT(count < 0);

    do {
        if (++count < 0)
        {
            newx    = oldx + (fCDx >> dshift);
            fCDx    += fCDDx >> ddshift;
            fCDDx   += fCDDDx;

            newy    = oldy + (fCDy >> dshift);
            fCDy    += fCDDy >> ddshift;
            fCDDy   += fCDDDy;
        }
        else    
        {
        
            newx    = fCLastX;
            newy    = fCLastY;
        }

        
        
        if (newy < oldy) {
            newy = oldy;
        }

        success = this->updateLine(oldx, oldy, newx, newy);
        oldx = newx;
        oldy = newy;
    } while (count < 0 && !success);

    fCx         = newx;
    fCy         = newy;
    fCurveCount = SkToS8(count);
    return success;
}

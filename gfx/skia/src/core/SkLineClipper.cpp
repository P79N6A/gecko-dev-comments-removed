






#include "SkLineClipper.h"


static SkScalar sect_with_horizontal(const SkPoint src[2], SkScalar Y) {
    SkScalar dy = src[1].fY - src[0].fY;
    if (SkScalarNearlyZero(dy)) {
        return SkScalarAve(src[0].fX, src[1].fX);
    } else {
#ifdef SK_SCALAR_IS_FLOAT
        
        
        double X0 = src[0].fX;
        double Y0 = src[0].fY;
        double X1 = src[1].fX;
        double Y1 = src[1].fY;
        double result = X0 + ((double)Y - Y0) * (X1 - X0) / (Y1 - Y0);
        return (float)result;
#else
        return src[0].fX + SkScalarMulDiv(Y - src[0].fY, src[1].fX - src[0].fX,
                                          dy);
#endif
    }
}


static SkScalar sect_with_vertical(const SkPoint src[2], SkScalar X) {
    SkScalar dx = src[1].fX - src[0].fX;
    if (SkScalarNearlyZero(dx)) {
        return SkScalarAve(src[0].fY, src[1].fY);
    } else {
#ifdef SK_SCALAR_IS_FLOAT
        
        
        double X0 = src[0].fX;
        double Y0 = src[0].fY;
        double X1 = src[1].fX;
        double Y1 = src[1].fY;
        double result = Y0 + ((double)X - X0) * (Y1 - Y0) / (X1 - X0);
        return (float)result;
#else
        return src[0].fY + SkScalarMulDiv(X - src[0].fX, src[1].fY - src[0].fY,
                                          dx);
#endif
    }
}



static inline bool nestedLT(SkScalar a, SkScalar b, SkScalar dim) {
    return a <= b && (a < b || dim > 0);
}



static inline bool containsNoEmptyCheck(const SkRect& outer,
                                        const SkRect& inner) {
    return  outer.fLeft <= inner.fLeft && outer.fTop <= inner.fTop &&
            outer.fRight >= inner.fRight && outer.fBottom >= inner.fBottom;
}

bool SkLineClipper::IntersectLine(const SkPoint src[2], const SkRect& clip,
                                  SkPoint dst[2]) {
    SkRect bounds;
    
    bounds.set(src, 2);
    if (containsNoEmptyCheck(clip, bounds)) {
        if (src != dst) {
            memcpy(dst, src, 2 * sizeof(SkPoint));
        }
        return true;
    }
    
    
    if (nestedLT(bounds.fRight, clip.fLeft, bounds.width()) ||
        nestedLT(clip.fRight, bounds.fLeft, bounds.width()) ||
        nestedLT(bounds.fBottom, clip.fTop, bounds.height()) ||
        nestedLT(clip.fBottom, bounds.fTop, bounds.height())) {
        return false;
    }

    int index0, index1;
    
    if (src[0].fY < src[1].fY) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    SkPoint tmp[2];
    memcpy(tmp, src, sizeof(tmp));

    
    if (tmp[index0].fY < clip.fTop) {
        tmp[index0].set(sect_with_horizontal(src, clip.fTop), clip.fTop);
    }
    if (tmp[index1].fY > clip.fBottom) {
        tmp[index1].set(sect_with_horizontal(src, clip.fBottom), clip.fBottom);
    }
    
    if (tmp[0].fX < tmp[1].fX) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    
    if ((tmp[index1].fX <= clip.fLeft || tmp[index0].fX >= clip.fRight) &&
        tmp[index0].fX < tmp[index1].fX) {
        
        return false;
    }

    if (tmp[index0].fX < clip.fLeft) {
        tmp[index0].set(clip.fLeft, sect_with_vertical(src, clip.fLeft));
    }
    if (tmp[index1].fX > clip.fRight) {
        tmp[index1].set(clip.fRight, sect_with_vertical(src, clip.fRight));
    }
#ifdef SK_DEBUG
    bounds.set(tmp, 2);
    SkASSERT(containsNoEmptyCheck(clip, bounds));
#endif
    memcpy(dst, tmp, sizeof(tmp));
    return true;
}

#ifdef SK_DEBUG


static bool is_between_unsorted(SkScalar value,
                                SkScalar limit0, SkScalar limit1) {
    if (limit0 < limit1) {
        return limit0 <= value && value <= limit1;
    } else {
        return limit1 <= value && value <= limit0;
    }
}
#endif

int SkLineClipper::ClipLine(const SkPoint pts[], const SkRect& clip,
                            SkPoint lines[]) {
    int index0, index1;

    if (pts[0].fY < pts[1].fY) {
        index0 = 0;
        index1 = 1;
    } else {
        index0 = 1;
        index1 = 0;
    }

    

    if (pts[index1].fY <= clip.fTop) {  
        return 0;
    }
    if (pts[index0].fY >= clip.fBottom) {  
        return 0;
    }
    
    

    SkPoint tmp[2];
    memcpy(tmp, pts, sizeof(tmp));

    
    if (pts[index0].fY < clip.fTop) {
        tmp[index0].set(sect_with_horizontal(pts, clip.fTop), clip.fTop);
        SkASSERT(is_between_unsorted(tmp[index0].fX, pts[0].fX, pts[1].fX));
    }
    if (tmp[index1].fY > clip.fBottom) {
        tmp[index1].set(sect_with_horizontal(pts, clip.fBottom), clip.fBottom);
        SkASSERT(is_between_unsorted(tmp[index1].fX, pts[0].fX, pts[1].fX));
    }

    

    
    SkPoint resultStorage[kMaxPoints];
    SkPoint* result;    
    int lineCount = 1;
    bool reverse;

    if (pts[0].fX < pts[1].fX) {
        index0 = 0;
        index1 = 1;
        reverse = false;
    } else {
        index0 = 1;
        index1 = 0;
        reverse = true;
    }

    if (tmp[index1].fX <= clip.fLeft) {  
        tmp[0].fX = tmp[1].fX = clip.fLeft;
        result = tmp;
        reverse = false;
    } else if (tmp[index0].fX >= clip.fRight) {    
        tmp[0].fX = tmp[1].fX = clip.fRight;
        result = tmp;
        reverse = false;
    } else {
        result = resultStorage;
        SkPoint* r = result;
        
        if (tmp[index0].fX < clip.fLeft) {
            r->set(clip.fLeft, tmp[index0].fY);
            r += 1;
            r->set(clip.fLeft, sect_with_vertical(tmp, clip.fLeft));
            SkASSERT(is_between_unsorted(r->fY, tmp[0].fY, tmp[1].fY));
        } else {
            *r = tmp[index0];
        }
        r += 1;

        if (tmp[index1].fX > clip.fRight) {
            r->set(clip.fRight, sect_with_vertical(tmp, clip.fRight));
            SkASSERT(is_between_unsorted(r->fY, tmp[0].fY, tmp[1].fY));
            r += 1;
            r->set(clip.fRight, tmp[index1].fY);
        } else {
            *r = tmp[index1];
        }

        lineCount = r - result;
    }

    
    if (reverse) {
        
        for (int i = 0; i <= lineCount; i++) {
            lines[lineCount - i] = result[i];
        }
    } else {
        memcpy(lines, result, (lineCount + 1) * sizeof(SkPoint));
    }
    return lineCount;
}


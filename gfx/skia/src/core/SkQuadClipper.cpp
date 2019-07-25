








#include "SkQuadClipper.h"
#include "SkGeometry.h"

static inline void clamp_le(SkScalar& value, SkScalar max) {
    if (value > max) {
        value = max;
    }
}

static inline void clamp_ge(SkScalar& value, SkScalar min) {
    if (value < min) {
        value = min;
    }
}

SkQuadClipper::SkQuadClipper() {}

void SkQuadClipper::setClip(const SkIRect& clip) {
    
    fClip.set(clip);
}



static bool chopMonoQuadAt(SkScalar c0, SkScalar c1, SkScalar c2,
                           SkScalar target, SkScalar* t) {
    



    SkScalar A = c0 - c1 - c1 + c2;
    SkScalar B = 2*(c1 - c0);
    SkScalar C = c0 - target;
    
    SkScalar roots[2];  
    int count = SkFindUnitQuadRoots(A, B, C, roots);
    if (count) {
        *t = roots[0];
        return true;
    }
    return false;
}

static bool chopMonoQuadAtY(SkPoint pts[3], SkScalar y, SkScalar* t) {
    return chopMonoQuadAt(pts[0].fY, pts[1].fY, pts[2].fY, y, t);
}







bool SkQuadClipper::clipQuad(const SkPoint srcPts[3], SkPoint dst[3]) {
    bool reverse;
    
    
    if (srcPts[0].fY > srcPts[2].fY) {
        dst[0] = srcPts[2];
        dst[1] = srcPts[1];
        dst[2] = srcPts[0];
        reverse = true;
    } else {
        memcpy(dst, srcPts, 3 * sizeof(SkPoint));
        reverse = false;
    }
    
    
    const SkScalar ctop = fClip.fTop;
    const SkScalar cbot = fClip.fBottom;
    if (dst[2].fY <= ctop || dst[0].fY >= cbot) {
        return false;
    }
    
    SkScalar t;
    SkPoint tmp[5]; 
    
    
    if (dst[0].fY < ctop) {
        if (chopMonoQuadAtY(dst, ctop, &t)) {
            
            SkChopQuadAt(dst, tmp, t);
            dst[0] = tmp[2];
            dst[1] = tmp[3];
        } else {
            
            
            for (int i = 0; i < 3; i++) {
                if (dst[i].fY < ctop) {
                    dst[i].fY = ctop;
                }
            }
        }
    }
    
    
    if (dst[2].fY > cbot) {
        if (chopMonoQuadAtY(dst, cbot, &t)) {
            SkChopQuadAt(dst, tmp, t);
            dst[1] = tmp[1];
            dst[2] = tmp[2];
        } else {
            
            
            for (int i = 0; i < 3; i++) {
                if (dst[i].fY > cbot) {
                    dst[i].fY = cbot;
                }
            }
        }
    }
    
    if (reverse) {
        SkTSwap<SkPoint>(dst[0], dst[2]);
    }
    return true;
}


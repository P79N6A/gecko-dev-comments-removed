






#include "SkDeviceLooper.h"

SkDeviceLooper::SkDeviceLooper(const SkBitmap& base,
                               const SkRasterClip& rc,
                               const SkIRect& bounds, bool aa)
: fBaseBitmap(base)
, fBaseRC(rc)
, fDelta(aa ? kAA_Delta : kBW_Delta)
{
    
    
    fCurrBitmap = NULL;
    fCurrRC = NULL;

    if (!rc.isEmpty()) {
        
        SkASSERT(SkIRect::MakeWH(base.width(), base.height()).contains(rc.getBounds()));
    }

    if (rc.isEmpty() || !fClippedBounds.intersect(bounds, rc.getBounds())) {
        fState = kDone_State;
    } else if (this->fitsInDelta(fClippedBounds)) {
        fState = kSimple_State;
    } else {
        
        
        fCurrOffset.set(fClippedBounds.left() - fDelta,
                        fClippedBounds.top());
        fState = kComplex_State;
    }
}

SkDeviceLooper::~SkDeviceLooper() {
}

void SkDeviceLooper::mapRect(SkRect* dst, const SkRect& src) const {
    SkASSERT(kDone_State != fState);
    SkASSERT(fCurrBitmap);
    SkASSERT(fCurrRC);

    *dst = src;
    dst->offset(SkIntToScalar(-fCurrOffset.fX),
                SkIntToScalar(-fCurrOffset.fY));
}

void SkDeviceLooper::mapMatrix(SkMatrix* dst, const SkMatrix& src) const {
    SkASSERT(kDone_State != fState);
    SkASSERT(fCurrBitmap);
    SkASSERT(fCurrRC);

    *dst = src;
    dst->postTranslate(SkIntToScalar(-fCurrOffset.fX),
                       SkIntToScalar(-fCurrOffset.fY));
}

bool SkDeviceLooper::computeCurrBitmapAndClip() {
    SkASSERT(kComplex_State == fState);

    SkIRect r = SkIRect::MakeXYWH(fCurrOffset.x(), fCurrOffset.y(),
                                  fDelta, fDelta);
    if (!fBaseBitmap.extractSubset(&fSubsetBitmap, r)) {
        fSubsetRC.setEmpty();
    } else {
        fSubsetBitmap.lockPixels();
        fBaseRC.translate(-r.left(), -r.top(), &fSubsetRC);
        (void)fSubsetRC.op(SkIRect::MakeWH(fDelta, fDelta),
                           SkRegion::kIntersect_Op);
    }

    fCurrBitmap = &fSubsetBitmap;
    fCurrRC = &fSubsetRC;
    return !fCurrRC->isEmpty();
}

static bool next_tile(const SkIRect& boundary, int delta, SkIPoint* offset) {
    
    if (offset->x() + delta < boundary.right()) {
        offset->fX += delta;
        return true;
    }

    
    offset->fX = boundary.left();
    if (offset->y() + delta < boundary.bottom()) {
        offset->fY += delta;
        return true;
    }

    
    return false;
}

bool SkDeviceLooper::next() {
    switch (fState) {
        case kDone_State:
            
            
            break;

        case kSimple_State:
            
            if (NULL == fCurrBitmap) {
                fCurrBitmap = &fBaseBitmap;
                fCurrRC = &fBaseRC;
                fCurrOffset.set(0, 0);
                return true;
            }
            
            break;

        case kComplex_State:
            
            

            while (next_tile(fClippedBounds, fDelta, &fCurrOffset)) {
                if (this->computeCurrBitmapAndClip()) {
                    return true;
                }
            }
            break;
    }
    fState = kDone_State;
    return false;
}

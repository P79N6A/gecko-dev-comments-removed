






#ifndef SkDeviceLooper_DEFINED
#define SkDeviceLooper_DEFINED

#include "SkBitmap.h"
#include "SkMatrix.h"
#include "SkRasterClip.h"

















class SkDeviceLooper {
public:
    SkDeviceLooper(const SkBitmap& base, const SkRasterClip&,
                   const SkIRect& bounds, bool aa);
    ~SkDeviceLooper();

    const SkBitmap& getBitmap() const {
        SkASSERT(kDone_State != fState);
        SkASSERT(fCurrBitmap);
        return *fCurrBitmap;
    }

    const SkRasterClip& getRC() const {
        SkASSERT(kDone_State != fState);
        SkASSERT(fCurrRC);
        return *fCurrRC;
    }

    void mapRect(SkRect* dst, const SkRect& src) const;
    void mapMatrix(SkMatrix* dst, const SkMatrix& src) const;

    








    bool next();

private:
    const SkBitmap&     fBaseBitmap;
    const SkRasterClip& fBaseRC;

    enum State {
        kDone_State,    
        kSimple_State,  
        kComplex_State
    };

    
    SkBitmap            fSubsetBitmap;
    SkRasterClip        fSubsetRC;

    const SkBitmap*     fCurrBitmap;
    const SkRasterClip* fCurrRC;
    SkIRect             fClippedBounds;
    SkIPoint            fCurrOffset;
    int                 fDelta;
    State               fState;

    enum Delta {
        kBW_Delta = 1 << 14,        
        kAA_Delta = kBW_Delta >> 2  
    };

    bool fitsInDelta(const SkIRect& r) const {
        return r.right() < fDelta && r.bottom() < fDelta;
    }

    bool computeCurrBitmapAndClip();
};

#endif

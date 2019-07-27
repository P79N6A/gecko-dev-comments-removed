






#ifndef SkGrPixelRef_DEFINED
#define SkGrPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkPixelRef.h"
#include "GrTexture.h"
#include "GrRenderTarget.h"






class SK_API SkROLockPixelsPixelRef : public SkPixelRef {
public:
    SK_DECLARE_INST_COUNT(SkROLockPixelsPixelRef)
    SkROLockPixelsPixelRef(const SkImageInfo&);
    virtual ~SkROLockPixelsPixelRef();

protected:
    virtual bool onNewLockPixels(LockRec*) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual bool onLockPixelsAreWritable() const SK_OVERRIDE;   

private:
    SkBitmap    fBitmap;
    typedef SkPixelRef INHERITED;
};




class SK_API SkGrPixelRef : public SkROLockPixelsPixelRef {
public:
    SK_DECLARE_INST_COUNT(SkGrPixelRef)
    




    SkGrPixelRef(const SkImageInfo&, GrSurface*, bool transferCacheLock = false);
    virtual ~SkGrPixelRef();

    
    virtual GrTexture* getTexture() SK_OVERRIDE;

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset) SK_OVERRIDE;
    virtual SkPixelRef* deepCopy(SkColorType, const SkIRect* subset) SK_OVERRIDE;

private:
    GrSurface*  fSurface;
    bool        fUnlock;   

    typedef SkROLockPixelsPixelRef INHERITED;
};

#endif

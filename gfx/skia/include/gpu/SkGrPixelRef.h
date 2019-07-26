









#ifndef SkGrPixelRef_DEFINED
#define SkGrPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkPixelRef.h"
#include "GrTexture.h"
#include "GrRenderTarget.h"






class SK_API SkROLockPixelsPixelRef : public SkPixelRef {
public:
    SkROLockPixelsPixelRef();
    virtual ~SkROLockPixelsPixelRef();

protected:
    
    virtual void* onLockPixels(SkColorTable** ptr);
    virtual void onUnlockPixels();
    virtual bool onLockPixelsAreWritable() const;   

private:
    SkBitmap    fBitmap;
    typedef SkPixelRef INHERITED;
};




class SK_API SkGrPixelRef : public SkROLockPixelsPixelRef {
public:
    




    SkGrPixelRef(GrSurface* surface, bool transferCacheLock = false);
    virtual ~SkGrPixelRef();

    
    virtual SkGpuTexture* getTexture() SK_OVERRIDE;

    SK_DECLARE_UNFLATTENABLE_OBJECT()

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset) SK_OVERRIDE;
    virtual SkPixelRef* deepCopy(SkBitmap::Config dstConfig) SK_OVERRIDE;

private:
    GrSurface*  fSurface;
    bool        fUnlock;   

    typedef SkROLockPixelsPixelRef INHERITED;
};

#endif


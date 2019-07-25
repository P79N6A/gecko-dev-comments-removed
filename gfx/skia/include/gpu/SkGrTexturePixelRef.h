









#ifndef SkGrTexturePixelRef_DEFINED
#define SkGrTexturePixelRef_DEFINED

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




class SK_API SkGrTexturePixelRef : public SkROLockPixelsPixelRef {
public:
            SkGrTexturePixelRef(GrTexture*);
    virtual ~SkGrTexturePixelRef();

    
    virtual SkGpuTexture* getTexture();

    SK_DECLARE_UNFLATTENABLE_OBJECT()

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset);

    
    virtual SkPixelRef* deepCopy(SkBitmap::Config dstConfig) SK_OVERRIDE;

private:
    GrTexture*  fTexture;
    typedef SkROLockPixelsPixelRef INHERITED;
};




class SK_API SkGrRenderTargetPixelRef : public SkROLockPixelsPixelRef {
public:
            SkGrRenderTargetPixelRef(GrRenderTarget* rt);
    virtual ~SkGrRenderTargetPixelRef();

    
    virtual SkGpuTexture* getTexture();

    SK_DECLARE_UNFLATTENABLE_OBJECT()

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset);

    
    virtual SkPixelRef* deepCopy(SkBitmap::Config dstConfig) SK_OVERRIDE;

private:
    GrRenderTarget*  fRenderTarget;
    typedef SkROLockPixelsPixelRef INHERITED;
};

#endif


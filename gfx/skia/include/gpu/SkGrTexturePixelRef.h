









#ifndef SkGrTexturePixelRef_DEFINED
#define SkGrTexturePixelRef_DEFINED

#include "SkBitmap.h"
#include "SkPixelRef.h"
#include "GrTexture.h"
#include "GrRenderTarget.h"






class SkROLockPixelsPixelRef : public SkPixelRef {
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




class SkGrTexturePixelRef : public SkROLockPixelsPixelRef {
public:
            SkGrTexturePixelRef(GrTexture*);
    virtual ~SkGrTexturePixelRef();

    
    virtual SkGpuTexture* getTexture();

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset);

private:
    GrTexture*  fTexture;
    typedef SkROLockPixelsPixelRef INHERITED;
};




class SkGrRenderTargetPixelRef : public SkROLockPixelsPixelRef {
public:
            SkGrRenderTargetPixelRef(GrRenderTarget* rt);
    virtual ~SkGrRenderTargetPixelRef();

    
    virtual SkGpuTexture* getTexture();

protected:
    
    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subset);

private:
    GrRenderTarget*  fRenderTarget;
    typedef SkROLockPixelsPixelRef INHERITED;
};

#endif


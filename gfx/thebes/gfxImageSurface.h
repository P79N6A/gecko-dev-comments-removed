




#ifndef GFX_IMAGESURFACE_H
#define GFX_IMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxPoint.h"



class gfxSubimageSurface;

namespace mozilla {
namespace gfx {
class SourceSurface;
}
}






class THEBES_API gfxImageSurface : public gfxASurface {
public:
    








    gfxImageSurface(unsigned char *aData, const gfxIntSize& aSize,
                    long aStride, gfxImageFormat aFormat);

    






    gfxImageSurface(const gfxIntSize& size, gfxImageFormat format, bool aClear = true);
    gfxImageSurface(cairo_surface_t *csurf);

    virtual ~gfxImageSurface();

    
    gfxImageFormat Format() const { return mFormat; }

    virtual const gfxIntSize GetSize() const { return mSize; }
    int32_t Width() const { return mSize.width; }
    int32_t Height() const { return mSize.height; }

    



    int32_t Stride() const { return mStride; }
    



    unsigned char* Data() const { return mData; } 
    


    int32_t GetDataSize() const { return mStride*mSize.height; }

    
    bool CopyFrom (gfxImageSurface *other);

    



    bool CopyFrom (mozilla::gfx::SourceSurface *aSurface);

    


    already_AddRefed<gfxSubimageSurface> GetSubimage(const gfxRect& aRect);

    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface();

    
    virtual void MovePixels(const nsIntRect& aSourceRect,
                            const nsIntPoint& aDestTopLeft) MOZ_OVERRIDE;

    static long ComputeStride(const gfxIntSize&, gfxImageFormat);
protected:
    gfxImageSurface();
    void InitWithData(unsigned char *aData, const gfxIntSize& aSize,
                      long aStride, gfxImageFormat aFormat);
    void InitFromSurface(cairo_surface_t *csurf);
    long ComputeStride() const { return ComputeStride(mSize, mFormat); }


    void MakeInvalid();

    gfxIntSize mSize;
    bool mOwnsData;
    unsigned char *mData;
    gfxImageFormat mFormat;
    long mStride;
};

class THEBES_API gfxSubimageSurface : public gfxImageSurface {
protected:
    friend class gfxImageSurface;
    gfxSubimageSurface(gfxImageSurface* aParent,
                       unsigned char* aData,
                       const gfxIntSize& aSize);
private:
    nsRefPtr<gfxImageSurface> mParent;
};

#endif 

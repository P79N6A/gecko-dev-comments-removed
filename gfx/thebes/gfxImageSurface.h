




#ifndef GFX_IMAGESURFACE_H
#define GFX_IMAGESURFACE_H

#include "mozilla/MemoryReporting.h"
#include "mozilla/RefPtr.h"
#include "gfxASurface.h"
#include "nsAutoPtr.h"
#include "nsSize.h"



class gfxSubimageSurface;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
class SourceSurface;
}
}






class gfxImageSurface : public gfxASurface {
public:
    








    gfxImageSurface(unsigned char *aData, const mozilla::gfx::IntSize& aSize,
                    long aStride, gfxImageFormat aFormat);

    






    gfxImageSurface(const mozilla::gfx::IntSize& size, gfxImageFormat format, bool aClear = true);

    














    gfxImageSurface(const mozilla::gfx::IntSize& aSize, gfxImageFormat aFormat,
                    long aStride, int32_t aMinimalAllocation, bool aClear);

    explicit gfxImageSurface(cairo_surface_t *csurf);

    virtual ~gfxImageSurface();

    
    gfxImageFormat Format() const { return mFormat; }

    virtual const mozilla::gfx::IntSize GetSize() const override { return mSize; }
    int32_t Width() const { return mSize.width; }
    int32_t Height() const { return mSize.height; }

    



    int32_t Stride() const { return mStride; }
    



    unsigned char* Data() const { return mData; } 
    


    int32_t GetDataSize() const { return mStride*mSize.height; }

    
    bool CopyFrom (gfxImageSurface *other);

    



    bool CopyFrom (mozilla::gfx::SourceSurface *aSurface);

    



    bool CopyTo (mozilla::gfx::SourceSurface *aSurface);

    



    virtual already_AddRefed<mozilla::gfx::DataSourceSurface> CopyToB8G8R8A8DataSourceSurface();

    


    already_AddRefed<gfxSubimageSurface> GetSubimage(const gfxRect& aRect);

    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface() override;

    
    static long ComputeStride(const mozilla::gfx::IntSize&, gfxImageFormat);

    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        override;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        override;
    virtual bool SizeOfIsMeasured() const override;

protected:
    gfxImageSurface();
    void InitWithData(unsigned char *aData, const mozilla::gfx::IntSize& aSize,
                      long aStride, gfxImageFormat aFormat);
    




    void AllocateAndInit(long aStride, int32_t aMinimalAllocation, bool aClear);
    void InitFromSurface(cairo_surface_t *csurf);

    long ComputeStride() const { return ComputeStride(mSize, mFormat); }


    void MakeInvalid();

    mozilla::gfx::IntSize mSize;
    bool mOwnsData;
    unsigned char *mData;
    gfxImageFormat mFormat;
    long mStride;
};

class gfxSubimageSurface : public gfxImageSurface {
protected:
    friend class gfxImageSurface;
    gfxSubimageSurface(gfxImageSurface* aParent,
                       unsigned char* aData,
                       const mozilla::gfx::IntSize& aSize,
                       gfxImageFormat aFormat);
private:
    nsRefPtr<gfxImageSurface> mParent;
};

#endif 

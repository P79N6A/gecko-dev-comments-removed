




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
    








    gfxImageSurface(unsigned char *aData, const gfxIntSize& aSize,
                    long aStride, gfxImageFormat aFormat);

    






    gfxImageSurface(const gfxIntSize& size, gfxImageFormat format, bool aClear = true);

    














    gfxImageSurface(const gfxIntSize& aSize, gfxImageFormat aFormat,
                    long aStride, int32_t aMinimalAllocation, bool aClear);

    explicit gfxImageSurface(cairo_surface_t *csurf);

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

    



    bool CopyTo (mozilla::gfx::SourceSurface *aSurface);

    



    virtual mozilla::TemporaryRef<mozilla::gfx::DataSourceSurface> CopyToB8G8R8A8DataSourceSurface();

    


    already_AddRefed<gfxSubimageSurface> GetSubimage(const gfxRect& aRect);

    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface();

    
    static long ComputeStride(const gfxIntSize&, gfxImageFormat);

    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        MOZ_OVERRIDE;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        MOZ_OVERRIDE;
    virtual bool SizeOfIsMeasured() const MOZ_OVERRIDE;

protected:
    gfxImageSurface();
    void InitWithData(unsigned char *aData, const gfxIntSize& aSize,
                      long aStride, gfxImageFormat aFormat);
    




    void AllocateAndInit(long aStride, int32_t aMinimalAllocation, bool aClear);
    void InitFromSurface(cairo_surface_t *csurf);

    long ComputeStride() const { return ComputeStride(mSize, mFormat); }


    void MakeInvalid();

    gfxIntSize mSize;
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
                       const gfxIntSize& aSize,
                       gfxImageFormat aFormat);
private:
    nsRefPtr<gfxImageSurface> mParent;
};

#endif 

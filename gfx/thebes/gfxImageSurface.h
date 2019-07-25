




































#ifndef GFX_IMAGESURFACE_H
#define GFX_IMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxPoint.h"



class gfxSubimageSurface;






class THEBES_API gfxImageSurface : public gfxASurface {
public:
    








    gfxImageSurface(unsigned char *aData, const gfxIntSize& aSize,
                    long aStride, gfxImageFormat aFormat);

    






    gfxImageSurface(const gfxIntSize& size, gfxImageFormat format);
    gfxImageSurface(cairo_surface_t *csurf);

    virtual ~gfxImageSurface();

    
    gfxImageFormat Format() const { return mFormat; }

    virtual const gfxIntSize GetSize() const { return mSize; }
    PRInt32 Width() const { return mSize.width; }
    PRInt32 Height() const { return mSize.height; }

    



    PRInt32 Stride() const { return mStride; }
    



    unsigned char* Data() const { return mData; } 
    


    PRInt32 GetDataSize() const { return mStride*mSize.height; }

    
    bool CopyFrom (gfxImageSurface *other);

    


    already_AddRefed<gfxSubimageSurface> GetSubimage(const gfxRect& aRect);

    virtual already_AddRefed<gfxImageSurface> GetAsImageSurface();

    
    NS_OVERRIDE
    virtual void MovePixels(const nsIntRect& aSourceRect,
                            const nsIntPoint& aDestTopLeft);

protected:
    gfxImageSurface();
    void InitWithData(unsigned char *aData, const gfxIntSize& aSize,
                      long aStride, gfxImageFormat aFormat);
    void InitFromSurface(cairo_surface_t *csurf);
    long ComputeStride() const { return ComputeStride(mSize, mFormat); }

    static long ComputeStride(const gfxIntSize&, gfxImageFormat);

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

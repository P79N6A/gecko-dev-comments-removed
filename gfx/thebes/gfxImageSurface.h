




































#ifndef GFX_IMAGESURFACE_H
#define GFX_IMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxPoint.h"








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

    
    PRBool CopyFrom (gfxImageSurface *other);

protected:
    gfxImageSurface();
    void InitFromSurface(cairo_surface_t *csurf);
    long ComputeStride() const;

    gfxIntSize mSize;
    PRBool mOwnsData;
    unsigned char *mData;
    gfxImageFormat mFormat;
    long mStride;
};

#endif 

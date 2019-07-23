




































#ifndef GFX_IMAGESURFACE_H
#define GFX_IMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxPoint.h"








class THEBES_API gfxImageSurface : public gfxASurface {
public:
    









    gfxImageSurface(const gfxIntSize& size, gfxImageFormat format);
    gfxImageSurface(cairo_surface_t *csurf);

    virtual ~gfxImageSurface();

    
    gfxImageFormat Format() const { return mFormat; }

    const gfxIntSize& GetSize() const { return mSize; }

    



    long Stride() const { return mStride; }
    



    unsigned char* Data() { return mData; } 

private:
    long ComputeStride() const;

    gfxIntSize mSize;
    PRBool mOwnsData;
    unsigned char *mData;
    gfxImageFormat mFormat;
    long mStride;
};

#endif 

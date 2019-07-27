




#ifndef GFX_QUARTZIMAGESURFACE_H
#define GFX_QUARTZIMAGESURFACE_H

#include "gfxASurface.h"
#include "nsSize.h"

class gfxImageSurface;

class gfxQuartzImageSurface : public gfxASurface {
public:
    explicit gfxQuartzImageSurface(gfxImageSurface *imageSurface);
    explicit gfxQuartzImageSurface(cairo_surface_t *csurf);

    virtual ~gfxQuartzImageSurface();

    already_AddRefed<gfxImageSurface> GetAsImageSurface();
    virtual int32_t KnownMemoryUsed();
    virtual const mozilla::gfx::IntSize GetSize() const { return mSize; }

protected:
    mozilla::gfx::IntSize mSize;

private:
    mozilla::gfx::IntSize ComputeSize();
};

#endif 

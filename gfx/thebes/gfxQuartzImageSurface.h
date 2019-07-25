




































#ifndef GFX_QUARTZIMAGESURFACE_H
#define GFX_QUARTZIMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

class THEBES_API gfxQuartzImageSurface : public gfxASurface {
public:
    gfxQuartzImageSurface(gfxImageSurface *imageSurface);
    gfxQuartzImageSurface(cairo_surface_t *csurf);

    virtual ~gfxQuartzImageSurface();

    already_AddRefed<gfxImageSurface> GetAsImageSurface();
};

#endif 

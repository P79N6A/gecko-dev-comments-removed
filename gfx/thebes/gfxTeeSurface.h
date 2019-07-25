




































#ifndef GFX_TEESURFACE_H
#define GFX_TEESURFACE_H

#include "gfxASurface.h"
#include "nsTArray.h"






class THEBES_API gfxTeeSurface : public gfxASurface {
public:
    gfxTeeSurface(cairo_surface_t *csurf);
    gfxTeeSurface(gfxASurface **aSurfaces, PRInt32 aSurfaceCount);

    virtual const gfxIntSize GetSize() const;

    void GetSurfaces(nsTArray<nsRefPtr<gfxASurface> > *aSurfaces);
};

#endif 

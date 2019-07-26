




#ifndef GFX_TEESURFACE_H
#define GFX_TEESURFACE_H

#include "gfxASurface.h"
#include "nsTArray.h"
#include "nsSize.h"










class gfxTeeSurface : public gfxASurface {
public:
    gfxTeeSurface(cairo_surface_t *csurf);
    gfxTeeSurface(gfxASurface **aSurfaces, int32_t aSurfaceCount);

    virtual const gfxIntSize GetSize() const;

    


    void GetSurfaces(nsTArray<nsRefPtr<gfxASurface> > *aSurfaces);
};

#endif 

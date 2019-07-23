




































#ifndef GFX_DIRECTFBSURFACE_H
#define GFX_DIRECTFBSURFACE_H

#include "gfxASurface.h"

extern "C" {
#include "direct/messages.h"

typedef struct _IDirectFB IDirectFB;
typedef struct _IDirectFBSurface IDirectFBSurface;

}

class THEBES_API gfxDirectFBSurface : public gfxASurface {
public:
    gfxDirectFBSurface(IDirectFB *dfb, IDirectFBSurface *surface);
    gfxDirectFBSurface(IDirectFBSurface *surface);
    gfxDirectFBSurface(cairo_surface_t *csurf);

    gfxDirectFBSurface(const gfxIntSize& size, gfxImageFormat format);

    virtual ~gfxDirectFBSurface();

    IDirectFB* DirectFB() { return mDFB; }
    IDirectFBSurface* DirectFBSurface() { return mDFBSurface; }

protected:
    IDirectFB *mDFB;
    IDirectFBSurface *mDFBSurface;
};

#endif 






































#include "gfxTeeSurface.h"




#ifdef MOZ_TREE_CAIRO
#include "cairo.h"
#else
#include "cairo-tee.h"
#endif

gfxTeeSurface::gfxTeeSurface(cairo_surface_t *csurf)
{
    Init(csurf, PR_TRUE);
}

gfxTeeSurface::gfxTeeSurface(gfxASurface **aSurfaces, PRInt32 aSurfaceCount)
{
    NS_ASSERTION(aSurfaceCount > 0, "Must have a least one surface");
    cairo_surface_t *csurf = cairo_tee_surface_create(aSurfaces[0]->CairoSurface());
    Init(csurf, PR_FALSE);

    for (PRInt32 i = 1; i < aSurfaceCount; ++i) {
        cairo_tee_surface_add(csurf, aSurfaces[i]->CairoSurface());
    }
}

const gfxIntSize
gfxTeeSurface::GetSize() const
{
    nsRefPtr<gfxASurface> master = Wrap(cairo_tee_surface_index(mSurface, 0));
    return master->GetSize();
}

void
gfxTeeSurface::GetSurfaces(nsTArray<nsRefPtr<gfxASurface> >* aSurfaces)
{
    for (PRInt32 i = 0; ; ++i) {
        cairo_surface_t *csurf = cairo_tee_surface_index(mSurface, i);
        if (cairo_surface_status(csurf))
            break;
        nsRefPtr<gfxASurface> *elem = aSurfaces->AppendElement();
        if (!elem)
            return;
        *elem = Wrap(csurf);
    }
}

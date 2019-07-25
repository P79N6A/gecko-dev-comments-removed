




































#include "gfxQuartzImageSurface.h"

#include "cairo-quartz.h"
#include "cairo-quartz-image.h"

gfxQuartzImageSurface::gfxQuartzImageSurface(gfxImageSurface *imageSurface)
{
    if (imageSurface->CairoSurface() == NULL)
        return;

    cairo_surface_t *surf = cairo_quartz_image_surface_create (imageSurface->CairoSurface());
    Init (surf);
}

gfxQuartzImageSurface::gfxQuartzImageSurface(cairo_surface_t *csurf)
{
    Init (csurf, true);
}

gfxQuartzImageSurface::~gfxQuartzImageSurface()
{
}

PRInt32
gfxQuartzImageSurface::KnownMemoryUsed()
{
  
  
  nsRefPtr<gfxImageSurface> imgSurface = GetAsImageSurface();
  if (imgSurface)
    return imgSurface->KnownMemoryUsed();
  return 0;
}

already_AddRefed<gfxImageSurface>
gfxQuartzImageSurface::GetAsImageSurface()
{
    if (!mSurfaceValid)
        return nsnull;

    cairo_surface_t *isurf = cairo_quartz_image_surface_get_image (CairoSurface());
    if (!isurf) {
        NS_WARNING ("Couldn't obtain an image surface from a QuartzImageSurface?!");
        return nsnull;
    }

    nsRefPtr<gfxASurface> asurf = gfxASurface::Wrap(isurf);
    gfxImageSurface *imgsurf = (gfxImageSurface*) asurf.get();
    NS_ADDREF(imgsurf);
    return imgsurf;
}

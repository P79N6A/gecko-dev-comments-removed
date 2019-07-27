




#include "gfxQuartzImageSurface.h"
#include "gfxImageSurface.h"

#include "cairo-quartz-image.h"

gfxQuartzImageSurface::gfxQuartzImageSurface(gfxImageSurface *imageSurface)
{
    if (imageSurface->CairoSurface() == nullptr)
        return;

    cairo_surface_t *surf = cairo_quartz_image_surface_create (imageSurface->CairoSurface());
    Init (surf);
    mSize = ComputeSize();
}

gfxQuartzImageSurface::gfxQuartzImageSurface(cairo_surface_t *csurf)
{
    Init (csurf, true);
    mSize = ComputeSize();
}

gfxQuartzImageSurface::~gfxQuartzImageSurface()
{
}

mozilla::gfx::IntSize
gfxQuartzImageSurface::ComputeSize()
{
  if (mSurfaceValid) {
    cairo_surface_t* isurf = cairo_quartz_image_surface_get_image(mSurface);
    if (isurf) {
      return mozilla::gfx::IntSize(cairo_image_surface_get_width(isurf),
                                   cairo_image_surface_get_height(isurf));
    }
  }

  
  
  return mozilla::gfx::IntSize(-1, -1);
}

int32_t
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
        return nullptr;

    cairo_surface_t *isurf = cairo_quartz_image_surface_get_image (CairoSurface());
    if (!isurf) {
        NS_WARNING ("Couldn't obtain an image surface from a QuartzImageSurface?!");
        return nullptr;
    }

    nsRefPtr<gfxImageSurface> result = gfxASurface::Wrap(isurf).downcast<gfxImageSurface>();
    result->SetOpaqueRect(GetOpaqueRect());

    return result.forget();
}

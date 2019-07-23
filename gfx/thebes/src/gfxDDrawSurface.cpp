




































#include "gfxDDrawSurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

#include "cairo.h"
#include "cairo-ddraw.h"

#include "nsString.h"

gfxDDrawSurface::gfxDDrawSurface(LPDIRECTDRAW lpdd,
                                 const gfxIntSize& size, gfxImageFormat imageFormat)
{
    if (!CheckSurfaceSize(size))
        return;

    cairo_surface_t *surf = cairo_ddraw_surface_create(lpdd, (cairo_format_t)imageFormat,
                                                       size.width, size.height);
    Init(surf);
}

gfxDDrawSurface::gfxDDrawSurface(gfxDDrawSurface * psurf, const RECT & rect)
{
    cairo_surface_t *surf = cairo_ddraw_surface_create_alias(psurf->CairoSurface(),
                                                             rect.left, rect.top,
                                                             rect.right - rect.left,
                                                             rect.bottom - rect.top);
    Init(surf);
}

gfxDDrawSurface::gfxDDrawSurface(cairo_surface_t *csurf)
{
    Init(csurf, PR_TRUE);
}

gfxDDrawSurface::~gfxDDrawSurface()
{
}

LPDIRECTDRAWSURFACE gfxDDrawSurface::GetDDSurface()
{
    return cairo_ddraw_surface_get_ddraw_surface(CairoSurface());
}

nsresult gfxDDrawSurface::BeginPrinting(const nsAString& aTitle,
                                          const nsAString& aPrintToFileName)
{
    return NS_OK;
}

nsresult gfxDDrawSurface::EndPrinting()
{
    return NS_OK;
}

nsresult gfxDDrawSurface::AbortPrinting()
{
    return NS_OK;
}

nsresult gfxDDrawSurface::BeginPage()
{
    return NS_OK;
}

nsresult gfxDDrawSurface::EndPage()
{
    return NS_OK;
}

PRInt32 gfxDDrawSurface::GetDefaultContextFlags() const
{
    return 0;
}

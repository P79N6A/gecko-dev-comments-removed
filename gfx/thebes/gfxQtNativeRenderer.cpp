




#include "gfxQtNativeRenderer.h"
#include "gfxContext.h"
#include "gfxUtils.h"
#include "gfxXlibSurface.h"

nsresult
gfxQtNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                          uint32_t flags, Screen* screen, Visual* visual)
{
    Display *dpy = DisplayOfScreen(screen);
    bool isOpaque = (flags & DRAW_IS_OPAQUE) ? true : false;
    int screenNumber = screen - ScreenOfDisplay(dpy, 0);

    if (!isOpaque) {
        int depth = 32;
        XVisualInfo vinfo;
        int foundVisual = XMatchVisualInfo(dpy, screenNumber,
                                           depth, TrueColor,
                                           &vinfo);
        if (!foundVisual)
            return NS_ERROR_FAILURE;

        visual = vinfo.visual;
    }

    nsRefPtr<gfxXlibSurface> xsurf =
        gfxXlibSurface::Create(screen, visual,
                               gfxIntSize(size.width, size.height));

    if (!isOpaque) {
        gfxUtils::ClearThebesSurface(xsurf);
    }

    nsresult rv = DrawWithXlib(xsurf->CairoSurface(), nsIntPoint(0, 0), nullptr, 0);

    if (NS_FAILED(rv))
        return rv;

    ctx->SetSource(xsurf);
    ctx->Paint();

    return rv;
}






































#include "gfxQtNativeRenderer.h"
#include "gfxContext.h"
#include "gfxXlibSurface.h"

nsresult
gfxQtNativeRenderer::Draw(gfxContext* ctx, nsIntSize size,
                          PRUint32 flags, Screen* screen, Visual* visual,
                          DrawOutput* output)
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
        nsRefPtr<gfxContext> tempCtx = new gfxContext(xsurf);
        tempCtx->SetOperator(gfxContext::OPERATOR_CLEAR);
        tempCtx->Paint();
    }

    nsresult rv = DrawWithXlib(xsurf.get(), nsIntPoint(0, 0), NULL, 0);

    if (NS_FAILED(rv))
        return rv;

    ctx->SetSource(xsurf);
    ctx->Paint();

    return rv;
}

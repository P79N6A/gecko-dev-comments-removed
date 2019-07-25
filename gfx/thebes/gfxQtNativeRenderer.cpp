




































#include <QX11Info>

#include "gfxQtNativeRenderer.h"
#include "gfxContext.h"
#include "gfxXlibSurface.h"

nsresult
gfxQtNativeRenderer::Draw(gfxContext* ctx, int width, int height,
                          PRUint32 flags, DrawOutput* output)
{
    Display *dpy = QX11Info().display();
    PRBool isOpaque = (flags & DRAW_IS_OPAQUE) ? PR_TRUE : PR_FALSE;
    int screen = QX11Info().screen();
    Visual *visual = static_cast<Visual*>(QX11Info().visual());
    Colormap colormap = QX11Info().colormap();
    PRBool allocColormap = PR_FALSE;

    if (!isOpaque) {
        int depth = 32;
        XVisualInfo vinfo;
        int foundVisual = XMatchVisualInfo(dpy, screen,
                                           depth, TrueColor,
                                           &vinfo);
        if (!foundVisual)
            return NS_ERROR_FAILURE;

        if (visual != vinfo.visual) {
            allocColormap = PR_TRUE;
            visual = vinfo.visual;
            colormap = XCreateColormap(dpy,
                                       RootWindow(dpy, screen),
                                       visual, AllocNone);
        }
    }

    nsRefPtr<gfxXlibSurface> xsurf =
        gfxXlibSurface::Create(ScreenOfDisplay(dpy, screen), visual,
                               gfxIntSize(width, height));

    if (!isOpaque) {
        nsRefPtr<gfxContext> tempCtx = new gfxContext(xsurf);
        tempCtx->SetOperator(gfxContext::OPERATOR_CLEAR);
        tempCtx->Paint();
    }

    nsresult rv = NativeDraw(xsurf.get(), colormap, 0, 0, NULL, 0);

    if (!allocColormap)
        XFreeColormap(dpy, colormap);

    if (NS_FAILED(rv))
        return rv;

    ctx->SetSource(xsurf);
    ctx->Paint();

    return rv;
}

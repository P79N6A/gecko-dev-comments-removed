





































#include "gfxXlibSurface.h"

#include "cairo.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"




#define XLIB_IMAGE_SIDE_SIZE_LIMIT 0x7fff

gfxXlibSurface::gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual)
    : mPixmapTaken(PR_FALSE), mDisplay(dpy), mDrawable(drawable)
{
    DoSizeQuery();
    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, drawable, visual, mSize.width, mSize.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual, const gfxIntSize& size)
    : mPixmapTaken(PR_FALSE), mDisplay(dpy), mDrawable(drawable), mSize(size)
{
    if (!CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT))
        return;

    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, drawable, visual, mSize.width, mSize.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Display *dpy, Visual *visual, const gfxIntSize& size, int depth)
    : mPixmapTaken(PR_FALSE), mDisplay(dpy), mSize(size)

{
    if (!CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT))
        return;

    mDrawable = (Drawable)XCreatePixmap(dpy,
                                        RootWindow(dpy, DefaultScreen(dpy)),
                                        mSize.width, mSize.height,
                                        depth ? depth : DefaultDepth(dpy, DefaultScreen(dpy)));

    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, mDrawable, visual, mSize.width, mSize.height);

    Init(surf);
    TakePixmap();
}

gfxXlibSurface::gfxXlibSurface(Display *dpy, Drawable drawable, XRenderPictFormat *format,
                               const gfxIntSize& size)
    : mPixmapTaken(PR_FALSE), mDisplay(dpy), mDrawable(drawable), mSize(size)
{
    if (!CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT))
        return;

    cairo_surface_t *surf = cairo_xlib_surface_create_with_xrender_format(dpy, drawable,
                                                                          ScreenOfDisplay(dpy,DefaultScreen(dpy)),
                                                                          format, mSize.width, mSize.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Display *dpy, XRenderPictFormat *format, const gfxIntSize& size)
    : mPixmapTaken(PR_FALSE), mDisplay(dpy), mSize(size)
{
    mDrawable = (Drawable)XCreatePixmap(dpy,
                                        RootWindow(dpy, DefaultScreen(dpy)),
                                        mSize.width, mSize.height,
                                        format->depth);

    cairo_surface_t *surf = cairo_xlib_surface_create_with_xrender_format(dpy, mDrawable,
                                                                          ScreenOfDisplay(dpy,DefaultScreen(dpy)),
                                                                          format, mSize.width, mSize.height);
    Init(surf);
    TakePixmap();
}

gfxXlibSurface::gfxXlibSurface(cairo_surface_t *csurf)
    : mPixmapTaken(PR_FALSE),
      mSize(cairo_xlib_surface_get_width(csurf),
            cairo_xlib_surface_get_height(csurf))
{
    mDrawable = cairo_xlib_surface_get_drawable(csurf);
    mDisplay = cairo_xlib_surface_get_display(csurf);

    Init(csurf, PR_TRUE);
}

gfxXlibSurface::~gfxXlibSurface()
{
    if (mPixmapTaken) {
        XFreePixmap (mDisplay, mDrawable);
    }
}

void
gfxXlibSurface::DoSizeQuery()
{
    
    Window root_ignore;
    int x_ignore, y_ignore;
    unsigned int bwidth_ignore, width, height, depth;

    XGetGeometry(mDisplay,
                 mDrawable,
                 &root_ignore, &x_ignore, &y_ignore,
                 &width, &height,
                 &bwidth_ignore, &depth);

    mSize.width = width;
    mSize.height = height;
}

XRenderPictFormat*
gfxXlibSurface::FindRenderFormat(Display *dpy, gfxImageFormat format)
{
    switch (format) {
        case ImageFormatARGB32:
            return XRenderFindStandardFormat (dpy, PictStandardARGB32);
            break;
        case ImageFormatRGB24:
            return XRenderFindStandardFormat (dpy, PictStandardRGB24);
            break;
        case ImageFormatRGB16_565: {
            
            
            
            Visual *visual = NULL;
            Screen *screen = DefaultScreenOfDisplay(dpy);
            int j;
            for (j = 0; j < screen->ndepths; j++) {
                Depth *d = &screen->depths[j];
                if (d->depth == 16 && d->nvisuals && &d->visuals[0]) {
                    if (d->visuals[0].red_mask   == 0xf800 &&
                        d->visuals[0].green_mask == 0x7e0 &&
                        d->visuals[0].blue_mask  == 0x1f)
                        visual = &d->visuals[0];
                    break;
                }
            }
            if (!visual)
                return NULL;
            return XRenderFindVisualFormat(dpy, visual);
            break;
        }
        case ImageFormatA8:
            return XRenderFindStandardFormat (dpy, PictStandardA8);
            break;
        case ImageFormatA1:
            return XRenderFindStandardFormat (dpy, PictStandardA1);
            break;
        default:
            return NULL;
    }

    return (XRenderPictFormat*)NULL;
}

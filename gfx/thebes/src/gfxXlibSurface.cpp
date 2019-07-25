





































#include "gfxXlibSurface.h"

#include "cairo.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"

static cairo_user_data_key_t pixmap_free_key;

typedef struct {
    Display* dpy;
    Pixmap pixmap;
} pixmap_free_struct;

static void pixmap_free_func (void *);

#define XLIB_IMAGE_SIDE_SIZE_LIMIT 0xffff

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
    : mPixmapTaken(PR_FALSE), mSize(-1.0, -1.0)
{
    mDrawable = cairo_xlib_surface_get_drawable(csurf);
    mDisplay = cairo_xlib_surface_get_display(csurf);

    Init(csurf, PR_TRUE);
}

gfxXlibSurface::~gfxXlibSurface()
{
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

void
gfxXlibSurface::TakePixmap()
{
    if (mPixmapTaken)
        return;

    pixmap_free_struct *pfs = new pixmap_free_struct;
    pfs->dpy = mDisplay;
    pfs->pixmap = mDrawable;

    cairo_surface_set_user_data (CairoSurface(),
                                 &pixmap_free_key,
                                 pfs,
                                 pixmap_free_func);

    mPixmapTaken = PR_TRUE;
}

void
pixmap_free_func (void *data)
{
    pixmap_free_struct *pfs = (pixmap_free_struct*) data;

    XFreePixmap (pfs->dpy, pfs->pixmap);

    delete pfs;
}

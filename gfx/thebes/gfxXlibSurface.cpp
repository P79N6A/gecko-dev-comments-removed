





































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
    NS_ASSERTION(CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT),
                 "Bad size");

    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, drawable, visual, mSize.width, mSize.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Screen *screen, Drawable drawable, XRenderPictFormat *format,
                               const gfxIntSize& size)
    : mPixmapTaken(PR_FALSE), mDisplay(DisplayOfScreen(screen)),
      mDrawable(drawable), mSize(size)
{
    NS_ASSERTION(CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT),
                 "Bad Size");

    cairo_surface_t *surf =
        cairo_xlib_surface_create_with_xrender_format(mDisplay, drawable,
                                                      screen, format,
                                                      mSize.width, mSize.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(cairo_surface_t *csurf)
    : mPixmapTaken(PR_FALSE),
      mSize(cairo_xlib_surface_get_width(csurf),
            cairo_xlib_surface_get_height(csurf))
{
    NS_PRECONDITION(cairo_surface_status(csurf) == 0,
                    "Not expecting an error surface");

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

static Drawable
CreatePixmap(Screen *screen, const gfxIntSize& size, unsigned int depth,
             Drawable relatedDrawable)
{
    if (!gfxASurface::CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT))
        return None;

    if (relatedDrawable == None) {
        relatedDrawable = RootWindowOfScreen(screen);
    }
    Display *dpy = DisplayOfScreen(screen);
    return XCreatePixmap(dpy, relatedDrawable,
                         size.width, size.height, depth);
}


already_AddRefed<gfxXlibSurface>
gfxXlibSurface::Create(Screen *screen, Visual *visual,
                       const gfxIntSize& size, Drawable relatedDrawable)
{
    Drawable drawable =
        CreatePixmap(screen, size, DepthOfVisual(screen, visual),
                     relatedDrawable);
    if (!drawable)
        return nsnull;

    nsRefPtr<gfxXlibSurface> result =
        new gfxXlibSurface(DisplayOfScreen(screen), drawable, visual, size);
    result->TakePixmap();

    if (result->CairoStatus() != 0)
        return nsnull;

    return result.forget();
}


already_AddRefed<gfxXlibSurface>
gfxXlibSurface::Create(Screen *screen, XRenderPictFormat *format,
                       const gfxIntSize& size, Drawable relatedDrawable)
{
    Drawable drawable =
        CreatePixmap(screen, size, format->depth, relatedDrawable);
    if (!drawable)
        return nsnull;

    nsRefPtr<gfxXlibSurface> result =
        new gfxXlibSurface(screen, drawable, format, size);
    result->TakePixmap();

    if (result->CairoStatus() != 0)
        return nsnull;

    return result.forget();
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


int
gfxXlibSurface::DepthOfVisual(const Screen* screen, const Visual* visual)
{
    for (int d = 0; d < screen->ndepths; d++) {
        const Depth& d_info = screen->depths[d];
        if (visual >= &d_info.visuals[0]
            && visual < &d_info.visuals[d_info.nvisuals])
            return d_info.depth;
    }

    NS_ERROR("Visual not on Screen.");
    return 0;
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

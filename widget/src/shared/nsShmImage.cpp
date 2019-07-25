







































#if defined(MOZ_WIDGET_GTK2) || defined(MOZ_WIDGET_GTK3)
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#elif defined(MOZ_WIDGET_QT)
#include <QWidget>
#endif

#include "nsShmImage.h"
#include "gfxPlatform.h"
#include "gfxImageSurface.h"

#ifdef MOZ_HAVE_SHMIMAGE

using namespace mozilla::ipc;



static bool gShmAvailable = true;
bool nsShmImage::UseShm()
{
    return gfxPlatform::GetPlatform()->
        ScreenReferenceSurface()->GetType() == gfxASurface::SurfaceTypeImage
        && gShmAvailable;
}

already_AddRefed<nsShmImage>
nsShmImage::Create(const gfxIntSize& aSize,
                   Visual* aVisual, unsigned int aDepth)
{
    Display* dpy = DISPLAY();

    nsRefPtr<nsShmImage> shm = new nsShmImage();
    shm->mImage = XShmCreateImage(dpy, aVisual, aDepth,
                                  ZPixmap, nsnull,
                                  &(shm->mInfo),
                                  aSize.width, aSize.height);
    if (!shm->mImage) {
        return nsnull;
    }

    size_t size = SharedMemory::PageAlignedSize(
        shm->mImage->bytes_per_line * shm->mImage->height);
    shm->mSegment = new SharedMemorySysV();
    if (!shm->mSegment->Create(size) || !shm->mSegment->Map(size)) {
        return nsnull;
    }

    shm->mInfo.shmid = shm->mSegment->GetHandle();
    shm->mInfo.shmaddr =
        shm->mImage->data = static_cast<char*>(shm->mSegment->memory());
    shm->mInfo.readOnly = False;

    int xerror = 0;
#if defined(MOZ_WIDGET_GTK2) || defined(MOZ_WIDGET_GTK3)
    gdk_error_trap_push();
    Status attachOk = XShmAttach(dpy, &shm->mInfo);
    XSync(dpy, False);
    xerror = gdk_error_trap_pop();
#elif defined(MOZ_WIDGET_QT)
    Status attachOk = XShmAttach(dpy, &shm->mInfo);
#endif

    if (!attachOk || xerror) {
        
        
        gShmAvailable = false;
        return nsnull;
    }

    shm->mXAttached = true;
    shm->mSize = aSize;
    switch (shm->mImage->depth) {
    case 24:
        
        if ((shm->mImage->red_mask == 0xff0000) &&
            (shm->mImage->green_mask == 0xff00) &&
            (shm->mImage->blue_mask == 0xff)) {
            shm->mFormat = gfxASurface::ImageFormatRGB24;
            break;
        }
        goto unsupported;
    case 16:
        shm->mFormat = gfxASurface::ImageFormatRGB16_565; break;
    unsupported:
    default:
        NS_WARNING("Unsupported XShm Image format!");
        gShmAvailable = false;
        return nsnull;
    }
    return shm.forget();
}

already_AddRefed<gfxASurface>
nsShmImage::AsSurface()
{
    return nsRefPtr<gfxASurface>(
        new gfxImageSurface(static_cast<unsigned char*>(mSegment->memory()),
                            mSize,
                            mImage->bytes_per_line,
                            mFormat)
        ).forget();
}

#if defined(MOZ_WIDGET_GTK2)
void
nsShmImage::Put(GdkWindow* aWindow, GdkRectangle* aRects, GdkRectangle* aEnd)
{
    GdkDrawable* gd;
    gint dx, dy;
    gdk_window_get_internal_paint_info(aWindow, &gd, &dx, &dy);

    Display* dpy = gdk_x11_get_default_xdisplay();
    Drawable d = GDK_DRAWABLE_XID(gd);

    GC gc = XCreateGC(dpy, d, 0, nsnull);
    for (GdkRectangle* r = aRects; r < aEnd; r++) {
        XShmPutImage(dpy, d, gc, mImage,
                     r->x, r->y,
                     r->x - dx, r->y - dy,
                     r->width, r->height,
                     False);
    }
    XFreeGC(dpy, gc);

    
    
    
    
    
    
    XSync(dpy, False);
}

#elif defined(MOZ_WIDGET_GTK3)
void
nsShmImage::Put(GdkWindow* aWindow, cairo_rectangle_list_t* aRects)
{
    Display* dpy = gdk_x11_get_default_xdisplay();
    Drawable d = GDK_WINDOW_XID(aWindow);
    int dx = 0, dy = 0;

    GC gc = XCreateGC(dpy, d, 0, nsnull);
    cairo_rectangle_t r;
    for (int i = 0; i < aRects->num_rectangles; i++) {
        r = aRects->rectangles[i];
        XShmPutImage(dpy, d, gc, mImage,
                     r.x, r.y,
                     r.x - dx, r.y - dy,
                     r.width, r.height,
                     False);
    }

    XFreeGC(dpy, gc);

    
    
    
    
    
    
    XSync(dpy, False);
}

#elif defined(MOZ_WIDGET_QT)
void
nsShmImage::Put(QWidget* aWindow, QRect& aRect)
{
    Display* dpy = aWindow->x11Info().display();
    Drawable d = aWindow->handle();

    GC gc = XCreateGC(dpy, d, 0, nsnull);
    
    QRect inter = aRect.intersected(aWindow->rect());
    XShmPutImage(dpy, d, gc, mImage,
                 inter.x(), inter.y(),
                 inter.x(), inter.y(),
                 inter.width(), inter.height(),
                 False);
    XFreeGC(dpy, gc);
}
#endif

already_AddRefed<gfxASurface>
nsShmImage::EnsureShmImage(const gfxIntSize& aSize, Visual* aVisual, unsigned int aDepth,
               nsRefPtr<nsShmImage>& aImage)
{
    if (!aImage || aImage->Size() != aSize) {
        
        
        
        
        
        aImage = nsShmImage::Create(aSize, aVisual, aDepth);
    }
    return !aImage ? nsnull : aImage->AsSurface();
}

#endif  

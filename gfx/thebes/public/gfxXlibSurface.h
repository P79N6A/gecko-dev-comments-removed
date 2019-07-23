





































#ifndef GFX_XLIBSURFACE_H
#define GFX_XLIBSURFACE_H

#include "gfxASurface.h"

#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>

class THEBES_API gfxXlibSurface : public gfxASurface {
public:
    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual);

    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual, const gfxIntSize& size);

    
    
    
    
    
    gfxXlibSurface(Display *dpy, Visual *visual, const gfxIntSize& size, int depth = 0);

    gfxXlibSurface(Display* dpy, Drawable drawable, XRenderPictFormat *format,
                   const gfxIntSize& size);

    gfxXlibSurface(Display* dpy, XRenderPictFormat *format,
                   const gfxIntSize& size);

    gfxXlibSurface(cairo_surface_t *csurf);

    virtual ~gfxXlibSurface();

    const gfxIntSize& GetSize() {
        if (mSize.width == -1 || mSize.height == -1)
            DoSizeQuery();

        return mSize;
    }

    Display* XDisplay() { return mDisplay; }
    Drawable XDrawable() { return mDrawable; }

    static XRenderPictFormat *FindRenderFormat(Display *dpy, gfxImageFormat format);

    
    
    void TakePixmap();

protected:
    
    PRBool mPixmapTaken;
    
    Display *mDisplay;
    Drawable mDrawable;

    void DoSizeQuery();

    gfxIntSize mSize;
};

#endif 

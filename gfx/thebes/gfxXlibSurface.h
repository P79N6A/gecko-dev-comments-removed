





































#ifndef GFX_XLIBSURFACE_H
#define GFX_XLIBSURFACE_H

#include "gfxASurface.h"

#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
#include "GLXLibrary.h"
#endif

class THEBES_API gfxXlibSurface : public gfxASurface {
public:
    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual);

    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual, const gfxIntSize& size);

    
    
    gfxXlibSurface(Screen *screen, Drawable drawable, XRenderPictFormat *format,
                   const gfxIntSize& size);

    gfxXlibSurface(cairo_surface_t *csurf);

    
    
    
    
    static already_AddRefed<gfxXlibSurface>
    Create(Screen *screen, Visual *visual, const gfxIntSize& size,
           Drawable relatedDrawable = None);
    static already_AddRefed<gfxXlibSurface>
    Create(Screen* screen, XRenderPictFormat *format, const gfxIntSize& size,
           Drawable relatedDrawable = None);

    virtual ~gfxXlibSurface();

    virtual already_AddRefed<gfxASurface>
    CreateSimilarSurface(gfxContentType aType, const gfxIntSize& aSize);

    virtual const gfxIntSize GetSize() const { return mSize; }

    Display* XDisplay() { return mDisplay; }
    Screen* XScreen();
    Drawable XDrawable() { return mDrawable; }
    XRenderPictFormat* XRenderFormat();

    static int DepthOfVisual(const Screen* screen, const Visual* visual);
    static Visual* FindVisual(Screen* screen, gfxImageFormat format);
    static XRenderPictFormat *FindRenderFormat(Display *dpy, gfxImageFormat format);

    
    
    void TakePixmap();

    
    
    
    Drawable ReleasePixmap();

    
    bool GetColormapAndVisual(Colormap* colormap, Visual **visual);

    
    
    virtual gfxASurface::MemoryLocation GetMemoryLocation() const;

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
    GLXPixmap GetGLXPixmap();
#endif

protected:
    
    bool mPixmapTaken;
    
    Display *mDisplay;
    Drawable mDrawable;

    void DoSizeQuery();

    gfxIntSize mSize;

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
    GLXPixmap mGLXPixmap;
#endif
};

#endif 

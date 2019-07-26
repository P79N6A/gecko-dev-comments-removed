




#ifndef GFX_XLIBSURFACE_H
#define GFX_XLIBSURFACE_H

#include "gfxASurface.h"

#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>

#if defined(GL_PROVIDER_GLX)
#include "GLXLibrary.h"
#endif

class gfxXlibSurface : public gfxASurface {
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
    virtual void Finish() MOZ_OVERRIDE;

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

#if defined(GL_PROVIDER_GLX)
    GLXPixmap GetGLXPixmap();
#endif

    
    
    
    
    
    bool IsPadSlow() {
        
        
        return VendorRelease(mDisplay) >= 60700000 ||
            VendorRelease(mDisplay) < 10699000;
    }

protected:
    
    bool mPixmapTaken;
    
    Display *mDisplay;
    Drawable mDrawable;

    void DoSizeQuery();

    gfxIntSize mSize;

#if defined(GL_PROVIDER_GLX)
    GLXPixmap mGLXPixmap;
#endif
};

#endif 

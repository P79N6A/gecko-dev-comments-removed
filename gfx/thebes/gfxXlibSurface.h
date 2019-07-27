




#ifndef GFX_XLIBSURFACE_H
#define GFX_XLIBSURFACE_H

#include "gfxASurface.h"

#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>

#if defined(GL_PROVIDER_GLX)
#include "GLXLibrary.h"
#endif

#include "nsSize.h"




#define XLIB_IMAGE_SIDE_SIZE_LIMIT 0x7fff


class gfxXlibSurface final : public gfxASurface {
public:
    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual);

    
    
    gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual, const gfxIntSize& size);

    
    
    gfxXlibSurface(Screen *screen, Drawable drawable, XRenderPictFormat *format,
                   const gfxIntSize& size);

    explicit gfxXlibSurface(cairo_surface_t *csurf);

    
    
    
    
    static already_AddRefed<gfxXlibSurface>
    Create(Screen *screen, Visual *visual, const gfxIntSize& size,
           Drawable relatedDrawable = None);
    static cairo_surface_t *
    CreateCairoSurface(Screen *screen, Visual *visual, const gfxIntSize& size,
                       Drawable relatedDrawable = None);
    static already_AddRefed<gfxXlibSurface>
    Create(Screen* screen, XRenderPictFormat *format, const gfxIntSize& size,
           Drawable relatedDrawable = None);

    virtual ~gfxXlibSurface();

    virtual already_AddRefed<gfxASurface>
    CreateSimilarSurface(gfxContentType aType,
                         const gfxIntSize& aSize) override;
    virtual void Finish() override;

    virtual const gfxIntSize GetSize() const override;

    Display* XDisplay() { return mDisplay; }
    Screen* XScreen();
    Drawable XDrawable() { return mDrawable; }
    XRenderPictFormat* XRenderFormat();

    static int DepthOfVisual(const Screen* screen, const Visual* visual);
    static Visual* FindVisual(Screen* screen, gfxImageFormat format);
    static XRenderPictFormat *FindRenderFormat(Display *dpy, gfxImageFormat format);
    static bool GetColormapAndVisual(cairo_surface_t* aXlibSurface, Colormap* colormap, Visual **visual);

    
    
    void TakePixmap();

    
    
    
    Drawable ReleasePixmap();

    
    bool GetColormapAndVisual(Colormap* colormap, Visual **visual);

    
    
    virtual gfxMemoryLocation GetMemoryLocation() const override;

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

    const gfxIntSize DoSizeQuery();

#if defined(GL_PROVIDER_GLX)
    GLXPixmap mGLXPixmap;
#endif
};

#endif 

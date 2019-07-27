




#include "gfxXlibSurface.h"

#include "cairo.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#include <X11/Xlibint.h>  
#undef max // Xlibint.h defines this and it breaks std::max
#undef min // Xlibint.h defines this and it breaks std::min

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsAlgorithm.h"
#include "mozilla/Preferences.h"
#include <algorithm>
#include "mozilla/CheckedInt.h"

using namespace mozilla;

gfxXlibSurface::gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual)
    : mPixmapTaken(false), mDisplay(dpy), mDrawable(drawable)
#if defined(GL_PROVIDER_GLX)
    , mGLXPixmap(None)
#endif
{
    const gfxIntSize size = DoSizeQuery();
    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, drawable, visual, size.width, size.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Display *dpy, Drawable drawable, Visual *visual, const gfxIntSize& size)
    : mPixmapTaken(false), mDisplay(dpy), mDrawable(drawable)
#if defined(GL_PROVIDER_GLX)
    , mGLXPixmap(None)
#endif
{
    NS_ASSERTION(CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT),
                 "Bad size");

    cairo_surface_t *surf = cairo_xlib_surface_create(dpy, drawable, visual, size.width, size.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(Screen *screen, Drawable drawable, XRenderPictFormat *format,
                               const gfxIntSize& size)
    : mPixmapTaken(false), mDisplay(DisplayOfScreen(screen)),
      mDrawable(drawable)
#if defined(GL_PROVIDER_GLX)
      , mGLXPixmap(None)
#endif
{
    NS_ASSERTION(CheckSurfaceSize(size, XLIB_IMAGE_SIDE_SIZE_LIMIT),
                 "Bad Size");

    cairo_surface_t *surf =
        cairo_xlib_surface_create_with_xrender_format(mDisplay, drawable,
                                                      screen, format,
                                                      size.width, size.height);
    Init(surf);
}

gfxXlibSurface::gfxXlibSurface(cairo_surface_t *csurf)
    : mPixmapTaken(false)
#if defined(GL_PROVIDER_GLX)
      , mGLXPixmap(None)
#endif
{
    NS_PRECONDITION(cairo_surface_status(csurf) == 0,
                    "Not expecting an error surface");

    mDrawable = cairo_xlib_surface_get_drawable(csurf);
    mDisplay = cairo_xlib_surface_get_display(csurf);

    Init(csurf, true);
}

gfxXlibSurface::~gfxXlibSurface()
{
#if defined(GL_PROVIDER_GLX)
    if (mGLXPixmap) {
        gl::sGLXLibrary.DestroyPixmap(mDisplay, mGLXPixmap);
    }
#endif
    
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
                         std::max(1, size.width), std::max(1, size.height),
                         depth);
}

void
gfxXlibSurface::TakePixmap()
{
    NS_ASSERTION(!mPixmapTaken, "I already own the Pixmap!");
    mPixmapTaken = true;

    
    
    unsigned int bitDepth = cairo_xlib_surface_get_depth(CairoSurface());
    MOZ_ASSERT((bitDepth % 8) == 0, "Memory used not recorded correctly");    

    
    
    gfxIntSize size = GetSize();
    CheckedInt32 totalBytes = CheckedInt32(size.width) * CheckedInt32(size.height) * (bitDepth/8);

    
    
    
    MOZ_ASSERT(totalBytes.isValid(),"Did not expect to exceed 2Gb image");
    if (totalBytes.isValid()) {
        RecordMemoryUsed(totalBytes.value());
    }
}

Drawable
gfxXlibSurface::ReleasePixmap() {
    NS_ASSERTION(mPixmapTaken, "I don't own the Pixmap!");
    mPixmapTaken = false;
    RecordMemoryFreed();
    return mDrawable;
}

static cairo_user_data_key_t gDestroyPixmapKey;

struct DestroyPixmapClosure {
    DestroyPixmapClosure(Drawable d, Screen *s) : mPixmap(d), mScreen(s) {}
    Drawable mPixmap;
    Screen *mScreen;
};

static void
DestroyPixmap(void *data)
{
    DestroyPixmapClosure *closure = static_cast<DestroyPixmapClosure*>(data);
    XFreePixmap(DisplayOfScreen(closure->mScreen), closure->mPixmap);
    delete closure;
}


cairo_surface_t *
gfxXlibSurface::CreateCairoSurface(Screen *screen, Visual *visual,
                                   const gfxIntSize& size, Drawable relatedDrawable)
{
    Drawable drawable =
        CreatePixmap(screen, size, DepthOfVisual(screen, visual),
                     relatedDrawable);
    if (!drawable)
        return nullptr;

    cairo_surface_t* surface =
        cairo_xlib_surface_create(DisplayOfScreen(screen), drawable, visual,
                                  size.width, size.height);
    if (cairo_surface_status(surface)) {
        cairo_surface_destroy(surface);
        XFreePixmap(DisplayOfScreen(screen), drawable);
        return nullptr;
    }

    DestroyPixmapClosure *closure = new DestroyPixmapClosure(drawable, screen);
    cairo_surface_set_user_data(surface, &gDestroyPixmapKey,
                                closure, DestroyPixmap);
    return surface;
}


already_AddRefed<gfxXlibSurface>
gfxXlibSurface::Create(Screen *screen, Visual *visual,
                       const gfxIntSize& size, Drawable relatedDrawable)
{
    Drawable drawable =
        CreatePixmap(screen, size, DepthOfVisual(screen, visual),
                     relatedDrawable);
    if (!drawable)
        return nullptr;

    nsRefPtr<gfxXlibSurface> result =
        new gfxXlibSurface(DisplayOfScreen(screen), drawable, visual, size);
    result->TakePixmap();

    if (result->CairoStatus() != 0)
        return nullptr;

    return result.forget();
}


already_AddRefed<gfxXlibSurface>
gfxXlibSurface::Create(Screen *screen, XRenderPictFormat *format,
                       const gfxIntSize& size, Drawable relatedDrawable)
{
    Drawable drawable =
        CreatePixmap(screen, size, format->depth, relatedDrawable);
    if (!drawable)
        return nullptr;

    nsRefPtr<gfxXlibSurface> result =
        new gfxXlibSurface(screen, drawable, format, size);
    result->TakePixmap();

    if (result->CairoStatus() != 0)
        return nullptr;

    return result.forget();
}

static bool GetForce24bppPref()
{
    return Preferences::GetBool("mozilla.widget.force-24bpp", false);
}

already_AddRefed<gfxASurface>
gfxXlibSurface::CreateSimilarSurface(gfxContentType aContent,
                                     const gfxIntSize& aSize)
{
    if (!mSurface || !mSurfaceValid) {
      return nullptr;
    }

    if (aContent == gfxContentType::COLOR) {
        
        
        
        static bool force24bpp = GetForce24bppPref();
        if (force24bpp
            && cairo_xlib_surface_get_depth(CairoSurface()) != 24) {
            XRenderPictFormat* format =
                XRenderFindStandardFormat(mDisplay, PictStandardRGB24);
            if (format) {
                
                
                
                
                
                Screen* screen = cairo_xlib_surface_get_screen(CairoSurface());
                nsRefPtr<gfxXlibSurface> depth24reference =
                    gfxXlibSurface::Create(screen, format,
                                           gfxIntSize(1, 1), mDrawable);
                if (depth24reference)
                    return depth24reference->
                        gfxASurface::CreateSimilarSurface(aContent, aSize);
            }
        }
    }

    return gfxASurface::CreateSimilarSurface(aContent, aSize);
}

void
gfxXlibSurface::Finish()
{
#if defined(GL_PROVIDER_GLX)
    if (mGLXPixmap) {
        gl::sGLXLibrary.DestroyPixmap(mDisplay, mGLXPixmap);
        mGLXPixmap = None;
    }
#endif
    gfxASurface::Finish();
}

const gfxIntSize
gfxXlibSurface::GetSize() const
{
    if (!mSurfaceValid)
        return gfxIntSize(0,0);

    return gfxIntSize(cairo_xlib_surface_get_width(mSurface),
                      cairo_xlib_surface_get_height(mSurface));
}

const gfxIntSize
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

    return gfxIntSize(width, height);
}

class DisplayTable {
public:
    static bool GetColormapAndVisual(Screen* screen,
                                       XRenderPictFormat* format,
                                       Visual* visual, Colormap* colormap,
                                       Visual** visualForColormap);

private:
    struct ColormapEntry {
        XRenderPictFormat* mFormat;
        
        
        
        Screen* mScreen;
        Visual* mVisual;
        Colormap mColormap;
    };

    class DisplayInfo {
    public:
        explicit DisplayInfo(Display* display) : mDisplay(display) { }
        Display* mDisplay;
        nsTArray<ColormapEntry> mColormapEntries;
    };

    
    class FindDisplay {
    public:
        bool Equals(const DisplayInfo& info, const Display *display) const
        {
            return info.mDisplay == display;
        }
    };

    static int DisplayClosing(Display *display, XExtCodes* codes);

    nsTArray<DisplayInfo> mDisplays;
    static DisplayTable* sDisplayTable;
};

DisplayTable* DisplayTable::sDisplayTable;
























 bool
DisplayTable::GetColormapAndVisual(Screen* aScreen, XRenderPictFormat* aFormat,
                                   Visual* aVisual, Colormap* aColormap,
                                   Visual** aVisualForColormap)

{
    Display* display = DisplayOfScreen(aScreen);

    
    Visual *defaultVisual = DefaultVisualOfScreen(aScreen);
    if (aVisual == defaultVisual
        || (aFormat
            && aFormat == XRenderFindVisualFormat(display, defaultVisual)))
    {
        *aColormap = DefaultColormapOfScreen(aScreen);
        *aVisualForColormap = defaultVisual;
        return true;
    }

    
    if (!aVisual || aVisual->c_class != TrueColor)
        return false;

    if (!sDisplayTable) {
        sDisplayTable = new DisplayTable();
    }

    nsTArray<DisplayInfo>* displays = &sDisplayTable->mDisplays;
    size_t d = displays->IndexOf(display, 0, FindDisplay());

    if (d == displays->NoIndex) {
        d = displays->Length();
        
        
        XExtCodes *codes = XAddExtension(display);
        if (!codes)
            return false;

        XESetCloseDisplay(display, codes->extension, DisplayClosing);
        
        displays->AppendElement(display);
    }

    nsTArray<ColormapEntry>* entries =
        &displays->ElementAt(d).mColormapEntries;

    
    
    for (uint32_t i = 0; i < entries->Length(); ++i) {
        const ColormapEntry& entry = entries->ElementAt(i);
        
        
        
        if ((aFormat && entry.mFormat == aFormat && entry.mScreen == aScreen)
            || aVisual == entry.mVisual) {
            *aColormap = entry.mColormap;
            *aVisualForColormap = entry.mVisual;
            return true;
        }
    }

    
    Colormap colormap = XCreateColormap(display, RootWindowOfScreen(aScreen),
                                        aVisual, AllocNone);
    ColormapEntry* newEntry = entries->AppendElement();
    newEntry->mFormat = aFormat;
    newEntry->mScreen = aScreen;
    newEntry->mVisual = aVisual;
    newEntry->mColormap = colormap;

    *aColormap = colormap;
    *aVisualForColormap = aVisual;
    return true;
}

 int
DisplayTable::DisplayClosing(Display *display, XExtCodes* codes)
{
    
    
    sDisplayTable->mDisplays.RemoveElement(display, FindDisplay());
    if (sDisplayTable->mDisplays.Length() == 0) {
        delete sDisplayTable;
        sDisplayTable = nullptr;
    }
    return 0;
}


bool
gfxXlibSurface::GetColormapAndVisual(cairo_surface_t* aXlibSurface,
                                     Colormap* aColormap, Visual** aVisual)
{
    XRenderPictFormat* format =
        cairo_xlib_surface_get_xrender_format(aXlibSurface);
    Screen* screen = cairo_xlib_surface_get_screen(aXlibSurface);
    Visual* visual = cairo_xlib_surface_get_visual(aXlibSurface);

    return DisplayTable::GetColormapAndVisual(screen, format, visual,
                                              aColormap, aVisual);
}

bool
gfxXlibSurface::GetColormapAndVisual(Colormap* aColormap, Visual** aVisual)
{
    if (!mSurfaceValid)
        return false;

    return GetColormapAndVisual(CairoSurface(), aColormap, aVisual);
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
    

Visual*
gfxXlibSurface::FindVisual(Screen *screen, gfxImageFormat format)
{
    int depth;
    unsigned long red_mask, green_mask, blue_mask;
    switch (format) {
        case gfxImageFormat::ARGB32:
            depth = 32;
            red_mask = 0xff0000;
            green_mask = 0xff00;
            blue_mask = 0xff;
            break;
        case gfxImageFormat::RGB24:
            depth = 24;
            red_mask = 0xff0000;
            green_mask = 0xff00;
            blue_mask = 0xff;
            break;
        case gfxImageFormat::RGB16_565:
            depth = 16;
            red_mask = 0xf800;
            green_mask = 0x7e0;
            blue_mask = 0x1f;
            break;
        case gfxImageFormat::A8:
        case gfxImageFormat::A1:
        default:
            return nullptr;
    }

    for (int d = 0; d < screen->ndepths; d++) {
        const Depth& d_info = screen->depths[d];
        if (d_info.depth != depth)
            continue;

        for (int v = 0; v < d_info.nvisuals; v++) {
            Visual* visual = &d_info.visuals[v];

            if (visual->c_class == TrueColor &&
                visual->red_mask == red_mask &&
                visual->green_mask == green_mask &&
                visual->blue_mask == blue_mask)
                return visual;
        }
    }

    return nullptr;
}


XRenderPictFormat*
gfxXlibSurface::FindRenderFormat(Display *dpy, gfxImageFormat format)
{
    switch (format) {
        case gfxImageFormat::ARGB32:
            return XRenderFindStandardFormat (dpy, PictStandardARGB32);
        case gfxImageFormat::RGB24:
            return XRenderFindStandardFormat (dpy, PictStandardRGB24);
        case gfxImageFormat::RGB16_565: {
            
            
            
            Visual *visual = FindVisual(DefaultScreenOfDisplay(dpy), format);
            if (!visual)
                return nullptr;
            return XRenderFindVisualFormat(dpy, visual);
        }
        case gfxImageFormat::A8:
            return XRenderFindStandardFormat (dpy, PictStandardA8);
        case gfxImageFormat::A1:
            return XRenderFindStandardFormat (dpy, PictStandardA1);
        default:
            break;
    }

    return nullptr;
}

Screen*
gfxXlibSurface::XScreen()
{
    return cairo_xlib_surface_get_screen(CairoSurface());
}

XRenderPictFormat*
gfxXlibSurface::XRenderFormat()
{
    return cairo_xlib_surface_get_xrender_format(CairoSurface());
}

#if defined(GL_PROVIDER_GLX)
GLXPixmap
gfxXlibSurface::GetGLXPixmap()
{
    if (!mGLXPixmap) {
#ifdef DEBUG
        
        
        
        cairo_surface_has_show_text_glyphs(CairoSurface());
        NS_ASSERTION(CairoStatus() != CAIRO_STATUS_SURFACE_FINISHED,
            "GetGLXPixmap called after surface finished");
#endif
        mGLXPixmap = gl::sGLXLibrary.CreatePixmap(this);
    }
    return mGLXPixmap;
}
#endif

gfxMemoryLocation
gfxXlibSurface::GetMemoryLocation() const
{
    return gfxMemoryLocation::OUT_OF_PROCESS;
}

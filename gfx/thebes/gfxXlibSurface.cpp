





































#include "gfxXlibSurface.h"

#include "cairo.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#include <X11/Xlibint.h>	
#include "nsTArray.h"




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

class DisplayTable {
public:
    static PRBool GetColormapAndVisual(Screen* screen,
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
        DisplayInfo(Display* display) : mDisplay(display) { }
        Display* mDisplay;
        nsTArray<ColormapEntry> mColormapEntries;
    };

    
    class FindDisplay {
    public:
        PRBool Equals(const DisplayInfo& info, const Display *display) const
        {
            return info.mDisplay == display;
        }
    };

    static int DisplayClosing(Display *display, XExtCodes* codes);

    nsTArray<DisplayInfo> mDisplays;
    static DisplayTable* sDisplayTable;
};

DisplayTable* DisplayTable::sDisplayTable;
























 PRBool
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
        return PR_TRUE;
    }

    
    if (!aVisual || aVisual->c_class != TrueColor)
        return PR_FALSE;

    if (!sDisplayTable) {
        sDisplayTable = new DisplayTable();
    }

    nsTArray<DisplayInfo>* displays = &sDisplayTable->mDisplays;
    PRUint32 d = displays->IndexOf(display, 0, FindDisplay());

    if (d == displays->NoIndex) {
        d = displays->Length();
        
        
        XExtCodes *codes = XAddExtension(display);
        if (!codes)
            return PR_FALSE;

        XESetCloseDisplay(display, codes->extension, DisplayClosing);
        
        displays->AppendElement(display);
    }

    nsTArray<ColormapEntry>* entries =
        &displays->ElementAt(d).mColormapEntries;

    
    
    for (PRUint32 i = 0; i < entries->Length(); ++i) {
        const ColormapEntry& entry = entries->ElementAt(i);
        
        
        
        if ((aFormat && entry.mFormat == aFormat && entry.mScreen == aScreen)
            || aVisual == entry.mVisual) {
            *aColormap = entry.mColormap;
            *aVisualForColormap = entry.mVisual;
            return PR_TRUE;
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
    return PR_TRUE;
}

 int
DisplayTable::DisplayClosing(Display *display, XExtCodes* codes)
{
    
    
    sDisplayTable->mDisplays.RemoveElement(display, FindDisplay());
    if (sDisplayTable->mDisplays.Length() == 0) {
        delete sDisplayTable;
        sDisplayTable = nsnull;
    }
    return 0;
}

PRBool
gfxXlibSurface::GetColormapAndVisual(Colormap* aColormap, Visual** aVisual)
{
    if (!mSurfaceValid)
        return PR_FALSE;

    XRenderPictFormat* format =
        cairo_xlib_surface_get_xrender_format(CairoSurface());
    Screen* screen = cairo_xlib_surface_get_screen(CairoSurface());
    Visual* visual = cairo_xlib_surface_get_visual(CairoSurface());

    return DisplayTable::GetColormapAndVisual(screen, format, visual,
                                              aColormap, aVisual);
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

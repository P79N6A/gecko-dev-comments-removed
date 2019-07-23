





































#define PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_ENGINE

#include "gfxPlatformGtk.h"

#include "gfxFontconfigUtils.h"

#include "cairo.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "gfxImageSurface.h"
#include "gfxXlibSurface.h"

#include "gfxPangoFonts.h"

#ifdef MOZ_ENABLE_GLITZ
#include "gfxGlitzSurface.h"
#include "glitz-glx.h"
#endif

#include <fontconfig/fontconfig.h>

#ifndef THEBES_USE_PANGO_CAIRO
#include <pango/pangoxft.h>
#endif 

#include <pango/pango-font.h>

#include "nsMathUtils.h"

PRInt32 gfxPlatformGtk::sDPI = -1;
gfxFontconfigUtils *gfxPlatformGtk::sFontconfigUtils = nsnull;

static cairo_user_data_key_t cairo_gdk_window_key;
static cairo_user_data_key_t cairo_gdk_pixmap_key;
static void do_gdk_pixmap_unref (void *data)
{
    GdkPixmap *pmap = (GdkPixmap*)data;
    gdk_pixmap_unref (pmap);
}

gfxPlatformGtk::gfxPlatformGtk()
{
#ifdef MOZ_ENABLE_GLITZ
    if (UseGlitz())
        glitz_glx_init(NULL);
#endif
    if (!sFontconfigUtils)
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();
}

gfxPlatformGtk::~gfxPlatformGtk()
{
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nsnull;
#ifndef THEBES_USE_PANGO_CAIRO
    pango_xft_shutdown_display(GDK_DISPLAY(), 0);
#endif

#if 0
    
    
    
    
    FcFini();
#endif
}

already_AddRefed<gfxASurface>
gfxPlatformGtk::CreateOffscreenSurface(const gfxIntSize& size,
                                       gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *newSurface = nsnull;

    int glitzf;
    int xrenderFormatID;
    switch (imageFormat) {
        case gfxASurface::ImageFormatARGB32:
            glitzf = 0; 
            xrenderFormatID = PictStandardARGB32;
            break;
        case gfxASurface::ImageFormatRGB24:
            glitzf = 1; 
            xrenderFormatID = PictStandardRGB24;
            break;
        case gfxASurface::ImageFormatA8:
            glitzf = 2; 
            xrenderFormatID = PictStandardA8;
            break;
        case gfxASurface::ImageFormatA1:
            glitzf = 3; 
            xrenderFormatID = PictStandardA1;
            break;
        default:
            return nsnull;
    }

    
    
    
    Display* display = GDK_DISPLAY();
    if (!UseGlitz()) {
        GdkPixmap* pixmap = nsnull;
        XRenderPictFormat* xrenderFormat =
            XRenderFindStandardFormat(display, xrenderFormatID);
        if (!xrenderFormat) {
            
            
            GdkVisual* vis;

            if (imageFormat == gfxASurface::ImageFormatRGB24) {
                vis = gdk_rgb_get_visual();
                if (vis->type == GDK_VISUAL_TRUE_COLOR)
                    pixmap = gdk_pixmap_new(nsnull, size.width, size.height, vis->depth);
            }

            if (pixmap) {
                gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), nsnull);
                newSurface = new gfxXlibSurface(display,
                                                GDK_PIXMAP_XID(GDK_DRAWABLE(pixmap)),
                                                GDK_VISUAL_XVISUAL(vis),
                                                size);
            } else {
                
                newSurface = new gfxImageSurface(gfxIntSize(size.width, size.height), imageFormat);
            }
        } else {
            pixmap = gdk_pixmap_new(nsnull, size.width, size.height,
                                    xrenderFormat->depth);
            gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), nsnull);

            newSurface = new gfxXlibSurface(display,
                                            GDK_PIXMAP_XID(GDK_DRAWABLE(pixmap)),
                                            xrenderFormat,
                                            size);
        }

        if (pixmap && newSurface) {
            
            
            newSurface->SetData(&cairo_gdk_pixmap_key,
                                pixmap,
                                do_gdk_pixmap_unref);
        }

    } else {
#ifdef MOZ_ENABLE_GLITZ
        glitz_drawable_format_t *gdformat = glitz_glx_find_pbuffer_format
            (display,
             gdk_x11_get_default_screen(),
             0, NULL, 0);

        glitz_drawable_t *gdraw =
            glitz_glx_create_pbuffer_drawable(display,
                                              DefaultScreen(display),
                                              gdformat,
                                              size.width,
                                              size.height);
        glitz_format_t *gformat =
            glitz_find_standard_format(gdraw, (glitz_format_name_t)glitzf);

        glitz_surface_t *gsurf =
            glitz_surface_create(gdraw,
                                 gformat,
                                 size.width,
                                 size.height,
                                 0,
                                 NULL);

        glitz_surface_attach(gsurf, gdraw, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);
        newSurface = new gfxGlitzSurface(gdraw, gsurf, PR_TRUE);
#endif
    }

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

GdkDrawable*
gfxPlatformGtk::GetSurfaceGdkDrawable(gfxASurface *aSurf)
{
    GdkDrawable *gd;
    gd = (GdkDrawable*) cairo_surface_get_user_data(aSurf->CairoSurface(), &cairo_gdk_pixmap_key);
    if (gd)
        return gd;

    gd = (GdkDrawable*) cairo_surface_get_user_data(aSurf->CairoSurface(), &cairo_gdk_window_key);
    if (gd)
        return gd;

    return nsnull;
}

void
gfxPlatformGtk::SetSurfaceGdkWindow(gfxASurface *aSurf,
                                    GdkWindow *win)
{
    cairo_surface_set_user_data(aSurf->CairoSurface(),
                                &cairo_gdk_window_key,
                                win,
                                nsnull);
}

nsresult
gfxPlatformGtk::GetFontList(const nsACString& aLangGroup,
                            const nsACString& aGenericFamily,
                            nsStringArray& aListOfFonts)
{
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
}

nsresult
gfxPlatformGtk::UpdateFontList()
{
    return sFontconfigUtils->UpdateFontList();
}

nsresult
gfxPlatformGtk::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure,
                                PRBool& aAborted)
{
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback,
                                             aClosure, aAborted);
}

gfxFontGroup *
gfxPlatformGtk::CreateFontGroup(const nsAString &aFamilies,
                                const gfxFontStyle *aStyle)
{
    return new gfxPangoFontGroup(aFamilies, aStyle);
}

static PRInt32
GetXftDPI()
{
  char *val = XGetDefault(GDK_DISPLAY(), "Xft", "dpi");
  if (val) {
    char *e;
    double d = strtod(val, &e);

    if (e != val)
      return NS_lround(d);
  }

  return -1;
}

static PRInt32
GetDPIFromPangoFont()
{
#ifndef THEBES_USE_PANGO_CAIRO
    PangoContext* ctx = pango_xft_get_context(GDK_DISPLAY(), 0);
    gdk_pango_context_set_colormap(ctx, gdk_rgb_get_cmap());
#else
    PangoContext* ctx =
        pango_cairo_font_map_create_context(
          PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_get_default()));
#endif

    if (!ctx) {
        return 0;
    }

    double dblDPI = 0.0f;
    GList *items = nsnull;
    PangoItem *item = nsnull;
    PangoFcFont *fcfont = nsnull;
    
    PangoAttrList *al = pango_attr_list_new();

    if (!al) {
        goto cleanup;
    }

    
    items = pango_itemize(ctx, "a", 0, 1, al, NULL);

    if (!items) {
        goto cleanup;
    }

    item = (PangoItem*)items->data;

    if (!item) {
        goto cleanup;
    }

    fcfont = PANGO_FC_FONT(item->analysis.font);

    if (!fcfont) {
        goto cleanup;
    }

    FcPatternGetDouble(fcfont->font_pattern, FC_DPI, 0, &dblDPI);

 cleanup:   
    if (al)
        pango_attr_list_unref(al);
    if (item)
        pango_item_free(item);
    if (items)
        g_list_free(items);
    if (ctx)
        g_object_unref(ctx);

    return NS_lround(dblDPI);
}


void
gfxPlatformGtk::InitDPI()
{
    sDPI = GetXftDPI();
    if (sDPI <= 0) {
        sDPI = GetDPIFromPangoFont();
        if (sDPI <= 0) {
            
            sDPI = 96;
        }
    }
}






#define PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_ENGINE

#include "gfxPlatformGtk.h"
#include "prenv.h"

#include "nsUnicharUtils.h"
#include "nsUnicodeProperties.h"
#include "gfx2DGlue.h"
#include "gfxFontconfigUtils.h"
#include "gfxPangoFonts.h"
#include "gfxContext.h"
#include "gfxUserFontSet.h"
#include "gfxUtils.h"
#include "gfxFT2FontBase.h"

#include "mozilla/gfx/2D.h"

#include "cairo.h"
#include <gtk/gtk.h>

#include "gfxImageSurface.h"
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include "gfxXlibSurface.h"
#include "cairo-xlib.h"
#include "mozilla/Preferences.h"


#if defined(__APPLE__) && defined(Status)
#undef Status
#endif

#endif 

#include <fontconfig/fontconfig.h>

#include "nsMathUtils.h"

#define GDK_PIXMAP_SIZE_MAX 32767

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;

gfxFontconfigUtils *gfxPlatformGtk::sFontconfigUtils = nullptr;

#if (MOZ_WIDGET_GTK == 2)
static cairo_user_data_key_t cairo_gdk_drawable_key;
#endif

#ifdef MOZ_X11
    bool gfxPlatformGtk::sUseXRender = true;
#endif

gfxPlatformGtk::gfxPlatformGtk()
{
    if (!sFontconfigUtils)
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();
#ifdef MOZ_X11
    sUseXRender = mozilla::Preferences::GetBool("gfx.xrender.enabled");
#endif

    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO) | BackendTypeBit(BackendType::SKIA);
    uint32_t contentMask = BackendTypeBit(BackendType::CAIRO) | BackendTypeBit(BackendType::SKIA);
    InitBackendPrefs(canvasMask, BackendType::CAIRO,
                     contentMask, BackendType::CAIRO);
}

gfxPlatformGtk::~gfxPlatformGtk()
{
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nullptr;

    gfxPangoFontGroup::Shutdown();
}

already_AddRefed<gfxASurface>
gfxPlatformGtk::CreateOffscreenSurface(const IntSize& size,
                                       gfxContentType contentType)
{
    nsRefPtr<gfxASurface> newSurface;
    bool needsClear = true;
    gfxImageFormat imageFormat = OptimalFormatForContent(contentType);
#ifdef MOZ_X11
    
    
    
    GdkScreen *gdkScreen = gdk_screen_get_default();
    if (gdkScreen) {
        
        
        if (UseXRender() && !UseImageOffscreenSurfaces()) {
            Screen *screen = gdk_x11_screen_get_xscreen(gdkScreen);
            XRenderPictFormat* xrenderFormat =
                gfxXlibSurface::FindRenderFormat(DisplayOfScreen(screen),
                                                 imageFormat);

            if (xrenderFormat) {
                newSurface = gfxXlibSurface::Create(screen, xrenderFormat,
                                                    ThebesIntSize(size));
            }
        } else {
            
            
            newSurface = new gfxImageSurface(ThebesIntSize(size), imageFormat);
            
            
            needsClear = false;
        }
    }
#endif

    if (!newSurface) {
        
        
        
        newSurface = new gfxImageSurface(ThebesIntSize(size), imageFormat);
    }

    if (newSurface->CairoStatus()) {
        newSurface = nullptr; 
    }

    if (newSurface && needsClear) {
        gfxUtils::ClearThebesSurface(newSurface);
    }

    return newSurface.forget();
}

nsresult
gfxPlatformGtk::GetFontList(nsIAtom *aLangGroup,
                            const nsACString& aGenericFamily,
                            nsTArray<nsString>& aListOfFonts)
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
gfxPlatformGtk::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    return sFontconfigUtils->GetStandardFamilyName(aFontName, aFamilyName);
}

gfxFontGroup *
gfxPlatformGtk::CreateFontGroup(const FontFamilyList& aFontFamilyList,
                                const gfxFontStyle *aStyle,
                                gfxUserFontSet *aUserFontSet)
{
    return new gfxPangoFontGroup(aFontFamilyList, aStyle, aUserFontSet);
}

gfxFontEntry*
gfxPlatformGtk::LookupLocalFont(const nsAString& aFontName,
                                uint16_t aWeight,
                                int16_t aStretch,
                                bool aItalic)
{
    return gfxPangoFontGroup::NewFontEntry(aFontName, aWeight,
                                           aStretch, aItalic);
}

gfxFontEntry* 
gfxPlatformGtk::MakePlatformFont(const nsAString& aFontName,
                                 uint16_t aWeight,
                                 int16_t aStretch,
                                 bool aItalic,
                                 const uint8_t* aFontData,
                                 uint32_t aLength)
{
    
    return gfxPangoFontGroup::NewFontEntry(aFontName, aWeight,
                                           aStretch, aItalic,
                                           aFontData, aLength);
}

bool
gfxPlatformGtk::IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    
    
    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_WOFF     |
                        gfxUserFontSet::FLAG_FORMAT_OPENTYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE)) {
        return true;
    }

    
    if (aFormatFlags != 0) {
        return false;
    }

    
    return true;
}

static int32_t sDPI = 0;

int32_t
gfxPlatformGtk::GetDPI()
{
    if (!sDPI) {
        
        GdkScreen *screen = gdk_screen_get_default();
        gtk_settings_get_for_screen(screen);
        sDPI = int32_t(round(gdk_screen_get_resolution(screen)));
        if (sDPI <= 0) {
            
            sDPI = 96;
        }
    }
    return sDPI;
}

gfxImageFormat
gfxPlatformGtk::GetOffscreenFormat()
{
    
    GdkScreen *screen = gdk_screen_get_default();
    if (screen && gdk_visual_get_depth(gdk_visual_get_system()) == 16) {
        return gfxImageFormat::RGB16_565;
    }

    return gfxImageFormat::RGB24;
}

static int sDepth = 0;

int
gfxPlatformGtk::GetScreenDepth() const
{
    if (!sDepth) {
        GdkScreen *screen = gdk_screen_get_default();
        if (screen) {
            sDepth = gdk_visual_get_depth(gdk_visual_get_system());
        } else {
            sDepth = 24;
        }

    }

    return sDepth;
}

void
gfxPlatformGtk::GetPlatformCMSOutputProfile(void *&mem, size_t &size)
{
    mem = nullptr;
    size = 0;

#ifdef MOZ_X11
    const char EDID1_ATOM_NAME[] = "XFree86_DDC_EDID1_RAWDATA";
    const char ICC_PROFILE_ATOM_NAME[] = "_ICC_PROFILE";

    Atom edidAtom, iccAtom;
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    
    
    
    if (!dpy)
        return;
 
    Window root = gdk_x11_get_default_root_xwindow();

    Atom retAtom;
    int retFormat;
    unsigned long retLength, retAfter;
    unsigned char *retProperty ;

    iccAtom = XInternAtom(dpy, ICC_PROFILE_ATOM_NAME, TRUE);
    if (iccAtom) {
        
        if (Success == XGetWindowProperty(dpy, root, iccAtom,
                                          0, INT_MAX ,
                                          False, AnyPropertyType,
                                          &retAtom, &retFormat, &retLength,
                                          &retAfter, &retProperty)) {

            if (retLength > 0) {
                void *buffer = malloc(retLength);
                if (buffer) {
                    memcpy(buffer, retProperty, retLength);
                    mem = buffer;
                    size = retLength;
                }
            }

            XFree(retProperty);
            if (size > 0) {
#ifdef DEBUG_tor
                fprintf(stderr,
                        "ICM profile read from %s successfully\n",
                        ICC_PROFILE_ATOM_NAME);
#endif
                return;
            }
        }
    }

    edidAtom = XInternAtom(dpy, EDID1_ATOM_NAME, TRUE);
    if (edidAtom) {
        if (Success == XGetWindowProperty(dpy, root, edidAtom, 0, 32,
                                          False, AnyPropertyType,
                                          &retAtom, &retFormat, &retLength,
                                          &retAfter, &retProperty)) {
            double gamma;
            qcms_CIE_xyY whitePoint;
            qcms_CIE_xyYTRIPLE primaries;

            if (retLength != 128) {
#ifdef DEBUG_tor
                fprintf(stderr, "Short EDID data\n");
#endif
                return;
            }

            

            gamma = (100 + retProperty[0x17]) / 100.0;
            whitePoint.x = ((retProperty[0x21] << 2) |
                            (retProperty[0x1a] >> 2 & 3)) / 1024.0;
            whitePoint.y = ((retProperty[0x22] << 2) |
                            (retProperty[0x1a] >> 0 & 3)) / 1024.0;
            whitePoint.Y = 1.0;

            primaries.red.x = ((retProperty[0x1b] << 2) |
                               (retProperty[0x19] >> 6 & 3)) / 1024.0;
            primaries.red.y = ((retProperty[0x1c] << 2) |
                               (retProperty[0x19] >> 4 & 3)) / 1024.0;
            primaries.red.Y = 1.0;

            primaries.green.x = ((retProperty[0x1d] << 2) |
                                 (retProperty[0x19] >> 2 & 3)) / 1024.0;
            primaries.green.y = ((retProperty[0x1e] << 2) |
                                 (retProperty[0x19] >> 0 & 3)) / 1024.0;
            primaries.green.Y = 1.0;

            primaries.blue.x = ((retProperty[0x1f] << 2) |
                               (retProperty[0x1a] >> 6 & 3)) / 1024.0;
            primaries.blue.y = ((retProperty[0x20] << 2) |
                               (retProperty[0x1a] >> 4 & 3)) / 1024.0;
            primaries.blue.Y = 1.0;

            XFree(retProperty);

#ifdef DEBUG_tor
            fprintf(stderr, "EDID gamma: %f\n", gamma);
            fprintf(stderr, "EDID whitepoint: %f %f %f\n",
                    whitePoint.x, whitePoint.y, whitePoint.Y);
            fprintf(stderr, "EDID primaries: [%f %f %f] [%f %f %f] [%f %f %f]\n",
                    primaries.Red.x, primaries.Red.y, primaries.Red.Y,
                    primaries.Green.x, primaries.Green.y, primaries.Green.Y,
                    primaries.Blue.x, primaries.Blue.y, primaries.Blue.Y);
#endif

            qcms_data_create_rgb_with_gamma(whitePoint, primaries, gamma, &mem, &size);

#ifdef DEBUG_tor
            if (size > 0) {
                fprintf(stderr,
                        "ICM profile read from %s successfully\n",
                        EDID1_ATOM_NAME);
            }
#endif
        }
    }
#endif
}


#if (MOZ_WIDGET_GTK == 2)
void
gfxPlatformGtk::SetGdkDrawable(cairo_surface_t *target,
                               GdkDrawable *drawable)
{
    if (cairo_surface_status(target))
        return;

    g_object_ref(drawable);

    cairo_surface_set_user_data (target,
                                 &cairo_gdk_drawable_key,
                                 drawable,
                                 g_object_unref);
}

GdkDrawable *
gfxPlatformGtk::GetGdkDrawable(cairo_surface_t *target)
{
    if (cairo_surface_status(target))
        return nullptr;

    GdkDrawable *result;

    result = (GdkDrawable*) cairo_surface_get_user_data (target,
                                                         &cairo_gdk_drawable_key);
    if (result)
        return result;

#ifdef MOZ_X11
    if (cairo_surface_get_type(target) != CAIRO_SURFACE_TYPE_XLIB)
        return nullptr;

    
    result = (GdkDrawable*) gdk_xid_table_lookup(cairo_xlib_surface_get_drawable(target));
    if (result) {
        SetGdkDrawable(target, result);
        return result;
    }
#endif

    return nullptr;
}
#endif

TemporaryRef<ScaledFont>
gfxPlatformGtk::GetScaledFontForFont(DrawTarget* aTarget, gfxFont *aFont)
{
    return GetScaledFontForFontWithCairoSkia(aTarget, aFont);
}

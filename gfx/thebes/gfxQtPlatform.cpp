




#include <QPixmap>
#include <QWindow>
#ifdef MOZ_X11
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#endif
#include <QGuiApplication>
#include <QScreen>

#include "gfxQtPlatform.h"

#include "gfxFontconfigUtils.h"

#include "mozilla/gfx/2D.h"

#include "cairo.h"

#include "gfxImageSurface.h"
#include "gfxQPainterSurface.h"
#include "nsUnicodeProperties.h"

#include "gfxPangoFonts.h"
#include "gfxContext.h"
#include "gfxUserFontSet.h"

#include "nsUnicharUtils.h"

#include "nsMathUtils.h"
#include "nsTArray.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#include "prenv.h"
#endif

#include "qcms.h"

#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::unicode;
using namespace mozilla::gfx;

gfxFontconfigUtils *gfxQtPlatform::sFontconfigUtils = nullptr;

static gfxImageFormat sOffscreenFormat = gfxImageFormat::RGB24;

gfxQtPlatform::gfxQtPlatform()
{
    if (!sFontconfigUtils)
        sFontconfigUtils = gfxFontconfigUtils::GetFontconfigUtils();

    mScreenDepth = qApp->primaryScreen()->depth();
    if (mScreenDepth == 16) {
        sOffscreenFormat = gfxImageFormat::RGB16_565;
    }
    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO) | BackendTypeBit(BackendType::SKIA);
    uint32_t contentMask = BackendTypeBit(BackendType::CAIRO) | BackendTypeBit(BackendType::SKIA);
    InitBackendPrefs(canvasMask, BackendType::CAIRO,
                     contentMask, BackendType::CAIRO);
}

gfxQtPlatform::~gfxQtPlatform()
{
    gfxFontconfigUtils::Shutdown();
    sFontconfigUtils = nullptr;

    gfxPangoFontGroup::Shutdown();
}

#ifdef MOZ_X11
Display*
gfxQtPlatform::GetXDisplay(QWindow* aWindow)
{
    return (Display*)(qApp->platformNativeInterface()->
        nativeResourceForScreen("display", aWindow ? aWindow->screen() : qApp->primaryScreen()));
}

Screen*
gfxQtPlatform::GetXScreen(QWindow* aWindow)
{
    return ScreenOfDisplay(GetXDisplay(aWindow),
        (int)(intptr_t)qApp->platformNativeInterface()->
            nativeResourceForScreen("screen", aWindow ? aWindow->screen() : qApp->primaryScreen()));
}
#endif

already_AddRefed<gfxASurface>
gfxQtPlatform::CreateOffscreenSurface(const IntSize& size,
                                      gfxContentType contentType)
{
    gfxImageFormat imageFormat = OptimalFormatForContent(contentType);

    nsRefPtr<gfxASurface> newSurface =
        new gfxImageSurface(gfxIntSize(size.width, size.height), imageFormat);

    return newSurface.forget();
}

already_AddRefed<gfxASurface>
gfxQtPlatform::OptimizeImage(gfxImageSurface *aSurface,
                             gfxImageFormat format)
{
    
    if (OptimalFormatForContent(gfxASurface::ContentFromFormat(format)) ==
        format) {
        return nullptr;
    }

    return gfxPlatform::OptimizeImage(aSurface, format);
}


nsresult
gfxQtPlatform::GetFontList(nsIAtom *aLangGroup,
                           const nsACString& aGenericFamily,
                           nsTArray<nsString>& aListOfFonts)
{
    return sFontconfigUtils->GetFontList(aLangGroup, aGenericFamily,
                                         aListOfFonts);
}

nsresult
gfxQtPlatform::UpdateFontList()
{
    return sFontconfigUtils->UpdateFontList();
}

nsresult
gfxQtPlatform::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure,
                                bool& aAborted)
{
    return sFontconfigUtils->ResolveFontName(aFontName, aCallback,
                                             aClosure, aAborted);
}

nsresult
gfxQtPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    return sFontconfigUtils->GetStandardFamilyName(aFontName, aFamilyName);
}

gfxFontGroup *
gfxQtPlatform::CreateFontGroup(const nsAString &aFamilies,
                               const gfxFontStyle *aStyle,
                               gfxUserFontSet* aUserFontSet)
{
    return new gfxPangoFontGroup(aFamilies, aStyle, aUserFontSet);
}

gfxFontEntry*
gfxQtPlatform::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                const nsAString& aFontName)
{
    return gfxPangoFontGroup::NewFontEntry(*aProxyEntry, aFontName);
}

gfxFontEntry*
gfxQtPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                 const uint8_t *aFontData, uint32_t aLength)
{
    
    return gfxPangoFontGroup::NewFontEntry(*aProxyEntry,
                                           aFontData, aLength);
}

bool
gfxQtPlatform::SupportsOffMainThreadCompositing()
{
#if defined(MOZ_X11) && !defined(NIGHTLY_BUILD)
  return (PR_GetEnv("MOZ_USE_OMTC") != nullptr) ||
         (PR_GetEnv("MOZ_OMTC_ENABLED") != nullptr);
#else
  return true;
#endif
}

bool
gfxQtPlatform::IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags)
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

void
gfxQtPlatform::GetPlatformCMSOutputProfile(void *&mem, size_t &size)
{
    mem = nullptr;
    size = 0;
}

int32_t
gfxQtPlatform::GetDPI()
{
    return qApp->primaryScreen()->logicalDotsPerInch();
}

gfxImageFormat
gfxQtPlatform::GetOffscreenFormat()
{
    return sOffscreenFormat;
}

int
gfxQtPlatform::GetScreenDepth() const
{
    return mScreenDepth;
}

TemporaryRef<ScaledFont>
gfxQtPlatform::GetScaledFontForFont(DrawTarget* aTarget, gfxFont* aFont)
{
    return GetScaledFontForFontWithCairoSkia(aTarget, aFont);
}






#include "gfxPlatformMac.h"

#include "gfxImageSurface.h"
#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"
#include "mozilla/gfx/2D.h"

#include "gfxMacPlatformFontList.h"
#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxUserFontSet.h"

#include "nsTArray.h"
#include "mozilla/Preferences.h"
#include "qcms.h"
#include "gfx2DGlue.h"

#include <dlfcn.h>

#include "nsCocoaFeatures.h"

using namespace mozilla;
using namespace mozilla::gfx;


enum {
   kAutoActivationDisabled = 1
};
typedef uint32_t AutoActivationSetting;



static void 
DisableFontActivation()
{
    
    CFBundleRef mainBundle = ::CFBundleGetMainBundle();
    CFStringRef mainBundleID = nullptr;

    if (mainBundle) {
        mainBundleID = ::CFBundleGetIdentifier(mainBundle);
    }

    
    if (!mainBundleID) {
        NS_WARNING("missing bundle ID, packaging set up incorrectly");
        return;
    }

    
    void (*CTFontManagerSetAutoActivationSettingPtr)
            (CFStringRef, AutoActivationSetting);
    CTFontManagerSetAutoActivationSettingPtr =
        (void (*)(CFStringRef, AutoActivationSetting))
        dlsym(RTLD_DEFAULT, "CTFontManagerSetAutoActivationSetting");

    
    if (CTFontManagerSetAutoActivationSettingPtr) {
        CTFontManagerSetAutoActivationSettingPtr(mainBundleID,
                                                 kAutoActivationDisabled);
    }
}

gfxPlatformMac::gfxPlatformMac()
{
    DisableFontActivation();
    mFontAntiAliasingThreshold = ReadAntiAliasingThreshold();

    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO) |
                          BackendTypeBit(BackendType::SKIA) |
                          BackendTypeBit(BackendType::COREGRAPHICS);
    uint32_t contentMask = BackendTypeBit(BackendType::COREGRAPHICS);
    InitBackendPrefs(canvasMask, BackendType::COREGRAPHICS,
                     contentMask, BackendType::COREGRAPHICS);
}

gfxPlatformMac::~gfxPlatformMac()
{
    gfxCoreTextShaper::Shutdown();
}

gfxPlatformFontList*
gfxPlatformMac::CreatePlatformFontList()
{
    gfxPlatformFontList* list = new gfxMacPlatformFontList();
    if (NS_SUCCEEDED(list->InitFontList())) {
        return list;
    }
    gfxPlatformFontList::Shutdown();
    return nullptr;
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenSurface(const IntSize& size,
                                       gfxContentType contentType)
{
    nsRefPtr<gfxASurface> newSurface =
      new gfxQuartzSurface(ThebesIntSize(size),
                           OptimalFormatForContent(contentType));
    return newSurface.forget();
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenImageSurface(const gfxIntSize& aSize,
                                            gfxContentType aContentType)
{
    nsRefPtr<gfxASurface> surface =
        CreateOffscreenSurface(aSize.ToIntSize(), aContentType);
#ifdef DEBUG
    nsRefPtr<gfxImageSurface> imageSurface = surface->GetAsImageSurface();
    NS_ASSERTION(imageSurface, "Surface cannot be converted to a gfxImageSurface");
#endif
    return surface.forget();
}


already_AddRefed<gfxASurface>
gfxPlatformMac::OptimizeImage(gfxImageSurface *aSurface,
                              gfxImageFormat format)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();
    nsRefPtr<gfxImageSurface> isurf = aSurface;

    if (format != aSurface->Format()) {
        isurf = new gfxImageSurface (surfaceSize, format);
        if (!isurf->CopyFrom (aSurface)) {
            
            nsRefPtr<gfxASurface> ret = aSurface;
            return ret.forget();
        }
    }

    return nullptr;
}

TemporaryRef<ScaledFont>
gfxPlatformMac::GetScaledFontForFont(DrawTarget* aTarget, gfxFont *aFont)
{
    gfxMacFont *font = static_cast<gfxMacFont*>(aFont);
    return font->GetScaledFont(aTarget);
}

nsresult
gfxPlatformMac::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure, bool& aAborted)
{
    nsAutoString resolvedName;
    if (!gfxPlatformFontList::PlatformFontList()->
             ResolveFontName(aFontName, resolvedName)) {
        aAborted = false;
        return NS_OK;
    }
    aAborted = !(*aCallback)(resolvedName, aClosure);
    return NS_OK;
}

nsresult
gfxPlatformMac::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(aFontName, aFamilyName);
    return NS_OK;
}

gfxFontGroup *
gfxPlatformMac::CreateFontGroup(const nsAString &aFamilies,
                                const gfxFontStyle *aStyle,
                                gfxUserFontSet *aUserFontSet)
{
    return new gfxFontGroup(aFamilies, aStyle, aUserFontSet);
}


gfxFontEntry* 
gfxPlatformMac::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                const nsAString& aFontName)
{
    return gfxPlatformFontList::PlatformFontList()->LookupLocalFont(aProxyEntry, 
                                                                    aFontName);
}

gfxFontEntry* 
gfxPlatformMac::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                 const uint8_t *aFontData, uint32_t aLength)
{
    
    
    
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aProxyEntry,
                                                                     aFontData,
                                                                     aLength);
}

bool
gfxPlatformMac::IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_WOFF     |
                        gfxUserFontSet::FLAG_FORMAT_OPENTYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE_AAT)) {
        return true;
    }

    
    if (aFormatFlags != 0) {
        return false;
    }

    
    return true;
}


nsresult
gfxPlatformMac::GetFontList(nsIAtom *aLangGroup,
                            const nsACString& aGenericFamily,
                            nsTArray<nsString>& aListOfFonts)
{
    gfxPlatformFontList::PlatformFontList()->GetFontList(aLangGroup, aGenericFamily, aListOfFonts);
    return NS_OK;
}

nsresult
gfxPlatformMac::UpdateFontList()
{
    gfxPlatformFontList::PlatformFontList()->UpdateFontList();
    return NS_OK;
}

static const char kFontArialUnicodeMS[] = "Arial Unicode MS";
static const char kFontAppleBraille[] = "Apple Braille";
static const char kFontAppleSymbols[] = "Apple Symbols";
static const char kFontGeneva[] = "Geneva";
static const char kFontGeezaPro[] = "Geeza Pro";
static const char kFontHiraginoKakuGothic[] = "Hiragino Kaku Gothic ProN";
static const char kFontLucidaGrande[] = "Lucida Grande";
static const char kFontMenlo[] = "Menlo";
static const char kFontPlantagenetCherokee[] = "Plantagenet Cherokee";
static const char kFontSTHeiti[] = "STHeiti";

void
gfxPlatformMac::GetCommonFallbackFonts(const uint32_t aCh,
                                       int32_t aRunScript,
                                       nsTArray<const char*>& aFontList)
{
    aFontList.AppendElement(kFontLucidaGrande);

    if (!IS_IN_BMP(aCh)) {
        uint32_t p = aCh >> 16;
        if (p == 1) {
            aFontList.AppendElement(kFontAppleSymbols);
            aFontList.AppendElement(kFontGeneva);
        }
    } else {
        uint32_t b = (aCh >> 8) & 0xff;

        switch (b) {
        case 0x03:
        case 0x05:
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x07:
            aFontList.AppendElement(kFontGeezaPro);
            break;
        case 0x10:
            aFontList.AppendElement(kFontMenlo);
            break;
        case 0x13:  
            aFontList.AppendElement(kFontPlantagenetCherokee);
            break;
        case 0x18:  
            aFontList.AppendElement(kFontSTHeiti);
            break;
        case 0x1d:
        case 0x1e:
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x20:  
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2e:
            aFontList.AppendElement(kFontAppleSymbols);
            aFontList.AppendElement(kFontMenlo);
            aFontList.AppendElement(kFontGeneva);
            aFontList.AppendElement(kFontHiraginoKakuGothic);
            break;
        case 0x2c:
        case 0x2d:
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x28:  
            aFontList.AppendElement(kFontAppleBraille);
            break;
        case 0x4d:
            aFontList.AppendElement(kFontAppleSymbols);
            break;
        case 0xa0:  
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
            aFontList.AppendElement(kFontSTHeiti);
            break;
        case 0xa6:
        case 0xa7:
            aFontList.AppendElement(kFontGeneva);
            aFontList.AppendElement(kFontAppleSymbols);
            break;
        case 0xfc:
        case 0xff:
            aFontList.AppendElement(kFontAppleSymbols);
            break;
        default:
            break;
        }
    }

    
    aFontList.AppendElement(kFontArialUnicodeMS);
}

uint32_t
gfxPlatformMac::ReadAntiAliasingThreshold()
{
    uint32_t threshold = 0;  
    
    
    bool useAntiAliasingThreshold = Preferences::GetBool("gfx.use_text_smoothing_setting", false);

    
    if (!useAntiAliasingThreshold)
        return threshold;
        
    
    CFNumberRef prefValue = (CFNumberRef)CFPreferencesCopyAppValue(CFSTR("AppleAntiAliasingThreshold"), kCFPreferencesCurrentApplication);

    if (prefValue) {
        if (!CFNumberGetValue(prefValue, kCFNumberIntType, &threshold)) {
            threshold = 0;
        }
        CFRelease(prefValue);
    }

    return threshold;
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateThebesSurfaceAliasForDrawTarget_hack(mozilla::gfx::DrawTarget *aTarget)
{
  if (aTarget->GetType() == BackendType::COREGRAPHICS) {
    CGContextRef cg = static_cast<CGContextRef>(aTarget->GetNativeSurface(NativeSurfaceType::CGCONTEXT));
    unsigned char* data = (unsigned char*)CGBitmapContextGetData(cg);
    size_t bpp = CGBitmapContextGetBitsPerPixel(cg);
    size_t stride = CGBitmapContextGetBytesPerRow(cg);
    gfxIntSize size(aTarget->GetSize().width, aTarget->GetSize().height);
    nsRefPtr<gfxImageSurface> imageSurface = new gfxImageSurface(data, size, stride, bpp == 2
                                                                                     ? gfxImageFormat::RGB16_565
                                                                                     : gfxImageFormat::ARGB32);
    
    
    
    return imageSurface.forget();
  } else {
    return GetThebesSurfaceForDrawTarget(aTarget);
  }
}

already_AddRefed<gfxASurface>
gfxPlatformMac::GetThebesSurfaceForDrawTarget(DrawTarget *aTarget)
{
  if (aTarget->GetType() == BackendType::COREGRAPHICS_ACCELERATED) {
    RefPtr<SourceSurface> source = aTarget->Snapshot();
    RefPtr<DataSourceSurface> sourceData = source->GetDataSurface();
    unsigned char* data = sourceData->GetData();
    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(data, ThebesIntSize(sourceData->GetSize()), sourceData->Stride(),
                                                         gfxImageFormat::ARGB32);
    
    nsRefPtr<gfxImageSurface> cpy = new gfxImageSurface(ThebesIntSize(sourceData->GetSize()), gfxImageFormat::ARGB32);
    cpy->CopyFrom(surf);
    return cpy.forget();
  } else if (aTarget->GetType() == BackendType::COREGRAPHICS) {
    CGContextRef cg = static_cast<CGContextRef>(aTarget->GetNativeSurface(NativeSurfaceType::CGCONTEXT));

    
    IntSize intSize = aTarget->GetSize();
    gfxIntSize size(intSize.width, intSize.height);

    nsRefPtr<gfxASurface> surf =
      new gfxQuartzSurface(cg, size);

    return surf.forget();
  }

  return gfxPlatform::GetThebesSurfaceForDrawTarget(aTarget);
}

bool
gfxPlatformMac::UseAcceleratedCanvas()
{
  
  return nsCocoaFeatures::OnLionOrLater() && Preferences::GetBool("gfx.canvas.azure.accelerated", false);
}

bool
gfxPlatformMac::SupportsOffMainThreadCompositing()
{
  return true;
}

void
gfxPlatformMac::GetPlatformCMSOutputProfile(void* &mem, size_t &size)
{
    mem = nullptr;
    size = 0;

    CGColorSpaceRef cspace = ::CGDisplayCopyColorSpace(::CGMainDisplayID());
    if (!cspace) {
        cspace = ::CGColorSpaceCreateDeviceRGB();
    }
    if (!cspace) {
        return;
    }

    CFDataRef iccp = ::CGColorSpaceCopyICCProfile(cspace);

    ::CFRelease(cspace);

    if (!iccp) {
        return;
    }

    
    size = static_cast<size_t>(::CFDataGetLength(iccp));
    if (size > 0) {
        void *data = malloc(size);
        if (data) {
            memcpy(data, ::CFDataGetBytePtr(iccp), size);
            mem = data;
        } else {
            size = 0;
        }
    }

    ::CFRelease(iccp);
}

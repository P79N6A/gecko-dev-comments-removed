




#include "gfxPlatformMac.h"

#include "gfxImageSurface.h"
#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/QuartzSupport.h"

#include "gfxMacPlatformFontList.h"
#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxUserFontSet.h"

#include "nsCRT.h"
#include "nsTArray.h"
#include "nsUnicodeRange.h"

#include "mozilla/Preferences.h"

#include "qcms.h"

#include <dlfcn.h>

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
    CFStringRef mainBundleID = NULL;

    if (mainBundle) {
        mainBundleID = ::CFBundleGetIdentifier(mainBundle);
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
    mOSXVersion = 0;
    OSXVersion();

    DisableFontActivation();
    mFontAntiAliasingThreshold = ReadAntiAliasingThreshold();

    uint32_t canvasMask = (1 << BACKEND_CAIRO) | (1 << BACKEND_SKIA) | (1 << BACKEND_COREGRAPHICS);
    uint32_t contentMask = 0;
    InitBackendPrefs(canvasMask, contentMask);
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
gfxPlatformMac::CreateOffscreenSurface(const gfxIntSize& size,
                                       gfxASurface::gfxContentType contentType)
{
    gfxASurface *newSurface = nullptr;

    newSurface = new gfxQuartzSurface(size, OptimalFormatForContent(contentType));

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenImageSurface(const gfxIntSize& aSize,
                                            gfxASurface::gfxContentType aContentType)
{
    nsRefPtr<gfxASurface> surface = CreateOffscreenSurface(aSize, aContentType);
#ifdef DEBUG
    nsRefPtr<gfxImageSurface> imageSurface = surface->GetAsImageSurface();
    NS_ASSERTION(imageSurface, "Surface cannot be converted to a gfxImageSurface");
#endif
    return surface.forget();
}


already_AddRefed<gfxASurface>
gfxPlatformMac::OptimizeImage(gfxImageSurface *aSurface,
                              gfxASurface::gfxImageFormat format)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();
    nsRefPtr<gfxImageSurface> isurf = aSurface;

    if (format != aSurface->Format()) {
        isurf = new gfxImageSurface (surfaceSize, format);
        if (!isurf->CopyFrom (aSurface)) {
            
            NS_ADDREF(aSurface);
            return aSurface;
        }
    }

    nsRefPtr<gfxASurface> ret = new gfxQuartzImageSurface(isurf);
    return ret.forget();
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
static const char kFontAppleMyungjo[] = "AppleMyungjo";
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


int32_t 
gfxPlatformMac::OSXVersion()
{
    if (!mOSXVersion) {
        
        OSErr err = ::Gestalt(gestaltSystemVersion, reinterpret_cast<SInt32*>(&mOSXVersion));
        if (err != noErr) {
            
            NS_ERROR("Couldn't determine OS X version, assuming 10.4");
            mOSXVersion = MAC_OS_X_VERSION_10_4_HEX;
        }
    }
    return mOSXVersion;
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
  if (aTarget->GetType() == BACKEND_COREGRAPHICS) {
    CGContextRef cg = static_cast<CGContextRef>(aTarget->GetNativeSurface(NATIVE_SURFACE_CGCONTEXT));
    unsigned char* data = (unsigned char*)CGBitmapContextGetData(cg);
    size_t bpp = CGBitmapContextGetBitsPerPixel(cg);
    size_t stride = CGBitmapContextGetBytesPerRow(cg);
    gfxIntSize size(aTarget->GetSize().width, aTarget->GetSize().height);
    nsRefPtr<gfxImageSurface> imageSurface = new gfxImageSurface(data, size, stride, bpp == 2
                                                                                     ? gfxImageFormat::ImageFormatRGB16_565
                                                                                     : gfxImageFormat::ImageFormatARGB32);
    
    
    
    return imageSurface.forget();
  } else {
    return GetThebesSurfaceForDrawTarget(aTarget);
  }
}

already_AddRefed<gfxASurface>
gfxPlatformMac::GetThebesSurfaceForDrawTarget(DrawTarget *aTarget)
{
  if (aTarget->GetType() == BACKEND_COREGRAPHICS_ACCELERATED) {
    RefPtr<SourceSurface> source = aTarget->Snapshot();
    RefPtr<DataSourceSurface> sourceData = source->GetDataSurface();
    unsigned char* data = sourceData->GetData();
    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(data, ThebesIntSize(sourceData->GetSize()), sourceData->Stride(),
                                                         gfxImageSurface::ImageFormatARGB32);
    
    nsRefPtr<gfxImageSurface> cpy = new gfxImageSurface(ThebesIntSize(sourceData->GetSize()), gfxImageSurface::ImageFormatARGB32);
    cpy->CopyFrom(surf);
    return cpy.forget();
  } else if (aTarget->GetType() == BACKEND_COREGRAPHICS) {
    CGContextRef cg = static_cast<CGContextRef>(aTarget->GetNativeSurface(NATIVE_SURFACE_CGCONTEXT));

    
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
  
  return OSXVersion() >= 0x1070 && Preferences::GetBool("gfx.canvas.azure.accelerated", false);
}

qcms_profile *
gfxPlatformMac::GetPlatformCMSOutputProfile()
{
    qcms_profile *profile = nullptr;
    CMProfileRef cmProfile;
    CMProfileLocation *location;
    UInt32 locationSize;

    












    CGDirectDisplayID displayID = CGMainDisplayID();

    CMError err = CMGetProfileByAVID(static_cast<CMDisplayIDType>(displayID), &cmProfile);
    if (err != noErr)
        return nullptr;

    
    err = NCMGetProfileLocation(cmProfile, NULL, &locationSize);
    if (err != noErr)
        return nullptr;

    
    location = static_cast<CMProfileLocation*>(malloc(locationSize));
    if (!location)
        goto fail_close;

    err = NCMGetProfileLocation(cmProfile, location, &locationSize);
    if (err != noErr)
        goto fail_location;

    switch (location->locType) {
#ifndef __LP64__
    case cmFileBasedProfile: {
        FSRef fsRef;
        if (!FSpMakeFSRef(&location->u.fileLoc.spec, &fsRef)) {
            char path[512];
            if (!FSRefMakePath(&fsRef, reinterpret_cast<UInt8*>(path), sizeof(path))) {
                profile = qcms_profile_from_path(path);
#ifdef DEBUG_tor
                if (profile)
                    fprintf(stderr,
                            "ICM profile read from %s fileLoc successfully\n", path);
#endif
            }
        }
        break;
    }
#endif
    case cmPathBasedProfile:
        profile = qcms_profile_from_path(location->u.pathLoc.path);
#ifdef DEBUG_tor
        if (profile)
            fprintf(stderr,
                    "ICM profile read from %s pathLoc successfully\n",
                    device.u.pathLoc.path);
#endif
        break;
    default:
#ifdef DEBUG_tor
        fprintf(stderr, "Unhandled ColorSync profile location\n");
#endif
        break;
    }

fail_location:
    free(location);
fail_close:
    CMCloseProfile(cmProfile);
    return profile;
}

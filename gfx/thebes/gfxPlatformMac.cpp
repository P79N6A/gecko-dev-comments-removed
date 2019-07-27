




#include "gfxPlatformMac.h"

#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"
#include "mozilla/gfx/2D.h"

#include "gfxMacPlatformFontList.h"
#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxTextRun.h"
#include "gfxUserFontSet.h"

#include "nsTArray.h"
#include "mozilla/Preferences.h"
#include "mozilla/VsyncDispatcher.h"
#include "qcms.h"
#include "gfx2DGlue.h"

#include <dlfcn.h>
#include <CoreVideo/CoreVideo.h>

#include "nsCocoaFeatures.h"
#include "mozilla/layers/CompositorParent.h"
#include "VsyncSource.h"

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
    uint32_t contentMask = BackendTypeBit(BackendType::COREGRAPHICS) |
                           BackendTypeBit(BackendType::SKIA);
    InitBackendPrefs(canvasMask, BackendType::COREGRAPHICS,
                     contentMask, BackendType::COREGRAPHICS);

    
    
    
    
    struct rlimit limits;
    if (getrlimit(RLIMIT_NOFILE, &limits) == 0) {
        limits.rlim_cur = std::min(rlim_t(OPEN_MAX), limits.rlim_max);
        if (setrlimit(RLIMIT_NOFILE, &limits) != 0) {
            NS_WARNING("Unable to bump RLIMIT_NOFILE to the maximum number on this OS");
        }
    }
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
      new gfxQuartzSurface(size, OptimalFormatForContent(contentType));
    return newSurface.forget();
}

TemporaryRef<ScaledFont>
gfxPlatformMac::GetScaledFontForFont(DrawTarget* aTarget, gfxFont *aFont)
{
    gfxMacFont *font = static_cast<gfxMacFont*>(aFont);
    return font->GetScaledFont(aTarget);
}

nsresult
gfxPlatformMac::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(aFontName, aFamilyName);
    return NS_OK;
}

gfxFontGroup *
gfxPlatformMac::CreateFontGroup(const FontFamilyList& aFontFamilyList,
                                const gfxFontStyle *aStyle,
                                gfxUserFontSet *aUserFontSet)
{
    return new gfxFontGroup(aFontFamilyList, aStyle, aUserFontSet);
}


gfxFontEntry* 
gfxPlatformMac::LookupLocalFont(const nsAString& aFontName,
                                uint16_t aWeight,
                                int16_t aStretch,
                                bool aItalic)
{
    return gfxPlatformFontList::PlatformFontList()->LookupLocalFont(aFontName,
                                                                    aWeight,
                                                                    aStretch,
                                                                    aItalic);
}

gfxFontEntry* 
gfxPlatformMac::MakePlatformFont(const nsAString& aFontName,
                                 uint16_t aWeight,
                                 int16_t aStretch,
                                 bool aItalic,
                                 const uint8_t* aFontData,
                                 uint32_t aLength)
{
    
    
    
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aFontName,
                                                                     aWeight,
                                                                     aStretch,
                                                                     aItalic,
                                                                     aFontData,
                                                                     aLength);
}

bool
gfxPlatformMac::IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMATS_COMMON |
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
static const char kFontAppleColorEmoji[] = "Apple Color Emoji";
static const char kFontAppleSymbols[] = "Apple Symbols";
static const char kFontDevanagariSangamMN[] = "Devanagari Sangam MN";
static const char kFontEuphemiaUCAS[] = "Euphemia UCAS";
static const char kFontGeneva[] = "Geneva";
static const char kFontGeezaPro[] = "Geeza Pro";
static const char kFontGujaratiSangamMN[] = "Gujarati Sangam MN";
static const char kFontGurmukhiMN[] = "Gurmukhi MN";
static const char kFontHiraginoKakuGothic[] = "Hiragino Kaku Gothic ProN";
static const char kFontHiraginoSansGB[] = "Hiragino Sans GB";
static const char kFontKefa[] = "Kefa";
static const char kFontKhmerMN[] = "Khmer MN";
static const char kFontLaoMN[] = "Lao MN";
static const char kFontLucidaGrande[] = "Lucida Grande";
static const char kFontMenlo[] = "Menlo";
static const char kFontMicrosoftTaiLe[] = "Microsoft Tai Le";
static const char kFontMingLiUExtB[] = "MingLiU-ExtB";
static const char kFontMyanmarMN[] = "Myanmar MN";
static const char kFontPlantagenetCherokee[] = "Plantagenet Cherokee";
static const char kFontSimSunExtB[] = "SimSun-ExtB";
static const char kFontSongtiSC[] = "Songti SC";
static const char kFontSTHeiti[] = "STHeiti";
static const char kFontSTIXGeneral[] = "STIXGeneral";
static const char kFontTamilMN[] = "Tamil MN";

void
gfxPlatformMac::GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                       int32_t aRunScript,
                                       nsTArray<const char*>& aFontList)
{
    if (aNextCh == 0xfe0f) {
        aFontList.AppendElement(kFontAppleColorEmoji);
    }

    aFontList.AppendElement(kFontLucidaGrande);

    if (!IS_IN_BMP(aCh)) {
        uint32_t p = aCh >> 16;
        uint32_t b = aCh >> 8;
        if (p == 1) {
            if (b >= 0x1f0 && b < 0x1f7) {
                aFontList.AppendElement(kFontAppleColorEmoji);
            } else {
                aFontList.AppendElement(kFontAppleSymbols);
                aFontList.AppendElement(kFontSTIXGeneral);
                aFontList.AppendElement(kFontGeneva);
            }
        } else if (p == 2) {
            
            aFontList.AppendElement(kFontMingLiUExtB);
            aFontList.AppendElement(kFontSimSunExtB);
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
        case 0x09:
            aFontList.AppendElement(kFontDevanagariSangamMN);
            break;
        case 0x0a:
            aFontList.AppendElement(kFontGurmukhiMN);
            aFontList.AppendElement(kFontGujaratiSangamMN);
            break;
        case 0x0b:
            aFontList.AppendElement(kFontTamilMN);
            break;
        case 0x0e:
            aFontList.AppendElement(kFontLaoMN);
            break;
        case 0x0f:
            aFontList.AppendElement(kFontSongtiSC);
            break;
        case 0x10:
            aFontList.AppendElement(kFontMenlo);
            aFontList.AppendElement(kFontMyanmarMN);
            break;
        case 0x13:  
            aFontList.AppendElement(kFontPlantagenetCherokee);
            aFontList.AppendElement(kFontKefa);
            break;
        case 0x14:  
        case 0x15:
        case 0x16:
            aFontList.AppendElement(kFontEuphemiaUCAS);
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x18:  
            aFontList.AppendElement(kFontSTHeiti);
            aFontList.AppendElement(kFontEuphemiaUCAS);
            break;
        case 0x19:  
            aFontList.AppendElement(kFontKhmerMN);
            aFontList.AppendElement(kFontMicrosoftTaiLe);
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
            aFontList.AppendElement(kFontHiraginoKakuGothic);
            aFontList.AppendElement(kFontAppleSymbols);
            aFontList.AppendElement(kFontMenlo);
            aFontList.AppendElement(kFontSTIXGeneral);
            aFontList.AppendElement(kFontGeneva);
            aFontList.AppendElement(kFontAppleColorEmoji);
            break;
        case 0x2c:
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x2d:
            aFontList.AppendElement(kFontKefa);
            aFontList.AppendElement(kFontGeneva);
            break;
        case 0x28:  
            aFontList.AppendElement(kFontAppleBraille);
            break;
        case 0x31:
            aFontList.AppendElement(kFontHiraginoSansGB);
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
        case 0xab:
            aFontList.AppendElement(kFontKefa);
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

bool
gfxPlatformMac::UseAcceleratedCanvas()
{
  
  return nsCocoaFeatures::OnLionOrLater() && Preferences::GetBool("gfx.canvas.azure.accelerated", false);
}

bool
gfxPlatformMac::UseProgressivePaint()
{
  
  
  return nsCocoaFeatures::OnLionOrLater() && gfxPlatform::UseProgressivePaint();
}


static CVReturn VsyncCallback(CVDisplayLinkRef aDisplayLink,
                              const CVTimeStamp* aNow,
                              const CVTimeStamp* aOutputTime,
                              CVOptionFlags aFlagsIn,
                              CVOptionFlags* aFlagsOut,
                              void* aDisplayLinkContext);

class OSXVsyncSource final : public VsyncSource
{
public:
  OSXVsyncSource()
  {
  }

  virtual Display& GetGlobalDisplay() override
  {
    return mGlobalDisplay;
  }

  class OSXDisplay final : public VsyncSource::Display
  {
  public:
    OSXDisplay()
      : mDisplayLink(nullptr)
    {
      MOZ_ASSERT(NS_IsMainThread());
      mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    }

    ~OSXDisplay()
    {
      MOZ_ASSERT(NS_IsMainThread());
      mTimer->Cancel();
      mTimer = nullptr;
      DisableVsync();
    }

    static void RetryEnableVsync(nsITimer* aTimer, void* aOsxDisplay)
    {
      MOZ_ASSERT(NS_IsMainThread());
      OSXDisplay* osxDisplay = static_cast<OSXDisplay*>(aOsxDisplay);
      MOZ_ASSERT(osxDisplay);
      osxDisplay->EnableVsync();
    }

    virtual void EnableVsync() override
    {
      MOZ_ASSERT(NS_IsMainThread());
      if (IsVsyncEnabled()) {
        return;
      }

      
      
      
      
      if (CVDisplayLinkCreateWithActiveCGDisplays(&mDisplayLink) != kCVReturnSuccess) {
        NS_WARNING("Could not create a display link with all active displays. Retrying\n");
        CVDisplayLinkRelease(mDisplayLink);
        mDisplayLink = nullptr;

        
        
        
        
        
        
        
        
        
        
        
        
        uint32_t delay = 100;
        mTimer->InitWithFuncCallback(RetryEnableVsync, this, delay, nsITimer::TYPE_ONE_SHOT);
        return;
      }

      if (CVDisplayLinkSetOutputCallback(mDisplayLink, &VsyncCallback, this) != kCVReturnSuccess) {
        NS_WARNING("Could not set displaylink output callback");
        CVDisplayLinkRelease(mDisplayLink);
        mDisplayLink = nullptr;
        return;
      }

      mPreviousTimestamp = TimeStamp::Now();
      if (CVDisplayLinkStart(mDisplayLink) != kCVReturnSuccess) {
        NS_WARNING("Could not activate the display link");
        CVDisplayLinkRelease(mDisplayLink);
        mDisplayLink = nullptr;
      }
    }

    virtual void DisableVsync() override
    {
      MOZ_ASSERT(NS_IsMainThread());
      if (!IsVsyncEnabled()) {
        return;
      }

      
      if (mDisplayLink) {
        CVDisplayLinkRelease(mDisplayLink);
        mDisplayLink = nullptr;
      }
    }

    virtual bool IsVsyncEnabled() override
    {
      MOZ_ASSERT(NS_IsMainThread());
      return mDisplayLink != nullptr;
    }

    
    
    
    
    
    TimeStamp mPreviousTimestamp;

  private:
    
    CVDisplayLinkRef   mDisplayLink;
    nsRefPtr<nsITimer> mTimer;
  }; 

private:
  virtual ~OSXVsyncSource()
  {
  }

  OSXDisplay mGlobalDisplay;
}; 

static CVReturn VsyncCallback(CVDisplayLinkRef aDisplayLink,
                              const CVTimeStamp* aNow,
                              const CVTimeStamp* aOutputTime,
                              CVOptionFlags aFlagsIn,
                              CVOptionFlags* aFlagsOut,
                              void* aDisplayLinkContext)
{
  
  OSXVsyncSource::OSXDisplay* display = (OSXVsyncSource::OSXDisplay*) aDisplayLinkContext;
  int64_t nextVsyncTimestamp = aOutputTime->hostTime;
  mozilla::TimeStamp nextVsync = mozilla::TimeStamp::FromSystemTime(nextVsyncTimestamp);

  mozilla::TimeStamp previousVsync = display->mPreviousTimestamp;
  display->mPreviousTimestamp = nextVsync;
  MOZ_ASSERT(TimeStamp::Now() > previousVsync);

  display->NotifyVsync(previousVsync);
  return kCVReturnSuccess;
}

already_AddRefed<mozilla::gfx::VsyncSource>
gfxPlatformMac::CreateHardwareVsyncSource()
{
  nsRefPtr<VsyncSource> osxVsyncSource = new OSXVsyncSource();
  VsyncSource::Display& primaryDisplay = osxVsyncSource->GetGlobalDisplay();
  primaryDisplay.EnableVsync();
  if (!primaryDisplay.IsVsyncEnabled()) {
    NS_WARNING("OS X Vsync source not enabled. Falling back to software vsync.\n");
    return gfxPlatform::CreateHardwareVsyncSource();
  }

  primaryDisplay.DisableVsync();
  return osxVsyncSource.forget();
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








#include "nsMediaFeatures.h"
#include "nsGkAtoms.h"
#include "nsCSSKeywords.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsCSSValue.h"
#ifdef XP_WIN
#include "mozilla/LookAndFeel.h"
#endif
#include "nsCSSRuleProcessor.h"
#include "nsDeviceContext.h"
#include "nsIDocument.h"

using namespace mozilla;

static const nsCSSProps::KTableValue kOrientationKeywords[] = {
  eCSSKeyword_portrait,                 NS_STYLE_ORIENTATION_PORTRAIT,
  eCSSKeyword_landscape,                NS_STYLE_ORIENTATION_LANDSCAPE,
  eCSSKeyword_UNKNOWN,                  -1
};

static const nsCSSProps::KTableValue kScanKeywords[] = {
  eCSSKeyword_progressive,              NS_STYLE_SCAN_PROGRESSIVE,
  eCSSKeyword_interlace,                NS_STYLE_SCAN_INTERLACE,
  eCSSKeyword_UNKNOWN,                  -1
};

#ifdef XP_WIN
struct WindowsThemeName {
    LookAndFeel::WindowsTheme id;
    const wchar_t* name;
};


const WindowsThemeName themeStrings[] = {
    { LookAndFeel::eWindowsTheme_Aero,       L"aero" },
    { LookAndFeel::eWindowsTheme_AeroLite,   L"aero-lite" },
    { LookAndFeel::eWindowsTheme_LunaBlue,   L"luna-blue" },
    { LookAndFeel::eWindowsTheme_LunaOlive,  L"luna-olive" },
    { LookAndFeel::eWindowsTheme_LunaSilver, L"luna-silver" },
    { LookAndFeel::eWindowsTheme_Royale,     L"royale" },
    { LookAndFeel::eWindowsTheme_Zune,       L"zune" },
    { LookAndFeel::eWindowsTheme_Generic,    L"generic" }
};

struct OperatingSystemVersionInfo {
    LookAndFeel::OperatingSystemVersion id;
    const wchar_t* name;
};


const OperatingSystemVersionInfo osVersionStrings[] = {
    { LookAndFeel::eOperatingSystemVersion_WindowsXP,     L"windows-xp" },
    { LookAndFeel::eOperatingSystemVersion_WindowsVista,  L"windows-vista" },
    { LookAndFeel::eOperatingSystemVersion_Windows7,      L"windows-win7" },
    { LookAndFeel::eOperatingSystemVersion_Windows8,      L"windows-win8" },
    { LookAndFeel::eOperatingSystemVersion_Windows10,     L"windows-win10" }
};
#endif


static nsSize
GetSize(nsPresContext* aPresContext)
{
    nsSize size;
    if (aPresContext->IsRootPaginatedDocument())
        
        size = aPresContext->GetPageSize();
    else
        size = aPresContext->GetVisibleArea().Size();
    return size;
}

static nsresult
GetWidth(nsPresContext* aPresContext, const nsMediaFeature*,
         nsCSSValue& aResult)
{
    nsSize size = GetSize(aPresContext);
    float pixelWidth = aPresContext->AppUnitsToFloatCSSPixels(size.width);
    aResult.SetFloatValue(pixelWidth, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetHeight(nsPresContext* aPresContext, const nsMediaFeature*,
          nsCSSValue& aResult)
{
    nsSize size = GetSize(aPresContext);
    float pixelHeight = aPresContext->AppUnitsToFloatCSSPixels(size.height);
    aResult.SetFloatValue(pixelHeight, eCSSUnit_Pixel);
    return NS_OK;
}

inline static nsDeviceContext*
GetDeviceContextFor(nsPresContext* aPresContext)
{
  
  
  
  
  
  return aPresContext->DeviceContext();
}

static bool
ShouldResistFingerprinting(nsPresContext* aPresContext)
{
    return nsContentUtils::ShouldResistFingerprinting(aPresContext->GetDocShell());
}


static nsSize
GetDeviceSize(nsPresContext* aPresContext)
{
    nsSize size;

    if (ShouldResistFingerprinting(aPresContext) || aPresContext->IsDeviceSizePageSize()) {
        size = GetSize(aPresContext);
    } else if (aPresContext->IsRootPaginatedDocument()) {
        
        
        
        size = aPresContext->GetPageSize();
    } else {
        GetDeviceContextFor(aPresContext)->
            GetDeviceSurfaceDimensions(size.width, size.height);
    }
    return size;
}

static nsresult
GetDeviceWidth(nsPresContext* aPresContext, const nsMediaFeature*,
               nsCSSValue& aResult)
{
    nsSize size = GetDeviceSize(aPresContext);
    float pixelWidth = aPresContext->AppUnitsToFloatCSSPixels(size.width);
    aResult.SetFloatValue(pixelWidth, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetDeviceHeight(nsPresContext* aPresContext, const nsMediaFeature*,
                nsCSSValue& aResult)
{
    nsSize size = GetDeviceSize(aPresContext);
    float pixelHeight = aPresContext->AppUnitsToFloatCSSPixels(size.height);
    aResult.SetFloatValue(pixelHeight, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetOrientation(nsPresContext* aPresContext, const nsMediaFeature*,
               nsCSSValue& aResult)
{
    nsSize size = GetSize(aPresContext);
    int32_t orientation;
    if (size.width > size.height) {
        orientation = NS_STYLE_ORIENTATION_LANDSCAPE;
    } else {
        
        orientation = NS_STYLE_ORIENTATION_PORTRAIT;
    }

    aResult.SetIntValue(orientation, eCSSUnit_Enumerated);
    return NS_OK;
}

static nsresult
GetDeviceOrientation(nsPresContext* aPresContext, const nsMediaFeature*,
                     nsCSSValue& aResult)
{
    nsSize size = GetDeviceSize(aPresContext);
    int32_t orientation;
    if (size.width > size.height) {
        orientation = NS_STYLE_ORIENTATION_LANDSCAPE;
    } else {
        
        orientation = NS_STYLE_ORIENTATION_PORTRAIT;
    }

    aResult.SetIntValue(orientation, eCSSUnit_Enumerated);
    return NS_OK;
}

static nsresult
GetIsResourceDocument(nsPresContext* aPresContext, const nsMediaFeature*,
                      nsCSSValue& aResult)
{
  nsIDocument* doc = aPresContext->Document();
  aResult.SetIntValue(doc && doc->IsResourceDoc() ? 1 : 0, eCSSUnit_Integer);
  return NS_OK;
}


static nsresult
MakeArray(const nsSize& aSize, nsCSSValue& aResult)
{
    nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);

    a->Item(0).SetIntValue(aSize.width, eCSSUnit_Integer);
    a->Item(1).SetIntValue(aSize.height, eCSSUnit_Integer);

    aResult.SetArrayValue(a, eCSSUnit_Array);
    return NS_OK;
}

static nsresult
GetAspectRatio(nsPresContext* aPresContext, const nsMediaFeature*,
               nsCSSValue& aResult)
{
    return MakeArray(GetSize(aPresContext), aResult);
}

static nsresult
GetDeviceAspectRatio(nsPresContext* aPresContext, const nsMediaFeature*,
                     nsCSSValue& aResult)
{
    return MakeArray(GetDeviceSize(aPresContext), aResult);
}

static nsresult
GetColor(nsPresContext* aPresContext, const nsMediaFeature*,
         nsCSSValue& aResult)
{
    uint32_t depth = 24; 

    if (!ShouldResistFingerprinting(aPresContext)) {
        
        
        
        
        nsDeviceContext *dx = GetDeviceContextFor(aPresContext);
        dx->GetDepth(depth);
    }

    
    
    
    depth /= 3;
    aResult.SetIntValue(int32_t(depth), eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetColorIndex(nsPresContext* aPresContext, const nsMediaFeature*,
              nsCSSValue& aResult)
{
    
    
    
    
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetMonochrome(nsPresContext* aPresContext, const nsMediaFeature*,
              nsCSSValue& aResult)
{
    
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetResolution(nsPresContext* aPresContext, const nsMediaFeature*,
              nsCSSValue& aResult)
{
    float dpi = 96; 

    if (!ShouldResistFingerprinting(aPresContext)) {
      
      
      dpi = float(nsPresContext::AppUnitsPerCSSInch()) /
            float(aPresContext->AppUnitsPerDevPixel());
    }

    aResult.SetFloatValue(dpi, eCSSUnit_Inch);
    return NS_OK;
}

static nsresult
GetScan(nsPresContext* aPresContext, const nsMediaFeature*,
        nsCSSValue& aResult)
{
    
    
    aResult.Reset();
    return NS_OK;
}

static nsresult
GetGrid(nsPresContext* aPresContext, const nsMediaFeature*,
        nsCSSValue& aResult)
{
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetDevicePixelRatio(nsPresContext* aPresContext, const nsMediaFeature*,
                    nsCSSValue& aResult)
{
    if (!ShouldResistFingerprinting(aPresContext)) {
        float ratio = aPresContext->CSSPixelsToDevPixels(1.0f);
        aResult.SetFloatValue(ratio, eCSSUnit_Number);
    } else {
        aResult.SetFloatValue(1.0, eCSSUnit_Number);
    }
    return NS_OK;
}

static nsresult
GetSystemMetric(nsPresContext* aPresContext, const nsMediaFeature* aFeature,
                nsCSSValue& aResult)
{
    aResult.Reset();
    if (ShouldResistFingerprinting(aPresContext)) {
        
        
        return NS_OK;
    }

    MOZ_ASSERT(aFeature->mValueType == nsMediaFeature::eBoolInteger,
               "unexpected type");
    nsIAtom *metricAtom = *aFeature->mData.mMetric;
    bool hasMetric = nsCSSRuleProcessor::HasSystemMetric(metricAtom);
    aResult.SetIntValue(hasMetric ? 1 : 0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetWindowsTheme(nsPresContext* aPresContext, const nsMediaFeature* aFeature,
                nsCSSValue& aResult)
{
    aResult.Reset();
    if (ShouldResistFingerprinting(aPresContext)) {
        return NS_OK;
    }

#ifdef XP_WIN
    uint8_t windowsThemeId =
        nsCSSRuleProcessor::GetWindowsThemeIdentifier();

    
    if (windowsThemeId == LookAndFeel::eWindowsTheme_Classic)
        return NS_OK;

    
    for (size_t i = 0; i < ArrayLength(themeStrings); ++i) {
        if (windowsThemeId == themeStrings[i].id) {
            aResult.SetStringValue(nsDependentString(themeStrings[i].name),
                                   eCSSUnit_Ident);
            break;
        }
    }
#endif
    return NS_OK;
}

static nsresult
GetOperatinSystemVersion(nsPresContext* aPresContext, const nsMediaFeature* aFeature,
                         nsCSSValue& aResult)
{
    aResult.Reset();
    if (ShouldResistFingerprinting(aPresContext)) {
        return NS_OK;
    }

#ifdef XP_WIN
    int32_t metricResult;
    if (NS_SUCCEEDED(
          LookAndFeel::GetInt(LookAndFeel::eIntID_OperatingSystemVersionIdentifier,
                              &metricResult))) {
        for (size_t i = 0; i < ArrayLength(osVersionStrings); ++i) {
            if (metricResult == osVersionStrings[i].id) {
                aResult.SetStringValue(nsDependentString(osVersionStrings[i].name),
                                       eCSSUnit_Ident);
                break;
            }
        }
    }
#endif
    return NS_OK;
}

static nsresult
GetIsGlyph(nsPresContext* aPresContext, const nsMediaFeature* aFeature,
          nsCSSValue& aResult)
{
    aResult.SetIntValue(aPresContext->IsGlyph() ? 1 : 0, eCSSUnit_Integer);
    return NS_OK;
}










 const nsMediaFeature
nsMediaFeatures::features[] = {
    {
        &nsGkAtoms::width,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nullptr },
        GetWidth
    },
    {
        &nsGkAtoms::height,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nullptr },
        GetHeight
    },
    {
        &nsGkAtoms::deviceWidth,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nullptr },
        GetDeviceWidth
    },
    {
        &nsGkAtoms::deviceHeight,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nullptr },
        GetDeviceHeight
    },
    {
        &nsGkAtoms::orientation,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eEnumerated,
        { kOrientationKeywords },
        GetOrientation
    },
    {
        &nsGkAtoms::aspectRatio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eIntRatio,
        { nullptr },
        GetAspectRatio
    },
    {
        &nsGkAtoms::deviceAspectRatio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eIntRatio,
        { nullptr },
        GetDeviceAspectRatio
    },
    {
        &nsGkAtoms::color,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nullptr },
        GetColor
    },
    {
        &nsGkAtoms::colorIndex,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nullptr },
        GetColorIndex
    },
    {
        &nsGkAtoms::monochrome,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nullptr },
        GetMonochrome
    },
    {
        &nsGkAtoms::resolution,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eResolution,
        { nullptr },
        GetResolution
    },
    {
        &nsGkAtoms::scan,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eEnumerated,
        { kScanKeywords },
        GetScan
    },
    {
        &nsGkAtoms::grid,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { nullptr },
        GetGrid
    },

    
    {
        &nsGkAtoms::_moz_device_pixel_ratio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eFloat,
        { nullptr },
        GetDevicePixelRatio
    },
    {
        &nsGkAtoms::_moz_device_orientation,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eEnumerated,
        { kOrientationKeywords },
        GetDeviceOrientation
    },
    {
        &nsGkAtoms::_moz_is_resource_document,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { nullptr },
        GetIsResourceDocument
    },
    {
        &nsGkAtoms::_moz_color_picker_available,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::color_picker_available },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_scrollbar_start_backward,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::scrollbar_start_backward },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_scrollbar_start_forward,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::scrollbar_start_forward },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_scrollbar_end_backward,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::scrollbar_end_backward },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_scrollbar_end_forward,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::scrollbar_end_forward },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_scrollbar_thumb_proportional,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::scrollbar_thumb_proportional },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_images_in_menus,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::images_in_menus },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_images_in_buttons,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::images_in_buttons },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_overlay_scrollbars,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::overlay_scrollbars },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_windows_default_theme,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::windows_default_theme },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_mac_graphite_theme,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::mac_graphite_theme },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_mac_lion_theme,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::mac_lion_theme },
        GetSystemMetric
    },
    {
      &nsGkAtoms::_moz_mac_yosemite_theme,
      nsMediaFeature::eMinMaxNotAllowed,
      nsMediaFeature::eBoolInteger,
      { &nsGkAtoms::mac_yosemite_theme },
      GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_windows_compositor,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::windows_compositor },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_windows_classic,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::windows_classic },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_windows_glass,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::windows_glass },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_touch_enabled,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::touch_enabled },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_menubar_drag,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::menubar_drag },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_windows_theme,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eIdent,
        { nullptr },
        GetWindowsTheme
    },
    {
        &nsGkAtoms::_moz_os_version,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eIdent,
        { nullptr },
        GetOperatinSystemVersion
    },

    {
        &nsGkAtoms::_moz_swipe_animation_enabled,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::swipe_animation_enabled },
        GetSystemMetric
    },

    {
        &nsGkAtoms::_moz_physical_home_button,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::physical_home_button },
        GetSystemMetric
    },

    
    
    
    {
        &nsGkAtoms::_moz_is_glyph,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { nullptr },
        GetIsGlyph
    },
    
    {
        nullptr,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nullptr },
        nullptr
    },
};








































#include "nsMediaFeatures.h"
#include "nsGkAtoms.h"
#include "nsCSSKeywords.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDeviceContext.h"
#include "nsCSSValue.h"
#include "nsIDocShell.h"
#include "nsLayoutUtils.h"
#include "nsCSSRuleProcessor.h"

static const PRInt32 kOrientationKeywords[] = {
  eCSSKeyword_portrait,                 NS_STYLE_ORIENTATION_PORTRAIT,
  eCSSKeyword_landscape,                NS_STYLE_ORIENTATION_LANDSCAPE,
  eCSSKeyword_UNKNOWN,                  -1
};

static const PRInt32 kScanKeywords[] = {
  eCSSKeyword_progressive,              NS_STYLE_SCAN_PROGRESSIVE,
  eCSSKeyword_interlace,                NS_STYLE_SCAN_INTERLACE,
  eCSSKeyword_UNKNOWN,                  -1
};


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

inline static nsIDeviceContext*
GetDeviceContextFor(nsPresContext* aPresContext)
{
  
  
  
  
  
  return aPresContext->DeviceContext();
}


static nsSize
GetDeviceSize(nsPresContext* aPresContext)
{
    nsSize size;
    if (aPresContext->IsRootPaginatedDocument())
        
        
        
        size = aPresContext->GetPageSize();
    else
        GetDeviceContextFor(aPresContext)->
            GetDeviceSurfaceDimensions(size.width, size.height);
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
    PRInt32 orientation;
    if (size.width > size.height) {
        orientation = NS_STYLE_ORIENTATION_LANDSCAPE;
    } else {
        
        orientation = NS_STYLE_ORIENTATION_PORTRAIT;
    }

    aResult.SetIntValue(orientation, eCSSUnit_Enumerated);
    return NS_OK;
}


static nsresult
MakeArray(const nsSize& aSize, nsCSSValue& aResult)
{
    nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);
    NS_ENSURE_TRUE(a, NS_ERROR_OUT_OF_MEMORY);

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
    
    
    
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    PRUint32 depth;
    dx->GetDepth(depth);
    
    
    
    depth /= 3;
    aResult.SetIntValue(PRInt32(depth), eCSSUnit_Integer);
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
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    float dpi = float(dx->AppUnitsPerInch()) / float(dx->AppUnitsPerDevPixel());
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
GetSystemMetric(nsPresContext* aPresContext, const nsMediaFeature* aFeature,
                nsCSSValue& aResult)
{
    NS_ABORT_IF_FALSE(aFeature->mValueType == nsMediaFeature::eBoolInteger,
                      "unexpected type");
    nsIAtom *metricAtom = *aFeature->mData.mMetric;
    PRBool hasMetric = nsCSSRuleProcessor::HasSystemMetric(metricAtom);
    aResult.SetIntValue(hasMetric ? 1 : 0, eCSSUnit_Integer);
    return NS_OK;
}










 const nsMediaFeature
nsMediaFeatures::features[] = {
    {
        &nsGkAtoms::width,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nsnull },
        GetWidth
    },
    {
        &nsGkAtoms::height,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nsnull },
        GetHeight
    },
    {
        &nsGkAtoms::deviceWidth,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nsnull },
        GetDeviceWidth
    },
    {
        &nsGkAtoms::deviceHeight,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        { nsnull },
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
        { nsnull },
        GetAspectRatio
    },
    {
        &nsGkAtoms::deviceAspectRatio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eIntRatio,
        { nsnull },
        GetDeviceAspectRatio
    },
    {
        &nsGkAtoms::color,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nsnull },
        GetColor
    },
    {
        &nsGkAtoms::colorIndex,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nsnull },
        GetColorIndex
    },
    {
        &nsGkAtoms::monochrome,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nsnull },
        GetMonochrome
    },
    {
        &nsGkAtoms::resolution,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eResolution,
        { nsnull },
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
        { nsnull },
        GetGrid
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
        &nsGkAtoms::_moz_touch_enabled,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::touch_enabled },
        GetSystemMetric
    },
    {
        &nsGkAtoms::_moz_maemo_classic,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        { &nsGkAtoms::maemo_classic },
        GetSystemMetric
    },

    
    {
        nsnull,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        { nsnull },
        nsnull
    },
};

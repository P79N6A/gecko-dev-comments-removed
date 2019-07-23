






































#include "nsMediaFeatures.h"
#include "nsGkAtoms.h"
#include "nsCSSKeywords.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDeviceContext.h"
#include "nsCSSValue.h"
#include "nsIDocShell.h"
#include "nsLayoutUtils.h"

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

static nsresult
GetWidth(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    nscoord width = aPresContext->GetVisibleArea().width;
    float pixelWidth = aPresContext->AppUnitsToFloatCSSPixels(width);
    aResult.SetFloatValue(pixelWidth, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetHeight(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    nscoord height = aPresContext->GetVisibleArea().height;
    float pixelHeight = aPresContext->AppUnitsToFloatCSSPixels(height);
    aResult.SetFloatValue(pixelHeight, eCSSUnit_Pixel);
    return NS_OK;
}

static nsIDeviceContext*
GetDeviceContextFor(nsPresContext* aPresContext)
{
  
  
  
  
  nsIDeviceContext* ctx = nsLayoutUtils::GetDeviceContextForScreenInfo(
    nsCOMPtr<nsIDocShell>(do_QueryInterface(
      nsCOMPtr<nsISupports>(aPresContext->GetContainer()))));
  if (!ctx) {
    ctx = aPresContext->DeviceContext();
  }
  return ctx;
}

static nsresult
GetDeviceWidth(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    nscoord width, height;
    dx->GetDeviceSurfaceDimensions(width, height);
    float pixelWidth = aPresContext->AppUnitsToFloatCSSPixels(width);
    aResult.SetFloatValue(pixelWidth, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetDeviceHeight(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    nscoord width, height;
    dx->GetDeviceSurfaceDimensions(width, height);
    float pixelHeight = aPresContext->AppUnitsToFloatCSSPixels(height);
    aResult.SetFloatValue(pixelHeight, eCSSUnit_Pixel);
    return NS_OK;
}

static nsresult
GetOrientation(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    nsSize size = aPresContext->GetVisibleArea().Size();
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
GetAspectRatio(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);
    NS_ENSURE_TRUE(a, NS_ERROR_OUT_OF_MEMORY);

    nsSize size = aPresContext->GetVisibleArea().Size();
    a->Item(0).SetIntValue(size.width, eCSSUnit_Integer);
    a->Item(1).SetIntValue(size.height, eCSSUnit_Integer);

    aResult.SetArrayValue(a, eCSSUnit_Array);
    return NS_OK;
}

static nsresult
GetDeviceAspectRatio(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);
    NS_ENSURE_TRUE(a, NS_ERROR_OUT_OF_MEMORY);

    
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    nscoord width, height;
    dx->GetDeviceSurfaceDimensions(width, height);
    a->Item(0).SetIntValue(width, eCSSUnit_Integer);
    a->Item(1).SetIntValue(height, eCSSUnit_Integer);

    aResult.SetArrayValue(a, eCSSUnit_Array);
    return NS_OK;
}


static nsresult
GetColor(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    PRUint32 depth;
    dx->GetDepth(depth);
    
    
    if (depth == 32) {
        depth = 24;
    }
    
    
    
    depth /= 3;
    aResult.SetIntValue(PRInt32(depth), eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetColorIndex(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    
    
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetMonochrome(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}

static nsresult
GetResolution(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    nsIDeviceContext *dx = GetDeviceContextFor(aPresContext);
    float dpi = float(dx->AppUnitsPerInch()) / float(dx->AppUnitsPerDevPixel());
    aResult.SetFloatValue(dpi, eCSSUnit_Inch);
    return NS_OK;
}

static nsresult
GetScan(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    aResult.Reset();
    return NS_OK;
}

static nsresult
GetGrid(nsPresContext* aPresContext, nsCSSValue& aResult)
{
    
    
    aResult.SetIntValue(0, eCSSUnit_Integer);
    return NS_OK;
}










 const nsMediaFeature
nsMediaFeatures::features[] = {
    {
        &nsGkAtoms::width,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        nsnull,
        GetWidth
    },
    {
        &nsGkAtoms::height,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        nsnull,
        GetHeight
    },
    {
        &nsGkAtoms::deviceWidth,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        nsnull,
        GetDeviceWidth
    },
    {
        &nsGkAtoms::deviceHeight,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eLength,
        nsnull,
        GetDeviceHeight
    },
    {
        &nsGkAtoms::orientation,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eEnumerated,
        kOrientationKeywords,
        GetOrientation
    },
    {
        &nsGkAtoms::aspectRatio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eIntRatio,
        nsnull,
        GetAspectRatio
    },
    {
        &nsGkAtoms::deviceAspectRatio,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eIntRatio,
        nsnull,
        GetDeviceAspectRatio
    },
    {
        &nsGkAtoms::color,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        nsnull,
        GetColor
    },
    {
        &nsGkAtoms::colorIndex,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        nsnull,
        GetColorIndex
    },
    {
        &nsGkAtoms::monochrome,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        nsnull,
        GetMonochrome
    },
    {
        &nsGkAtoms::resolution,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eResolution,
        nsnull,
        GetResolution
    },
    {
        &nsGkAtoms::scan,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eEnumerated,
        kScanKeywords,
        GetScan
    },
    {
        &nsGkAtoms::grid,
        nsMediaFeature::eMinMaxNotAllowed,
        nsMediaFeature::eBoolInteger,
        nsnull,
        GetGrid
    },
    
    {
        nsnull,
        nsMediaFeature::eMinMaxAllowed,
        nsMediaFeature::eInteger,
        nsnull,
        nsnull
    },
};

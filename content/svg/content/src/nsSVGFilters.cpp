




#include "mozilla/Util.h"

#include "nsSVGElement.h"
#include "nsGkAtoms.h"
#include "nsSVGNumber2.h"
#include "nsSVGNumberPair.h"
#include "nsSVGInteger.h"
#include "nsSVGIntegerPair.h"
#include "nsSVGBoolean.h"
#include "nsIDOMSVGFilters.h"
#include "nsCOMPtr.h"
#include "nsSVGFilterInstance.h"
#include "nsSVGEnum.h"
#include "SVGNumberList.h"
#include "SVGAnimatedNumberList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "nsSVGFilters.h"
#include "nsLayoutUtils.h"
#include "nsSVGUtils.h"
#include "nsStyleContext.h"
#include "nsIFrame.h"
#include "gfxContext.h"
#include "gfxMatrix.h"
#include "imgIContainer.h"
#include "nsNetUtil.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGString.h"
#include "gfxUtils.h"
#include "SVGContentUtils.h"
#include <algorithm>
#include "nsContentUtils.h"
#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGComponentTransferFunctionElement.h"
#include "mozilla/dom/SVGFEDistantLightElement.h"
#include "mozilla/dom/SVGFEFuncAElementBinding.h"
#include "mozilla/dom/SVGFEFuncBElementBinding.h"
#include "mozilla/dom/SVGFEFuncGElementBinding.h"
#include "mozilla/dom/SVGFEFuncRElementBinding.h"
#include "mozilla/dom/SVGFEPointLightElement.h"

#if defined(XP_WIN) 

#undef LoadImage
#endif

#define NUM_ENTRIES_IN_4x5_MATRIX 20

using namespace mozilla;
using namespace mozilla::dom;

void
CopyDataRect(uint8_t *aDest, const uint8_t *aSrc, uint32_t aStride,
             const nsIntRect& aDataRect)
{
  for (int32_t y = aDataRect.y; y < aDataRect.YMost(); y++) {
    memcpy(aDest + y * aStride + 4 * aDataRect.x,
           aSrc + y * aStride + 4 * aDataRect.x,
           4 * aDataRect.width);
  }
}

static void
CopyAndScaleDeviceOffset(const gfxImageSurface *aImage, gfxImageSurface *aResult,
                         gfxFloat kernelX, gfxFloat kernelY)
{
  gfxPoint deviceOffset = aImage->GetDeviceOffset();
  deviceOffset.x /= kernelX;
  deviceOffset.y /= kernelY;
  aResult->SetDeviceOffset(deviceOffset);
}



nsSVGElement::LengthInfo nsSVGFE::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::width, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::height, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFE,nsSVGFEBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFE,nsSVGFEBase)

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGFE, NS_SVG_FE_CID)

NS_INTERFACE_MAP_BEGIN(nsSVGFE)
   
   
   if ( aIID.Equals(NS_GET_IID(nsSVGFE)) )
     foundInterface = static_cast<nsISupports*>(static_cast<void*>(this));
   else
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEBase)




nsSVGFE::ScaleInfo
nsSVGFE::SetupScalingFilter(nsSVGFilterInstance *aInstance,
                            const Image *aSource, const Image *aTarget,
                            const nsIntRect& aDataRect,
                            nsSVGNumberPair *aKernelUnitLength)
{
  ScaleInfo result;
  result.mRescaling = aKernelUnitLength->IsExplicitlySet();
  if (!result.mRescaling) {
    result.mSource = aSource->mImage;
    result.mTarget = aTarget->mImage;
    result.mDataRect = aDataRect;
    return result;
  }

  gfxFloat kernelX = aInstance->GetPrimitiveNumber(SVGContentUtils::X,
                                                   aKernelUnitLength,
                                                   nsSVGNumberPair::eFirst);
  gfxFloat kernelY = aInstance->GetPrimitiveNumber(SVGContentUtils::Y,
                                                   aKernelUnitLength,
                                                   nsSVGNumberPair::eSecond);
  if (kernelX <= 0 || kernelY <= 0)
    return result;

  bool overflow = false;
  gfxIntSize scaledSize =
    nsSVGUtils::ConvertToSurfaceSize(gfxSize(aTarget->mImage->Width() / kernelX,
                                             aTarget->mImage->Height() / kernelY),
                                     &overflow);
  
  
  
  
  if (overflow || scaledSize.width <= 0 || scaledSize.height <= 0)
    return result;

  gfxRect r(aDataRect.x, aDataRect.y, aDataRect.width, aDataRect.height);
  r.Scale(1 / kernelX, 1 / kernelY);
  r.RoundOut();
  if (!gfxUtils::GfxRectToIntRect(r, &result.mDataRect))
    return result;

  
  
  
  result.mDataRect.IntersectRect(result.mDataRect,
                                 nsIntRect(nsIntPoint(), scaledSize));

  result.mSource = new gfxImageSurface(scaledSize,
                                       gfxASurface::ImageFormatARGB32);
  result.mTarget = new gfxImageSurface(scaledSize,
                                       gfxASurface::ImageFormatARGB32);
  if (!result.mSource || result.mSource->CairoStatus() ||
      !result.mTarget || result.mTarget->CairoStatus()) {
    result.mSource = nullptr;
    result.mTarget = nullptr;
    return result;
  }

  CopyAndScaleDeviceOffset(aSource->mImage, result.mSource, kernelX, kernelY);
  CopyAndScaleDeviceOffset(aTarget->mImage, result.mTarget, kernelX, kernelY);

  result.mRealTarget = aTarget->mImage;

  gfxContext ctx(result.mSource);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.Scale(double(scaledSize.width) / aTarget->mImage->Width(),
            double(scaledSize.height) / aTarget->mImage->Height());
  ctx.SetSource(aSource->mImage);
  ctx.Paint();

  

  return result;
}

void
nsSVGFE::FinishScalingFilter(ScaleInfo *aScaleInfo)
{
  if (!aScaleInfo->mRescaling)
    return;

  gfxIntSize scaledSize = aScaleInfo->mTarget->GetSize();

  gfxContext ctx(aScaleInfo->mRealTarget);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.Scale(double(aScaleInfo->mRealTarget->Width()) / scaledSize.width,
            double(aScaleInfo->mRealTarget->Height()) / scaledSize.height);
  ctx.SetSource(aScaleInfo->mTarget);
  ctx.Paint();
}

nsIntRect
nsSVGFE::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
                           const nsSVGFilterInstance& aInstance)
{
  nsIntRect r;
  for (uint32_t i = 0; i < aSourceBBoxes.Length(); ++i) {
    r.UnionRect(r, aSourceBBoxes[i]);
  }
  return r;
}

void
nsSVGFE::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
                                   nsTArray<nsIntRect>& aSourceBBoxes,
                                   const nsSVGFilterInstance& aInstance)
{
  for (uint32_t i = 0; i < aSourceBBoxes.Length(); ++i) {
    aSourceBBoxes[i] = aTargetBBox;
  }
}

nsIntRect
nsSVGFE::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                           const nsSVGFilterInstance& aInstance)
{
  nsIntRect r;
  for (uint32_t i = 0; i < aSourceChangeBoxes.Length(); ++i) {
    r.UnionRect(r, aSourceChangeBoxes[i]);
  }
  return r;
}

void
nsSVGFE::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
}

bool
nsSVGFE::AttributeAffectsRendering(int32_t aNameSpaceID,
                                   nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::x ||
          aAttribute == nsGkAtoms::y ||
          aAttribute == nsGkAtoms::width ||
          aAttribute == nsGkAtoms::height ||
          aAttribute == nsGkAtoms::result);
}





NS_IMETHODIMP nsSVGFE::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  *aX = X().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
nsSVGFE::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP nsSVGFE::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  *aY = Y().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
nsSVGFE::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP nsSVGFE::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = Width().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
nsSVGFE::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP nsSVGFE::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = Height().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
nsSVGFE::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP nsSVGFE::GetResult(nsIDOMSVGAnimatedString * *aResult)
{
  *aResult = Result().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedString>
nsSVGFE::Result()
{
  return GetResultImageName().ToDOMAnimatedString(this);
}




NS_IMETHODIMP_(bool)
nsSVGFE::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFiltersMap
  };
  
  return FindAttributeDependence(name, map) ||
    nsSVGFEBase::IsAttributeMapped(name);
}




 bool
nsSVGFE::HasValidDimensions() const
{
  return (!mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() ||
           mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() ||
           mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
nsSVGFE::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}



typedef nsSVGFE nsSVGFEGaussianBlurElementBase;

class nsSVGFEGaussianBlurElement : public nsSVGFEGaussianBlurElementBase,
                                   public nsIDOMSVGFEGaussianBlurElement
{
  friend nsresult NS_NewSVGFEGaussianBlurElement(nsIContent **aResult,
                                                 already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEGaussianBlurElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEGaussianBlurElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEGaussianBlurElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo >& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFEGAUSSIANBLURELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEGaussianBlurElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { STD_DEV };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

private:
  nsresult GetDXY(uint32_t *aDX, uint32_t *aDY, const nsSVGFilterInstance& aInstance);
  nsIntRect InflateRectForBlur(const nsIntRect& aRect, const nsSVGFilterInstance& aInstance);

  void GaussianBlur(const Image *aSource, const Image *aTarget,
                    const nsIntRect& aDataRect,
                    uint32_t aDX, uint32_t aDY);
};

nsSVGElement::NumberPairInfo nsSVGFEGaussianBlurElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::stdDeviation, 0, 0 }
};

nsSVGElement::StringInfo nsSVGFEGaussianBlurElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEGaussianBlur)




NS_IMPL_ADDREF_INHERITED(nsSVGFEGaussianBlurElement,nsSVGFEGaussianBlurElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEGaussianBlurElement,nsSVGFEGaussianBlurElementBase)

DOMCI_NODE_DATA(SVGFEGaussianBlurElement, nsSVGFEGaussianBlurElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEGaussianBlurElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEGaussianBlurElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEGaussianBlurElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEGaussianBlurElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEGaussianBlurElementBase)





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEGaussianBlurElement)






NS_IMETHODIMP nsSVGFEGaussianBlurElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEGaussianBlurElement::GetStdDeviationX(nsIDOMSVGAnimatedNumber * *aX)
{
  return mNumberPairAttributes[STD_DEV].ToDOMAnimatedNumber(aX, nsSVGNumberPair::eFirst, this);
}


NS_IMETHODIMP nsSVGFEGaussianBlurElement::GetStdDeviationY(nsIDOMSVGAnimatedNumber * *aY)
{
  return mNumberPairAttributes[STD_DEV].ToDOMAnimatedNumber(aY, nsSVGNumberPair::eSecond, this);
}

NS_IMETHODIMP
nsSVGFEGaussianBlurElement::SetStdDeviation(float stdDeviationX, float stdDeviationY)
{
  NS_ENSURE_FINITE2(stdDeviationX, stdDeviationY, NS_ERROR_ILLEGAL_VALUE);
  mNumberPairAttributes[STD_DEV].SetBaseValues(stdDeviationX, stdDeviationY, this);
  return NS_OK;
}



































static uint32_t ComputeScaledDivisor(uint32_t aDivisor)
{
  return UINT32_MAX/(255*aDivisor);
}
  
static void
BoxBlur(const uint8_t *aInput, uint8_t *aOutput,
        int32_t aStrideMinor, int32_t aStartMinor, int32_t aEndMinor,
        int32_t aLeftLobe, int32_t aRightLobe, bool aAlphaOnly)
{
  int32_t boxSize = aLeftLobe + aRightLobe + 1;
  int32_t scaledDivisor = ComputeScaledDivisor(boxSize);
  int32_t sums[4] = {0, 0, 0, 0};

  for (int32_t i=0; i < boxSize; i++) {
    int32_t pos = aStartMinor - aLeftLobe + i;
    pos = std::max(pos, aStartMinor);
    pos = std::min(pos, aEndMinor - 1);
#define SUM(j)     sums[j] += aInput[aStrideMinor*pos + j];
    SUM(0); SUM(1); SUM(2); SUM(3);
#undef SUM
  }

  aOutput += aStrideMinor*aStartMinor;
  if (aStartMinor + int32_t(boxSize) <= aEndMinor) {
    const uint8_t *lastInput = aInput + aStartMinor*aStrideMinor;
    const uint8_t *nextInput = aInput + (aStartMinor + aRightLobe + 1)*aStrideMinor;
#define OUTPUT(j)     aOutput[j] = (sums[j]*scaledDivisor) >> 24;
#define SUM(j)        sums[j] += nextInput[j] - lastInput[j];
    
#define OUTPUT_PIXEL() \
        if (!aAlphaOnly) { OUTPUT(GFX_ARGB32_OFFSET_B); \
                           OUTPUT(GFX_ARGB32_OFFSET_G); \
                           OUTPUT(GFX_ARGB32_OFFSET_R); } \
        OUTPUT(GFX_ARGB32_OFFSET_A);
#define SUM_PIXEL() \
        if (!aAlphaOnly) { SUM(GFX_ARGB32_OFFSET_B); \
                           SUM(GFX_ARGB32_OFFSET_G); \
                           SUM(GFX_ARGB32_OFFSET_R); } \
        SUM(GFX_ARGB32_OFFSET_A);
    for (int32_t minor = aStartMinor;
         minor < aStartMinor + aLeftLobe;
         minor++) {
      OUTPUT_PIXEL();
      SUM_PIXEL();
      nextInput += aStrideMinor;
      aOutput += aStrideMinor;
    }
    for (int32_t minor = aStartMinor + aLeftLobe;
         minor < aEndMinor - aRightLobe - 1;
         minor++) {
      OUTPUT_PIXEL();
      SUM_PIXEL();
      lastInput += aStrideMinor;
      nextInput += aStrideMinor;
      aOutput += aStrideMinor;
    }
    
    
    
    nextInput -= aStrideMinor;
    for (int32_t minor = aEndMinor - aRightLobe - 1; minor < aEndMinor; minor++) {
      OUTPUT_PIXEL();
      SUM_PIXEL();
      lastInput += aStrideMinor;
      aOutput += aStrideMinor;
#undef SUM_PIXEL
#undef SUM
    }
  } else {
    for (int32_t minor = aStartMinor; minor < aEndMinor; minor++) {
      int32_t tmp = minor - aLeftLobe;
      int32_t last = std::max(tmp, aStartMinor);
      int32_t next = std::min(tmp + int32_t(boxSize), aEndMinor - 1);

      OUTPUT_PIXEL();
#define SUM(j)     sums[j] += aInput[aStrideMinor*next + j] - \
                              aInput[aStrideMinor*last + j];
      if (!aAlphaOnly) { SUM(GFX_ARGB32_OFFSET_B);
                         SUM(GFX_ARGB32_OFFSET_G);
                         SUM(GFX_ARGB32_OFFSET_R); }
      SUM(GFX_ARGB32_OFFSET_A);
      aOutput += aStrideMinor;
#undef SUM
#undef OUTPUT_PIXEL
#undef OUTPUT
    }
  }
}

static uint32_t
GetBlurBoxSize(double aStdDev)
{
  NS_ASSERTION(aStdDev >= 0, "Negative standard deviations not allowed");

  double size = aStdDev*3*sqrt(2*M_PI)/4;
  
  uint32_t max = 1024;
  if (size > max)
    return max;
  return uint32_t(floor(size + 0.5));
}

nsresult
nsSVGFEGaussianBlurElement::GetDXY(uint32_t *aDX, uint32_t *aDY,
                                   const nsSVGFilterInstance& aInstance)
{
  float stdX = aInstance.GetPrimitiveNumber(SVGContentUtils::X,
                                            &mNumberPairAttributes[STD_DEV],
                                            nsSVGNumberPair::eFirst);
  float stdY = aInstance.GetPrimitiveNumber(SVGContentUtils::Y,
                                            &mNumberPairAttributes[STD_DEV],
                                            nsSVGNumberPair::eSecond);
  if (stdX < 0 || stdY < 0)
    return NS_ERROR_FAILURE;

  
  
  
  *aDX = GetBlurBoxSize(stdX);
  *aDY = GetBlurBoxSize(stdY);
  return NS_OK;
}

static bool
AreAllColorChannelsZero(const nsSVGFE::Image* aTarget)
{
  return aTarget->mConstantColorChannels &&
         aTarget->mImage->GetDataSize() >= 4 &&
         (*reinterpret_cast<uint32_t*>(aTarget->mImage->Data()) & 0x00FFFFFF) == 0;
}

void
nsSVGFEGaussianBlurElement::GaussianBlur(const Image *aSource,
                                         const Image *aTarget,                                         
                                         const nsIntRect& aDataRect,
                                         uint32_t aDX, uint32_t aDY)
{
  NS_ASSERTION(nsIntRect(0, 0, aTarget->mImage->Width(), aTarget->mImage->Height()).Contains(aDataRect),
               "aDataRect out of bounds");

  nsAutoArrayPtr<uint8_t> tmp(new uint8_t[aTarget->mImage->GetDataSize()]);
  if (!tmp)
    return;
  memset(tmp, 0, aTarget->mImage->GetDataSize());

  bool alphaOnly = AreAllColorChannelsZero(aTarget);
  
  const uint8_t* sourceData = aSource->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  if (aDX == 0) {
    CopyDataRect(tmp, sourceData, stride, aDataRect);
  } else {
    int32_t longLobe = aDX/2;
    int32_t shortLobe = (aDX & 1) ? longLobe : longLobe - 1;
    for (int32_t major = aDataRect.y; major < aDataRect.YMost(); ++major) {
      int32_t ms = major*stride;
      BoxBlur(sourceData + ms, tmp + ms, 4, aDataRect.x, aDataRect.XMost(), longLobe, shortLobe, alphaOnly);
      BoxBlur(tmp + ms, targetData + ms, 4, aDataRect.x, aDataRect.XMost(), shortLobe, longLobe, alphaOnly);
      BoxBlur(targetData + ms, tmp + ms, 4, aDataRect.x, aDataRect.XMost(), longLobe, longLobe, alphaOnly);
    }
  }

  if (aDY == 0) {
    CopyDataRect(targetData, tmp, stride, aDataRect);
  } else {
    int32_t longLobe = aDY/2;
    int32_t shortLobe = (aDY & 1) ? longLobe : longLobe - 1;
    for (int32_t major = aDataRect.x; major < aDataRect.XMost(); ++major) {
      int32_t ms = major*4;
      BoxBlur(tmp + ms, targetData + ms, stride, aDataRect.y, aDataRect.YMost(), longLobe, shortLobe, alphaOnly);
      BoxBlur(targetData + ms, tmp + ms, stride, aDataRect.y, aDataRect.YMost(), shortLobe, longLobe, alphaOnly);
      BoxBlur(tmp + ms, targetData + ms, stride, aDataRect.y, aDataRect.YMost(), longLobe, longLobe, alphaOnly);
    }
  }
}

static void
InflateRectForBlurDXY(nsIntRect* aRect, uint32_t aDX, uint32_t aDY)
{
  aRect->Inflate(3*(aDX/2), 3*(aDY/2));
}

static void
ClearRect(gfxImageSurface* aSurface, int32_t aX, int32_t aY,
          int32_t aXMost, int32_t aYMost)
{
  NS_ASSERTION(aX <= aXMost && aY <= aYMost, "Invalid rectangle");
  NS_ASSERTION(aX >= 0 && aY >= 0 && aXMost <= aSurface->Width() && aYMost <= aSurface->Height(),
               "Rectangle out of bounds");

  if (aX == aXMost || aY == aYMost)
    return;
  for (int32_t y = aY; y < aYMost; ++y) {
    memset(aSurface->Data() + aSurface->Stride()*y + aX*4, 0, (aXMost - aX)*4);
  }
}




static void
ClipTarget(nsSVGFilterInstance* aInstance, const nsSVGFE::Image* aTarget,
           const nsIntRect& aModifiedRect)
{
  nsIntPoint surfaceTopLeft = aInstance->GetSurfaceRect().TopLeft();

  NS_ASSERTION(aInstance->GetSurfaceRect().Contains(aModifiedRect + surfaceTopLeft),
               "Modified data area overflows the surface?");

  nsIntRect clip = aModifiedRect;
  nsSVGUtils::ClipToGfxRect(&clip,
    aTarget->mFilterPrimitiveSubregion - gfxPoint(surfaceTopLeft.x, surfaceTopLeft.y));

  ClearRect(aTarget->mImage, aModifiedRect.x, aModifiedRect.y, aModifiedRect.XMost(), clip.y);
  ClearRect(aTarget->mImage, aModifiedRect.x, clip.y, clip.x, clip.YMost());
  ClearRect(aTarget->mImage, clip.XMost(), clip.y, aModifiedRect.XMost(), clip.YMost());
  ClearRect(aTarget->mImage, aModifiedRect.x, clip.YMost(), aModifiedRect.XMost(), aModifiedRect.YMost());
}

static void
ClipComputationRectToSurface(nsSVGFilterInstance* aInstance,
                             nsIntRect* aDataRect)
{
  aDataRect->IntersectRect(*aDataRect,
          nsIntRect(nsIntPoint(0, 0), aInstance->GetSurfaceRect().Size()));
}

nsresult
nsSVGFEGaussianBlurElement::Filter(nsSVGFilterInstance* aInstance,
                                   const nsTArray<const Image*>& aSources,
                                   const Image* aTarget,
                                   const nsIntRect& rect)
{
  uint32_t dx, dy;
  nsresult rv = GetDXY(&dx, &dy, *aInstance);
  if (NS_FAILED(rv))
    return rv;

  nsIntRect computationRect = rect;
  InflateRectForBlurDXY(&computationRect, dx, dy);
  ClipComputationRectToSurface(aInstance, &computationRect);
  GaussianBlur(aSources[0], aTarget, computationRect, dx, dy);
  ClipTarget(aInstance, aTarget, computationRect);
  return NS_OK;
}

bool
nsSVGFEGaussianBlurElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                      nsIAtom* aAttribute) const
{
  return nsSVGFEGaussianBlurElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::stdDeviation));
}

void
nsSVGFEGaussianBlurElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
nsSVGFEGaussianBlurElement::InflateRectForBlur(const nsIntRect& aRect,
                                               const nsSVGFilterInstance& aInstance)
{
  uint32_t dX, dY;
  nsresult rv = GetDXY(&dX, &dY, aInstance);
  nsIntRect result = aRect;
  if (NS_SUCCEEDED(rv)) {
    InflateRectForBlurDXY(&result, dX, dY);
  }
  return result;
}

nsIntRect
nsSVGFEGaussianBlurElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return InflateRectForBlur(aSourceBBoxes[0], aInstance);
}

void
nsSVGFEGaussianBlurElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  aSourceBBoxes[0] = InflateRectForBlur(aTargetBBox, aInstance);
}

nsIntRect
nsSVGFEGaussianBlurElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                              const nsSVGFilterInstance& aInstance)
{
  return InflateRectForBlur(aSourceChangeBoxes[0], aInstance);
}




nsSVGElement::NumberPairAttributesInfo
nsSVGFEGaussianBlurElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                  ArrayLength(sNumberPairInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEGaussianBlurElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFE nsSVGFEColorMatrixElementBase;

class nsSVGFEColorMatrixElement : public nsSVGFEColorMatrixElementBase,
                                  public nsIDOMSVGFEColorMatrixElement
{
  friend nsresult NS_NewSVGFEColorMatrixElement(nsIContent **aResult,
                                                already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEColorMatrixElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEColorMatrixElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEColorMatrixElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  
  NS_DECL_NSIDOMSVGFECOLORMATRIXELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEColorMatrixElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) { return false; }

  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();
  virtual NumberListAttributesInfo GetNumberListInfo();

  enum { TYPE };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sTypeMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  enum { VALUES };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};

nsSVGEnumMapping nsSVGFEColorMatrixElement::sTypeMap[] = {
  {&nsGkAtoms::matrix, nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_MATRIX},
  {&nsGkAtoms::saturate, nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_SATURATE},
  {&nsGkAtoms::hueRotate, nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_HUE_ROTATE},
  {&nsGkAtoms::luminanceToAlpha, nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_LUMINANCE_TO_ALPHA},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFEColorMatrixElement::sEnumInfo[1] =
{
  { &nsGkAtoms::type,
    sTypeMap,
    nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_MATRIX
  }
};

nsSVGElement::StringInfo nsSVGFEColorMatrixElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

nsSVGElement::NumberListInfo nsSVGFEColorMatrixElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::values }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEColorMatrix)




NS_IMPL_ADDREF_INHERITED(nsSVGFEColorMatrixElement,nsSVGFEColorMatrixElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEColorMatrixElement,nsSVGFEColorMatrixElementBase)

DOMCI_NODE_DATA(SVGFEColorMatrixElement, nsSVGFEColorMatrixElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEColorMatrixElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEColorMatrixElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEColorMatrixElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEColorMatrixElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEColorMatrixElementBase)






NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEColorMatrixElement)






NS_IMETHODIMP nsSVGFEColorMatrixElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEColorMatrixElement::GetType(nsIDOMSVGAnimatedEnumeration * *aType)
{
  return mEnumAttributes[TYPE].ToDOMAnimatedEnum(aType, this);
}


NS_IMETHODIMP nsSVGFEColorMatrixElement::GetValues(nsISupports * *aValues)
{
  *aValues = DOMSVGAnimatedNumberList::GetDOMWrapper(&mNumberListAttributes[VALUES],
                                                     this, VALUES).get();
  return NS_OK;
}

void
nsSVGFEColorMatrixElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsresult
nsSVGFEColorMatrixElement::Filter(nsSVGFilterInstance *instance,
                                  const nsTArray<const Image*>& aSources,
                                  const Image* aTarget,
                                  const nsIntRect& rect)
{
  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  uint16_t type = mEnumAttributes[TYPE].GetAnimValue();
  const SVGNumberList &values = mNumberListAttributes[VALUES].GetAnimValue();

  if (!mNumberListAttributes[VALUES].IsExplicitlySet() &&
      (type == nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_MATRIX ||
       type == nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_SATURATE ||
       type == nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_HUE_ROTATE)) {
    
    CopyRect(aTarget, aSources[0], rect);
    return NS_OK;
  }

  static const float identityMatrix[] = 
    { 1, 0, 0, 0, 0,
      0, 1, 0, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 0, 1, 0 };

  static const float luminanceToAlphaMatrix[] = 
    { 0,       0,       0,       0, 0,
      0,       0,       0,       0, 0,
      0,       0,       0,       0, 0,
      0.2125f, 0.7154f, 0.0721f, 0, 0 };

  float colorMatrix[NUM_ENTRIES_IN_4x5_MATRIX];
  float s, c;

  switch (type) {
  case nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_MATRIX:

    if (values.Length() != NUM_ENTRIES_IN_4x5_MATRIX)
      return NS_ERROR_FAILURE;

    for(uint32_t j = 0; j < values.Length(); j++) {
      colorMatrix[j] = values[j];
    }
    break;
  case nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_SATURATE:

    if (values.Length() != 1)
      return NS_ERROR_FAILURE;

    s = values[0];

    if (s > 1 || s < 0)
      return NS_ERROR_FAILURE;

    memcpy(colorMatrix, identityMatrix, sizeof(colorMatrix));

    colorMatrix[0] = 0.213f + 0.787f * s;
    colorMatrix[1] = 0.715f - 0.715f * s;
    colorMatrix[2] = 0.072f - 0.072f * s;

    colorMatrix[5] = 0.213f - 0.213f * s;
    colorMatrix[6] = 0.715f + 0.285f * s;
    colorMatrix[7] = 0.072f - 0.072f * s;

    colorMatrix[10] = 0.213f - 0.213f * s;
    colorMatrix[11] = 0.715f - 0.715f * s;
    colorMatrix[12] = 0.072f + 0.928f * s;

    break;

  case nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_HUE_ROTATE:
  {
    memcpy(colorMatrix, identityMatrix, sizeof(colorMatrix));

    if (values.Length() != 1)
      return NS_ERROR_FAILURE;

    float hueRotateValue = values[0];

    c = static_cast<float>(cos(hueRotateValue * M_PI / 180));
    s = static_cast<float>(sin(hueRotateValue * M_PI / 180));

    memcpy(colorMatrix, identityMatrix, sizeof(colorMatrix));

    colorMatrix[0] = 0.213f + 0.787f * c - 0.213f * s;
    colorMatrix[1] = 0.715f - 0.715f * c - 0.715f * s;
    colorMatrix[2] = 0.072f - 0.072f * c + 0.928f * s;

    colorMatrix[5] = 0.213f - 0.213f * c + 0.143f * s;
    colorMatrix[6] = 0.715f + 0.285f * c + 0.140f * s;
    colorMatrix[7] = 0.072f - 0.072f * c - 0.283f * s;

    colorMatrix[10] = 0.213f - 0.213f * c - 0.787f * s;
    colorMatrix[11] = 0.715f - 0.715f * c + 0.715f * s;
    colorMatrix[12] = 0.072f + 0.928f * c + 0.072f * s;

    break;
  }

  case nsSVGFEColorMatrixElement::SVG_FECOLORMATRIX_TYPE_LUMINANCE_TO_ALPHA:

    memcpy(colorMatrix, luminanceToAlphaMatrix, sizeof(colorMatrix));
    break;

  default:
    return NS_ERROR_FAILURE;
  }

  for (int32_t x = rect.x; x < rect.XMost(); x++) {
    for (int32_t y = rect.y; y < rect.YMost(); y++) {
      uint32_t targIndex = y * stride + 4 * x;

      float col[4];
      for (int i = 0, row = 0; i < 4; i++, row += 5) {
        col[i] =
          sourceData[targIndex + GFX_ARGB32_OFFSET_R] * colorMatrix[row + 0] +
          sourceData[targIndex + GFX_ARGB32_OFFSET_G] * colorMatrix[row + 1] +
          sourceData[targIndex + GFX_ARGB32_OFFSET_B] * colorMatrix[row + 2] +
          sourceData[targIndex + GFX_ARGB32_OFFSET_A] * colorMatrix[row + 3] +
          255 *                                         colorMatrix[row + 4];
        col[i] = clamped(col[i], 0.f, 255.f);
      }
      targetData[targIndex + GFX_ARGB32_OFFSET_R] =
        static_cast<uint8_t>(col[0]);
      targetData[targIndex + GFX_ARGB32_OFFSET_G] =
        static_cast<uint8_t>(col[1]);
      targetData[targIndex + GFX_ARGB32_OFFSET_B] =
        static_cast<uint8_t>(col[2]);
      targetData[targIndex + GFX_ARGB32_OFFSET_A] =
        static_cast<uint8_t>(col[3]);
    }
  }
  return NS_OK;
}

bool
nsSVGFEColorMatrixElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                     nsIAtom* aAttribute) const
{
  return nsSVGFEColorMatrixElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::type ||
           aAttribute == nsGkAtoms::values));
}




nsSVGElement::EnumAttributesInfo
nsSVGFEColorMatrixElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEColorMatrixElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

nsSVGElement::NumberListAttributesInfo
nsSVGFEColorMatrixElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}



typedef nsSVGFE nsSVGFECompositeElementBase;

class nsSVGFECompositeElement : public nsSVGFECompositeElementBase,
                                public nsIDOMSVGFECompositeElement
{
  friend nsresult NS_NewSVGFECompositeElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFECompositeElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFECompositeElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFECompositeElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFECOMPOSITEELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFECompositeElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { K1, K2, K3, K4 };
  nsSVGNumber2 mNumberAttributes[4];
  static NumberInfo sNumberInfo[4];

  enum { OPERATOR };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sOperatorMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1, IN2 };
  nsSVGString mStringAttributes[3];
  static StringInfo sStringInfo[3];
};

nsSVGElement::NumberInfo nsSVGFECompositeElement::sNumberInfo[4] =
{
  { &nsGkAtoms::k1, 0, false },
  { &nsGkAtoms::k2, 0, false },
  { &nsGkAtoms::k3, 0, false },
  { &nsGkAtoms::k4, 0, false }
};

nsSVGEnumMapping nsSVGFECompositeElement::sOperatorMap[] = {
  {&nsGkAtoms::over, nsSVGFECompositeElement::SVG_OPERATOR_OVER},
  {&nsGkAtoms::in, nsSVGFECompositeElement::SVG_OPERATOR_IN},
  {&nsGkAtoms::out, nsSVGFECompositeElement::SVG_OPERATOR_OUT},
  {&nsGkAtoms::atop, nsSVGFECompositeElement::SVG_OPERATOR_ATOP},
  {&nsGkAtoms::xor_, nsSVGFECompositeElement::SVG_OPERATOR_XOR},
  {&nsGkAtoms::arithmetic, nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFECompositeElement::sEnumInfo[1] =
{
  { &nsGkAtoms::_operator,
    sOperatorMap,
    nsIDOMSVGFECompositeElement::SVG_OPERATOR_OVER
  }
};

nsSVGElement::StringInfo nsSVGFECompositeElement::sStringInfo[3] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true },
  { &nsGkAtoms::in2, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEComposite)




NS_IMPL_ADDREF_INHERITED(nsSVGFECompositeElement,nsSVGFECompositeElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFECompositeElement,nsSVGFECompositeElementBase)

DOMCI_NODE_DATA(SVGFECompositeElement, nsSVGFECompositeElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFECompositeElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFECompositeElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFECompositeElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFECompositeElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFECompositeElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFECompositeElement)





NS_IMETHODIMP nsSVGFECompositeElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetIn2(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN2].ToDOMAnimatedString(aIn, this);
}



NS_IMETHODIMP nsSVGFECompositeElement::GetOperator(nsIDOMSVGAnimatedEnumeration * *aOperator)
{
  return mEnumAttributes[OPERATOR].ToDOMAnimatedEnum(aOperator, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK1(nsIDOMSVGAnimatedNumber * *aK1)
{
  return mNumberAttributes[K1].ToDOMAnimatedNumber(aK1, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK2(nsIDOMSVGAnimatedNumber * *aK2)
{
  return mNumberAttributes[K2].ToDOMAnimatedNumber(aK2, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK3(nsIDOMSVGAnimatedNumber * *aK3)
{
  return mNumberAttributes[K3].ToDOMAnimatedNumber(aK3, this);
}


NS_IMETHODIMP nsSVGFECompositeElement::GetK4(nsIDOMSVGAnimatedNumber * *aK4)
{
  return mNumberAttributes[K4].ToDOMAnimatedNumber(aK4, this);
}

NS_IMETHODIMP
nsSVGFECompositeElement::SetK(float k1, float k2, float k3, float k4)
{
  NS_ENSURE_FINITE4(k1, k2, k3, k4, NS_ERROR_ILLEGAL_VALUE);
  mNumberAttributes[K1].SetBaseValue(k1, this);
  mNumberAttributes[K2].SetBaseValue(k2, this);
  mNumberAttributes[K3].SetBaseValue(k3, this);
  mNumberAttributes[K4].SetBaseValue(k4, this);
  return NS_OK;
}

nsresult
nsSVGFECompositeElement::Filter(nsSVGFilterInstance *instance,
                                const nsTArray<const Image*>& aSources,
                                const Image* aTarget,
                                const nsIntRect& rect)
{
  uint16_t op = mEnumAttributes[OPERATOR].GetAnimValue();

  
  if (op == nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC) {
    float k1, k2, k3, k4;
    GetAnimatedNumberValues(&k1, &k2, &k3, &k4, nullptr);

    
    CopyRect(aTarget, aSources[0], rect);

    uint8_t* sourceData = aSources[1]->mImage->Data();
    uint8_t* targetData = aTarget->mImage->Data();
    uint32_t stride = aTarget->mImage->Stride();

    
    float k1Scaled = k1 / 255.0f;
    float k4Scaled = k4*255.0f;
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      for (int32_t y = rect.y; y < rect.YMost(); y++) {
        uint32_t targIndex = y * stride + 4 * x;
        for (int32_t i = 0; i < 4; i++) {
          uint8_t i1 = targetData[targIndex + i];
          uint8_t i2 = sourceData[targIndex + i];
          float result = k1Scaled*i1*i2 + k2*i1 + k3*i2 + k4Scaled;
          targetData[targIndex + i] =
                       static_cast<uint8_t>(clamped(result, 0.f, 255.f));
        }
      }
    }
    return NS_OK;
  }

  

  gfxContext ctx(aTarget->mImage);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetSource(aSources[1]->mImage);
  
  ctx.Clip(aTarget->mFilterPrimitiveSubregion);
  ctx.Paint();

  if (op < SVG_OPERATOR_OVER || op > SVG_OPERATOR_XOR) {
    return NS_ERROR_FAILURE;
  }
  static const gfxContext::GraphicsOperator opMap[] = {
                                           gfxContext::OPERATOR_DEST,
                                           gfxContext::OPERATOR_OVER,
                                           gfxContext::OPERATOR_IN,
                                           gfxContext::OPERATOR_OUT,
                                           gfxContext::OPERATOR_ATOP,
                                           gfxContext::OPERATOR_XOR };
  ctx.SetOperator(opMap[op]);
  ctx.SetSource(aSources[0]->mImage);
  ctx.Paint();
  return NS_OK;
}

bool
nsSVGFECompositeElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                   nsIAtom* aAttribute) const
{
  return nsSVGFECompositeElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::in2 ||
           aAttribute == nsGkAtoms::k1 ||
           aAttribute == nsGkAtoms::k2 ||
           aAttribute == nsGkAtoms::k3 ||
           aAttribute == nsGkAtoms::k4 ||
           aAttribute == nsGkAtoms::_operator));
}

void
nsSVGFECompositeElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN2], this));
}

nsIntRect
nsSVGFECompositeElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  uint16_t op = mEnumAttributes[OPERATOR].GetAnimValue();

  if (op == nsSVGFECompositeElement::SVG_OPERATOR_ARITHMETIC) {
    
    
    
    
    
    return GetMaxRect();
  }

  if (op == nsSVGFECompositeElement::SVG_OPERATOR_IN ||
      op == nsSVGFECompositeElement::SVG_OPERATOR_ATOP) {
    
    
    return aSourceBBoxes[1];
  }

  
  
  
  return nsSVGFECompositeElementBase::ComputeTargetBBox(aSourceBBoxes, aInstance);
}




nsSVGElement::NumberAttributesInfo
nsSVGFECompositeElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFECompositeElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFECompositeElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFE nsSVGFEComponentTransferElementBase;

class nsSVGFEComponentTransferElement : public nsSVGFEComponentTransferElementBase,
                                        public nsIDOMSVGFEComponentTransferElement
{
  friend nsresult NS_NewSVGFEComponentTransferElement(nsIContent **aResult,
                                                      already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEComponentTransferElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEComponentTransferElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEComponentTransferElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  
  NS_DECL_NSIDOMSVGFECOMPONENTTRANSFERELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEComponentTransferElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) { return false; }

  virtual StringAttributesInfo GetStringInfo();

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

nsSVGElement::StringInfo nsSVGFEComponentTransferElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEComponentTransfer)




NS_IMPL_ADDREF_INHERITED(nsSVGFEComponentTransferElement,nsSVGFEComponentTransferElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEComponentTransferElement,nsSVGFEComponentTransferElementBase)

DOMCI_NODE_DATA(SVGFEComponentTransferElement, nsSVGFEComponentTransferElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEComponentTransferElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEComponentTransferElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEComponentTransferElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEComponentTransferElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEComponentTransferElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEComponentTransferElement)





NS_IMETHODIMP
nsSVGFEComponentTransferElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}




nsSVGElement::StringAttributesInfo
nsSVGFEComponentTransferElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



nsresult
nsSVGFEComponentTransferElement::Filter(nsSVGFilterInstance *instance,
                                        const nsTArray<const Image*>& aSources,
                                        const Image* aTarget,
                                        const nsIntRect& rect)
{
  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  uint8_t tableR[256], tableG[256], tableB[256], tableA[256];
  for (int i=0; i<256; i++)
    tableR[i] = tableG[i] = tableB[i] = tableA[i] = i;
  uint8_t* tables[] = { tableR, tableG, tableB, tableA };
  for (nsIContent* childContent = nsINode::GetFirstChild();
       childContent;
       childContent = childContent->GetNextSibling()) {

    nsRefPtr<SVGComponentTransferFunctionElement> child;
    CallQueryInterface(childContent,
            (SVGComponentTransferFunctionElement**)getter_AddRefs(child));
    if (child) {
      if (!child->GenerateLookupTable(tables[child->GetChannel()])) {
        return NS_ERROR_FAILURE;
      }
    }
  }

  for (int32_t y = rect.y; y < rect.YMost(); y++) {
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      int32_t targIndex = y * stride + x * 4;
      targetData[targIndex + GFX_ARGB32_OFFSET_B] =
        tableB[sourceData[targIndex + GFX_ARGB32_OFFSET_B]];
      targetData[targIndex + GFX_ARGB32_OFFSET_G] =
        tableG[sourceData[targIndex + GFX_ARGB32_OFFSET_G]];
      targetData[targIndex + GFX_ARGB32_OFFSET_R] =
        tableR[sourceData[targIndex + GFX_ARGB32_OFFSET_R]];
      targetData[targIndex + GFX_ARGB32_OFFSET_A] =
        tableA[sourceData[targIndex + GFX_ARGB32_OFFSET_A]];
    }
  }
  return NS_OK;
}

bool
nsSVGFEComponentTransferElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                           nsIAtom* aAttribute) const
{
  return nsSVGFEComponentTransferElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          aAttribute == nsGkAtoms::in);
}

void
nsSVGFEComponentTransferElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

namespace mozilla {
namespace dom {

nsSVGElement::NumberListInfo SVGComponentTransferFunctionElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::tableValues }
};

nsSVGElement::NumberInfo SVGComponentTransferFunctionElement::sNumberInfo[5] =
{
  { &nsGkAtoms::slope,     1, false },
  { &nsGkAtoms::intercept, 0, false },
  { &nsGkAtoms::amplitude, 1, false },
  { &nsGkAtoms::exponent,  1, false },
  { &nsGkAtoms::offset,    0, false }
};

nsSVGEnumMapping SVGComponentTransferFunctionElement::sTypeMap[] = {
  {&nsGkAtoms::identity,
   nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY},
  {&nsGkAtoms::table,
   nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_TABLE},
  {&nsGkAtoms::discrete,
   nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE},
  {&nsGkAtoms::linear,
   nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_LINEAR},
  {&nsGkAtoms::gamma,
   nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_GAMMA},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGComponentTransferFunctionElement::sEnumInfo[1] =
{
  { &nsGkAtoms::type,
    sTypeMap,
    nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY
  }
};




NS_IMPL_ADDREF_INHERITED(SVGComponentTransferFunctionElement,SVGComponentTransferFunctionElementBase)
NS_IMPL_RELEASE_INHERITED(SVGComponentTransferFunctionElement,SVGComponentTransferFunctionElementBase)

NS_DEFINE_STATIC_IID_ACCESSOR(SVGComponentTransferFunctionElement, NS_SVG_FE_COMPONENT_TRANSFER_FUNCTION_ELEMENT_CID)

NS_INTERFACE_MAP_BEGIN(SVGComponentTransferFunctionElement)
   
   
   if ( aIID.Equals(NS_GET_IID(SVGComponentTransferFunctionElement)) )
     foundInterface = static_cast<nsISupports*>(static_cast<void*>(this));
   else
NS_INTERFACE_MAP_END_INHERITING(SVGComponentTransferFunctionElementBase)





bool
SVGComponentTransferFunctionElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                                 nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::tableValues ||
          aAttribute == nsGkAtoms::slope ||
          aAttribute == nsGkAtoms::intercept ||
          aAttribute == nsGkAtoms::amplitude ||
          aAttribute == nsGkAtoms::exponent ||
          aAttribute == nsGkAtoms::offset ||
          aAttribute == nsGkAtoms::type);
}





already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGComponentTransferFunctionElement::Type()
{
  return mEnumAttributes[TYPE].ToDOMAnimatedEnum(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetType(nsIDOMSVGAnimatedEnumeration * *aType)
{
  *aType = Type().get();
  return NS_OK;
}


already_AddRefed<DOMSVGAnimatedNumberList>
SVGComponentTransferFunctionElement::TableValues()
{
  return DOMSVGAnimatedNumberList::GetDOMWrapper(
    &mNumberListAttributes[TABLEVALUES], this, TABLEVALUES);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetTableValues(nsISupports * *aTableValues)
{
  *aTableValues = TableValues().get();
  return NS_OK;
}


already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGComponentTransferFunctionElement::Slope()
{
  return mNumberAttributes[SLOPE].ToDOMAnimatedNumber(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetSlope(nsIDOMSVGAnimatedNumber * *aSlope)
{
  *aSlope = Slope().get();
  return NS_OK;
}


already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGComponentTransferFunctionElement::Intercept()
{
  return mNumberAttributes[INTERCEPT].ToDOMAnimatedNumber(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetIntercept(nsIDOMSVGAnimatedNumber * *aIntercept)
{
  *aIntercept = Intercept().get();
  return NS_OK;
}


already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGComponentTransferFunctionElement::Amplitude()
{
  return mNumberAttributes[AMPLITUDE].ToDOMAnimatedNumber(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetAmplitude(nsIDOMSVGAnimatedNumber * *aAmplitude)
{
  *aAmplitude = Amplitude().get();
  return NS_OK;
}


already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGComponentTransferFunctionElement::Exponent()
{
  return mNumberAttributes[EXPONENT].ToDOMAnimatedNumber(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetExponent(nsIDOMSVGAnimatedNumber * *aExponent)
{
  *aExponent = Exponent().get();
  return NS_OK;
}


already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGComponentTransferFunctionElement::Offset()
{
  return mNumberAttributes[OFFSET].ToDOMAnimatedNumber(this);
}
NS_IMETHODIMP SVGComponentTransferFunctionElement::GetOffset(nsIDOMSVGAnimatedNumber * *aOffset)
{
  *aOffset = Offset().get();
  return NS_OK;
}

bool
SVGComponentTransferFunctionElement::GenerateLookupTable(uint8_t *aTable)
{
  uint16_t type = mEnumAttributes[TYPE].GetAnimValue();

  float slope, intercept, amplitude, exponent, offset;
  GetAnimatedNumberValues(&slope, &intercept, &amplitude, 
                          &exponent, &offset, nullptr);

  const SVGNumberList &tableValues =
    mNumberListAttributes[TABLEVALUES].GetAnimValue();
  uint32_t tvLength = tableValues.Length();

  uint32_t i;

  switch (type) {
  case nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_TABLE:
  {
    if (tableValues.Length() < 2)
      return false;

    for (i = 0; i < 256; i++) {
      uint32_t k = (i * (tvLength - 1)) / 255;
      float v1 = tableValues[k];
      float v2 = tableValues[std::min(k + 1, tvLength - 1)];
      int32_t val =
        int32_t(255 * (v1 + (i/255.0f - k/float(tvLength-1))*(tvLength - 1)*(v2 - v1)));
      val = std::min(255, val);
      val = std::max(0, val);
      aTable[i] = val;
    }
    break;
  }

  case nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE:
  {
    if (tableValues.Length() < 1)
      return false;

    for (i = 0; i < 256; i++) {
      uint32_t k = (i * tvLength) / 255;
      k = std::min(k, tvLength - 1);
      float v = tableValues[k];
      int32_t val = int32_t(255 * v);
      val = std::min(255, val);
      val = std::max(0, val);
      aTable[i] = val;
    }
    break;
  }

  case nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_LINEAR:
  {
    for (i = 0; i < 256; i++) {
      int32_t val = int32_t(slope * i + 255 * intercept);
      val = std::min(255, val);
      val = std::max(0, val);
      aTable[i] = val;
    }
    break;
  }

  case nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_GAMMA:
  {
    for (i = 0; i < 256; i++) {
      int32_t val = int32_t(255 * (amplitude * pow(i / 255.0f, exponent) + offset));
      val = std::min(255, val);
      val = std::max(0, val);
      aTable[i] = val;
    }
    break;
  }

  case nsIDOMSVGComponentTransferFunctionElement::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY:
  default:
    break;
  }
  return true;
}




nsSVGElement::NumberListAttributesInfo
SVGComponentTransferFunctionElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}

nsSVGElement::EnumAttributesInfo
SVGComponentTransferFunctionElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::NumberAttributesInfo
SVGComponentTransferFunctionElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

NS_IMPL_ISUPPORTS_INHERITED5(SVGFEFuncRElement,
                             SVGComponentTransferFunctionElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIDOMSVGComponentTransferFunctionElement,
                             nsIDOMSVGFEFuncRElement)

 JSObject*
SVGFEFuncRElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGFEFuncRElementBinding::Wrap(aCx, aScope, this);
}

} 
} 

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFuncR)

namespace mozilla {
namespace dom {

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEFuncRElement)

NS_IMPL_ISUPPORTS_INHERITED5(SVGFEFuncGElement,
                             SVGComponentTransferFunctionElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIDOMSVGComponentTransferFunctionElement,
                             nsIDOMSVGFEFuncGElement)

 JSObject*
SVGFEFuncGElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGFEFuncGElementBinding::Wrap(aCx, aScope, this);
}

} 
} 

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFuncG)

namespace mozilla {
namespace dom {

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEFuncGElement)

NS_IMPL_ISUPPORTS_INHERITED5(SVGFEFuncBElement,
                             SVGComponentTransferFunctionElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIDOMSVGComponentTransferFunctionElement,
                             nsIDOMSVGFEFuncBElement)

 JSObject*
SVGFEFuncBElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGFEFuncBElementBinding::Wrap(aCx, aScope, this);
}

} 
} 

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFuncB)

namespace mozilla {
namespace dom {

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEFuncBElement)

NS_IMPL_ISUPPORTS_INHERITED5(SVGFEFuncAElement,
                             SVGComponentTransferFunctionElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIDOMSVGComponentTransferFunctionElement,
                             nsIDOMSVGFEFuncAElement)

 JSObject*
SVGFEFuncAElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGFEFuncAElementBinding::Wrap(aCx, aScope, this);
}

} 
} 

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEFuncA)

namespace mozilla {
namespace dom {

NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEFuncAElement)

} 
} 



typedef nsSVGFE nsSVGFEOffsetElementBase;

class nsSVGFEOffsetElement : public nsSVGFEOffsetElementBase,
                             public nsIDOMSVGFEOffsetElement
{
  friend nsresult NS_NewSVGFEOffsetElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEOffsetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEOffsetElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEOffsetElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFEOFFSETELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEOffsetElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsIntPoint GetOffset(const nsSVGFilterInstance& aInstance);
  
  virtual NumberAttributesInfo GetNumberInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { DX, DY };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

nsSVGElement::NumberInfo nsSVGFEOffsetElement::sNumberInfo[2] =
{
  { &nsGkAtoms::dx, 0, false },
  { &nsGkAtoms::dy, 0, false }
};

nsSVGElement::StringInfo nsSVGFEOffsetElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEOffset)




NS_IMPL_ADDREF_INHERITED(nsSVGFEOffsetElement,nsSVGFEOffsetElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEOffsetElement,nsSVGFEOffsetElementBase)

DOMCI_NODE_DATA(SVGFEOffsetElement, nsSVGFEOffsetElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEOffsetElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEOffsetElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEOffsetElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEOffsetElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEOffsetElementBase)





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEOffsetElement)






NS_IMETHODIMP nsSVGFEOffsetElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEOffsetElement::GetDx(nsIDOMSVGAnimatedNumber * *aDx)
{
  return mNumberAttributes[DX].ToDOMAnimatedNumber(aDx, this);
}


NS_IMETHODIMP nsSVGFEOffsetElement::GetDy(nsIDOMSVGAnimatedNumber * *aDy)
{
  return mNumberAttributes[DY].ToDOMAnimatedNumber(aDy, this);
}

nsIntPoint
nsSVGFEOffsetElement::GetOffset(const nsSVGFilterInstance& aInstance)
{
  return nsIntPoint(int32_t(aInstance.GetPrimitiveNumber(
                              SVGContentUtils::X, &mNumberAttributes[DX])),
                    int32_t(aInstance.GetPrimitiveNumber(
                              SVGContentUtils::Y, &mNumberAttributes[DY])));
}

nsresult
nsSVGFEOffsetElement::Filter(nsSVGFilterInstance *instance,
                             const nsTArray<const Image*>& aSources,
                             const Image* aTarget,
                             const nsIntRect& rect)
{
  nsIntPoint offset = GetOffset(*instance);

  gfxContext ctx(aTarget->mImage);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  
  ctx.Clip(aTarget->mFilterPrimitiveSubregion);
  ctx.Translate(gfxPoint(offset.x, offset.y));
  ctx.SetSource(aSources[0]->mImage);
  ctx.Paint();

  return NS_OK;
}

bool
nsSVGFEOffsetElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                nsIAtom* aAttribute) const
{
  return nsSVGFEOffsetElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::dx ||
           aAttribute == nsGkAtoms::dy));
}

void
nsSVGFEOffsetElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
nsSVGFEOffsetElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return aSourceBBoxes[0] + GetOffset(aInstance);
}

nsIntRect
nsSVGFEOffsetElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                        const nsSVGFilterInstance& aInstance)
{
  return aSourceChangeBoxes[0] + GetOffset(aInstance);
}

void
nsSVGFEOffsetElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  aSourceBBoxes[0] = aTargetBBox - GetOffset(aInstance);
}




nsSVGElement::NumberAttributesInfo
nsSVGFEOffsetElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEOffsetElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFE nsSVGFETurbulenceElementBase;

class nsSVGFETurbulenceElement : public nsSVGFETurbulenceElementBase,
                                 public nsIDOMSVGFETurbulenceElement
{
  friend nsresult NS_NewSVGFETurbulenceElement(nsIContent **aResult,
                                               already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFETurbulenceElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFETurbulenceElementBase(aNodeInfo) {}

public:
  virtual bool SubregionIsUnionOfRegions() { return false; }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFETurbulenceElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFETURBULENCEELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFETurbulenceElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual IntegerAttributesInfo GetIntegerInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { SEED }; 
  nsSVGNumber2 mNumberAttributes[1];
  static NumberInfo sNumberInfo[1];

  enum { BASE_FREQ };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { OCTAVES };
  nsSVGInteger mIntegerAttributes[1];
  static IntegerInfo sIntegerInfo[1];

  enum { TYPE, STITCHTILES };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sTypeMap[];
  static nsSVGEnumMapping sStitchTilesMap[];
  static EnumInfo sEnumInfo[2];

  enum { RESULT };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

private:

  




  






#define RAND_M 2147483647	/* 2**31 - 1 */
#define RAND_A 16807		/* 7**5; primitive root of m */
#define RAND_Q 127773		/* m / a */
#define RAND_R 2836		/* m % a */

  int32_t SetupSeed(int32_t aSeed) {
    if (aSeed <= 0)
      aSeed = -(aSeed % (RAND_M - 1)) + 1;
    if (aSeed > RAND_M - 1)
      aSeed = RAND_M - 1;
    return aSeed;
  }

  uint32_t Random(uint32_t aSeed) {
    int32_t result = RAND_A * (aSeed % RAND_Q) - RAND_R * (aSeed / RAND_Q);
    if (result <= 0)
      result += RAND_M;
    return result;
  }
#undef RAND_M
#undef RAND_A
#undef RAND_Q
#undef RAND_R

  const static int sBSize = 0x100;
  const static int sBM = 0xff;
  const static int sPerlinN = 0x1000;
  const static int sNP = 12;			
  const static int sNM = 0xfff;

  int32_t mLatticeSelector[sBSize + sBSize + 2];
  double mGradient[4][sBSize + sBSize + 2][2];
  struct StitchInfo {
    int mWidth;			
    int mHeight;
    int mWrapX;			
    int mWrapY;
  };

  void InitSeed(int32_t aSeed);
  double Noise2(int aColorChannel, double aVec[2], StitchInfo *aStitchInfo);
  double
  Turbulence(int aColorChannel, double *aPoint, double aBaseFreqX,
             double aBaseFreqY, int aNumOctaves, bool aFractalSum,
             bool aDoStitching, double aTileX, double aTileY,
             double aTileWidth, double aTileHeight);
};

nsSVGElement::NumberInfo nsSVGFETurbulenceElement::sNumberInfo[1] =
{
  { &nsGkAtoms::seed, 0, false }
};

nsSVGElement::NumberPairInfo nsSVGFETurbulenceElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::baseFrequency, 0, 0 }
};

nsSVGElement::IntegerInfo nsSVGFETurbulenceElement::sIntegerInfo[1] =
{
  { &nsGkAtoms::numOctaves, 1 }
};

nsSVGEnumMapping nsSVGFETurbulenceElement::sTypeMap[] = {
  {&nsGkAtoms::fractalNoise,
   nsIDOMSVGFETurbulenceElement::SVG_TURBULENCE_TYPE_FRACTALNOISE},
  {&nsGkAtoms::turbulence,
   nsIDOMSVGFETurbulenceElement::SVG_TURBULENCE_TYPE_TURBULENCE},
  {nullptr, 0}
};

nsSVGEnumMapping nsSVGFETurbulenceElement::sStitchTilesMap[] = {
  {&nsGkAtoms::stitch,
   nsIDOMSVGFETurbulenceElement::SVG_STITCHTYPE_STITCH},
  {&nsGkAtoms::noStitch,
   nsIDOMSVGFETurbulenceElement::SVG_STITCHTYPE_NOSTITCH},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFETurbulenceElement::sEnumInfo[2] =
{
  { &nsGkAtoms::type,
    sTypeMap,
    nsIDOMSVGFETurbulenceElement::SVG_TURBULENCE_TYPE_TURBULENCE
  },
  { &nsGkAtoms::stitchTiles,
    sStitchTilesMap,
    nsIDOMSVGFETurbulenceElement::SVG_STITCHTYPE_NOSTITCH
  }
};

nsSVGElement::StringInfo nsSVGFETurbulenceElement::sStringInfo[1] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FETurbulence)




NS_IMPL_ADDREF_INHERITED(nsSVGFETurbulenceElement,nsSVGFETurbulenceElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFETurbulenceElement,nsSVGFETurbulenceElementBase)

DOMCI_NODE_DATA(SVGFETurbulenceElement, nsSVGFETurbulenceElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFETurbulenceElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFETurbulenceElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFETurbulenceElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFETurbulenceElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFETurbulenceElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFETurbulenceElement)





NS_IMETHODIMP nsSVGFETurbulenceElement::GetBaseFrequencyX(nsIDOMSVGAnimatedNumber * *aX)
{
  return mNumberPairAttributes[BASE_FREQ].ToDOMAnimatedNumber(aX, nsSVGNumberPair::eFirst, this);
}


NS_IMETHODIMP nsSVGFETurbulenceElement::GetBaseFrequencyY(nsIDOMSVGAnimatedNumber * *aY)
{
  return mNumberPairAttributes[BASE_FREQ].ToDOMAnimatedNumber(aY, nsSVGNumberPair::eSecond, this);
}


NS_IMETHODIMP nsSVGFETurbulenceElement::GetNumOctaves(nsIDOMSVGAnimatedInteger * *aNum)
{
  return mIntegerAttributes[OCTAVES].ToDOMAnimatedInteger(aNum, this);
}


NS_IMETHODIMP nsSVGFETurbulenceElement::GetSeed(nsIDOMSVGAnimatedNumber * *aSeed)
{
  return mNumberAttributes[SEED].ToDOMAnimatedNumber(aSeed, this);
}


NS_IMETHODIMP nsSVGFETurbulenceElement::GetStitchTiles(nsIDOMSVGAnimatedEnumeration * *aStitch)
{
  return mEnumAttributes[STITCHTILES].ToDOMAnimatedEnum(aStitch, this);
}


NS_IMETHODIMP nsSVGFETurbulenceElement::GetType(nsIDOMSVGAnimatedEnumeration * *aType)
{
  return mEnumAttributes[TYPE].ToDOMAnimatedEnum(aType, this);
}

nsresult
nsSVGFETurbulenceElement::Filter(nsSVGFilterInstance *instance,
                                 const nsTArray<const Image*>& aSources,
                                 const Image* aTarget,
                                 const nsIntRect& rect)
{
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  nsIntRect filterSubregion(int32_t(aTarget->mFilterPrimitiveSubregion.X()),
                            int32_t(aTarget->mFilterPrimitiveSubregion.Y()),
                            int32_t(aTarget->mFilterPrimitiveSubregion.Width()),
                            int32_t(aTarget->mFilterPrimitiveSubregion.Height()));

  float fX = mNumberPairAttributes[BASE_FREQ].GetAnimValue(nsSVGNumberPair::eFirst);
  float fY = mNumberPairAttributes[BASE_FREQ].GetAnimValue(nsSVGNumberPair::eSecond);
  float seed = mNumberAttributes[OCTAVES].GetAnimValue();
  int32_t octaves = mIntegerAttributes[OCTAVES].GetAnimValue();
  uint16_t type = mEnumAttributes[TYPE].GetAnimValue();
  uint16_t stitch = mEnumAttributes[STITCHTILES].GetAnimValue();

  InitSeed((int32_t)seed);

  
  float filterX = instance->GetFilterRegion().X();
  float filterY = instance->GetFilterRegion().Y();
  float filterWidth = instance->GetFilterRegion().Width();
  float filterHeight = instance->GetFilterRegion().Height();

  bool doStitch = false;
  if (stitch == nsIDOMSVGFETurbulenceElement::SVG_STITCHTYPE_STITCH) {
    doStitch = true;

    float lowFreq, hiFreq;

    lowFreq = floor(filterWidth * fX) / filterWidth;
    hiFreq = ceil(filterWidth * fX) / filterWidth;
    if (fX / lowFreq < hiFreq / fX)
      fX = lowFreq;
    else
      fX = hiFreq;

    lowFreq = floor(filterHeight * fY) / filterHeight;
    hiFreq = ceil(filterHeight * fY) / filterHeight;
    if (fY / lowFreq < hiFreq / fY)
      fY = lowFreq;
    else
      fY = hiFreq;
  }
  for (int32_t y = rect.y; y < rect.YMost(); y++) {
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      int32_t targIndex = y * stride + x * 4;
      double point[2];
      point[0] = filterX + (filterWidth * (x + instance->GetSurfaceRect().x)) / (filterSubregion.width - 1);
      point[1] = filterY + (filterHeight * (y + instance->GetSurfaceRect().y)) / (filterSubregion.height - 1);

      float col[4];
      if (type == nsIDOMSVGFETurbulenceElement::SVG_TURBULENCE_TYPE_TURBULENCE) {
        for (int i = 0; i < 4; i++)
          col[i] = Turbulence(i, point, fX, fY, octaves, false,
                              doStitch, filterX, filterY, filterWidth, filterHeight) * 255;
      } else {
        for (int i = 0; i < 4; i++)
          col[i] = (Turbulence(i, point, fX, fY, octaves, true,
                               doStitch, filterX, filterY, filterWidth, filterHeight) * 255 + 255) / 2;
      }
      for (int i = 0; i < 4; i++) {
        col[i] = std::min(col[i], 255.f);
        col[i] = std::max(col[i], 0.f);
      }

      uint8_t r, g, b, a;
      a = uint8_t(col[3]);
      FAST_DIVIDE_BY_255(r, unsigned(col[0]) * a);
      FAST_DIVIDE_BY_255(g, unsigned(col[1]) * a);
      FAST_DIVIDE_BY_255(b, unsigned(col[2]) * a);

      targetData[targIndex + GFX_ARGB32_OFFSET_B] = b;
      targetData[targIndex + GFX_ARGB32_OFFSET_G] = g;
      targetData[targIndex + GFX_ARGB32_OFFSET_R] = r;
      targetData[targIndex + GFX_ARGB32_OFFSET_A] = a;
    }
  }

  return NS_OK;
}

bool
nsSVGFETurbulenceElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                    nsIAtom* aAttribute) const
{
  return nsSVGFETurbulenceElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::seed ||
           aAttribute == nsGkAtoms::baseFrequency ||
           aAttribute == nsGkAtoms::numOctaves ||
           aAttribute == nsGkAtoms::type ||
           aAttribute == nsGkAtoms::stitchTiles));
}

void
nsSVGFETurbulenceElement::InitSeed(int32_t aSeed)
{
  double s;
  int i, j, k;
  aSeed = SetupSeed(aSeed);
  for (k = 0; k < 4; k++) {
    for (i = 0; i < sBSize; i++) {
      mLatticeSelector[i] = i;
      for (j = 0; j < 2; j++) {
        mGradient[k][i][j] =
          (double) (((aSeed =
                      Random(aSeed)) % (sBSize + sBSize)) - sBSize) / sBSize;
      }
      s = double (sqrt
                  (mGradient[k][i][0] * mGradient[k][i][0] +
                   mGradient[k][i][1] * mGradient[k][i][1]));
      mGradient[k][i][0] /= s;
      mGradient[k][i][1] /= s;
    }
  }
  while (--i) {
    k = mLatticeSelector[i];
    mLatticeSelector[i] = mLatticeSelector[j =
                                           (aSeed =
                                            Random(aSeed)) % sBSize];
    mLatticeSelector[j] = k;
  }
  for (i = 0; i < sBSize + 2; i++) {
    mLatticeSelector[sBSize + i] = mLatticeSelector[i];
    for (k = 0; k < 4; k++)
      for (j = 0; j < 2; j++)
        mGradient[k][sBSize + i][j] = mGradient[k][i][j];
  }
}

#define S_CURVE(t) ( t * t * (3. - 2. * t) )
#define LERP(t, a, b) ( a + t * (b - a) )
double
nsSVGFETurbulenceElement::Noise2(int aColorChannel, double aVec[2],
                                 StitchInfo *aStitchInfo)
{
  int bx0, bx1, by0, by1, b00, b10, b01, b11;
  double rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
  register long i, j;
  t = aVec[0] + sPerlinN;
  bx0 = (int) t;
  bx1 = bx0 + 1;
  rx0 = t - (int) t;
  rx1 = rx0 - 1.0f;
  t = aVec[1] + sPerlinN;
  by0 = (int) t;
  by1 = by0 + 1;
  ry0 = t - (int) t;
  ry1 = ry0 - 1.0f;
  
  if (aStitchInfo != NULL) {
    if (bx0 >= aStitchInfo->mWrapX)
      bx0 -= aStitchInfo->mWidth;
    if (bx1 >= aStitchInfo->mWrapX)
      bx1 -= aStitchInfo->mWidth;
    if (by0 >= aStitchInfo->mWrapY)
      by0 -= aStitchInfo->mHeight;
    if (by1 >= aStitchInfo->mWrapY)
      by1 -= aStitchInfo->mHeight;
  }
  bx0 &= sBM;
  bx1 &= sBM;
  by0 &= sBM;
  by1 &= sBM;
  i = mLatticeSelector[bx0];
  j = mLatticeSelector[bx1];
  b00 = mLatticeSelector[i + by0];
  b10 = mLatticeSelector[j + by0];
  b01 = mLatticeSelector[i + by1];
  b11 = mLatticeSelector[j + by1];
  sx = double (S_CURVE(rx0));
  sy = double (S_CURVE(ry0));
  q = mGradient[aColorChannel][b00];
  u = rx0 * q[0] + ry0 * q[1];
  q = mGradient[aColorChannel][b10];
  v = rx1 * q[0] + ry0 * q[1];
  a = LERP(sx, u, v);
  q = mGradient[aColorChannel][b01];
  u = rx0 * q[0] + ry1 * q[1];
  q = mGradient[aColorChannel][b11];
  v = rx1 * q[0] + ry1 * q[1];
  b = LERP(sx, u, v);
  return LERP(sy, a, b);
}
#undef S_CURVE
#undef LERP

double
nsSVGFETurbulenceElement::Turbulence(int aColorChannel, double *aPoint,
                                     double aBaseFreqX, double aBaseFreqY,
                                     int aNumOctaves, bool aFractalSum,
                                     bool aDoStitching,
                                     double aTileX, double aTileY,
                                     double aTileWidth, double aTileHeight)
{
  StitchInfo stitch;
  StitchInfo *stitchInfo = NULL; 
  
  if (aDoStitching) {
    
    
    if (aBaseFreqX != 0.0) {
      double loFreq = double (floor(aTileWidth * aBaseFreqX)) / aTileWidth;
      double hiFreq = double (ceil(aTileWidth * aBaseFreqX)) / aTileWidth;
      if (aBaseFreqX / loFreq < hiFreq / aBaseFreqX)
        aBaseFreqX = loFreq;
      else
        aBaseFreqX = hiFreq;
    }
    if (aBaseFreqY != 0.0) {
      double loFreq = double (floor(aTileHeight * aBaseFreqY)) / aTileHeight;
      double hiFreq = double (ceil(aTileHeight * aBaseFreqY)) / aTileHeight;
      if (aBaseFreqY / loFreq < hiFreq / aBaseFreqY)
        aBaseFreqY = loFreq;
      else
        aBaseFreqY = hiFreq;
    }
    
    stitchInfo = &stitch;
    stitch.mWidth = int (aTileWidth * aBaseFreqX + 0.5f);
    stitch.mWrapX = int (aTileX * aBaseFreqX + sPerlinN + stitch.mWidth);
    stitch.mHeight = int (aTileHeight * aBaseFreqY + 0.5f);
    stitch.mWrapY = int (aTileY * aBaseFreqY + sPerlinN + stitch.mHeight);
  }
  double sum = 0.0f;
  double vec[2];
  vec[0] = aPoint[0] * aBaseFreqX;
  vec[1] = aPoint[1] * aBaseFreqY;
  double ratio = 1;
  for (int octave = 0; octave < aNumOctaves; octave++) {
    if (aFractalSum)
      sum += double (Noise2(aColorChannel, vec, stitchInfo) / ratio);
    else
      sum += double (fabs(Noise2(aColorChannel, vec, stitchInfo)) / ratio);
    vec[0] *= 2;
    vec[1] *= 2;
    ratio *= 2;
    if (stitchInfo != NULL) {
      
      
      stitch.mWidth *= 2;
      stitch.mWrapX = 2 * stitch.mWrapX - sPerlinN;
      stitch.mHeight *= 2;
      stitch.mWrapY = 2 * stitch.mWrapY - sPerlinN;
    }
  }
  return sum;
}

nsIntRect
nsSVGFETurbulenceElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return GetMaxRect();
}




nsSVGElement::NumberAttributesInfo
nsSVGFETurbulenceElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::NumberPairAttributesInfo
nsSVGFETurbulenceElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                 ArrayLength(sNumberPairInfo));
}

nsSVGElement::IntegerAttributesInfo
nsSVGFETurbulenceElement::GetIntegerInfo()
{
  return IntegerAttributesInfo(mIntegerAttributes, sIntegerInfo,
                               ArrayLength(sIntegerInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFETurbulenceElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFETurbulenceElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFE nsSVGFEMorphologyElementBase;

class nsSVGFEMorphologyElement : public nsSVGFEMorphologyElementBase,
                                 public nsIDOMSVGFEMorphologyElement
{
  friend nsresult NS_NewSVGFEMorphologyElement(nsIContent **aResult,
                                               already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEMorphologyElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEMorphologyElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEMorphologyElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFEMORPHOLOGYELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEMorphologyElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  void GetRXY(int32_t *aRX, int32_t *aRY, const nsSVGFilterInstance& aInstance);
  nsIntRect InflateRect(const nsIntRect& aRect, const nsSVGFilterInstance& aInstance);

  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { RADIUS };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { OPERATOR };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sOperatorMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

nsSVGElement::NumberPairInfo nsSVGFEMorphologyElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::radius, 0, 0 }
};

nsSVGEnumMapping nsSVGFEMorphologyElement::sOperatorMap[] = {
  {&nsGkAtoms::erode, nsSVGFEMorphologyElement::SVG_OPERATOR_ERODE},
  {&nsGkAtoms::dilate, nsSVGFEMorphologyElement::SVG_OPERATOR_DILATE},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFEMorphologyElement::sEnumInfo[1] =
{
  { &nsGkAtoms::_operator,
    sOperatorMap,
    nsSVGFEMorphologyElement::SVG_OPERATOR_ERODE
  }
};

nsSVGElement::StringInfo nsSVGFEMorphologyElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEMorphology)




NS_IMPL_ADDREF_INHERITED(nsSVGFEMorphologyElement,nsSVGFEMorphologyElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEMorphologyElement,nsSVGFEMorphologyElementBase)

DOMCI_NODE_DATA(SVGFEMorphologyElement, nsSVGFEMorphologyElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEMorphologyElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEMorphologyElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEMorphologyElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEMorphologyElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEMorphologyElementBase)





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEMorphologyElement)






NS_IMETHODIMP nsSVGFEMorphologyElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEMorphologyElement::GetOperator(nsIDOMSVGAnimatedEnumeration * *aOperator)
{
  return mEnumAttributes[OPERATOR].ToDOMAnimatedEnum(aOperator, this);
}


NS_IMETHODIMP nsSVGFEMorphologyElement::GetRadiusX(nsIDOMSVGAnimatedNumber * *aX)
{
  return mNumberPairAttributes[RADIUS].ToDOMAnimatedNumber(aX, nsSVGNumberPair::eFirst, this);
}


NS_IMETHODIMP nsSVGFEMorphologyElement::GetRadiusY(nsIDOMSVGAnimatedNumber * *aY)
{
  return mNumberPairAttributes[RADIUS].ToDOMAnimatedNumber(aY, nsSVGNumberPair::eSecond, this);
}

NS_IMETHODIMP
nsSVGFEMorphologyElement::SetRadius(float rx, float ry)
{
  NS_ENSURE_FINITE2(rx, ry, NS_ERROR_ILLEGAL_VALUE);
  mNumberPairAttributes[RADIUS].SetBaseValues(rx, ry, this);
  return NS_OK;
}

void
nsSVGFEMorphologyElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
nsSVGFEMorphologyElement::InflateRect(const nsIntRect& aRect,
                                      const nsSVGFilterInstance& aInstance)
{
  int32_t rx, ry;
  GetRXY(&rx, &ry, aInstance);
  nsIntRect result = aRect;
  result.Inflate(std::max(0, rx), std::max(0, ry));
  return result;
}

nsIntRect
nsSVGFEMorphologyElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  return InflateRect(aSourceBBoxes[0], aInstance);
}

void
nsSVGFEMorphologyElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  aSourceBBoxes[0] = InflateRect(aTargetBBox, aInstance);
}

nsIntRect
nsSVGFEMorphologyElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                            const nsSVGFilterInstance& aInstance)
{
  return InflateRect(aSourceChangeBoxes[0], aInstance);
}

#define MORPHOLOGY_EPSILON 0.0001

void
nsSVGFEMorphologyElement::GetRXY(int32_t *aRX, int32_t *aRY,
                                 const nsSVGFilterInstance& aInstance)
{
  
  
  
  
  *aRX = NSToIntCeil(aInstance.GetPrimitiveNumber(SVGContentUtils::X,
                                                  &mNumberPairAttributes[RADIUS],
                                                  nsSVGNumberPair::eFirst) -
                     MORPHOLOGY_EPSILON);
  *aRY = NSToIntCeil(aInstance.GetPrimitiveNumber(SVGContentUtils::Y,
                                                  &mNumberPairAttributes[RADIUS],
                                                  nsSVGNumberPair::eSecond) -
                     MORPHOLOGY_EPSILON);
}

nsresult
nsSVGFEMorphologyElement::Filter(nsSVGFilterInstance *instance,
                                 const nsTArray<const Image*>& aSources,
                                 const Image* aTarget,
                                 const nsIntRect& rect)
{
  int32_t rx, ry;
  GetRXY(&rx, &ry, *instance);

  if (rx < 0 || ry < 0) {
    
    return NS_OK;
  }
  if (rx == 0 && ry == 0) {
    return NS_OK;
  }

  
  rx = std::min(rx, 100000);
  ry = std::min(ry, 100000);

  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  int32_t stride = aTarget->mImage->Stride();
  uint8_t extrema[4];         
  uint16_t op = mEnumAttributes[OPERATOR].GetAnimValue();

  
  for (int32_t y = rect.y; y < rect.YMost(); y++) {
    int32_t startY = std::max(0, y - ry);
    
    
    
    int32_t endY = std::min(y + ry, instance->GetSurfaceHeight() - 1);
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      int32_t startX = std::max(0, x - rx);
      int32_t endX = std::min(x + rx, instance->GetSurfaceWidth() - 1);
      int32_t targIndex = y * stride + 4 * x;

      for (int32_t i = 0; i < 4; i++) {
        extrema[i] = sourceData[targIndex + i];
      }
      for (int32_t y1 = startY; y1 <= endY; y1++) {
        for (int32_t x1 = startX; x1 <= endX; x1++) {
          for (int32_t i = 0; i < 4; i++) {
            uint8_t pixel = sourceData[y1 * stride + 4 * x1 + i];
            if ((extrema[i] > pixel &&
                 op == nsSVGFEMorphologyElement::SVG_OPERATOR_ERODE) ||
                (extrema[i] < pixel &&
                 op == nsSVGFEMorphologyElement::SVG_OPERATOR_DILATE)) {
              extrema[i] = pixel;
            }
          }
        }
      }
      targetData[targIndex  ] = extrema[0];
      targetData[targIndex+1] = extrema[1];
      targetData[targIndex+2] = extrema[2];
      targetData[targIndex+3] = extrema[3];
    }
  }
  return NS_OK;
}

bool
nsSVGFEMorphologyElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                    nsIAtom* aAttribute) const
{
  return nsSVGFEMorphologyElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::radius ||
           aAttribute == nsGkAtoms::_operator));
}




nsSVGElement::NumberPairAttributesInfo
nsSVGFEMorphologyElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                  ArrayLength(sNumberPairInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFEMorphologyElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEMorphologyElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFE nsSVGFEConvolveMatrixElementBase;

class nsSVGFEConvolveMatrixElement : public nsSVGFEConvolveMatrixElementBase,
                                     public nsIDOMSVGFEConvolveMatrixElement
{
  friend nsresult NS_NewSVGFEConvolveMatrixElement(nsIContent **aResult,
                                                   already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEConvolveMatrixElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEConvolveMatrixElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEConvolveMatrixElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFECONVOLVEMATRIXELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEConvolveMatrixElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnPremultipledAlpha(int32_t) {
    return !mBooleanAttributes[PRESERVEALPHA].GetAnimValue();
  }

  virtual NumberAttributesInfo GetNumberInfo();
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual IntegerAttributesInfo GetIntegerInfo();
  virtual IntegerPairAttributesInfo GetIntegerPairInfo();
  virtual BooleanAttributesInfo GetBooleanInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();
  virtual NumberListAttributesInfo GetNumberListInfo();

  enum { DIVISOR, BIAS };
  nsSVGNumber2 mNumberAttributes[2];
  static NumberInfo sNumberInfo[2];

  enum { KERNEL_UNIT_LENGTH };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { TARGET_X, TARGET_Y };
  nsSVGInteger mIntegerAttributes[2];
  static IntegerInfo sIntegerInfo[2];

  enum { ORDER };
  nsSVGIntegerPair mIntegerPairAttributes[1];
  static IntegerPairInfo sIntegerPairInfo[1];

  enum { PRESERVEALPHA };
  nsSVGBoolean mBooleanAttributes[1];
  static BooleanInfo sBooleanInfo[1];

  enum { EDGEMODE };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sEdgeModeMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  enum { KERNELMATRIX };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};

nsSVGElement::NumberInfo nsSVGFEConvolveMatrixElement::sNumberInfo[2] =
{
  { &nsGkAtoms::divisor, 1, false },
  { &nsGkAtoms::bias, 0, false }
};

nsSVGElement::NumberPairInfo nsSVGFEConvolveMatrixElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::kernelUnitLength, 0, 0 }
};

nsSVGElement::IntegerInfo nsSVGFEConvolveMatrixElement::sIntegerInfo[2] =
{
  { &nsGkAtoms::targetX, 0 },
  { &nsGkAtoms::targetY, 0 }
};

nsSVGElement::IntegerPairInfo nsSVGFEConvolveMatrixElement::sIntegerPairInfo[1] =
{
  { &nsGkAtoms::order, 3, 3 }
};

nsSVGElement::BooleanInfo nsSVGFEConvolveMatrixElement::sBooleanInfo[1] =
{
  { &nsGkAtoms::preserveAlpha, false }
};

nsSVGEnumMapping nsSVGFEConvolveMatrixElement::sEdgeModeMap[] = {
  {&nsGkAtoms::duplicate, nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_DUPLICATE},
  {&nsGkAtoms::wrap, nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_WRAP},
  {&nsGkAtoms::none, nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_NONE},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFEConvolveMatrixElement::sEnumInfo[1] =
{
  { &nsGkAtoms::edgeMode,
    sEdgeModeMap,
    nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_DUPLICATE
  }
};

nsSVGElement::StringInfo nsSVGFEConvolveMatrixElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};

nsSVGElement::NumberListInfo nsSVGFEConvolveMatrixElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::kernelMatrix }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEConvolveMatrix)




NS_IMPL_ADDREF_INHERITED(nsSVGFEConvolveMatrixElement,nsSVGFEConvolveMatrixElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEConvolveMatrixElement,nsSVGFEConvolveMatrixElementBase)

DOMCI_NODE_DATA(SVGFEConvolveMatrixElement, nsSVGFEConvolveMatrixElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEConvolveMatrixElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEConvolveMatrixElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEConvolveMatrixElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEConvolveMatrixElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEConvolveMatrixElementBase)





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEConvolveMatrixElement)




NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetOrderX(nsIDOMSVGAnimatedInteger * *aOrderX)
{
  return mIntegerPairAttributes[ORDER].ToDOMAnimatedInteger(aOrderX, nsSVGIntegerPair::eFirst, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetOrderY(nsIDOMSVGAnimatedInteger * *aOrderY)
{
  return mIntegerPairAttributes[ORDER].ToDOMAnimatedInteger(aOrderY, nsSVGIntegerPair::eSecond, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetKernelMatrix(nsISupports * *aKernelMatrix)
{
  *aKernelMatrix = DOMSVGAnimatedNumberList::GetDOMWrapper(&mNumberListAttributes[KERNELMATRIX],
                                                           this, KERNELMATRIX).get();
  return NS_OK;
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetTargetX(nsIDOMSVGAnimatedInteger * *aTargetX)
{
  return mIntegerAttributes[TARGET_X].ToDOMAnimatedInteger(aTargetX, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetTargetY(nsIDOMSVGAnimatedInteger * *aTargetY)
{
  return mIntegerAttributes[TARGET_Y].ToDOMAnimatedInteger(aTargetY, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetEdgeMode(nsIDOMSVGAnimatedEnumeration * *aEdgeMode)
{
  return mEnumAttributes[EDGEMODE].ToDOMAnimatedEnum(aEdgeMode, this);
}

NS_IMETHODIMP nsSVGFEConvolveMatrixElement::GetPreserveAlpha(nsISupports * *aPreserveAlpha)
{
  return mBooleanAttributes[PRESERVEALPHA].ToDOMAnimatedBoolean(aPreserveAlpha, this);
}

NS_IMETHODIMP
nsSVGFEConvolveMatrixElement::GetDivisor(nsIDOMSVGAnimatedNumber **aDivisor)
{
  return mNumberAttributes[DIVISOR].ToDOMAnimatedNumber(aDivisor, this);
}

NS_IMETHODIMP
nsSVGFEConvolveMatrixElement::GetBias(nsIDOMSVGAnimatedNumber **aBias)
{
  return mNumberAttributes[BIAS].ToDOMAnimatedNumber(aBias, this);
}

NS_IMETHODIMP
nsSVGFEConvolveMatrixElement::GetKernelUnitLengthX(nsIDOMSVGAnimatedNumber **aKernelX)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelX,
                                                                       nsSVGNumberPair::eFirst,
                                                                       this);
}

NS_IMETHODIMP
nsSVGFEConvolveMatrixElement::GetKernelUnitLengthY(nsIDOMSVGAnimatedNumber **aKernelY)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelY,
                                                                       nsSVGNumberPair::eSecond,
                                                                       this);
}

void
nsSVGFEConvolveMatrixElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

nsIntRect
nsSVGFEConvolveMatrixElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
        const nsSVGFilterInstance& aInstance)
{
  
  
  
  return GetMaxRect();
}

void
nsSVGFEConvolveMatrixElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  
  
  
}

nsIntRect
nsSVGFEConvolveMatrixElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                                const nsSVGFilterInstance& aInstance)
{
  
  
  return GetMaxRect();
}

static int32_t BoundInterval(int32_t aVal, int32_t aMax)
{
  aVal = std::max(aVal, 0);
  return std::min(aVal, aMax - 1);
}

static int32_t WrapInterval(int32_t aVal, int32_t aMax)
{
  aVal = aVal % aMax;
  return aVal < 0 ? aMax + aVal : aVal;
}

static void
ConvolvePixel(const uint8_t *aSourceData,
              uint8_t *aTargetData,
              int32_t aWidth, int32_t aHeight,
              int32_t aStride,
              int32_t aX, int32_t aY,
              uint16_t aEdgeMode,
              const float *aKernel,
              float aDivisor, float aBias,
              bool aPreserveAlpha,
              int32_t aOrderX, int32_t aOrderY,
              int32_t aTargetX, int32_t aTargetY)
{
  float sum[4] = {0, 0, 0, 0};
  aBias *= 255;
  int32_t offsets[4] = {GFX_ARGB32_OFFSET_R,
                        GFX_ARGB32_OFFSET_G,
                        GFX_ARGB32_OFFSET_B,
                        GFX_ARGB32_OFFSET_A } ;
  int32_t channels = aPreserveAlpha ? 3 : 4;

  for (int32_t y = 0; y < aOrderY; y++) {
    int32_t sampleY = aY + y - aTargetY;
    bool overscanY = sampleY < 0 || sampleY >= aHeight;
    for (int32_t x = 0; x < aOrderX; x++) {
      int32_t sampleX = aX + x - aTargetX;
      bool overscanX = sampleX < 0 || sampleX >= aWidth;
      for (int32_t i = 0; i < channels; i++) {
        if (overscanY || overscanX) {
          switch (aEdgeMode) {
            case nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_DUPLICATE:
              sum[i] +=
                aSourceData[BoundInterval(sampleY, aHeight) * aStride +
                            BoundInterval(sampleX, aWidth) * 4 + offsets[i]] *
                aKernel[aOrderX * y + x];
              break;
            case nsSVGFEConvolveMatrixElement::SVG_EDGEMODE_WRAP:
              sum[i] +=
                aSourceData[WrapInterval(sampleY, aHeight) * aStride +
                            WrapInterval(sampleX, aWidth) * 4 + offsets[i]] *
                aKernel[aOrderX * y + x];
              break;
            default:
              break;
          }
        } else {
          sum[i] +=
            aSourceData[sampleY * aStride + 4 * sampleX + offsets[i]] *
            aKernel[aOrderX * y + x];
        }
      }
    }
  }
  for (int32_t i = 0; i < channels; i++) {
    aTargetData[aY * aStride + 4 * aX + offsets[i]] =
      BoundInterval(static_cast<int32_t>(sum[i] / aDivisor + aBias), 256);
  }
  if (aPreserveAlpha) {
    aTargetData[aY * aStride + 4 * aX + GFX_ARGB32_OFFSET_A] =
      aSourceData[aY * aStride + 4 * aX + GFX_ARGB32_OFFSET_A];
  }
}

nsresult
nsSVGFEConvolveMatrixElement::Filter(nsSVGFilterInstance *instance,
                                     const nsTArray<const Image*>& aSources,
                                     const Image* aTarget,
                                     const nsIntRect& rect)
{
  const SVGNumberList &kernelMatrix =
    mNumberListAttributes[KERNELMATRIX].GetAnimValue();
  uint32_t kmLength = kernelMatrix.Length();

  int32_t orderX = mIntegerPairAttributes[ORDER].GetAnimValue(nsSVGIntegerPair::eFirst);
  int32_t orderY = mIntegerPairAttributes[ORDER].GetAnimValue(nsSVGIntegerPair::eSecond);

  if (orderX <= 0 || orderY <= 0 ||
      static_cast<uint32_t>(orderX * orderY) != kmLength) {
    return NS_ERROR_FAILURE;
  }

  int32_t targetX, targetY;
  GetAnimatedIntegerValues(&targetX, &targetY, nullptr);

  if (mIntegerAttributes[TARGET_X].IsExplicitlySet()) {
    if (targetX < 0 || targetX >= orderX)
      return NS_ERROR_FAILURE;
  } else {
    targetX = orderX / 2;
  }
  if (mIntegerAttributes[TARGET_Y].IsExplicitlySet()) {
    if (targetY < 0 || targetY >= orderY)
      return NS_ERROR_FAILURE;
  } else {
    targetY = orderY / 2;
  }

  if (orderX > NS_SVG_OFFSCREEN_MAX_DIMENSION ||
      orderY > NS_SVG_OFFSCREEN_MAX_DIMENSION)
    return NS_ERROR_FAILURE;
  nsAutoArrayPtr<float> kernel(new float[orderX * orderY]);
  if (!kernel)
    return NS_ERROR_FAILURE;
  for (uint32_t i = 0; i < kmLength; i++) {
    kernel[kmLength - 1 - i] = kernelMatrix[i];
  }

  float divisor;
  if (mNumberAttributes[DIVISOR].IsExplicitlySet()) {
    divisor = mNumberAttributes[DIVISOR].GetAnimValue();
    if (divisor == 0)
      return NS_ERROR_FAILURE;
  } else {
    divisor = kernel[0];
    for (uint32_t i = 1; i < kmLength; i++)
      divisor += kernel[i];
    if (divisor == 0)
      divisor = 1;
  }

  ScaleInfo info = SetupScalingFilter(instance, aSources[0], aTarget, rect,
                                      &mNumberPairAttributes[KERNEL_UNIT_LENGTH]);
  if (!info.mTarget)
    return NS_ERROR_FAILURE;

  uint16_t edgeMode = mEnumAttributes[EDGEMODE].GetAnimValue();
  bool preserveAlpha = mBooleanAttributes[PRESERVEALPHA].GetAnimValue();

  float bias = mNumberAttributes[BIAS].GetAnimValue();

  const nsIntRect& dataRect = info.mDataRect;
  int32_t stride = info.mSource->Stride();
  int32_t width = info.mSource->GetSize().width;
  int32_t height = info.mSource->GetSize().height;
  uint8_t *sourceData = info.mSource->Data();
  uint8_t *targetData = info.mTarget->Data();

  for (int32_t y = dataRect.y; y < dataRect.YMost(); y++) {
    for (int32_t x = dataRect.x; x < dataRect.XMost(); x++) {
      ConvolvePixel(sourceData, targetData,
                    width, height, stride,
                    x, y,
                    edgeMode, kernel, divisor, bias, preserveAlpha,
                    orderX, orderY, targetX, targetY);
    }
  }

  FinishScalingFilter(&info);

  return NS_OK;
}

bool
nsSVGFEConvolveMatrixElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                        nsIAtom* aAttribute) const
{
  return nsSVGFEConvolveMatrixElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::divisor ||
           aAttribute == nsGkAtoms::bias ||
           aAttribute == nsGkAtoms::kernelUnitLength ||
           aAttribute == nsGkAtoms::targetX ||
           aAttribute == nsGkAtoms::targetY ||
           aAttribute == nsGkAtoms::order ||
           aAttribute == nsGkAtoms::preserveAlpha||
           aAttribute == nsGkAtoms::edgeMode ||
           aAttribute == nsGkAtoms::kernelMatrix));
}




nsSVGElement::NumberAttributesInfo
nsSVGFEConvolveMatrixElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::NumberPairAttributesInfo
nsSVGFEConvolveMatrixElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                  ArrayLength(sNumberPairInfo));
}

nsSVGElement::IntegerAttributesInfo
nsSVGFEConvolveMatrixElement::GetIntegerInfo()
{
  return IntegerAttributesInfo(mIntegerAttributes, sIntegerInfo,
                               ArrayLength(sIntegerInfo));
}

nsSVGElement::IntegerPairAttributesInfo
nsSVGFEConvolveMatrixElement::GetIntegerPairInfo()
{
  return IntegerPairAttributesInfo(mIntegerPairAttributes, sIntegerPairInfo,
                                   ArrayLength(sIntegerPairInfo));
}

nsSVGElement::BooleanAttributesInfo
nsSVGFEConvolveMatrixElement::GetBooleanInfo()
{
  return BooleanAttributesInfo(mBooleanAttributes, sBooleanInfo,
                               ArrayLength(sBooleanInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFEConvolveMatrixElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEConvolveMatrixElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

nsSVGElement::NumberListAttributesInfo
nsSVGFEConvolveMatrixElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}



typedef SVGFEUnstyledElement nsSVGFESpotLightElementBase;

class nsSVGFESpotLightElement : public nsSVGFESpotLightElementBase,
                                public nsIDOMSVGFESpotLightElement
{
  friend nsresult NS_NewSVGFESpotLightElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo);
  friend class nsSVGFELightingElement;
protected:
  nsSVGFESpotLightElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFESpotLightElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFESPOTLIGHTELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFESpotLightElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual NumberAttributesInfo GetNumberInfo();

  enum { X, Y, Z, POINTS_AT_X, POINTS_AT_Y, POINTS_AT_Z,
         SPECULAR_EXPONENT, LIMITING_CONE_ANGLE };
  nsSVGNumber2 mNumberAttributes[8];
  static NumberInfo sNumberInfo[8];
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FESpotLight)

nsSVGElement::NumberInfo nsSVGFESpotLightElement::sNumberInfo[8] =
{
  { &nsGkAtoms::x, 0, false },
  { &nsGkAtoms::y, 0, false },
  { &nsGkAtoms::z, 0, false },
  { &nsGkAtoms::pointsAtX, 0, false },
  { &nsGkAtoms::pointsAtY, 0, false },
  { &nsGkAtoms::pointsAtZ, 0, false },
  { &nsGkAtoms::specularExponent, 1, false },
  { &nsGkAtoms::limitingConeAngle, 0, false }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFESpotLightElement,nsSVGFESpotLightElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFESpotLightElement,nsSVGFESpotLightElementBase)

DOMCI_NODE_DATA(SVGFESpotLightElement, nsSVGFESpotLightElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFESpotLightElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGFESpotLightElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFESpotLightElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFESpotLightElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFESpotLightElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFESpotLightElement)




bool
nsSVGFESpotLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                   nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::x ||
          aAttribute == nsGkAtoms::y ||
          aAttribute == nsGkAtoms::z ||
          aAttribute == nsGkAtoms::pointsAtX ||
          aAttribute == nsGkAtoms::pointsAtY ||
          aAttribute == nsGkAtoms::pointsAtZ ||
          aAttribute == nsGkAtoms::specularExponent ||
          aAttribute == nsGkAtoms::limitingConeAngle);
}




NS_IMETHODIMP
nsSVGFESpotLightElement::GetX(nsIDOMSVGAnimatedNumber **aX)
{
  return mNumberAttributes[X].ToDOMAnimatedNumber(aX, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetY(nsIDOMSVGAnimatedNumber **aY)
{
  return mNumberAttributes[Y].ToDOMAnimatedNumber(aY, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetZ(nsIDOMSVGAnimatedNumber **aZ)
{
  return mNumberAttributes[Z].ToDOMAnimatedNumber(aZ, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetPointsAtX(nsIDOMSVGAnimatedNumber **aX)
{
  return mNumberAttributes[POINTS_AT_X].ToDOMAnimatedNumber(aX, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetPointsAtY(nsIDOMSVGAnimatedNumber **aY)
{
  return mNumberAttributes[POINTS_AT_Y].ToDOMAnimatedNumber(aY, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetPointsAtZ(nsIDOMSVGAnimatedNumber **aZ)
{
  return mNumberAttributes[POINTS_AT_Z].ToDOMAnimatedNumber(aZ, this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetSpecularExponent(nsIDOMSVGAnimatedNumber **aExponent)
{
  return mNumberAttributes[SPECULAR_EXPONENT].ToDOMAnimatedNumber(aExponent,
                                                                  this);
}

NS_IMETHODIMP
nsSVGFESpotLightElement::GetLimitingConeAngle(nsIDOMSVGAnimatedNumber **aAngle)
{
  return mNumberAttributes[LIMITING_CONE_ANGLE].ToDOMAnimatedNumber(aAngle,
                                                                    this);
}




nsSVGElement::NumberAttributesInfo
nsSVGFESpotLightElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}



typedef nsSVGFE nsSVGFELightingElementBase;

class nsSVGFELightingElement : public nsSVGFELightingElementBase
{
protected:
  nsSVGFELightingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFELightingElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFELightingElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  
  
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFELightingElementBase::)

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance*,
                              int32_t, Image*) { return true; }
  virtual void
  LightPixel(const float *N, const float *L,
             nscolor color, uint8_t *targetData) = 0;

  virtual NumberAttributesInfo GetNumberInfo();
  virtual NumberPairAttributesInfo GetNumberPairInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { SURFACE_SCALE, DIFFUSE_CONSTANT, SPECULAR_CONSTANT, SPECULAR_EXPONENT };
  nsSVGNumber2 mNumberAttributes[4];
  static NumberInfo sNumberInfo[4];

  enum { KERNEL_UNIT_LENGTH };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

nsSVGElement::NumberInfo nsSVGFELightingElement::sNumberInfo[4] =
{
  { &nsGkAtoms::surfaceScale, 1, false },
  { &nsGkAtoms::diffuseConstant, 1, false },
  { &nsGkAtoms::specularConstant, 1, false },
  { &nsGkAtoms::specularExponent, 1, false }
};

nsSVGElement::NumberPairInfo nsSVGFELightingElement::sNumberPairInfo[1] =
{
  { &nsGkAtoms::kernelUnitLength, 0, 0 }
};

nsSVGElement::StringInfo nsSVGFELightingElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFELightingElement,nsSVGFELightingElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFELightingElement,nsSVGFELightingElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGFELightingElement) 
NS_INTERFACE_MAP_END_INHERITING(nsSVGFELightingElementBase)




NS_IMETHODIMP_(bool)
nsSVGFELightingElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sLightingEffectsMap
  };

  return FindAttributeDependence(name, map) ||
    nsSVGFELightingElementBase::IsAttributeMapped(name);
}

void
nsSVGFELightingElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}

void
nsSVGFELightingElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  
  
  
}

nsIntRect
nsSVGFELightingElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                          const nsSVGFilterInstance& aInstance)
{
  
  return GetMaxRect();
}

#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
#define NORMALIZE(vec) \
  PR_BEGIN_MACRO \
    float norm = sqrt(DOT(vec, vec)); \
    vec[0] /= norm; \
    vec[1] /= norm; \
    vec[2] /= norm; \
  PR_END_MACRO

static int32_t
Convolve3x3(const uint8_t *index, int32_t stride,
            const int8_t kernel[3][3]
#ifdef DEBUG
            , const uint8_t *minData, const uint8_t *maxData
#endif 
)
{
  int32_t sum = 0;
  for (int32_t y = 0; y < 3; y++) {
    for (int32_t x = 0; x < 3; x++) {
      int8_t k = kernel[y][x];
      if (k) {
        const uint8_t *valPtr = index + (4 * (x - 1) + stride * (y - 1));
        NS_ASSERTION(valPtr >= minData, "out of bounds read (before buffer)");
        NS_ASSERTION(valPtr < maxData,  "out of bounds read (after buffer)");
        sum += k * (*valPtr);
      }
    }
  }
  return sum;
}

static void
GenerateNormal(float *N, const uint8_t *data, int32_t stride,
               int32_t surfaceWidth, int32_t surfaceHeight,
               int32_t x, int32_t y, float surfaceScale)
{
  
  
  static const int8_t Kx[3][3][3][3] =
    { { { {  0,  0,  0}, { 0, -2,  2}, { 0, -1,  1} },
        { {  0,  0,  0}, {-2,  0,  2}, {-1,  0,  1} },
        { {  0,  0,  0}, {-2,  2,  0}, {-1,  1,  0} } },
      { { {  0, -1,  1}, { 0, -2,  2}, { 0, -1,  1} },
        { { -1,  0,  1}, {-2,  0,  2}, {-1,  0,  1} },
        { { -1,  1,  0}, {-2,  2,  0}, {-1,  1,  0} } },
      { { {  0, -1,  1}, { 0, -2,  2}, { 0,  0,  0} },
        { { -1,  0,  1}, {-2,  0,  2}, { 0,  0,  0} },
        { { -1,  1,  0}, {-2,  2,  0}, { 0,  0,  0} } } };
  static const int8_t Ky[3][3][3][3] =
    { { { {  0,  0,  0}, { 0, -2, -1}, { 0,  2,  1} },
        { {  0,  0,  0}, {-1, -2, -1}, { 1,  2,  1} },
        { {  0,  0,  0}, {-1, -2,  1}, { 1,  2,  0} } },
      { { {  0, -2, -1}, { 0,  0,  0}, { 0,  2,  1} },
        { { -1, -2, -1}, { 0,  0,  0}, { 1,  2,  1} },
        { { -1, -2,  0}, { 0,  0,  0}, { 1,  2,  0} } },
      { { {  0, -2, -1}, { 0,  2,  1}, { 0,  0,  0} },
        { { -1, -2, -1}, { 1,  2,  1}, { 0,  0,  0} },
        { { -1, -2,  0}, { 1,  2,  0}, { 0,  0,  0} } } };
  static const float FACTORx[3][3] =
    { { 2.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0 },
      { 1.0 / 2.0, 1.0 / 4.0, 1.0 / 2.0 },
      { 2.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0 } };
  static const float FACTORy[3][3] =
    { { 2.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0 },
      { 1.0 / 3.0, 1.0 / 4.0, 1.0 / 3.0 },
      { 2.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0 } };

  
  if (surfaceWidth == 1 || surfaceHeight == 1) {
    
    N[0] = 0;
    N[1] = 0;
    N[2] = 255;
    return;
  }

  int8_t xflag, yflag;
  if (x == 0) {
    xflag = 0;
  } else if (x == surfaceWidth - 1) {
    xflag = 2;
  } else {
    xflag = 1;
  }
  if (y == 0) {
    yflag = 0;
  } else if (y == surfaceHeight - 1) {
    yflag = 2;
  } else {
    yflag = 1;
  }

  const uint8_t *index = data + y * stride + 4 * x + GFX_ARGB32_OFFSET_A;

#ifdef DEBUG
  
  const uint8_t* minData = data;
  const uint8_t* maxData = minData + (surfaceHeight * surfaceWidth * stride);

  
  
  NS_ASSERTION(index >= minData, "index points before buffer start");
  NS_ASSERTION(index < maxData, "index points after buffer end");
#endif 

  N[0] = -surfaceScale * FACTORx[yflag][xflag] *
    Convolve3x3(index, stride, Kx[yflag][xflag]
#ifdef DEBUG
                , minData, maxData
#endif 
                );

  N[1] = -surfaceScale * FACTORy[yflag][xflag] *
    Convolve3x3(index, stride, Ky[yflag][xflag]
#ifdef DEBUG
                , minData, maxData
#endif 
                );
  N[2] = 255;
  NORMALIZE(N);
}

nsresult
nsSVGFELightingElement::Filter(nsSVGFilterInstance *instance,
                               const nsTArray<const Image*>& aSources,
                               const Image* aTarget,
                               const nsIntRect& rect)
{
  ScaleInfo info = SetupScalingFilter(instance, aSources[0], aTarget, rect,
                                      &mNumberPairAttributes[KERNEL_UNIT_LENGTH]);
  if (!info.mTarget)
    return NS_ERROR_FAILURE;

  SVGFEDistantLightElement* distantLight;
  SVGFEPointLightElement* pointLight;
  nsCOMPtr<nsIDOMSVGFESpotLightElement> spotLight;

  nsIFrame* frame = GetPrimaryFrame();
  if (!frame) return NS_ERROR_FAILURE;
  nsStyleContext* style = frame->StyleContext();

  nscolor lightColor = style->StyleSVGReset()->mLightingColor;

  
  for (nsCOMPtr<nsIContent> child = nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    distantLight = child->IsSVG(nsGkAtoms::feDistantLight) ?
                     static_cast<SVGFEDistantLightElement*>(child.get()) : nullptr;
    pointLight = child->IsSVG(nsGkAtoms::fePointLight) ?
                   static_cast<SVGFEPointLightElement*>(child.get()) : nullptr;
    spotLight = do_QueryInterface(child);
    if (distantLight || pointLight || spotLight)
      break;
  }

  if (!distantLight && !pointLight && !spotLight)
    return NS_ERROR_FAILURE;

  const float radPerDeg = M_PI/180.0;

  float L[3];
  if (distantLight) {
    float azimuth, elevation;
    distantLight->GetAnimatedNumberValues(&azimuth,
                                          &elevation,
                                          nullptr);
    L[0] = cos(azimuth * radPerDeg) * cos(elevation * radPerDeg);
    L[1] = sin(azimuth * radPerDeg) * cos(elevation * radPerDeg);
    L[2] = sin(elevation * radPerDeg);
  }
  float lightPos[3], pointsAt[3], specularExponent;
  float cosConeAngle = 0;
  if (pointLight) {
    pointLight->GetAnimatedNumberValues(lightPos,
                                        lightPos + 1,
                                        lightPos + 2,
                                        nullptr);
    instance->ConvertLocation(lightPos);
  }
  if (spotLight) {
    float limitingConeAngle;
    nsSVGFESpotLightElement* spot = 
      static_cast<nsSVGFESpotLightElement*>(spotLight.get());
    spot->GetAnimatedNumberValues(lightPos,
                                  lightPos + 1,
                                  lightPos + 2,
                                  pointsAt,
                                  pointsAt + 1,
                                  pointsAt + 2,
                                  &specularExponent,
                                  &limitingConeAngle,
                                  nullptr);
    instance->ConvertLocation(lightPos);
    instance->ConvertLocation(pointsAt);

    if (spot->mNumberAttributes[nsSVGFESpotLightElement::LIMITING_CONE_ANGLE].
                                  IsExplicitlySet()) {
      cosConeAngle = std::max<double>(cos(limitingConeAngle * radPerDeg), 0.0);
    }
  }

  float surfaceScale = mNumberAttributes[SURFACE_SCALE].GetAnimValue();

  const nsIntRect& dataRect = info.mDataRect;
  int32_t stride = info.mSource->Stride();
  uint8_t *sourceData = info.mSource->Data();
  uint8_t *targetData = info.mTarget->Data();
  int32_t surfaceWidth = info.mSource->Width();
  int32_t surfaceHeight = info.mSource->Height();
  
  for (int32_t y = dataRect.y; y < dataRect.YMost(); y++) {
    for (int32_t x = dataRect.x; x < dataRect.XMost(); x++) {
      int32_t index = y * stride + x * 4;

      float N[3];
      GenerateNormal(N, sourceData, stride, surfaceWidth, surfaceHeight,
                     x, y, surfaceScale);

      if (pointLight || spotLight) {
        gfxPoint pt = instance->FilterSpaceToUserSpace(
                gfxPoint(x + instance->GetSurfaceRect().x,
                         y + instance->GetSurfaceRect().y));
        float Z = surfaceScale * sourceData[index + GFX_ARGB32_OFFSET_A] / 255;

        L[0] = lightPos[0] - pt.x;
        L[1] = lightPos[1] - pt.y;
        L[2] = lightPos[2] - Z;
        NORMALIZE(L);
      }

      nscolor color;

      if (spotLight) {
        float S[3];
        S[0] = pointsAt[0] - lightPos[0];
        S[1] = pointsAt[1] - lightPos[1];
        S[2] = pointsAt[2] - lightPos[2];
        NORMALIZE(S);
        float dot = -DOT(L, S);
        float tmp = pow(dot, specularExponent);
        if (dot < cosConeAngle) tmp = 0;
        color = NS_RGB(uint8_t(NS_GET_R(lightColor) * tmp),
                       uint8_t(NS_GET_G(lightColor) * tmp),
                       uint8_t(NS_GET_B(lightColor) * tmp));
      } else {
        color = lightColor;
      }

      LightPixel(N, L, color, targetData + index);
    }
  }

  FinishScalingFilter(&info);

  return NS_OK;
}

bool
nsSVGFELightingElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                  nsIAtom* aAttribute) const
{
  return nsSVGFELightingElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::surfaceScale ||
           aAttribute == nsGkAtoms::kernelUnitLength));
}




nsSVGElement::NumberAttributesInfo
nsSVGFELightingElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::NumberPairAttributesInfo
nsSVGFELightingElement::GetNumberPairInfo()
{
  return NumberPairAttributesInfo(mNumberPairAttributes, sNumberPairInfo,
                                  ArrayLength(sNumberPairInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFELightingElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}



typedef nsSVGFELightingElement nsSVGFEDiffuseLightingElementBase;

class nsSVGFEDiffuseLightingElement : public nsSVGFEDiffuseLightingElementBase,
                                      public nsIDOMSVGFEDiffuseLightingElement
{
  friend nsresult NS_NewSVGFEDiffuseLightingElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFEDiffuseLightingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEDiffuseLightingElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGFEDIFFUSELIGHTINGELEMENT

  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEDiffuseLightingElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEDiffuseLightingElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual void LightPixel(const float *N, const float *L,
                          nscolor color, uint8_t *targetData);

};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEDiffuseLighting)




NS_IMPL_ADDREF_INHERITED(nsSVGFEDiffuseLightingElement,nsSVGFEDiffuseLightingElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEDiffuseLightingElement,nsSVGFEDiffuseLightingElementBase)

DOMCI_NODE_DATA(SVGFEDiffuseLightingElement, nsSVGFEDiffuseLightingElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEDiffuseLightingElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEDiffuseLightingElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEDiffuseLightingElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEDiffuseLightingElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEDiffuseLightingElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEDiffuseLightingElement)




NS_IMETHODIMP
nsSVGFEDiffuseLightingElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}

NS_IMETHODIMP
nsSVGFEDiffuseLightingElement::GetSurfaceScale(nsIDOMSVGAnimatedNumber **aScale)
{
  return mNumberAttributes[SURFACE_SCALE].ToDOMAnimatedNumber(aScale,
                                                              this);
}

NS_IMETHODIMP
nsSVGFEDiffuseLightingElement::GetDiffuseConstant(nsIDOMSVGAnimatedNumber **aConstant)
{
  return mNumberAttributes[DIFFUSE_CONSTANT].ToDOMAnimatedNumber(aConstant,
                                                              this);
}

NS_IMETHODIMP
nsSVGFEDiffuseLightingElement::GetKernelUnitLengthX(nsIDOMSVGAnimatedNumber **aKernelX)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelX,
                                                                       nsSVGNumberPair::eFirst,
                                                                       this);
}

NS_IMETHODIMP
nsSVGFEDiffuseLightingElement::GetKernelUnitLengthY(nsIDOMSVGAnimatedNumber **aKernelY)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelY,
                                                                       nsSVGNumberPair::eSecond,
                                                                       this);
}

bool
nsSVGFEDiffuseLightingElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                         nsIAtom* aAttribute) const
{
  return nsSVGFEDiffuseLightingElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          aAttribute == nsGkAtoms::diffuseConstant);
}




void
nsSVGFEDiffuseLightingElement::LightPixel(const float *N, const float *L,
                                          nscolor color, uint8_t *targetData)
{
  float diffuseNL =
    mNumberAttributes[DIFFUSE_CONSTANT].GetAnimValue() * DOT(N, L);

  if (diffuseNL < 0) diffuseNL = 0;

  targetData[GFX_ARGB32_OFFSET_B] =
    std::min(uint32_t(diffuseNL * NS_GET_B(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_G] =
    std::min(uint32_t(diffuseNL * NS_GET_G(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_R] =
    std::min(uint32_t(diffuseNL * NS_GET_R(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_A] = 255;
}



typedef nsSVGFELightingElement nsSVGFESpecularLightingElementBase;

class nsSVGFESpecularLightingElement : public nsSVGFESpecularLightingElementBase,
                                       public nsIDOMSVGFESpecularLightingElement
{
  friend nsresult NS_NewSVGFESpecularLightingElement(nsIContent **aResult,
                                               already_AddRefed<nsINodeInfo> aNodeInfo);
protected:
  nsSVGFESpecularLightingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFESpecularLightingElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGFESPECULARLIGHTINGELEMENT

  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFESpecularLightingElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFESpecularLightingElementBase::)
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual void LightPixel(const float *N, const float *L,
                          nscolor color, uint8_t *targetData);

};

NS_IMPL_NS_NEW_SVG_ELEMENT(FESpecularLighting)




NS_IMPL_ADDREF_INHERITED(nsSVGFESpecularLightingElement,nsSVGFESpecularLightingElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFESpecularLightingElement,nsSVGFESpecularLightingElementBase)

DOMCI_NODE_DATA(SVGFESpecularLightingElement, nsSVGFESpecularLightingElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFESpecularLightingElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFESpecularLightingElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFESpecularLightingElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFESpecularLightingElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFESpecularLightingElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFESpecularLightingElement)




NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}

NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetSurfaceScale(nsIDOMSVGAnimatedNumber **aScale)
{
  return mNumberAttributes[SURFACE_SCALE].ToDOMAnimatedNumber(aScale,
                                                              this);
}

NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetSpecularConstant(nsIDOMSVGAnimatedNumber **aConstant)
{
  return mNumberAttributes[SPECULAR_CONSTANT].ToDOMAnimatedNumber(aConstant,
                                                                  this);
}

NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetSpecularExponent(nsIDOMSVGAnimatedNumber **aExponent)
{
  return mNumberAttributes[SPECULAR_EXPONENT].ToDOMAnimatedNumber(aExponent,
                                                                  this);
}

NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetKernelUnitLengthX(nsIDOMSVGAnimatedNumber **aKernelX)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelX,
                                                                       nsSVGNumberPair::eFirst,
                                                                       this);
}

NS_IMETHODIMP
nsSVGFESpecularLightingElement::GetKernelUnitLengthY(nsIDOMSVGAnimatedNumber **aKernelY)
{
  return mNumberPairAttributes[KERNEL_UNIT_LENGTH].ToDOMAnimatedNumber(aKernelY,
                                                                       nsSVGNumberPair::eSecond,
                                                                       this);
}




nsresult
nsSVGFESpecularLightingElement::Filter(nsSVGFilterInstance *instance,
                                       const nsTArray<const Image*>& aSources,
                                       const Image* aTarget,
                                       const nsIntRect& rect)
{
  float specularExponent = mNumberAttributes[SPECULAR_EXPONENT].GetAnimValue();

  
  if (specularExponent < 1 || specularExponent > 128)
    return NS_ERROR_FAILURE;

  return nsSVGFESpecularLightingElementBase::Filter(instance, aSources, aTarget, rect);
}

bool
nsSVGFESpecularLightingElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                          nsIAtom* aAttribute) const
{
  return nsSVGFESpecularLightingElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::specularConstant ||
           aAttribute == nsGkAtoms::specularExponent));
}

void
nsSVGFESpecularLightingElement::LightPixel(const float *N, const float *L,
                                           nscolor color, uint8_t *targetData)
{
  float H[3];
  H[0] = L[0];
  H[1] = L[1];
  H[2] = L[2] + 1;
  NORMALIZE(H);

  float kS = mNumberAttributes[SPECULAR_CONSTANT].GetAnimValue();
  float dotNH = DOT(N, H);

  bool invalid = dotNH <= 0 || kS <= 0;
  kS *= invalid ? 0 : 1;
  uint8_t minAlpha = invalid ? 255 : 0;

  float specularNH =
    kS * pow(dotNH, mNumberAttributes[SPECULAR_EXPONENT].GetAnimValue());

  targetData[GFX_ARGB32_OFFSET_B] =
    std::min(uint32_t(specularNH * NS_GET_B(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_G] =
    std::min(uint32_t(specularNH * NS_GET_G(color)), 255U);
  targetData[GFX_ARGB32_OFFSET_R] =
    std::min(uint32_t(specularNH * NS_GET_R(color)), 255U);

  targetData[GFX_ARGB32_OFFSET_A] =
    std::max(minAlpha, std::max(targetData[GFX_ARGB32_OFFSET_B],
                            std::max(targetData[GFX_ARGB32_OFFSET_G],
                                   targetData[GFX_ARGB32_OFFSET_R])));
}



typedef nsSVGFE nsSVGFEDisplacementMapElementBase;

class nsSVGFEDisplacementMapElement : public nsSVGFEDisplacementMapElementBase,
                                      public nsIDOMSVGFEDisplacementMapElement
{
protected:
  friend nsresult NS_NewSVGFEDisplacementMapElement(nsIContent **aResult,
                                                    already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGFEDisplacementMapElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGFEDisplacementMapElementBase(aNodeInfo) {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES(nsSVGFEDisplacementMapElementBase::)

  virtual nsresult Filter(nsSVGFilterInstance* aInstance,
                          const nsTArray<const Image*>& aSources,
                          const Image* aTarget,
                          const nsIntRect& aDataRect);
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;
  virtual nsSVGString& GetResultImageName() { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);
  virtual nsIntRect ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance);
  virtual void ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance);
  virtual nsIntRect ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
          const nsSVGFilterInstance& aInstance);

  
  NS_DECL_NSIDOMSVGFEDISPLACEMENTMAPELEMENT

  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFEDisplacementMapElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual bool OperatesOnSRGB(nsSVGFilterInstance* aInstance,
                              int32_t aInput, Image* aImage) {
    switch (aInput) {
    case 0:
      return aImage->mColorModel.mColorSpace == ColorModel::SRGB;
    case 1:
      return nsSVGFEDisplacementMapElementBase::OperatesOnSRGB(aInstance,
                                                               aInput, aImage);
    default:
      NS_ERROR("Will not give correct output color model");
      return false;
    }
  }
  virtual bool OperatesOnPremultipledAlpha(int32_t aInput) {
    return !(aInput == 1);
  }

  virtual NumberAttributesInfo GetNumberInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { SCALE };
  nsSVGNumber2 mNumberAttributes[1];
  static NumberInfo sNumberInfo[1];

  enum { CHANNEL_X, CHANNEL_Y };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sChannelMap[];
  static EnumInfo sEnumInfo[2];

  enum { RESULT, IN1, IN2 };
  nsSVGString mStringAttributes[3];
  static StringInfo sStringInfo[3];
};

nsSVGElement::NumberInfo nsSVGFEDisplacementMapElement::sNumberInfo[1] =
{
  { &nsGkAtoms::scale, 0, false },
};

nsSVGEnumMapping nsSVGFEDisplacementMapElement::sChannelMap[] = {
  {&nsGkAtoms::R, nsSVGFEDisplacementMapElement::SVG_CHANNEL_R},
  {&nsGkAtoms::G, nsSVGFEDisplacementMapElement::SVG_CHANNEL_G},
  {&nsGkAtoms::B, nsSVGFEDisplacementMapElement::SVG_CHANNEL_B},
  {&nsGkAtoms::A, nsSVGFEDisplacementMapElement::SVG_CHANNEL_A},
  {nullptr, 0}
};

nsSVGElement::EnumInfo nsSVGFEDisplacementMapElement::sEnumInfo[2] =
{
  { &nsGkAtoms::xChannelSelector,
    sChannelMap,
    nsSVGFEDisplacementMapElement::SVG_CHANNEL_A
  },
  { &nsGkAtoms::yChannelSelector,
    sChannelMap,
    nsSVGFEDisplacementMapElement::SVG_CHANNEL_A
  }
};

nsSVGElement::StringInfo nsSVGFEDisplacementMapElement::sStringInfo[3] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true },
  { &nsGkAtoms::in2, kNameSpaceID_None, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(FEDisplacementMap)




NS_IMPL_ADDREF_INHERITED(nsSVGFEDisplacementMapElement,nsSVGFEDisplacementMapElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEDisplacementMapElement,nsSVGFEDisplacementMapElementBase)

DOMCI_NODE_DATA(SVGFEDisplacementMapElement, nsSVGFEDisplacementMapElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEDisplacementMapElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFEDisplacementMapElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFilterPrimitiveStandardAttributes,
                           nsIDOMSVGFEDisplacementMapElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEDisplacementMapElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEDisplacementMapElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEDisplacementMapElement)





NS_IMETHODIMP nsSVGFEDisplacementMapElement::GetIn1(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN1].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEDisplacementMapElement::GetIn2(nsIDOMSVGAnimatedString * *aIn)
{
  return mStringAttributes[IN2].ToDOMAnimatedString(aIn, this);
}


NS_IMETHODIMP nsSVGFEDisplacementMapElement::GetScale(nsIDOMSVGAnimatedNumber * *aScale)
{
  return mNumberAttributes[SCALE].ToDOMAnimatedNumber(aScale, this);
}


NS_IMETHODIMP nsSVGFEDisplacementMapElement::GetXChannelSelector(nsIDOMSVGAnimatedEnumeration * *aChannel)
{
  return mEnumAttributes[CHANNEL_X].ToDOMAnimatedEnum(aChannel, this);
}


NS_IMETHODIMP nsSVGFEDisplacementMapElement::GetYChannelSelector(nsIDOMSVGAnimatedEnumeration * *aChannel)
{
  return mEnumAttributes[CHANNEL_Y].ToDOMAnimatedEnum(aChannel, this);
}

nsresult
nsSVGFEDisplacementMapElement::Filter(nsSVGFilterInstance *instance,
                                      const nsTArray<const Image*>& aSources,
                                      const Image* aTarget,
                                      const nsIntRect& rect)
{
  float scale = instance->GetPrimitiveNumber(SVGContentUtils::XY,
                                             &mNumberAttributes[SCALE]);
  if (scale == 0.0f) {
    CopyRect(aTarget, aSources[0], rect);
    return NS_OK;
  }

  int32_t width = instance->GetSurfaceWidth();
  int32_t height = instance->GetSurfaceHeight();

  uint8_t* sourceData = aSources[0]->mImage->Data();
  uint8_t* displacementData = aSources[1]->mImage->Data();
  uint8_t* targetData = aTarget->mImage->Data();
  uint32_t stride = aTarget->mImage->Stride();

  static const uint8_t dummyData[4] = { 0, 0, 0, 0 };

  static const uint16_t channelMap[5] = {
                             0,
                             GFX_ARGB32_OFFSET_R,
                             GFX_ARGB32_OFFSET_G,
                             GFX_ARGB32_OFFSET_B,
                             GFX_ARGB32_OFFSET_A };
  uint16_t xChannel = channelMap[mEnumAttributes[CHANNEL_X].GetAnimValue()];
  uint16_t yChannel = channelMap[mEnumAttributes[CHANNEL_Y].GetAnimValue()];

  double scaleOver255 = scale / 255.0;
  double scaleAdjustment = 0.5 - 0.5 * scale;

  for (int32_t y = rect.y; y < rect.YMost(); y++) {
    for (int32_t x = rect.x; x < rect.XMost(); x++) {
      uint32_t targIndex = y * stride + 4 * x;
      
      int32_t sourceX = x +
        NSToIntFloor(scaleOver255 * displacementData[targIndex + xChannel] +
                scaleAdjustment);
      int32_t sourceY = y +
        NSToIntFloor(scaleOver255 * displacementData[targIndex + yChannel] +
                scaleAdjustment);

      bool outOfBounds = sourceX < 0 || sourceX >= width ||
                         sourceY < 0 || sourceY >= height;
      const uint8_t* data;
      int32_t multiplier;
      if (outOfBounds) {
        data = dummyData;
        multiplier = 0;
      } else {
        data = sourceData;
        multiplier = 1;
      }
      *(uint32_t*)(targetData + targIndex) =
        *(uint32_t*)(data + multiplier * (sourceY * stride + 4 * sourceX));
    }
  }
  return NS_OK;
}

bool
nsSVGFEDisplacementMapElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                         nsIAtom* aAttribute) const
{
  return nsSVGFEDisplacementMapElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::in2 ||
           aAttribute == nsGkAtoms::scale ||
           aAttribute == nsGkAtoms::xChannelSelector ||
           aAttribute == nsGkAtoms::yChannelSelector));
}

void
nsSVGFEDisplacementMapElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN2], this));
}

nsIntRect
nsSVGFEDisplacementMapElement::ComputeTargetBBox(const nsTArray<nsIntRect>& aSourceBBoxes,
          const nsSVGFilterInstance& aInstance)
{
  
  
  
  return GetMaxRect();
}

void
nsSVGFEDisplacementMapElement::ComputeNeededSourceBBoxes(const nsIntRect& aTargetBBox,
          nsTArray<nsIntRect>& aSourceBBoxes, const nsSVGFilterInstance& aInstance)
{
  
  aSourceBBoxes[1] = aTargetBBox;
  
  
  
  
  
}

nsIntRect
nsSVGFEDisplacementMapElement::ComputeChangeBBox(const nsTArray<nsIntRect>& aSourceChangeBoxes,
                                                 const nsSVGFilterInstance& aInstance)
{
  
  
  return GetMaxRect();
}




nsSVGElement::NumberAttributesInfo
nsSVGFEDisplacementMapElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFEDisplacementMapElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFEDisplacementMapElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

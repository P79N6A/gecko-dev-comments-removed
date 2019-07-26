





#include "nsSVGFilterFrame.h"


#include "gfxASurface.h"
#include "gfxUtils.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGElement.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGFilterInstance.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"
#include "nsContentUtils.h"

using namespace mozilla::dom;

nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGFilterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGFilterFrame)








static nsIntRect
MapFrameRectToFilterSpace(const nsRect* aRect,
                          int32_t aAppUnitsPerCSSPx,
                          const gfxMatrix& aFrameSpaceInCSSPxToFilterSpace,
                          const gfxIntSize& aFilterRes)
{
  nsIntRect rect(0, 0, aFilterRes.width, aFilterRes.height);
  if (aRect) {
    gfxRect rectInCSSPx =
      nsLayoutUtils::RectToGfxRect(*aRect, aAppUnitsPerCSSPx);
    gfxRect rectInFilterSpace =
      aFrameSpaceInCSSPxToFilterSpace.TransformBounds(rectInCSSPx);
    rectInFilterSpace.RoundOut();
    nsIntRect intRect;
    if (gfxUtils::GfxRectToIntRect(rectInFilterSpace, &intRect)) {
      rect = intRect;
    }
  }
  return rect;
}






static gfxMatrix
GetUserToFrameSpaceInCSSPxTransform(nsIFrame *aFrame)
{
  gfxMatrix userToFrameSpaceInCSSPx;

  if ((aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    int32_t appUnitsPerCSSPx = aFrame->PresContext()->AppUnitsPerCSSPixel();
    
    
    
    
    
    
    
    
    
    
    if (aFrame->GetType() == nsGkAtoms::svgInnerSVGFrame) {
      userToFrameSpaceInCSSPx =
        static_cast<nsSVGElement*>(aFrame->GetContent())->
          PrependLocalTransformsTo(gfxMatrix());
    } else {
      gfxPoint targetsUserSpaceOffset =
        nsLayoutUtils::RectToGfxRect(aFrame->GetRect(), appUnitsPerCSSPx).
                         TopLeft();
      userToFrameSpaceInCSSPx.Translate(-targetsUserSpaceOffset);
    }
  }
  
  return userToFrameSpaceInCSSPx;
}

class nsSVGFilterFrame::AutoFilterReferencer
{
public:
  AutoFilterReferencer(nsSVGFilterFrame *aFrame)
    : mFrame(aFrame)
  {
    
    
    NS_ABORT_IF_FALSE(!mFrame->mLoopFlag, "Undetected reference loop!");
    mFrame->mLoopFlag = true;
  }
  ~AutoFilterReferencer() {
    mFrame->mLoopFlag = false;
  }
private:
  nsSVGFilterFrame *mFrame;
};

class NS_STACK_CLASS nsAutoFilterInstance {
public:
  nsAutoFilterInstance(nsIFrame *aTarget,
                       nsSVGFilterFrame *aFilterFrame,
                       nsSVGFilterPaintCallback *aPaint,
                       const nsRect *aPostFilterDirtyRect,
                       const nsRect *aPreFilterDirtyRect,
                       const nsRect *aOverridePreFilterVisualOverflowRect,
                       const gfxRect *aOverrideBBox = nullptr);
  ~nsAutoFilterInstance() {}

  
  
  nsSVGFilterInstance* get() { return mInstance; }

private:
  nsAutoPtr<nsSVGFilterInstance> mInstance;
};

nsAutoFilterInstance::nsAutoFilterInstance(nsIFrame *aTarget,
                                           nsSVGFilterFrame *aFilterFrame,
                                           nsSVGFilterPaintCallback *aPaint,
                                           const nsRect *aPostFilterDirtyRect,
                                           const nsRect *aPreFilterDirtyRect,
                                           const nsRect *aPreFilterVisualOverflowRectOverride,
                                           const gfxRect *aOverrideBBox)
{
  const SVGFilterElement *filter = aFilterFrame->GetFilterContent();

  uint16_t filterUnits =
    aFilterFrame->GetEnumValue(SVGFilterElement::FILTERUNITS);
  uint16_t primitiveUnits =
    aFilterFrame->GetEnumValue(SVGFilterElement::PRIMITIVEUNITS);

  gfxRect bbox = aOverrideBBox ? *aOverrideBBox : nsSVGUtils::GetBBox(aTarget);

  

  
  
  
  
  
  
  
  
  
  
  nsSVGLength2 XYWH[4];
  NS_ABORT_IF_FALSE(sizeof(filter->mLengthAttributes) == sizeof(XYWH),
                    "XYWH size incorrect");
  memcpy(XYWH, filter->mLengthAttributes, sizeof(filter->mLengthAttributes));
  XYWH[0] = *aFilterFrame->GetLengthValue(SVGFilterElement::X);
  XYWH[1] = *aFilterFrame->GetLengthValue(SVGFilterElement::Y);
  XYWH[2] = *aFilterFrame->GetLengthValue(SVGFilterElement::WIDTH);
  XYWH[3] = *aFilterFrame->GetLengthValue(SVGFilterElement::HEIGHT);
  
  gfxRect filterRegion = nsSVGUtils::GetRelativeRect(filterUnits,
    XYWH, bbox, aTarget);

  if (filterRegion.Width() <= 0 || filterRegion.Height() <= 0) {
    
    
    return;
  }

  
  
  
  
  

  gfxIntSize filterRes;
  const nsSVGIntegerPair* filterResAttrs =
    aFilterFrame->GetIntegerPairValue(SVGFilterElement::FILTERRES);
  if (filterResAttrs->IsExplicitlySet()) {
    int32_t filterResX = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eFirst);
    int32_t filterResY = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eSecond);
    if (filterResX <= 0 || filterResY <= 0) {
      
      return;
    }

    filterRegion.Scale(filterResX, filterResY);
    filterRegion.RoundOut();
    filterRegion.Scale(1.0 / filterResX, 1.0 / filterResY);
    
    
    bool overflow;
    filterRes =
      nsSVGUtils::ConvertToSurfaceSize(gfxSize(filterResX, filterResY),
                                       &overflow);
    
    
  } else {
    
    
    gfxMatrix canvasTM =
      nsSVGUtils::GetCanvasTM(aTarget, nsISVGChildFrame::FOR_OUTERSVG_TM);
    if (canvasTM.IsSingular()) {
      
      return;
    }
    float scale = nsSVGUtils::MaxExpansion(canvasTM);

    filterRegion.Scale(scale);
    filterRegion.RoundOut();
    
    
    bool overflow;
    filterRes = nsSVGUtils::ConvertToSurfaceSize(filterRegion.Size(),
                                                 &overflow);
    filterRegion.Scale(1.0 / scale);
  }

  

  gfxMatrix filterToUserSpace(filterRegion.Width() / filterRes.width, 0.0f,
                              0.0f, filterRegion.Height() / filterRes.height,
                              filterRegion.X(), filterRegion.Y());

  
  gfxMatrix filterToDeviceSpace;
  if (aPaint) {
    filterToDeviceSpace = filterToUserSpace *
              nsSVGUtils::GetCanvasTM(aTarget, nsISVGChildFrame::FOR_PAINTING);
  }

  

  int32_t appUnitsPerCSSPx = aTarget->PresContext()->AppUnitsPerCSSPixel();

  gfxMatrix filterToFrameSpaceInCSSPx =
    filterToUserSpace * GetUserToFrameSpaceInCSSPxTransform(aTarget);
  
  gfxMatrix frameSpaceInCSSPxTofilterSpace = filterToFrameSpaceInCSSPx;
  frameSpaceInCSSPxTofilterSpace.Invert();

  nsIntRect postFilterDirtyRect =
    MapFrameRectToFilterSpace(aPostFilterDirtyRect, appUnitsPerCSSPx,
                              frameSpaceInCSSPxTofilterSpace, filterRes);
  nsIntRect preFilterDirtyRect =
    MapFrameRectToFilterSpace(aPreFilterDirtyRect, appUnitsPerCSSPx,
                              frameSpaceInCSSPxTofilterSpace, filterRes);
  nsIntRect preFilterVisualOverflowRect;
  if (aPreFilterVisualOverflowRectOverride) {
    preFilterVisualOverflowRect =
      MapFrameRectToFilterSpace(aPreFilterVisualOverflowRectOverride,
                                appUnitsPerCSSPx,
                                frameSpaceInCSSPxTofilterSpace, filterRes);
  } else {
    nsRect preFilterVOR = aTarget->GetPreEffectsVisualOverflowRect();
    preFilterVisualOverflowRect =
      MapFrameRectToFilterSpace(&preFilterVOR, appUnitsPerCSSPx,
                                frameSpaceInCSSPxTofilterSpace, filterRes);
  }

  
  mInstance =
    new nsSVGFilterInstance(aTarget, aPaint, filter, bbox, filterRegion,
                            nsIntSize(filterRes.width, filterRes.height),
                            filterToDeviceSpace, filterToFrameSpaceInCSSPx,
                            preFilterVisualOverflowRect, postFilterDirtyRect,
                            preFilterDirtyRect, primitiveUnits);
}

uint16_t
nsSVGFilterFrame::GetEnumValue(uint32_t aIndex, nsIContent *aDefault)
{
  nsSVGEnum& thisEnum =
    static_cast<SVGFilterElement *>(mContent)->mEnumAttributes[aIndex];

  if (thisEnum.IsExplicitlySet())
    return thisEnum.GetAnimValue();

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetEnumValue(aIndex, aDefault) :
    static_cast<SVGFilterElement *>(aDefault)->
      mEnumAttributes[aIndex].GetAnimValue();
}

const nsSVGIntegerPair *
nsSVGFilterFrame::GetIntegerPairValue(uint32_t aIndex, nsIContent *aDefault)
{
  const nsSVGIntegerPair *thisIntegerPair =
    &static_cast<SVGFilterElement *>(mContent)->mIntegerPairAttributes[aIndex];

  if (thisIntegerPair->IsExplicitlySet())
    return thisIntegerPair;

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetIntegerPairValue(aIndex, aDefault) :
    &static_cast<SVGFilterElement *>(aDefault)->mIntegerPairAttributes[aIndex];
}

const nsSVGLength2 *
nsSVGFilterFrame::GetLengthValue(uint32_t aIndex, nsIContent *aDefault)
{
  const nsSVGLength2 *thisLength =
    &static_cast<SVGFilterElement *>(mContent)->mLengthAttributes[aIndex];

  if (thisLength->IsExplicitlySet())
    return thisLength;

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetLengthValue(aIndex, aDefault) :
    &static_cast<SVGFilterElement *>(aDefault)->mLengthAttributes[aIndex];
}

const SVGFilterElement *
nsSVGFilterFrame::GetFilterContent(nsIContent *aDefault)
{
  for (nsIContent* child = mContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    nsRefPtr<nsSVGFE> primitive;
    CallQueryInterface(child, (nsSVGFE**)getter_AddRefs(primitive));
    if (primitive) {
      return static_cast<SVGFilterElement *>(mContent);
    }
  }

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetFilterContent(aDefault) :
    static_cast<SVGFilterElement *>(aDefault);
}

nsSVGFilterFrame *
nsSVGFilterFrame::GetReferencedFilter()
{
  if (mNoHRefURI)
    return nullptr;

  nsSVGPaintingProperty *property = static_cast<nsSVGPaintingProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    
    SVGFilterElement *filter = static_cast<SVGFilterElement *>(mContent);
    nsAutoString href;
    filter->mStringAttributes[SVGFilterElement::HREF].GetAnimValue(href, filter);
    if (href.IsEmpty()) {
      mNoHRefURI = true;
      return nullptr; 
    }

    
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetPaintingProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nullptr;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nullptr;

  nsIAtom* frameType = result->GetType();
  if (frameType != nsGkAtoms::svgFilterFrame)
    return nullptr;

  return static_cast<nsSVGFilterFrame*>(result);
}

nsSVGFilterFrame *
nsSVGFilterFrame::GetReferencedFilterIfNotInUse()
{
  nsSVGFilterFrame *referenced = GetReferencedFilter();
  if (!referenced)
    return nullptr;

  if (referenced->mLoopFlag) {
    
    NS_WARNING("Filter reference loop detected while inheriting attribute!");
    return nullptr;
  }

  return referenced;
}

NS_IMETHODIMP
nsSVGFilterFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height ||
       aAttribute == nsGkAtoms::filterRes ||
       aAttribute == nsGkAtoms::filterUnits ||
       aAttribute == nsGkAtoms::primitiveUnits)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    mNoHRefURI = false;
    
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }
  return nsSVGFilterFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}

nsresult
nsSVGFilterFrame::PaintFilteredFrame(nsRenderingContext *aContext,
                                     nsIFrame *aFilteredFrame,
                                     nsSVGFilterPaintCallback *aPaintCallback,
                                     const nsRect *aDirtyArea)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, aPaintCallback,
                                aDirtyArea, nullptr, nullptr);
  if (!instance.get()) {
    return NS_OK;
  }
  nsRefPtr<gfxASurface> result;
  nsresult rv = instance.get()->Render(getter_AddRefs(result));
  if (NS_SUCCEEDED(rv) && result) {
    nsSVGUtils::CompositeSurfaceMatrix(aContext->ThebesContext(),
      result, instance.get()->GetFilterSpaceToDeviceSpaceTransform(), 1.0);
  }
  return rv;
}

static nsRect
TransformFilterSpaceToFrameSpace(nsSVGFilterInstance *aInstance,
                                 nsIntRect *aRect)
{
  gfxMatrix m = aInstance->GetFilterSpaceToFrameSpaceInCSSPxTransform();
  gfxRect r(aRect->x, aRect->y, aRect->width, aRect->height);
  r = m.TransformBounds(r);
  return nsLayoutUtils::RoundGfxRectToAppRect(r, aInstance->AppUnitsPerCSSPixel());
}

nsRect
nsSVGFilterFrame::GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                         const nsRect& aPreFilterDirtyRect)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, nullptr, nullptr,
                                &aPreFilterDirtyRect, nullptr);
  if (!instance.get()) {
    return nsRect();
  }
  
  
  
  nsIntRect dirtyRect;
  nsresult rv = instance.get()->ComputePostFilterDirtyRect(&dirtyRect);
  if (NS_SUCCEEDED(rv)) {
    return TransformFilterSpaceToFrameSpace(instance.get(), &dirtyRect);
  }
  return nsRect();
}

nsRect
nsSVGFilterFrame::GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                         const nsRect& aPostFilterDirtyRect)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, nullptr,
                                &aPostFilterDirtyRect, nullptr, nullptr);
  if (!instance.get()) {
    return nsRect();
  }
  
  
  nsIntRect neededRect;
  nsresult rv = instance.get()->ComputeSourceNeededRect(&neededRect);
  if (NS_SUCCEEDED(rv)) {
    return TransformFilterSpaceToFrameSpace(instance.get(), &neededRect);
  }
  return nsRect();
}

nsRect
nsSVGFilterFrame::GetPostFilterBounds(nsIFrame *aFilteredFrame,
                                      const gfxRect *aOverrideBBox,
                                      const nsRect *aPreFilterBounds)
{
  MOZ_ASSERT(!(aFilteredFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) ||
             !(aFilteredFrame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
             "Non-display SVG do not maintain visual overflow rects");

  nsAutoFilterInstance instance(aFilteredFrame, this, nullptr, nullptr,
                                aPreFilterBounds, aPreFilterBounds,
                                aOverrideBBox);
  if (!instance.get()) {
    return nsRect();
  }
  
  
  
  nsIntRect bbox;
  nsresult rv = instance.get()->ComputeOutputBBox(&bbox);
  if (NS_SUCCEEDED(rv)) {
    return TransformFilterSpaceToFrameSpace(instance.get(), &bbox);
  }
  return nsRect();
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGFilterFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::filter),
               "Content is not an SVG filter");

  return nsSVGFilterFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGFilterFrame::GetType() const
{
  return nsGkAtoms::svgFilterFrame;
}

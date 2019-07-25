





#include "nsSVGFilterFrame.h"


#include "gfxASurface.h"
#include "gfxUtils.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGFilterElement.h"
#include "nsSVGFilterInstance.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGUtils.h"

nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGFilterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGFilterFrame)

static nsIntRect
MapDeviceRectToFilterSpace(const gfxMatrix& aMatrix,
                           const gfxIntSize& aFilterSize,
                           const nsIntRect* aDeviceRect)
{
  nsIntRect rect(0, 0, aFilterSize.width, aFilterSize.height);
  if (aDeviceRect) {
    gfxRect r = aMatrix.TransformBounds(gfxRect(aDeviceRect->x, aDeviceRect->y,
                                                aDeviceRect->width, aDeviceRect->height));
    r.RoundOut();
    nsIntRect intRect;
    if (gfxUtils::GfxRectToIntRect(r, &intRect)) {
      rect = intRect;
    }
  }
  return rect;
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
                       const nsIntRect *aPostFilterDirtyRect,
                       const nsIntRect *aPreFilterDirtyRect,
                       const nsIntRect *aOverrideSourceBBox,
                       const gfxMatrix *aOverrideUserToDeviceSpace = nsnull);
  ~nsAutoFilterInstance() {}

  
  
  nsSVGFilterInstance* get() { return mInstance; }

private:
  nsAutoPtr<nsSVGFilterInstance> mInstance;
};

nsAutoFilterInstance::nsAutoFilterInstance(nsIFrame *aTarget,
                                           nsSVGFilterFrame *aFilterFrame,
                                           nsSVGFilterPaintCallback *aPaint,
                                           const nsIntRect *aPostFilterDirtyRect,
                                           const nsIntRect *aPreFilterDirtyRect,
                                           const nsIntRect *aOverrideSourceBBox,
                                           const gfxMatrix *aOverrideUserToDeviceSpace)
{
  const nsSVGFilterElement *filter = aFilterFrame->GetFilterContent();

  PRUint16 filterUnits =
    aFilterFrame->GetEnumValue(nsSVGFilterElement::FILTERUNITS);
  PRUint16 primitiveUnits =
    aFilterFrame->GetEnumValue(nsSVGFilterElement::PRIMITIVEUNITS);

  gfxRect bbox;
  if (aOverrideSourceBBox) {
    bbox = gfxRect(aOverrideSourceBBox->x, aOverrideSourceBBox->y,
                   aOverrideSourceBBox->width, aOverrideSourceBBox->height);
  } else {
    bbox = nsSVGUtils::GetBBox(aTarget);
  }

  

  
  
  
  
  
  
  
  
  
  
  nsSVGLength2 XYWH[4];
  NS_ABORT_IF_FALSE(sizeof(filter->mLengthAttributes) == sizeof(XYWH),
                    "XYWH size incorrect");
  memcpy(XYWH, filter->mLengthAttributes, sizeof(filter->mLengthAttributes));
  XYWH[0] = *aFilterFrame->GetLengthValue(nsSVGFilterElement::X);
  XYWH[1] = *aFilterFrame->GetLengthValue(nsSVGFilterElement::Y);
  XYWH[2] = *aFilterFrame->GetLengthValue(nsSVGFilterElement::WIDTH);
  XYWH[3] = *aFilterFrame->GetLengthValue(nsSVGFilterElement::HEIGHT);
  gfxRect filterRegion = nsSVGUtils::GetRelativeRect(filterUnits,
    XYWH, bbox, aTarget);

  if (filterRegion.Width() <= 0 || filterRegion.Height() <= 0) {
    
    
    return;
  }

  gfxMatrix userToDeviceSpace;
  if (aOverrideUserToDeviceSpace) {
    userToDeviceSpace = *aOverrideUserToDeviceSpace;
  } else {
    userToDeviceSpace = nsSVGUtils::GetCanvasTM(aTarget);
  }
  
  
  

  gfxIntSize filterRes;
  const nsSVGIntegerPair* filterResAttrs =
    aFilterFrame->GetIntegerPairValue(nsSVGFilterElement::FILTERRES);
  if (filterResAttrs->IsExplicitlySet()) {
    PRInt32 filterResX = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eFirst);
    PRInt32 filterResY = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eSecond);
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
    
    
    gfxMatrix canvasTM = nsSVGUtils::GetCanvasTM(aTarget);
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
  gfxMatrix filterToDeviceSpace = filterToUserSpace * userToDeviceSpace;
  
  
  gfxMatrix deviceToFilterSpace = filterToDeviceSpace;
  deviceToFilterSpace.Invert();

  nsIntRect postFilterDirtyRect =
    MapDeviceRectToFilterSpace(deviceToFilterSpace, filterRes, aPostFilterDirtyRect);
  nsIntRect preFilterDirtyRect =
    MapDeviceRectToFilterSpace(deviceToFilterSpace, filterRes, aPreFilterDirtyRect);
  nsIntRect targetBoundsDeviceSpace;
  nsISVGChildFrame* svgTarget = do_QueryFrame(aTarget);
  if (svgTarget) {
    if (aOverrideUserToDeviceSpace) {
      
      
      
      
      
      
      NS_ASSERTION(aPreFilterDirtyRect, "Who passed aOverrideUserToDeviceSpace?");
      targetBoundsDeviceSpace = *aPreFilterDirtyRect;
    } else {
      targetBoundsDeviceSpace =
        svgTarget->GetCoveredRegion().ToOutsidePixels(aTarget->
          PresContext()->AppUnitsPerDevPixel());
    }
  }
  nsIntRect targetBoundsFilterSpace =
    MapDeviceRectToFilterSpace(deviceToFilterSpace, filterRes, &targetBoundsDeviceSpace);

  
  mInstance = new nsSVGFilterInstance(aTarget, aPaint, filter, bbox, filterRegion,
                                      nsIntSize(filterRes.width, filterRes.height),
                                      filterToDeviceSpace, targetBoundsFilterSpace,
                                      postFilterDirtyRect, preFilterDirtyRect,
                                      primitiveUnits);
}

PRUint16
nsSVGFilterFrame::GetEnumValue(PRUint32 aIndex, nsIContent *aDefault)
{
  nsSVGEnum& thisEnum =
    static_cast<nsSVGFilterElement *>(mContent)->mEnumAttributes[aIndex];

  if (thisEnum.IsExplicitlySet())
    return thisEnum.GetAnimValue();

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetEnumValue(aIndex, aDefault) :
    static_cast<nsSVGFilterElement *>(aDefault)->
      mEnumAttributes[aIndex].GetAnimValue();
}

const nsSVGIntegerPair *
nsSVGFilterFrame::GetIntegerPairValue(PRUint32 aIndex, nsIContent *aDefault)
{
  const nsSVGIntegerPair *thisIntegerPair =
    &static_cast<nsSVGFilterElement *>(mContent)->mIntegerPairAttributes[aIndex];

  if (thisIntegerPair->IsExplicitlySet())
    return thisIntegerPair;

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetIntegerPairValue(aIndex, aDefault) :
    &static_cast<nsSVGFilterElement *>(aDefault)->mIntegerPairAttributes[aIndex];
}

const nsSVGLength2 *
nsSVGFilterFrame::GetLengthValue(PRUint32 aIndex, nsIContent *aDefault)
{
  const nsSVGLength2 *thisLength =
    &static_cast<nsSVGFilterElement *>(mContent)->mLengthAttributes[aIndex];

  if (thisLength->IsExplicitlySet())
    return thisLength;

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetLengthValue(aIndex, aDefault) :
    &static_cast<nsSVGFilterElement *>(aDefault)->mLengthAttributes[aIndex];
}

const nsSVGFilterElement *
nsSVGFilterFrame::GetFilterContent(nsIContent *aDefault)
{
  for (nsIContent* child = mContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    nsRefPtr<nsSVGFE> primitive;
    CallQueryInterface(child, (nsSVGFE**)getter_AddRefs(primitive));
    if (primitive) {
      return static_cast<nsSVGFilterElement *>(mContent);
    }
  }

  AutoFilterReferencer filterRef(this);

  nsSVGFilterFrame *next = GetReferencedFilterIfNotInUse();
  return next ? next->GetFilterContent(aDefault) :
    static_cast<nsSVGFilterElement *>(aDefault);
}

nsSVGFilterFrame *
nsSVGFilterFrame::GetReferencedFilter()
{
  if (mNoHRefURI)
    return nsnull;

  nsSVGPaintingProperty *property = static_cast<nsSVGPaintingProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    
    nsSVGFilterElement *filter = static_cast<nsSVGFilterElement *>(mContent);
    nsAutoString href;
    filter->mStringAttributes[nsSVGFilterElement::HREF].GetAnimValue(href, filter);
    if (href.IsEmpty()) {
      mNoHRefURI = true;
      return nsnull; 
    }

    
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetPaintingProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nsnull;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nsnull;

  nsIAtom* frameType = result->GetType();
  if (frameType != nsGkAtoms::svgFilterFrame)
    return nsnull;

  return static_cast<nsSVGFilterFrame*>(result);
}

nsSVGFilterFrame *
nsSVGFilterFrame::GetReferencedFilterIfNotInUse()
{
  nsSVGFilterFrame *referenced = GetReferencedFilter();
  if (!referenced)
    return nsnull;

  if (referenced->mLoopFlag) {
    
    NS_WARNING("Filter reference loop detected while inheriting attribute!");
    return nsnull;
  }

  return referenced;
}

NS_IMETHODIMP
nsSVGFilterFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height ||
       aAttribute == nsGkAtoms::filterRes ||
       aAttribute == nsGkAtoms::filterUnits ||
       aAttribute == nsGkAtoms::primitiveUnits)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    mNoHRefURI = false;
    
    nsSVGEffects::InvalidateRenderingObservers(this);
  }
  return nsSVGFilterFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}

nsresult
nsSVGFilterFrame::PaintFilteredFrame(nsRenderingContext *aContext,
                                     nsIFrame *aFilteredFrame,
                                     nsSVGFilterPaintCallback *aPaintCallback,
                                     const nsIntRect *aDirtyArea)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, aPaintCallback,
                                aDirtyArea, nsnull, nsnull);
  if (!instance.get())
    return NS_OK;

  nsRefPtr<gfxASurface> result;
  nsresult rv = instance.get()->Render(getter_AddRefs(result));
  if (NS_SUCCEEDED(rv) && result) {
    nsSVGUtils::CompositeSurfaceMatrix(aContext->ThebesContext(),
      result, instance.get()->GetFilterSpaceToDeviceSpaceTransform(), 1.0);
  }
  return rv;
}

static nsresult
TransformFilterSpaceToDeviceSpace(nsSVGFilterInstance *aInstance,
                                  nsIntRect *aRect)
{
  gfxMatrix m = aInstance->GetFilterSpaceToDeviceSpaceTransform();
  gfxRect r(aRect->x, aRect->y, aRect->width, aRect->height);
  r = m.TransformBounds(r);
  r.RoundOut();
  nsIntRect deviceRect;
  if (!gfxUtils::GfxRectToIntRect(r, &deviceRect))
    return NS_ERROR_FAILURE;
  *aRect = deviceRect;
  return NS_OK;
}

nsIntRect
nsSVGFilterFrame::GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                         const nsIntRect& aPreFilterDirtyRect)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, nsnull, nsnull,
                                &aPreFilterDirtyRect, nsnull);
  if (!instance.get())
    return nsIntRect();

  
  
  
  nsIntRect dirtyRect;
  nsresult rv = instance.get()->ComputePostFilterDirtyRect(&dirtyRect);
  if (NS_SUCCEEDED(rv)) {
    rv = TransformFilterSpaceToDeviceSpace(instance.get(), &dirtyRect);
    if (NS_SUCCEEDED(rv))
      return dirtyRect;
  }

  return nsIntRect();
}

nsIntRect
nsSVGFilterFrame::GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                         const nsIntRect& aPostFilterDirtyRect)
{
  nsAutoFilterInstance instance(aFilteredFrame, this, nsnull,
                                &aPostFilterDirtyRect, nsnull, nsnull);
  if (!instance.get())
    return nsIntRect();

  
  
  nsIntRect neededRect;
  nsresult rv = instance.get()->ComputeSourceNeededRect(&neededRect);
  if (NS_SUCCEEDED(rv)) {
    rv = TransformFilterSpaceToDeviceSpace(instance.get(), &neededRect);
    if (NS_SUCCEEDED(rv))
      return neededRect;
  }

  return nsIntRect();
}

nsIntRect
nsSVGFilterFrame::GetPostFilterBounds(nsIFrame *aFilteredFrame,
                                      const nsIntRect *aOverrideBBox,
                                      const nsIntRect *aPreFilterBounds)
{
  bool overrideCTM = false;
  gfxMatrix ctm;

  if (aFilteredFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    
    
    
    overrideCTM = true;
    PRInt32 appUnitsPerDevPixel =
      aFilteredFrame->PresContext()->AppUnitsPerDevPixel();
    float devPxPerCSSPx =
      1 / nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPixel);
    ctm.Scale(devPxPerCSSPx, devPxPerCSSPx);
  }

  nsAutoFilterInstance instance(aFilteredFrame, this, nsnull, nsnull,
                                aPreFilterBounds, aOverrideBBox,
                                overrideCTM ? &ctm : nsnull);
  if (!instance.get())
    return nsIntRect();

  
  
  
  nsIntRect bbox;
  nsresult rv = instance.get()->ComputeOutputBBox(&bbox);
  if (NS_SUCCEEDED(rv)) {
    rv = TransformFilterSpaceToDeviceSpace(instance.get(), &bbox);
    if (NS_SUCCEEDED(rv))
      return bbox;
  }
  
  return nsIntRect();
}
  
#ifdef DEBUG
NS_IMETHODIMP
nsSVGFilterFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGFilterElement> filter = do_QueryInterface(aContent);
  NS_ASSERTION(filter, "Content is not an SVG filter");

  return nsSVGFilterFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGFilterFrame::GetType() const
{
  return nsGkAtoms::svgFilterFrame;
}

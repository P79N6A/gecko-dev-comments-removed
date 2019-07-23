



































#include "nsSVGFilterFrame.h"
#include "nsIDocument.h"
#include "nsSVGMatrix.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGFilterElement.h"
#include "nsSVGFilters.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGRect.h"
#include "nsSVGFilterInstance.h"

nsIFrame*
NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGFilterFrame(aContext);
}

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
    if (NS_SUCCEEDED(nsSVGUtils::GfxRectToIntRect(r, &intRect))) {
      rect = intRect;
    }
  }
  return rect;
}

class NS_STACK_CLASS nsAutoFilterInstance {
public:
  nsAutoFilterInstance(nsIFrame *aTarget,
                       nsSVGFilterFrame *aFilterFrame,
                       nsSVGFilterPaintCallback *aPaint,
                       const nsIntRect *aDirtyOutputRect,
                       const nsIntRect *aDirtyInputRect,
                       const nsIntRect *aOverrideSourceBBox);
  ~nsAutoFilterInstance();

  
  
  nsSVGFilterInstance* get() { return mInstance; }

private:
  nsAutoPtr<nsSVGFilterInstance> mInstance;
  
  
  nsISVGChildFrame*              mTarget;
};

nsAutoFilterInstance::nsAutoFilterInstance(nsIFrame *aTarget,
                                           nsSVGFilterFrame *aFilterFrame,
                                           nsSVGFilterPaintCallback *aPaint,
                                           const nsIntRect *aDirtyOutputRect,
                                           const nsIntRect *aDirtyInputRect,
                                           const nsIntRect *aOverrideSourceBBox)
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm = nsSVGUtils::GetCanvasTM(aTarget);

  mTarget = do_QueryFrame(aTarget);
  if (mTarget) {
    mTarget->SetMatrixPropagation(PR_FALSE);
    mTarget->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION |
                              nsISVGChildFrame::TRANSFORM_CHANGED);
  }

  nsSVGFilterElement *filter = static_cast<nsSVGFilterElement*>(
          aFilterFrame->GetContent());

  PRUint16 units =
    filter->mEnumAttributes[nsSVGFilterElement::FILTERUNITS].GetAnimValue();
  PRUint16 primitiveUnits =
    filter->mEnumAttributes[nsSVGFilterElement::PRIMITIVEUNITS].GetAnimValue();
  nsCOMPtr<nsIDOMSVGRect> bbox;
  if (aOverrideSourceBBox) {
    NS_NewSVGRect(getter_AddRefs(bbox),
                  aOverrideSourceBBox->x, aOverrideSourceBBox->y,
                  aOverrideSourceBBox->width, aOverrideSourceBBox->height);
  } else {
    bbox = nsSVGUtils::GetBBox(aTarget);
  }
  if (!bbox && (units == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX ||
                primitiveUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX))
    return;

  gfxRect filterArea = nsSVGUtils::GetRelativeRect(units,
    &filter->mLengthAttributes[nsSVGFilterElement::X], bbox, aTarget);
  filterArea.RoundOut();

  
  if (filterArea.Width() <= 0 || filterArea.Height() <= 0)
    return;

  PRBool resultOverflows;
  gfxIntSize filterRes;

  
  if (filter->HasAttr(kNameSpaceID_None, nsGkAtoms::filterRes)) {
    PRInt32 filterResX, filterResY;
    filter->GetAnimatedIntegerValues(&filterResX, &filterResY, nsnull);

    filterRes =
      nsSVGUtils::ConvertToSurfaceSize(gfxSize(filterResX, filterResY),
                                       &resultOverflows);
  } else {
    float scale = nsSVGUtils::MaxExpansion(ctm);
#ifdef DEBUG_tor
    fprintf(stderr, "scale: %f\n", scale);
#endif

    filterRes =
      nsSVGUtils::ConvertToSurfaceSize(filterArea.size * scale,
                                       &resultOverflows);
  }

  
  if (filterRes.width <= 0 || filterRes.height <= 0)
    return;

  
  
  nsCOMPtr<nsIDOMSVGMatrix> scale, fini;
  NS_NewSVGMatrix(getter_AddRefs(scale),
                  filterArea.Width() / filterRes.width, 0.0f,
                  0.0f, filterArea.Height() / filterRes.height,
                  filterArea.X(), filterArea.Y());
  ctm->Multiply(scale, getter_AddRefs(fini));
  
  gfxMatrix finiM = nsSVGUtils::ConvertSVGMatrixToThebes(fini);
  
  finiM.Invert();

  nsIntRect dirtyOutputRect =
    MapDeviceRectToFilterSpace(finiM, filterRes, aDirtyOutputRect);
  nsIntRect dirtyInputRect =
    MapDeviceRectToFilterSpace(finiM, filterRes, aDirtyInputRect);

  
  mInstance = new nsSVGFilterInstance(aTarget, aPaint, filter, bbox, filterArea,
                                      nsIntSize(filterRes.width, filterRes.height),
                                      fini,
                                      dirtyOutputRect, dirtyInputRect,
                                      primitiveUnits);
}

nsAutoFilterInstance::~nsAutoFilterInstance()
{
  if (!mTarget)
    return;

  mTarget->SetMatrixPropagation(PR_TRUE);
  mTarget->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION |
                            nsISVGChildFrame::TRANSFORM_CHANGED);
}

nsresult
nsSVGFilterFrame::FilterPaint(nsSVGRenderState *aContext,
                              nsIFrame *aTarget,
                              nsSVGFilterPaintCallback *aPaintCallback,
                              const nsIntRect *aDirtyRect)
{
  nsAutoFilterInstance instance(aTarget, this, aPaintCallback,
    aDirtyRect, nsnull, nsnull);
  if (!instance.get())
    return NS_OK;

  nsRefPtr<gfxASurface> result;
  nsresult rv = instance.get()->Render(getter_AddRefs(result));
  if (NS_SUCCEEDED(rv) && result) {
    nsSVGUtils::CompositeSurfaceMatrix(aContext->GetGfxContext(),
      result, instance.get()->GetFilterSpaceToDeviceSpaceTransform(), 1.0);
  }
  return rv;
}

static nsresult
TransformFilterSpaceToDeviceSpace(nsSVGFilterInstance *aInstance, nsIntRect *aRect)
{
  gfxMatrix m = nsSVGUtils::ConvertSVGMatrixToThebes(
    aInstance->GetFilterSpaceToDeviceSpaceTransform());
  gfxRect r(aRect->x, aRect->y, aRect->width, aRect->height);
  r = m.TransformBounds(r);
  r.RoundOut();
  nsIntRect deviceRect;
  nsresult rv = nsSVGUtils::GfxRectToIntRect(r, &deviceRect);
  if (NS_FAILED(rv))
    return rv;
  *aRect = deviceRect;
  return NS_OK;
}

nsIntRect
nsSVGFilterFrame::GetInvalidationBBox(nsIFrame *aTarget, const nsIntRect& aRect)
{
  nsAutoFilterInstance instance(aTarget, this, nsnull, nsnull, &aRect, nsnull);
  if (!instance.get())
    return nsIntRect();

  
  
  
  nsIntRect dirtyRect;
  nsresult rv = instance.get()->ComputeOutputDirtyRect(&dirtyRect);
  if (NS_SUCCEEDED(rv)) {
    rv = TransformFilterSpaceToDeviceSpace(instance.get(), &dirtyRect);
    if (NS_SUCCEEDED(rv))
      return dirtyRect;
  }

  return nsIntRect();
}

nsIntRect
nsSVGFilterFrame::GetSourceForInvalidArea(nsIFrame *aTarget, const nsIntRect& aRect)
{
  nsAutoFilterInstance instance(aTarget, this, nsnull, &aRect, nsnull, nsnull);
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
nsSVGFilterFrame::GetFilterBBox(nsIFrame *aTarget, const nsIntRect *aSourceBBox)
{
  nsAutoFilterInstance instance(aTarget, this, nsnull, nsnull, nsnull, aSourceBBox);
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

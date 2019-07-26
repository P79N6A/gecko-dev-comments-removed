





#include "nsSVGMaskFrame.h"


#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGMaskElement.h"

using namespace mozilla::dom;




nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMaskFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMaskFrame)

already_AddRefed<gfxPattern>
nsSVGMaskFrame::ComputeMaskAlpha(nsRenderingContext *aContext,
                                 nsIFrame* aParent,
                                 const gfxMatrix &aMatrix,
                                 float aOpacity)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Mask loop detected!");
    return nullptr;
  }
  AutoMaskReferencer maskRef(this);

  SVGMaskElement *mask = static_cast<SVGMaskElement*>(mContent);

  uint16_t units =
    mask->mEnumAttributes[SVGMaskElement::MASKUNITS].GetAnimValue();
  gfxRect bbox;
  if (units == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    bbox = nsSVGUtils::GetBBox(aParent);
  }

  gfxRect maskArea = nsSVGUtils::GetRelativeRect(units,
    &mask->mLengthAttributes[SVGMaskElement::ATTR_X], bbox, aParent);

  gfxContext *gfx = aContext->ThebesContext();

  
  gfx->Save();
  nsSVGUtils::SetClipRect(gfx, aMatrix, maskArea);
  gfx->IdentityMatrix();
  gfxRect clipExtents = gfx->GetClipExtents();
  clipExtents.RoundOut();
  gfx->Restore();

  bool resultOverflows;
  gfxIntSize surfaceSize =
    nsSVGUtils::ConvertToSurfaceSize(gfxSize(clipExtents.Width(),
                                             clipExtents.Height()),
                                     &resultOverflows);

  
  if (surfaceSize.width <= 0 || surfaceSize.height <= 0)
    return nullptr;

  if (resultOverflows)
    return nullptr;

  nsRefPtr<gfxImageSurface> image =
    new gfxImageSurface(surfaceSize, gfxASurface::ImageFormatARGB32);
  if (!image || image->CairoStatus())
    return nullptr;

  
  
  
  
  
  
  gfxMatrix matrix =
    gfx->CurrentMatrix() * gfxMatrix().Translate(-clipExtents.TopLeft());

  nsRenderingContext tmpCtx;
  tmpCtx.Init(this->PresContext()->DeviceContext(), image);
  tmpCtx.ThebesContext()->SetMatrix(matrix);

  mMaskParent = aParent;
  if (mMaskParentMatrix) {
    *mMaskParentMatrix = aMatrix;
  } else {
    mMaskParentMatrix = new gfxMatrix(aMatrix);
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);
    }
    nsSVGUtils::PaintFrameWithEffects(&tmpCtx, nullptr, kid);
  }

  uint8_t *data   = image->Data();
  int32_t  stride = image->Stride();
  nsIntRect rect(0, 0, surfaceSize.width, surfaceSize.height);

  if (StyleSVGReset()->mMaskType == NS_STYLE_MASK_TYPE_LUMINANCE) {
    if (StyleSVG()->mColorInterpolation ==
        NS_STYLE_COLOR_INTERPOLATION_LINEARRGB) {
      nsSVGUtils::ComputeLinearRGBLuminanceMask(data, stride, rect, aOpacity);
    } else {
      nsSVGUtils::ComputesRGBLuminanceMask(data, stride, rect, aOpacity);
    }
  } else {
    nsSVGUtils::ComputeAlphaMask(data, stride, rect, aOpacity);
  }

  gfxPattern *retval = new gfxPattern(image);
  retval->SetMatrix(matrix);
  NS_IF_ADDREF(retval);
  return retval;
}

 void
nsSVGMaskFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGEffects::InvalidateDirectRenderingObservers(this);
  nsSVGMaskFrameBase::DidSetStyleContext(aOldStyleContext);
}

NS_IMETHODIMP
nsSVGMaskFrame::AttributeChanged(int32_t  aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height||
       aAttribute == nsGkAtoms::maskUnits ||
       aAttribute == nsGkAtoms::maskContentUnits)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGMaskFrameBase::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGMaskFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::mask),
               "Content is not an SVG mask");

  return nsSVGMaskFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMaskFrame::GetType() const
{
  return nsGkAtoms::svgMaskFrame;
}

gfxMatrix
nsSVGMaskFrame::GetCanvasTM(uint32_t aFor)
{
  NS_ASSERTION(mMaskParentMatrix, "null parent matrix");

  SVGMaskElement *mask = static_cast<SVGMaskElement*>(mContent);

  return nsSVGUtils::AdjustMatrixForUnits(
    mMaskParentMatrix ? *mMaskParentMatrix : gfxMatrix(),
    &mask->mEnumAttributes[SVGMaskElement::MASKCONTENTUNITS],
    mMaskParent);
}


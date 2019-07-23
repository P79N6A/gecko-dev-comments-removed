



































#include "nsIDocument.h"
#include "nsSVGMaskFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGMaskElement.h"
#include "nsIDOMSVGMatrix.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsSVGMatrix.h"




nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMaskFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMaskFrame)

already_AddRefed<gfxPattern>
nsSVGMaskFrame::ComputeMaskAlpha(nsSVGRenderState *aContext,
                                 nsIFrame* aParent,
                                 const gfxMatrix &aMatrix,
                                 float aOpacity)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Mask loop detected!");
    return nsnull;
  }
  AutoMaskReferencer maskRef(this);

  gfxContext *gfx = aContext->GetGfxContext();

  gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

  {
    nsSVGMaskElement *mask = static_cast<nsSVGMaskElement*>(mContent);

    PRUint16 units =
      mask->mEnumAttributes[nsSVGMaskElement::MASKUNITS].GetAnimValue();
    gfxRect bbox;
    if (units == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
      bbox = nsSVGUtils::GetBBox(aParent);
    }

    gfxRect maskArea = nsSVGUtils::GetRelativeRect(units,
      &mask->mLengthAttributes[nsSVGMaskElement::X], bbox, aParent);

    gfx->Save();
    nsSVGUtils::SetClipRect(gfx, aMatrix, maskArea);
  }

  mMaskParent = aParent;
  mMaskParentMatrix = NS_NewSVGMatrix(aMatrix);

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintFrameWithEffects(aContext, nsnull, kid);
  }

  gfxRect clipExtents = gfx->GetClipExtents();
  gfx->Restore();

  nsRefPtr<gfxPattern> pattern = gfx->PopGroup();
  if (!pattern || pattern->CairoStatus())
    return nsnull;

#ifdef DEBUG_tor
  fprintf(stderr, "clip extent: %f,%f %fx%f\n",
          clipExtents.X(), clipExtents.Y(),
          clipExtents.Width(), clipExtents.Height());
#endif

  PRBool resultOverflows;
  gfxIntSize surfaceSize =
    nsSVGUtils::ConvertToSurfaceSize(gfxSize(clipExtents.Width(),
                                             clipExtents.Height()),
                                     &resultOverflows);

  
  if (surfaceSize.width <= 0 || surfaceSize.height <= 0)
    return nsnull;

  if (resultOverflows)
    return nsnull;

  nsRefPtr<gfxImageSurface> image =
    new gfxImageSurface(surfaceSize, gfxASurface::ImageFormatARGB32);
  if (!image || image->CairoStatus())
    return nsnull;
  image->SetDeviceOffset(-clipExtents.pos);

  gfxContext transferCtx(image);
  transferCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
  transferCtx.SetPattern(pattern);
  transferCtx.Paint();

  PRUint8 *data   = image->Data();
  PRInt32  stride = image->Stride();

  nsIntRect rect(0, 0, surfaceSize.width, surfaceSize.height);
  nsSVGUtils::UnPremultiplyImageDataAlpha(data, stride, rect);
  nsSVGUtils::ConvertImageDataToLinearRGB(data, stride, rect);

  for (PRInt32 y = 0; y < surfaceSize.height; y++)
    for (PRInt32 x = 0; x < surfaceSize.width; x++) {
      PRUint8 *pixel = data + stride * y + 4 * x;

      
      PRUint8 alpha =
        static_cast<PRUint8>
                   ((pixel[GFX_ARGB32_OFFSET_R] * 0.2125 +
                        pixel[GFX_ARGB32_OFFSET_G] * 0.7154 +
                        pixel[GFX_ARGB32_OFFSET_B] * 0.0721) *
                       (pixel[GFX_ARGB32_OFFSET_A] / 255.0) * aOpacity);

      memset(pixel, alpha, 4);
    }

  gfxPattern *retval = new gfxPattern(image);
  NS_IF_ADDREF(retval);
  return retval;
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGMaskFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGMaskElement> mask = do_QueryInterface(aContent);
  NS_ASSERTION(mask, "Content is not an SVG mask");

  return nsSVGMaskFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMaskFrame::GetType() const
{
  return nsGkAtoms::svgMaskFrame;
}

gfxMatrix
nsSVGMaskFrame::GetCanvasTM()
{
  NS_ASSERTION(mMaskParentMatrix, "null parent matrix");

  nsSVGMaskElement *mask = static_cast<nsSVGMaskElement*>(mContent);

  return nsSVGUtils::AdjustMatrixForUnits(nsSVGUtils::ConvertSVGMatrixToThebes(mMaskParentMatrix),
                                          &mask->mEnumAttributes[nsSVGMaskElement::MASKCONTENTUNITS],
                                          mMaskParent);
}


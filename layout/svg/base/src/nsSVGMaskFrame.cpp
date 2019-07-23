



































#include "nsIDocument.h"
#include "nsSVGMaskFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGMaskElement.h"
#include "nsIDOMSVGMatrix.h"
#include "gfxContext.h"
#include "nsIDOMSVGRect.h"
#include "gfxImageSurface.h"




nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMaskFrame(aContext);
}

nsIContent *
NS_GetSVGMaskElement(nsIURI *aURI, nsIContent *aContent)
{
  nsIContent* content = nsContentUtils::GetReferencedElement(aURI, aContent);

  nsCOMPtr<nsIDOMSVGMaskElement> mask = do_QueryInterface(content);

  if (mask)
    return content;

  return nsnull;
}

NS_IMETHODIMP
nsSVGMaskFrame::InitSVG()
{
  nsresult rv = nsSVGMaskFrameBase::InitSVG();
  if (NS_FAILED(rv))
    return rv;

  mMaskParentMatrix = nsnull;
  mInUse = PR_FALSE;

  nsCOMPtr<nsIDOMSVGMaskElement> mask = do_QueryInterface(mContent);
  NS_ASSERTION(mask, "wrong content element");

  return NS_OK;
}


already_AddRefed<gfxPattern>
nsSVGMaskFrame::ComputeMaskAlpha(nsSVGRenderState *aContext,
                                 nsISVGChildFrame* aParent,
                                 nsIDOMSVGMatrix* aMatrix,
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
    nsIFrame *frame;
    CallQueryInterface(aParent, &frame);
    nsSVGElement *parent = static_cast<nsSVGElement*>(frame->GetContent());

    float x, y, width, height;

    nsSVGMaskElement *mask = static_cast<nsSVGMaskElement*>(mContent);

    nsSVGLength2 *tmpX, *tmpY, *tmpWidth, *tmpHeight;
    tmpX = &mask->mLengthAttributes[nsSVGMaskElement::X];
    tmpY = &mask->mLengthAttributes[nsSVGMaskElement::Y];
    tmpWidth = &mask->mLengthAttributes[nsSVGMaskElement::WIDTH];
    tmpHeight = &mask->mLengthAttributes[nsSVGMaskElement::HEIGHT];

    PRUint16 units =
      mask->mEnumAttributes[nsSVGMaskElement::MASKUNITS].GetAnimValue();

    if (units == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {

      aParent->SetMatrixPropagation(PR_FALSE);
      aParent->NotifyCanvasTMChanged(PR_TRUE);

      nsCOMPtr<nsIDOMSVGRect> bbox;
      aParent->GetBBox(getter_AddRefs(bbox));

      aParent->SetMatrixPropagation(PR_TRUE);
      aParent->NotifyCanvasTMChanged(PR_TRUE);

      if (!bbox)
        return nsnull;

#ifdef DEBUG_tor
      bbox->GetX(&x);
      bbox->GetY(&y);
      bbox->GetWidth(&width);
      bbox->GetHeight(&height);

      fprintf(stderr, "mask bbox: %f,%f %fx%f\n", x, y, width, height);
#endif

      bbox->GetX(&x);
      x += nsSVGUtils::ObjectSpace(bbox, tmpX);
      bbox->GetY(&y);
      y += nsSVGUtils::ObjectSpace(bbox, tmpY);
      width = nsSVGUtils::ObjectSpace(bbox, tmpWidth);
      height = nsSVGUtils::ObjectSpace(bbox, tmpHeight);
    } else {
      x = nsSVGUtils::UserSpace(parent, tmpX);
      y = nsSVGUtils::UserSpace(parent, tmpY);
      width = nsSVGUtils::UserSpace(parent, tmpWidth);
      height = nsSVGUtils::UserSpace(parent, tmpHeight);
    }

#ifdef DEBUG_tor
    fprintf(stderr, "mask clip: %f,%f %fx%f\n", x, y, width, height);
#endif

    gfx->Save();
    nsSVGUtils::SetClipRect(gfx, aMatrix, x, y, width, height);
  }

  mMaskParent = aParent,
  mMaskParentMatrix = aMatrix;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintChildWithEffects(aContext, nsnull, kid);
  }

  gfx->Restore();

  nsRefPtr<gfxPattern> pattern = gfx->PopGroup();
  if (!pattern)
    return nsnull;

  nsRefPtr<gfxASurface> surface = pattern->GetSurface();

  gfxRect clipExtents = gfx->GetClipExtents();

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

  gfxContext transferCtx(image);
  transferCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
  transferCtx.SetSource(surface, -clipExtents.pos);
  transferCtx.Paint();

  PRUint8 *data   = image->Data();
  PRInt32  stride = image->Stride();

  nsRect rect(0, 0, surfaceSize.width, surfaceSize.height);
  nsSVGUtils::UnPremultiplyImageDataAlpha(data, stride, rect);
  nsSVGUtils::ConvertImageDataToLinearRGB(data, stride, rect);

  for (PRUint32 y = 0; y < surfaceSize.height; y++)
    for (PRUint32 x = 0; x < surfaceSize.width; x++) {
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
  if (retval) {
    retval->SetMatrix(gfxMatrix().Translate(-clipExtents.pos));
    NS_ADDREF(retval);
  }
  return retval;
}

nsIAtom *
nsSVGMaskFrame::GetType() const
{
  return nsGkAtoms::svgMaskFrame;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGMaskFrame::GetCanvasTM()
{
  NS_ASSERTION(mMaskParentMatrix, "null parent matrix");

  nsCOMPtr<nsIDOMSVGMatrix> canvasTM = mMaskParentMatrix;

  
  nsSVGMaskElement *mask = static_cast<nsSVGMaskElement*>(mContent);

  PRUint16 contentUnits =
    mask->mEnumAttributes[nsSVGMaskElement::MASKCONTENTUNITS].GetAnimValue();

  if (mMaskParent &&
      contentUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    nsCOMPtr<nsIDOMSVGRect> rect;
    nsresult rv = mMaskParent->GetBBox(getter_AddRefs(rect));

    if (NS_SUCCEEDED(rv)) {
      float minx, miny, width, height;
      rect->GetX(&minx);
      rect->GetY(&miny);
      rect->GetWidth(&width);
      rect->GetHeight(&height);

      nsCOMPtr<nsIDOMSVGMatrix> tmp, fini;
      canvasTM->Translate(minx, miny, getter_AddRefs(tmp));
      tmp->ScaleNonUniform(width, height, getter_AddRefs(fini));
      canvasTM = fini;
    }
  }

  nsIDOMSVGMatrix* retval = canvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
}


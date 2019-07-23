



































#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGRect.h"
#include "nsIDocument.h"
#include "nsSVGMarkerFrame.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsSVGMatrix.h"
#include "nsSVGEffects.h"
#include "nsSVGMarkerElement.h"
#include "nsSVGPathGeometryElement.h"
#include "gfxContext.h"

nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerFrame(aContext);
}




NS_IMETHODIMP
nsSVGMarkerFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::markerUnits ||
       aAttribute == nsGkAtoms::refX ||
       aAttribute == nsGkAtoms::refY ||
       aAttribute == nsGkAtoms::markerWidth ||
       aAttribute == nsGkAtoms::markerHeight ||
       aAttribute == nsGkAtoms::orient ||
       aAttribute == nsGkAtoms::preserveAspectRatio ||
       aAttribute == nsGkAtoms::viewBox)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGMarkerFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGMarkerFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGMarkerElement> marker = do_QueryInterface(aContent);
  NS_ASSERTION(marker, "Content is not an SVG marker");

  return nsSVGMarkerFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMarkerFrame::GetType() const
{
  return nsGkAtoms::svgMarkerFrame;
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGMarkerFrame::GetCanvasTM()
{
  if (mInUse2) {
    
    
    
    nsCOMPtr<nsIDOMSVGMatrix> ident;
    NS_NewSVGMatrix(getter_AddRefs(ident));

    nsIDOMSVGMatrix *retval = ident.get();
    NS_IF_ADDREF(retval);
    return retval;
  }

  mInUse2 = PR_TRUE;

  

  NS_ASSERTION(mMarkedFrame, "null nsSVGPathGeometry frame");
  nsCOMPtr<nsIDOMSVGMatrix> markedTM;
  mMarkedFrame->GetCanvasTM(getter_AddRefs(markedTM));
  NS_ASSERTION(markedTM, "null marked TM");

  
  nsSVGMarkerElement *element = static_cast<nsSVGMarkerElement*>(mContent);

  
  nsCOMPtr<nsIDOMSVGMatrix> markerTM;
  element->GetMarkerTransform(mStrokeWidth, mX, mY, mAngle, getter_AddRefs(markerTM));

  
  nsCOMPtr<nsIDOMSVGMatrix> viewBoxTM;
  nsresult res =
    element->GetViewboxToViewportTransform(getter_AddRefs(viewBoxTM));

  nsCOMPtr<nsIDOMSVGMatrix> tmpTM;
  nsCOMPtr<nsIDOMSVGMatrix> resultTM;

  markedTM->Multiply(markerTM, getter_AddRefs(tmpTM));

  if (NS_SUCCEEDED(res) && viewBoxTM) {
    tmpTM->Multiply(viewBoxTM, getter_AddRefs(resultTM));
  } else {
    NS_WARNING("We should propagate the fact that the viewBox is invalid.");
    resultTM = tmpTM;
  }

  nsIDOMSVGMatrix *retval = resultTM.get();
  NS_IF_ADDREF(retval);

  mInUse2 = PR_FALSE;

  return retval;
}


nsresult
nsSVGMarkerFrame::PaintMark(nsSVGRenderState *aContext,
                            nsSVGPathGeometryFrame *aMarkedFrame,
                            nsSVGMark *aMark, float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return NS_OK;

  nsSVGMarkerElement *marker = static_cast<nsSVGMarkerElement*>(mContent);

  nsCOMPtr<nsIDOMSVGAnimatedRect> arect;
  nsresult rv = marker->GetViewBox(getter_AddRefs(arect));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMSVGRect> rect;
  rv = arect->GetAnimVal(getter_AddRefs(rect));
  NS_ENSURE_SUCCESS(rv, rv);

  float x, y, width, height;
  rect->GetX(&x);
  rect->GetY(&y);
  rect->GetWidth(&width);
  rect->GetHeight(&height);

  if (width <= 0.0f || height <= 0.0f) {
    
    return NS_OK;
  }

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAngle = aMark->angle;

  gfxContext *gfx = aContext->GetGfxContext();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    nsCOMPtr<nsIDOMSVGMatrix> matrix = GetCanvasTM();
    NS_ENSURE_TRUE(matrix, NS_ERROR_OUT_OF_MEMORY);

    gfx->Save();
    nsSVGUtils::SetClipRect(gfx, matrix, x, y, width, height);
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION |
                                 nsISVGChildFrame::TRANSFORM_CHANGED);
      nsSVGUtils::PaintFrameWithEffects(aContext, nsnull, kid);
    }
  }

  if (GetStyleDisplay()->IsScrollableOverflow())
    gfx->Restore();

  return NS_OK;
}


nsRect
nsSVGMarkerFrame::RegionMark(nsSVGPathGeometryFrame *aMarkedFrame,
                             const nsSVGMark *aMark, float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return nsRect(0,0,0,0);

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAngle = aMark->angle;

  
  for (nsIFrame* kid = mFrames.FirstChild();
       kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* child = do_QueryFrame(kid);
    if (child)
      child->UpdateCoveredRegion();
  }

  
  return nsSVGUtils::GetCoveredRegion(mFrames);
}

void
nsSVGMarkerFrame::SetParentCoordCtxProvider(nsSVGSVGElement *aContext)
{
  nsSVGMarkerElement *marker = static_cast<nsSVGMarkerElement*>(mContent);
  marker->SetParentCoordCtxProvider(aContext);
}




nsSVGMarkerFrame::AutoMarkerReferencer::AutoMarkerReferencer(
    nsSVGMarkerFrame *aFrame,
    nsSVGPathGeometryFrame *aMarkedFrame)
      : mFrame(aFrame)
{
  mFrame->mInUse = PR_TRUE;
  mFrame->mMarkedFrame = aMarkedFrame;

  nsSVGSVGElement *ctx =
    static_cast<nsSVGElement*>(aMarkedFrame->GetContent())->GetCtx();
  mFrame->SetParentCoordCtxProvider(ctx);
}

nsSVGMarkerFrame::AutoMarkerReferencer::~AutoMarkerReferencer()
{
  mFrame->SetParentCoordCtxProvider(nsnull);

  mFrame->mMarkedFrame = nsnull;
  mFrame->mInUse = PR_FALSE;
}

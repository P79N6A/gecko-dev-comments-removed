



































#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGRect.h"
#include "nsIDocument.h"
#include "nsSVGMarkerFrame.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsSVGMatrix.h"
#include "nsSVGMarkerElement.h"
#include "nsSVGPathGeometryElement.h"
#include "gfxContext.h"

nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerFrame(aContext);
}

nsIContent *
NS_GetSVGMarkerElement(nsIURI *aURI, nsIContent *aContent)
{
  nsIContent* content = nsContentUtils::GetReferencedElement(aURI, aContent);

  nsCOMPtr<nsIDOMSVGMarkerElement> marker = do_QueryInterface(content);

  if (marker)
    return content;

  return nsnull;
}

NS_IMETHODIMP
nsSVGMarkerFrame::InitSVG()
{
  nsresult rv = nsSVGMarkerFrameBase::InitSVG();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMSVGMarkerElement> marker = do_QueryInterface(mContent);
  NS_ASSERTION(marker, "wrong content element");

  mMarkedFrame = nsnull;
  mInUse = mInUse2 = PR_FALSE;

  return NS_OK;
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

  
  nsSVGMarkerElement *element = NS_STATIC_CAST(nsSVGMarkerElement*, mContent);

  
  nsCOMPtr<nsIDOMSVGMatrix> markerTM;
  element->GetMarkerTransform(mStrokeWidth, mX, mY, mAngle, getter_AddRefs(markerTM));

  
  nsCOMPtr<nsIDOMSVGMatrix> viewTM;
  element->GetViewboxToViewportTransform(getter_AddRefs(viewTM));

  nsCOMPtr<nsIDOMSVGMatrix> tmpTM;
  nsCOMPtr<nsIDOMSVGMatrix> resultTM;

  markedTM->Multiply(markerTM, getter_AddRefs(tmpTM));
  tmpTM->Multiply(viewTM, getter_AddRefs(resultTM));

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

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAngle = aMark->angle;

  gfxContext *gfx = aContext->GetGfxContext();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    nsSVGMarkerElement *marker = NS_STATIC_CAST(nsSVGMarkerElement*, mContent);

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

    nsCOMPtr<nsIDOMSVGMatrix> matrix = GetCanvasTM();
    NS_ENSURE_TRUE(matrix, NS_ERROR_OUT_OF_MEMORY);

    gfx->Save();
    nsSVGUtils::SetClipRect(gfx, matrix, x, y, width, height);
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyCanvasTMChanged(PR_TRUE);
      nsSVGUtils::PaintChildWithEffects(aContext, nsnull, kid);
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
    return nsRect();

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAngle = aMark->angle;

  
  for (nsIFrame* kid = mFrames.FirstChild();
       kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* child = nsnull;
    CallQueryInterface(kid, &child);
    if (child)
      child->UpdateCoveredRegion();
  }

  
  return nsSVGUtils::GetCoveredRegion(mFrames);
}


nsIAtom *
nsSVGMarkerFrame::GetType() const
{
  return nsGkAtoms::svgMarkerFrame;
}

void
nsSVGMarkerFrame::SetParentCoordCtxProvider(nsSVGSVGElement *aContext)
{
  nsSVGMarkerElement *marker = NS_STATIC_CAST(nsSVGMarkerElement*, mContent);
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
    NS_STATIC_CAST(nsSVGElement*, aMarkedFrame->GetContent())->GetCtx();
  mFrame->SetParentCoordCtxProvider(ctx);
}

nsSVGMarkerFrame::AutoMarkerReferencer::~AutoMarkerReferencer()
{
  mFrame->SetParentCoordCtxProvider(nsnull);

  mFrame->mMarkedFrame = nsnull;
  mFrame->mInUse = PR_FALSE;
}

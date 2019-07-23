



































#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsSVGClipPathFrame.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGGraphicElement.h"
#include "gfxContext.h"
#include "nsIDOMSVGRect.h"




nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(aContent);
  if (!transformable) {
#ifdef DEBUG
    printf("warning: trying to construct an SVGClipPathFrame for a content element that doesn't support the right interfaces\n");
#endif
    return nsnull;
  }

  return new (aPresShell) nsSVGClipPathFrame(aContext);
}

nsresult
NS_GetSVGClipPathFrame(nsSVGClipPathFrame **aResult,
                       nsIURI *aURI, nsIContent *aContent)
{
  *aResult = nsnull;

  
  nsIDocument *myDoc = aContent->GetCurrentDoc();
  if (!myDoc) {
    NS_WARNING("No document for this content!");
    return NS_ERROR_FAILURE;
  }
  nsIPresShell *presShell = myDoc->GetShellAt(0);
  if (!presShell) {
    NS_WARNING("no presshell");
    return NS_ERROR_FAILURE;
  }

  
  nsIFrame *cpframe;
  if (!NS_SUCCEEDED(nsSVGUtils::GetReferencedFrame(&cpframe, aURI, aContent, presShell)))
    return NS_ERROR_FAILURE;

  nsIAtom* frameType = cpframe->GetType();
  if (frameType != nsGkAtoms::svgClipPathFrame)
    return NS_ERROR_FAILURE;

  *aResult = (nsSVGClipPathFrame *)cpframe;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGClipPathFrame::InitSVG()
{
  nsresult rv = nsSVGClipPathFrameBase::InitSVG();
  if (NS_FAILED(rv))
    return rv;

  mClipParentMatrix = NULL;

  return NS_OK;
}

NS_IMETHODIMP
nsSVGClipPathFrame::ClipPaint(nsSVGRenderState* aContext,
                              nsISVGChildFrame* aParent,
                              nsCOMPtr<nsIDOMSVGMatrix> aMatrix)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  mInUse = PR_TRUE;

  mClipParent = aParent,
  mClipParentMatrix = aMatrix;

  PRBool isTrivial;
  IsTrivial(&isTrivial);

  nsAutoSVGRenderMode mode(aContext,
                           isTrivial ? nsSVGRenderState::CLIP
                                     : nsSVGRenderState::CLIP_MASK);

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyCanvasTMChanged(PR_TRUE);
      SVGFrame->PaintSVG(aContext, nsnull);
    }
  }

  if (isTrivial) {
    aContext->GetGfxContext()->Clip();
    aContext->GetGfxContext()->NewPath();
  }

  mInUse = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSVGClipPathFrame::ClipHitTest(nsISVGChildFrame* aParent,
                                nsCOMPtr<nsIDOMSVGMatrix> aMatrix,
                                float aX, float aY, PRBool *aHit)
{
  *aHit = PR_FALSE;

  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  mInUse = PR_TRUE;

  nsRect dirty;
  mClipParent = aParent,
  mClipParentMatrix = aMatrix;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      
      
      
      SVGFrame->NotifyCanvasTMChanged(PR_FALSE);

      nsIFrame *temp = nsnull;
      nsresult rv = SVGFrame->GetFrameForPointSVG(aX, aY, &temp);
      if (NS_SUCCEEDED(rv) && temp) {
        *aHit = PR_TRUE;
        mInUse = PR_FALSE;
        return NS_OK;
      }
    }
  }

  mInUse = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSVGClipPathFrame::IsTrivial(PRBool *aTrivial)
{
  *aTrivial = PR_TRUE;
  PRBool foundChild = PR_FALSE;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame *svgChild = nsnull;
    CallQueryInterface(kid, &svgChild);

    if (svgChild) {
      
      
      if (foundChild || svgChild->IsDisplayContainer()) {
        *aTrivial = PR_FALSE;
        return NS_OK;
      }
      foundChild = PR_TRUE;
    }
  }

  return NS_OK;
}

nsIAtom *
nsSVGClipPathFrame::GetType() const
{
  return nsGkAtoms::svgClipPathFrame;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGClipPathFrame::GetCanvasTM()
{
  NS_ASSERTION(mClipParentMatrix, "null parent matrix");

  nsSVGGraphicElement *element =
    NS_STATIC_CAST(nsSVGGraphicElement*, mContent);
  nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();

  nsCOMPtr<nsIDOMSVGMatrix> canvasTM;

  if (localTM)
    mClipParentMatrix->Multiply(localTM, getter_AddRefs(canvasTM));
  else
    canvasTM = mClipParentMatrix;

  
  PRUint16 units;
  nsCOMPtr<nsIDOMSVGClipPathElement> path = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> aEnum;
  path->GetClipPathUnits(getter_AddRefs(aEnum));
  aEnum->GetAnimVal(&units);
  
  if (mClipParent &&
      units == nsIDOMSVGClipPathElement::SVG_CPUNITS_OBJECTBOUNDINGBOX) {
    nsCOMPtr<nsIDOMSVGRect> rect;
    nsresult rv = mClipParent->GetBBox(getter_AddRefs(rect));

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

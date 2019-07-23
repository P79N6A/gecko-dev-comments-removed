



































#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsSVGClipPathFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGClipPathElement.h"
#include "gfxContext.h"
#include "nsIDOMSVGRect.h"




nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGClipPathElement> clipPath = do_QueryInterface(aContent);
  if (!clipPath) {
    NS_ERROR("Can't create frame! Content is not an SVG clipPath!");
    return nsnull;
  }

  return new (aPresShell) nsSVGClipPathFrame(aContext);
}

nsresult
nsSVGClipPathFrame::ClipPaint(nsSVGRenderState* aContext,
                              nsIFrame* aParent,
                              nsIDOMSVGMatrix *aMatrix)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent,
  mClipParentMatrix = aMatrix;

  PRBool isTrivial = IsTrivial();

  nsAutoSVGRenderMode mode(aContext,
                           isTrivial ? nsSVGRenderState::CLIP
                                     : nsSVGRenderState::CLIP_MASK);

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION | 
                                 nsISVGChildFrame::TRANSFORM_CHANGED);
      SVGFrame->PaintSVG(aContext, nsnull);
    }
  }

  if (isTrivial) {
    aContext->GetGfxContext()->Clip();
    aContext->GetGfxContext()->NewPath();
  }

  return NS_OK;
}

PRBool
nsSVGClipPathFrame::ClipHitTest(nsIFrame* aParent,
                                nsIDOMSVGMatrix *aMatrix,
                                const nsPoint &aPoint)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return PR_FALSE;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent,
  mClipParentMatrix = aMatrix;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      
      
      
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);

      if (SVGFrame->GetFrameForPoint(aPoint))
        return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
nsSVGClipPathFrame::IsTrivial()
{
  PRBool foundChild = PR_FALSE;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame *svgChild = nsnull;
    CallQueryInterface(kid, &svgChild);

    if (svgChild) {
      
      
      if (foundChild || svgChild->IsDisplayContainer())
        return PR_FALSE;
      foundChild = PR_TRUE;
    }
  }
  return PR_TRUE;
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

  nsSVGClipPathElement *clipPath = static_cast<nsSVGClipPathElement*>
                                              (mContent);

  nsCOMPtr<nsIDOMSVGMatrix> localTM = clipPath->GetLocalTransformMatrix();

  nsCOMPtr<nsIDOMSVGMatrix> canvasTM;

  if (localTM)
    mClipParentMatrix->Multiply(localTM, getter_AddRefs(canvasTM));
  else
    canvasTM = mClipParentMatrix;

  return nsSVGUtils::AdjustMatrixForUnits(canvasTM,
                                          &clipPath->mEnumAttributes[nsSVGClipPathElement::CLIPPATHUNITS],
                                          mClipParent);
}

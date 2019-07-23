



































#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsSVGClipPathFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGClipPathElement.h"
#include "gfxContext.h"
#include "nsSVGMatrix.h"




nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGClipPathFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGClipPathFrame)

nsresult
nsSVGClipPathFrame::ClipPaint(nsSVGRenderState* aContext,
                              nsIFrame* aParent,
                              const gfxMatrix &aMatrix)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent,
  mClipParentMatrix = NS_NewSVGMatrix(aMatrix);

  PRBool isTrivial = IsTrivial();

  nsAutoSVGRenderMode mode(aContext,
                           isTrivial ? nsSVGRenderState::CLIP
                                     : nsSVGRenderState::CLIP_MASK);

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
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
                                const gfxMatrix &aMatrix,
                                const nsPoint &aPoint)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return PR_FALSE;
  }
  AutoClipPathReferencer clipRef(this);

  mClipParent = aParent,
  mClipParentMatrix = NS_NewSVGMatrix(aMatrix);

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
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
    nsISVGChildFrame *svgChild = do_QueryFrame(kid);
    if (svgChild) {
      
      
      if (foundChild || svgChild->IsDisplayContainer())
        return PR_FALSE;
      foundChild = PR_TRUE;
    }
  }
  return PR_TRUE;
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGClipPathFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGClipPathElement> clipPath = do_QueryInterface(aContent);
  NS_ASSERTION(clipPath, "Content is not an SVG clipPath!");

  return nsSVGClipPathFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGClipPathFrame::GetType() const
{
  return nsGkAtoms::svgClipPathFrame;
}

gfxMatrix
nsSVGClipPathFrame::GetCanvasTM()
{
  nsSVGClipPathElement *content = static_cast<nsSVGClipPathElement*>(mContent);

  gfxMatrix tm = content->PrependLocalTransformTo(
    nsSVGUtils::ConvertSVGMatrixToThebes(mClipParentMatrix));

  return nsSVGUtils::AdjustMatrixForUnits(tm,
                                          &content->mEnumAttributes[nsSVGClipPathElement::CLIPPATHUNITS],
                                          mClipParent);
}

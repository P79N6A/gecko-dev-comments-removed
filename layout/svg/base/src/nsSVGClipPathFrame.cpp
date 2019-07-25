



































#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsSVGClipPathFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGEffects.h"
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

  mClipParent = aParent;
  mClipParentMatrix = NS_NewSVGMatrix(aMatrix);

  PRBool isTrivial = IsTrivial();

  nsAutoSVGRenderMode mode(aContext,
                           isTrivial ? nsSVGRenderState::CLIP
                                     : nsSVGRenderState::CLIP_MASK);


  gfxContext *gfx = aContext->GetGfxContext();

  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull);
  PRBool referencedClipIsTrivial;
  if (clipPathFrame) {
    referencedClipIsTrivial = clipPathFrame->IsTrivial();
    gfx->Save();
    if (referencedClipIsTrivial) {
      clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
    } else {
      gfx->PushGroup(gfxASurface::CONTENT_ALPHA);
    }
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::SUPPRESS_INVALIDATION | 
                                 nsISVGChildFrame::TRANSFORM_CHANGED);

      PRBool isOK = PR_TRUE;
      nsSVGClipPathFrame *clipPathFrame =
        nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(&isOK);
      if (!isOK) {
        continue;
      }

      PRBool isTrivial;

      if (clipPathFrame) {
        isTrivial = clipPathFrame->IsTrivial();
        gfx->Save();
        if (isTrivial) {
          clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
        } else {
          gfx->PushGroup(gfxASurface::CONTENT_ALPHA);
        }
      }

      SVGFrame->PaintSVG(aContext, nsnull);

      if (clipPathFrame) {
        if (!isTrivial) {
          gfx->PopGroupToSource();

          nsRefPtr<gfxPattern> clipMaskSurface;
          gfx->PushGroup(gfxASurface::CONTENT_ALPHA);

          clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
          clipMaskSurface = gfx->PopGroup();

          if (clipMaskSurface) {
            gfx->Mask(clipMaskSurface);
          }
        }
        gfx->Restore();
      }
    }
  }

  if (clipPathFrame) {
    if (!referencedClipIsTrivial) {
      gfx->PopGroupToSource();

      nsRefPtr<gfxPattern> clipMaskSurface;
      gfx->PushGroup(gfxASurface::CONTENT_ALPHA);

      clipPathFrame->ClipPaint(aContext, aParent, aMatrix);
      clipMaskSurface = gfx->PopGroup();

      if (clipMaskSurface) {
        gfx->Mask(clipMaskSurface);
      }
    }
    gfx->Restore();
  }

  if (isTrivial) {
    gfx->Clip();
    gfx->NewPath();
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

  mClipParent = aParent;
  mClipParentMatrix = NS_NewSVGMatrix(aMatrix);

  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull);
  if (clipPathFrame && !clipPathFrame->ClipHitTest(aParent, aMatrix, aPoint))
    return PR_FALSE;

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
  
  if (nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nsnull))
    return PR_FALSE;

  PRBool foundChild = PR_FALSE;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame *svgChild = do_QueryFrame(kid);
    if (svgChild) {
      
      
      if (foundChild || svgChild->IsDisplayContainer())
        return PR_FALSE;

      
      if (nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(nsnull))
        return PR_FALSE;

      foundChild = PR_TRUE;
    }
  }
  return PR_TRUE;
}

PRBool
nsSVGClipPathFrame::IsValid()
{
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return PR_FALSE;
  }
  AutoClipPathReferencer clipRef(this);

  PRBool isOK = PR_TRUE;
  nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(&isOK);
  if (!isOK) {
    return PR_FALSE;
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {

    nsIAtom *type = kid->GetType();

    if (type == nsGkAtoms::svgUseFrame) {
      for (nsIFrame* grandKid = kid->GetFirstPrincipalChild(); grandKid;
           grandKid = grandKid->GetNextSibling()) {

        nsIAtom *type = grandKid->GetType();

        if (type != nsGkAtoms::svgPathGeometryFrame &&
            type != nsGkAtoms::svgTextFrame) {
          return PR_FALSE;
        }
      }
      continue;
    }
    if (type != nsGkAtoms::svgPathGeometryFrame &&
        type != nsGkAtoms::svgTextFrame) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

NS_IMETHODIMP
nsSVGClipPathFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::transform) {
      nsSVGUtils::NotifyChildrenOfSVGChange(this,
                                            nsISVGChildFrame::TRANSFORM_CHANGED);
    }
    if (aAttribute == nsGkAtoms::clipPathUnits) {
      nsSVGEffects::InvalidateRenderingObservers(this);
    }
  }

  return nsSVGClipPathFrameBase::AttributeChanged(aNameSpaceID,
                                                  aAttribute, aModType);
}

NS_IMETHODIMP
nsSVGClipPathFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMSVGClipPathElement> clipPath = do_QueryInterface(aContent);
  NS_ASSERTION(clipPath, "Content is not an SVG clipPath!");
#endif

  AddStateBits(NS_STATE_SVG_CLIPPATH_CHILD);
  return nsSVGClipPathFrameBase::Init(aContent, aParent, aPrevInFlow);
}

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







































#include "nsSVGInnerSVGFrame.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsSVGSVGElement.h"
#include "nsSVGContainerFrame.h"
#include "gfxContext.h"

nsIFrame*
NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGInnerSVGFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGInnerSVGFrame)




NS_QUERYFRAME_HEAD(nsSVGInnerSVGFrame)
  NS_QUERYFRAME_ENTRY(nsSVGInnerSVGFrame)
  NS_QUERYFRAME_ENTRY(nsISVGSVGFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGInnerSVGFrameBase)

#ifdef DEBUG
NS_IMETHODIMP
nsSVGInnerSVGFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGSVGElement> svg = do_QueryInterface(aContent);
  NS_ASSERTION(svg, "Content is not an SVG 'svg' element!");

  return nsSVGInnerSVGFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGInnerSVGFrame::GetType() const
{
  return nsGkAtoms::svgInnerSVGFrame;
}




NS_IMETHODIMP
nsSVGInnerSVGFrame::PaintSVG(nsSVGRenderState *aContext,
                             const nsIntRect *aDirtyRect)
{
  gfxContextAutoSaveRestore autoSR;

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

    if (width <= 0 || height <= 0) {
      return NS_OK;
    }

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    gfxMatrix clipTransform = parent->GetCanvasTM();

    gfxContext *gfx = aContext->GetGfxContext();
    autoSR.SetContext(gfx);
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, x, y, width, height);
    nsSVGUtils::SetClipRect(gfx, clipTransform, clipRect);
  }

  return nsSVGInnerSVGFrameBase::PaintSVG(aContext, aDirtyRect);
}

void
nsSVGInnerSVGFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & COORD_CONTEXT_CHANGED) {

    nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(mContent);

    
    
    

    if (!(aFlags & TRANSFORM_CHANGED) &&
        (svg->mLengthAttributes[nsSVGSVGElement::X].IsPercentage() ||
         svg->mLengthAttributes[nsSVGSVGElement::Y].IsPercentage() ||
         (svg->mViewBox.IsValid() &&
          (svg->mLengthAttributes[nsSVGSVGElement::WIDTH].IsPercentage() ||
           svg->mLengthAttributes[nsSVGSVGElement::HEIGHT].IsPercentage())))) {
    
      aFlags |= TRANSFORM_CHANGED;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
  }

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  nsSVGInnerSVGFrameBase::NotifySVGChanged(aFlags);
}

NS_IMETHODIMP
nsSVGInnerSVGFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {

      if (static_cast<nsSVGSVGElement*>(mContent)->mViewBox.IsValid()) {

        
        mCanvasTM = nsnull;

        nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
      } else {
        nsSVGUtils::NotifyChildrenOfSVGChange(this, COORD_CONTEXT_CHANGED);
      }

    } else if (aAttribute == nsGkAtoms::transform ||
               aAttribute == nsGkAtoms::preserveAspectRatio ||
               aAttribute == nsGkAtoms::viewBox ||
               aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nsnull;

      nsSVGUtils::NotifyChildrenOfSVGChange(
          this, aAttribute == nsGkAtoms::viewBox ?
                  TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED : TRANSFORM_CHANGED);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGInnerSVGFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  if (GetStyleDisplay()->IsScrollableOverflow()) {
    nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);

    float clipX, clipY, clipWidth, clipHeight;
    content->GetAnimatedLengthValues(&clipX, &clipY, &clipWidth, &clipHeight, nsnull);

    if (!nsSVGUtils::HitTestRect(parent->GetCanvasTM(),
                                 clipX, clipY, clipWidth, clipHeight,
                                 PresContext()->AppUnitsToDevPixels(aPoint.x),
                                 PresContext()->AppUnitsToDevPixels(aPoint.y))) {
      return nsnull;
    }
  }

  return nsSVGInnerSVGFrameBase::GetFrameForPoint(aPoint);
}




void
nsSVGInnerSVGFrame::SuspendRedraw()
{
  if (GetParent()->GetStateBits() & NS_STATE_SVG_REDRAW_SUSPENDED)
    return;

  nsSVGUtils::NotifyRedrawSuspended(this);
}

void
nsSVGInnerSVGFrame::UnsuspendRedraw()
{
  if (GetParent()->GetStateBits() & NS_STATE_SVG_REDRAW_SUSPENDED)
    return;

  nsSVGUtils::NotifyRedrawUnsuspended(this);
}

void
nsSVGInnerSVGFrame::NotifyViewportChange()
{
  NS_ERROR("Inner SVG frames should not get Viewport changes.");
}




gfxMatrix
nsSVGInnerSVGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformTo(parent->GetCanvasTM());

    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}


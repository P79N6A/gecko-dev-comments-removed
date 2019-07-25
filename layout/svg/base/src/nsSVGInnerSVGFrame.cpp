





#include "nsSVGInnerSVGFrame.h"


#include "gfxContext.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGSVGElement.h"

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
nsSVGInnerSVGFrame::PaintSVG(nsRenderingContext *aContext,
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

    gfxContext *gfx = aContext->ThebesContext();
    autoSR.SetContext(gfx);
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, x, y, width, height);
    nsSVGUtils::SetClipRect(gfx, clipTransform, clipRect);
  }

  return nsSVGInnerSVGFrameBase::PaintSVG(aContext, aDirtyRect);
}

void
nsSVGInnerSVGFrame::UpdateBounds()
{
  
  
  float x, y, width, height;
  static_cast<nsSVGSVGElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);
  mRect = nsLayoutUtils::RoundGfxRectToAppRect(
                           gfxRect(x, y, width, height),
                           PresContext()->AppUnitsPerCSSPixel());
  nsSVGInnerSVGFrameBase::UpdateBounds();
}

void
nsSVGInnerSVGFrame::NotifySVGChanged(PRUint32 aFlags)
{
  NS_ABORT_IF_FALSE(!(aFlags & DO_NOT_NOTIFY_RENDERING_OBSERVERS) ||
                    (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "Must be NS_STATE_SVG_NONDISPLAY_CHILD!");

  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  if (aFlags & COORD_CONTEXT_CHANGED) {

    nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(mContent);

    bool xOrYIsPercentage =
      svg->mLengthAttributes[nsSVGSVGElement::X].IsPercentage() ||
      svg->mLengthAttributes[nsSVGSVGElement::Y].IsPercentage();
    bool widthOrHeightIsPercentage =
      svg->mLengthAttributes[nsSVGSVGElement::WIDTH].IsPercentage() ||
      svg->mLengthAttributes[nsSVGSVGElement::HEIGHT].IsPercentage();

    if (xOrYIsPercentage || widthOrHeightIsPercentage) {
      
      
      
      
      
      
      nsSVGUtils::ScheduleBoundsUpdate(this);
    }

    
    
    

    if (!(aFlags & TRANSFORM_CHANGED) &&
        (xOrYIsPercentage ||
         (widthOrHeightIsPercentage && svg->HasViewBox()))) {
      aFlags |= TRANSFORM_CHANGED;
    }

    if (svg->HasViewBox() || !widthOrHeightIsPercentage) {
      
      
      
      aFlags &= ~COORD_CONTEXT_CHANGED;

      if (!(aFlags & ~DO_NOT_NOTIFY_RENDERING_OBSERVERS)) {
        return; 
      }
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

    nsSVGSVGElement* content = static_cast<nsSVGSVGElement*>(mContent);

    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      nsSVGUtils::InvalidateAndScheduleBoundsUpdate(this);

      if (content->HasViewBoxOrSyntheticViewBox()) {
        
        mCanvasTM = nsnull;
        content->ChildrenOnlyTransformChanged();
        nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
      } else {
        PRUint32 flags = COORD_CONTEXT_CHANGED;
        if (mCanvasTM && mCanvasTM->IsSingular()) {
          mCanvasTM = nsnull;
          flags |= TRANSFORM_CHANGED;
        }
        nsSVGUtils::NotifyChildrenOfSVGChange(this, flags);
      }

    } else if (aAttribute == nsGkAtoms::transform ||
               aAttribute == nsGkAtoms::preserveAspectRatio ||
               aAttribute == nsGkAtoms::viewBox ||
               aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nsnull;

      nsSVGUtils::InvalidateAndScheduleBoundsUpdate(this);

      nsSVGUtils::NotifyChildrenOfSVGChange(
          this, aAttribute == nsGkAtoms::viewBox ?
                  TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED : TRANSFORM_CHANGED);

      if (aAttribute == nsGkAtoms::viewBox ||
          (aAttribute == nsGkAtoms::preserveAspectRatio &&
           content->HasViewBoxOrSyntheticViewBox())) {
        content->ChildrenOnlyTransformChanged();
      }
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
nsSVGInnerSVGFrame::NotifyViewportOrTransformChanged(PRUint32 aFlags)
{
  
  
  
  
  
  NS_ERROR("Not called for nsSVGInnerSVGFrame");
}




gfxMatrix
nsSVGInnerSVGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM());

    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}

bool
nsSVGInnerSVGFrame::HasChildrenOnlyTransform(gfxMatrix *aTransform) const
{
  nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);

  if (content->HasViewBoxOrSyntheticViewBox()) {
    
    if (aTransform) {
      *aTransform = content->GetViewBoxTransform();
    }
    return true;
  }
  return false;
}

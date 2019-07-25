





#include "nsSVGInnerSVGFrame.h"


#include "gfxContext.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGIntegrationUtils.h"
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
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  gfxContextAutoSaveRestore autoSR;

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

    if (width <= 0 || height <= 0) {
      return NS_OK;
    }

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    gfxMatrix clipTransform = parent->GetCanvasTM(FOR_PAINTING);

    gfxContext *gfx = aContext->ThebesContext();
    autoSR.SetContext(gfx);
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, x, y, width, height);
    nsSVGUtils::SetClipRect(gfx, clipTransform, clipRect);
  }

  return nsSVGInnerSVGFrameBase::PaintSVG(aContext, aDirtyRect);
}

void
nsSVGInnerSVGFrame::ReflowSVG()
{
  
  
  float x, y, width, height;
  static_cast<nsSVGSVGElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);
  mRect = nsLayoutUtils::RoundGfxRectToAppRect(
                           gfxRect(x, y, width, height),
                           PresContext()->AppUnitsPerCSSPixel());
  nsSVGInnerSVGFrameBase::ReflowSVG();
}

void
nsSVGInnerSVGFrame::NotifySVGChanged(PRUint32 aFlags)
{
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
      
      
      
      
      
      
      nsSVGUtils::ScheduleReflowSVG(this);
    }

    
    
    

    if (!(aFlags & TRANSFORM_CHANGED) &&
        (xOrYIsPercentage ||
         (widthOrHeightIsPercentage && svg->HasViewBox()))) {
      aFlags |= TRANSFORM_CHANGED;
    }

    if (svg->HasViewBox() || !widthOrHeightIsPercentage) {
      
      
      
      aFlags &= ~COORD_CONTEXT_CHANGED;

      if (!aFlags) {
        return; 
      }
    }
  }

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nullptr;
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
      nsSVGUtils::InvalidateAndScheduleReflowSVG(this);

      if (content->HasViewBoxOrSyntheticViewBox()) {
        
        mCanvasTM = nullptr;
        content->ChildrenOnlyTransformChanged();
        nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
      } else {
        PRUint32 flags = COORD_CONTEXT_CHANGED;
        if (mCanvasTM && mCanvasTM->IsSingular()) {
          mCanvasTM = nullptr;
          flags |= TRANSFORM_CHANGED;
        }
        nsSVGUtils::NotifyChildrenOfSVGChange(this, flags);
      }

    } else if (aAttribute == nsGkAtoms::transform ||
               aAttribute == nsGkAtoms::preserveAspectRatio ||
               aAttribute == nsGkAtoms::viewBox ||
               aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nullptr;

      nsSVGUtils::InvalidateAndScheduleReflowSVG(this);

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
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only hit-testing of non-display "
               "SVG should take this code path");

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);

    float clipX, clipY, clipWidth, clipHeight;
    content->GetAnimatedLengthValues(&clipX, &clipY, &clipWidth, &clipHeight, nullptr);

    if (!nsSVGUtils::HitTestRect(parent->GetCanvasTM(FOR_HIT_TESTING),
                                 clipX, clipY, clipWidth, clipHeight,
                                 PresContext()->AppUnitsToDevPixels(aPoint.x),
                                 PresContext()->AppUnitsToDevPixels(aPoint.y))) {
      return nullptr;
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
nsSVGInnerSVGFrame::GetCanvasTM(PRUint32 aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM(aFor));

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

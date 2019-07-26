





#include "nsSVGContainerFrame.h"


#include "nsSVGEffects.h"
#include "nsSVGElement.h"
#include "nsSVGUtils.h"
#include "SVGAnimatedTransformList.h"

using namespace mozilla;

NS_QUERYFRAME_HEAD(nsSVGContainerFrame)
  NS_QUERYFRAME_ENTRY(nsSVGContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGContainerFrameBase)

NS_QUERYFRAME_HEAD(nsSVGDisplayContainerFrame)
  NS_QUERYFRAME_ENTRY(nsSVGDisplayContainerFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGContainerFrame)

nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                        nsStyleContext* aContext)
{
  nsIFrame *frame = new (aPresShell) nsSVGContainerFrame(aContext);
  
  
  
  frame->AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  return frame;
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGContainerFrame)
NS_IMPL_FRAMEARENA_HELPERS(nsSVGDisplayContainerFrame)

NS_IMETHODIMP
nsSVGContainerFrame::AppendFrames(ChildListID  aListID,
                                  nsFrameList& aFrameList)
{
  return InsertFrames(aListID, mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsSVGContainerFrame::InsertFrames(ChildListID aListID,
                                  nsIFrame* aPrevFrame,
                                  nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  mFrames.InsertFrames(this, aPrevFrame, aFrameList);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGContainerFrame::RemoveFrame(ChildListID aListID,
                                 nsIFrame* aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  mFrames.DestroyFrame(aOldFrame);
  return NS_OK;
}

bool
nsSVGContainerFrame::UpdateOverflow()
{
  if (mState & NS_STATE_SVG_NONDISPLAY_CHILD) {
    
    
    return false;
  }
  return nsSVGContainerFrameBase::UpdateOverflow();
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::Init(nsIContent* aContent,
                                 nsIFrame* aParent,
                                 nsIFrame* aPrevInFlow)
{
  if (!(GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
    AddStateBits(aParent->GetStateBits() &
      (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_CLIPPATH_CHILD));
  }
  nsresult rv = nsSVGContainerFrame::Init(aContent, aParent, aPrevInFlow);
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  if (mContent->IsSVG() &&
      !static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return NS_OK;
  }
  return BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InsertFrames(ChildListID aListID,
                                         nsIFrame* aPrevFrame,
                                         nsFrameList& aFrameList)
{
  
  
  
  nsIFrame* firstOldFrame = aPrevFrame ?
    aPrevFrame->GetNextSibling() : GetChildList(aListID).FirstChild();
  nsIFrame* firstNewFrame = aFrameList.FirstChild();
  
  
  nsSVGContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);

  
  
  if (!(GetStateBits() &
        (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN |
         NS_STATE_SVG_NONDISPLAY_CHILD))) {
    for (nsIFrame* kid = firstNewFrame; kid != firstOldFrame;
         kid = kid->GetNextSibling()) {
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame) {
        NS_ABORT_IF_FALSE(!(kid->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                          "Check for this explicitly in the |if|, then");
        bool isFirstReflow = (kid->GetStateBits() & NS_FRAME_FIRST_REFLOW);
        
        kid->RemoveStateBits(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
                             NS_FRAME_HAS_DIRTY_CHILDREN);
        
        
        nsSVGUtils::ScheduleReflowSVG(kid);
        if (isFirstReflow) {
          
          kid->AddStateBits(NS_FRAME_FIRST_REFLOW);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::RemoveFrame(ChildListID aListID,
                                        nsIFrame* aOldFrame)
{
  nsSVGUtils::InvalidateBounds(aOldFrame);

  nsresult rv = nsSVGContainerFrame::RemoveFrame(aListID, aOldFrame);

  if (!(GetStateBits() & (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_IS_OUTER_SVG))) {
    nsSVGUtils::NotifyAncestorsOfFilterRegionChange(this);
  }

  return rv;
}

bool
nsSVGDisplayContainerFrame::IsSVGTransformed(gfxMatrix *aOwnTransform,
                                             gfxMatrix *aFromParentTransform) const
{
  bool foundTransform = false;

  
  nsIFrame *parent = GetParent();
  if (parent &&
      parent->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer)) {
    foundTransform = static_cast<nsSVGContainerFrame*>(parent)->
                       HasChildrenOnlyTransform(aFromParentTransform);
  }

  if (mContent->IsSVG()) {
    nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
    if (content->GetAnimatedTransformList() ||
        content->GetAnimateMotionTransform()) {
      if (aOwnTransform) {
        *aOwnTransform = content->PrependLocalTransformsTo(gfxMatrix(),
                                    nsSVGElement::eUserSpaceToParent);
      }
      foundTransform = true;
    }
  }
  return foundTransform;
}




NS_IMETHODIMP
nsSVGDisplayContainerFrame::PaintSVG(nsRenderingContext* aContext,
                                     const nsIntRect *aDirtyRect)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD) ||
               PresContext()->IsGlyph(),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintFrameWithEffects(aContext, aDirtyRect, kid);
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGDisplayContainerFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only hit-testing of a "
               "clipPath's contents should take this code path");
  return nsSVGUtils::HitTestChildren(this, aPoint);
}

NS_IMETHODIMP_(nsRect)
nsSVGDisplayContainerFrame::GetCoveredRegion()
{
  return nsSVGUtils::GetCoveredRegion(mFrames);
}

void
nsSVGDisplayContainerFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  NS_ABORT_IF_FALSE(GetType() != nsGkAtoms::svgOuterSVGFrame,
                    "Do not call on outer-<svg>");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  
  
  
  
  
  

  bool outerSVGHasHadFirstReflow =
    (GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) == 0;

  if (outerSVGHasHadFirstReflow) {
    mState &= ~NS_FRAME_FIRST_REFLOW; 
  }

  nsOverflowAreas overflowRects;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      NS_ABORT_IF_FALSE(!(kid->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                        "Check for this explicitly in the |if|, then");
      kid->AddStateBits(mState & NS_FRAME_IS_DIRTY);
      SVGFrame->ReflowSVG();

      
      
      
      ConsiderChildOverflow(overflowRects, kid);
    }
  }

  
  
  
  
  
  
  
  NS_ABORT_IF_FALSE(mContent->Tag() == nsGkAtoms::svg ||
                    (mContent->Tag() == nsGkAtoms::use &&
                     mRect.Size() == nsSize(0,0)) ||
                    mRect.IsEqualEdges(nsRect()),
                    "Only inner-<svg>/<use> is expected to have mRect set");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  FinishAndStoreOverflow(overflowRects, mRect.Size());

  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
}  

void
nsSVGDisplayContainerFrame::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  nsSVGUtils::NotifyChildrenOfSVGChange(this, aFlags);
}

SVGBBox
nsSVGDisplayContainerFrame::GetBBoxContribution(
  const gfxMatrix &aToBBoxUserspace,
  uint32_t aFlags)
{
  SVGBBox bboxUnion;

  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* svgKid = do_QueryFrame(kid);
    if (svgKid) {
      gfxMatrix transform = aToBBoxUserspace;
      nsIContent *content = kid->GetContent();
      if (content->IsSVG()) {
        transform = static_cast<nsSVGElement*>(content)->
                      PrependLocalTransformsTo(aToBBoxUserspace);
      }
      
      
      bboxUnion.UnionEdges(svgKid->GetBBoxContribution(transform, aFlags));
    }
    kid = kid->GetNextSibling();
  }

  return bboxUnion;
}

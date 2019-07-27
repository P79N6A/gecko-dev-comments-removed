





#include "nsSVGContainerFrame.h"


#include "nsCSSFrameConstructor.h"
#include "nsSVGEffects.h"
#include "nsSVGElement.h"
#include "nsSVGUtils.h"
#include "nsSVGAnimatedTransformList.h"
#include "SVGTextFrame.h"
#include "RestyleManager.h"

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
  
  
  
  frame->AddStateBits(NS_FRAME_IS_NONDISPLAY);
  return frame;
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGContainerFrame)
NS_IMPL_FRAMEARENA_HELPERS(nsSVGDisplayContainerFrame)

void
nsSVGContainerFrame::AppendFrames(ChildListID  aListID,
                                  nsFrameList& aFrameList)
{
  InsertFrames(aListID, mFrames.LastChild(), aFrameList);  
}

void
nsSVGContainerFrame::InsertFrames(ChildListID aListID,
                                  nsIFrame* aPrevFrame,
                                  nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  mFrames.InsertFrames(this, aPrevFrame, aFrameList);
}

void
nsSVGContainerFrame::RemoveFrame(ChildListID aListID,
                                 nsIFrame* aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  mFrames.DestroyFrame(aOldFrame);
}

bool
nsSVGContainerFrame::UpdateOverflow()
{
  if (mState & NS_FRAME_IS_NONDISPLAY) {
    
    
    return false;
  }
  return nsSVGContainerFrameBase::UpdateOverflow();
}
























 void
nsSVGContainerFrame::ReflowSVGNonDisplayText(nsIFrame* aContainer)
{
  NS_ASSERTION(aContainer->GetStateBits() & NS_FRAME_IS_DIRTY,
               "expected aContainer to be NS_FRAME_IS_DIRTY");
  NS_ASSERTION((aContainer->GetStateBits() & NS_FRAME_IS_NONDISPLAY) ||
               !aContainer->IsFrameOfType(nsIFrame::eSVG),
               "it is wasteful to call ReflowSVGNonDisplayText on a container "
               "frame that is not NS_FRAME_IS_NONDISPLAY");
  for (nsIFrame* kid = aContainer->GetFirstPrincipalChild(); kid;
       kid = kid->GetNextSibling()) {
    nsIAtom* type = kid->GetType();
    if (type == nsGkAtoms::svgTextFrame) {
      static_cast<SVGTextFrame*>(kid)->ReflowSVGNonDisplayText();
    } else {
      if (kid->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer) ||
          type == nsGkAtoms::svgForeignObjectFrame ||
          !kid->IsFrameOfType(nsIFrame::eSVG)) {
        ReflowSVGNonDisplayText(kid);
      }
    }
  }
}

void
nsSVGDisplayContainerFrame::Init(nsIContent*       aContent,
                                 nsContainerFrame* aParent,
                                 nsIFrame*         aPrevInFlow)
{
  if (!(GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
    AddStateBits(aParent->GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD);
  }
  nsSVGContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

void
nsSVGDisplayContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  
  if (mContent->IsSVG() &&
      !static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return;
  }
  return BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

void
nsSVGDisplayContainerFrame::InsertFrames(ChildListID aListID,
                                         nsIFrame* aPrevFrame,
                                         nsFrameList& aFrameList)
{
  
  
  
  nsIFrame* nextFrame = aPrevFrame ?
    aPrevFrame->GetNextSibling() : GetChildList(aListID).FirstChild();
  nsIFrame* firstNewFrame = aFrameList.FirstChild();
  
  
  nsSVGContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);

  
  
  if (!(GetStateBits() &
        (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN |
         NS_FRAME_IS_NONDISPLAY))) {
    for (nsIFrame* kid = firstNewFrame; kid != nextFrame;
         kid = kid->GetNextSibling()) {
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame) {
        NS_ABORT_IF_FALSE(!(kid->GetStateBits() & NS_FRAME_IS_NONDISPLAY),
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
}

void
nsSVGDisplayContainerFrame::RemoveFrame(ChildListID aListID,
                                        nsIFrame* aOldFrame)
{
  nsSVGEffects::InvalidateRenderingObservers(aOldFrame);

  
  
  
  SchedulePaint();
  PresContext()->RestyleManager()->PostRestyleEvent(
    mContent->AsElement(), nsRestyleHint(0), nsChangeHint_UpdateOverflow);

  nsSVGContainerFrame::RemoveFrame(aListID, aOldFrame);

  if (!(GetStateBits() & (NS_FRAME_IS_NONDISPLAY | NS_STATE_IS_OUTER_SVG))) {
    nsSVGUtils::NotifyAncestorsOfFilterRegionChange(this);
  }
}

bool
nsSVGDisplayContainerFrame::IsSVGTransformed(gfx::Matrix *aOwnTransform,
                                             gfx::Matrix *aFromParentTransform) const
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
    nsSVGAnimatedTransformList* transformList =
      content->GetAnimatedTransformList();
    if ((transformList && transformList->HasTransform()) ||
        content->GetAnimateMotionTransform()) {
      if (aOwnTransform) {
        *aOwnTransform = gfx::ToMatrix(content->PrependLocalTransformsTo(gfxMatrix(),
                                    nsSVGElement::eUserSpaceToParent));
      }
      foundTransform = true;
    }
  }
  return foundTransform;
}




nsresult
nsSVGDisplayContainerFrame::PaintSVG(nsRenderingContext* aContext,
                                     const gfxMatrix& aTransform,
                                     const nsIntRect *aDirtyRect)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY) ||
               PresContext()->IsGlyph(),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  const nsStyleDisplay *display = StyleDisplay();
  if (display->mOpacity == 0.0) {
    return NS_OK;
  }

  gfxMatrix matrix = aTransform;
  if (GetContent()->IsSVG()) { 
    matrix = static_cast<const nsSVGElement*>(GetContent())->
               PrependLocalTransformsTo(matrix,
                                        nsSVGElement::eChildToUserSpace);
    if (matrix.IsSingular()) {
      return NS_OK;
    }
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    gfxMatrix m = matrix;
    
    
    const nsIContent* content = kid->GetContent();
    if (content->IsSVG()) { 
      const nsSVGElement* element = static_cast<const nsSVGElement*>(content);
      if (!element->HasValidDimensions()) {
        continue; 
      }
      m = element->
            PrependLocalTransformsTo(m, nsSVGElement::eUserSpaceToParent);
      if (m.IsSingular()) {
        continue;
      }
    }
    nsSVGUtils::PaintFrameWithEffects(kid, aContext, m, aDirtyRect);
  }

  return NS_OK;
}

nsIFrame*
nsSVGDisplayContainerFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY),
               "If display lists are enabled, only hit-testing of a "
               "clipPath's contents should take this code path");
  return nsSVGUtils::HitTestChildren(this, aPoint);
}

nsRect
nsSVGDisplayContainerFrame::GetCoveredRegion()
{
  return nsSVGUtils::GetCoveredRegion(mFrames);
}

void
nsSVGDisplayContainerFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                    "ReflowSVG mechanism not designed for this");

  NS_ABORT_IF_FALSE(GetType() != nsGkAtoms::svgOuterSVGFrame,
                    "Do not call on outer-<svg>");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  
  
  
  
  
  

  bool isFirstReflow = (mState & NS_FRAME_FIRST_REFLOW);

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
      NS_ABORT_IF_FALSE(!(kid->GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                        "Check for this explicitly in the |if|, then");
      kid->AddStateBits(mState & NS_FRAME_IS_DIRTY);
      SVGFrame->ReflowSVG();

      
      
      
      ConsiderChildOverflow(overflowRects, kid);
    } else {
      
      
      
      NS_ASSERTION(kid->GetStateBits() & NS_FRAME_IS_NONDISPLAY,
                   "expected kid to be a NS_FRAME_IS_NONDISPLAY frame");
      if (kid->GetStateBits() & NS_FRAME_IS_DIRTY) {
        nsSVGContainerFrame* container = do_QueryFrame(kid);
        if (container && container->GetContent()->IsSVG()) {
          ReflowSVGNonDisplayText(container);
        }
      }
    }
  }

  
  
  
  
  
  
  
  NS_ABORT_IF_FALSE(mContent->Tag() == nsGkAtoms::svg ||
                    (mContent->Tag() == nsGkAtoms::use &&
                     mRect.Size() == nsSize(0,0)) ||
                    mRect.IsEqualEdges(nsRect()),
                    "Only inner-<svg>/<use> is expected to have mRect set");

  if (isFirstReflow) {
    
    
    
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
  const Matrix &aToBBoxUserspace,
  uint32_t aFlags)
{
  SVGBBox bboxUnion;

  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsIContent *content = kid->GetContent();
    nsISVGChildFrame* svgKid = do_QueryFrame(kid);
    
    if (svgKid && (!content->IsSVG() ||
                   static_cast<const nsSVGElement*>(content)->HasValidDimensions())) {

      gfxMatrix transform = gfx::ThebesMatrix(aToBBoxUserspace);
      if (content->IsSVG()) {
        transform = static_cast<nsSVGElement*>(content)->
                      PrependLocalTransformsTo(transform);
      }
      
      
      bboxUnion.UnionEdges(svgKid->GetBBoxContribution(gfx::ToMatrix(transform), aFlags));
    }
    kid = kid->GetNextSibling();
  }

  return bboxUnion;
}

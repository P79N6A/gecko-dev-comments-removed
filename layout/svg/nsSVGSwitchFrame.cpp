





#include "gfxRect.h"
#include "nsSVGEffects.h"
#include "nsSVGGFrame.h"
#include "mozilla/dom/SVGSwitchElement.h"
#include "nsSVGUtils.h"

class nsRenderingContext;

using namespace mozilla::gfx;

typedef nsSVGGFrame nsSVGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGSwitchFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGSwitch"), aResult);
  }
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  
  virtual nsresult PaintSVG(nsRenderingContext* aContext,
                            const nsIntRect *aDirtyRect,
                            nsIFrame* aTransformRoot) MOZ_OVERRIDE;
  nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;
  nsRect GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;

private:
  nsIFrame *GetActiveChildFrame();
};




nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGSwitchFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGSwitchFrame)

#ifdef DEBUG
void
nsSVGSwitchFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::svgSwitch),
               "Content is not an SVG switch");

  nsSVGSwitchFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

void
nsSVGSwitchFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  nsIFrame* kid = GetActiveChildFrame();
  if (kid) {
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
  }
}

nsresult
nsSVGSwitchFrame::PaintSVG(nsRenderingContext* aContext,
                           const nsIntRect *aDirtyRect,
                           nsIFrame* aTransformRoot)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  if (StyleDisplay()->mOpacity == 0.0)
    return NS_OK;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsSVGUtils::PaintFrameWithEffects(aContext, aDirtyRect, kid, aTransformRoot);
  }
  return NS_OK;
}


nsIFrame*
nsSVGSwitchFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY),
               "If display lists are enabled, only hit-testing of non-display "
               "SVG should take this code path");

  nsIFrame *kid = GetActiveChildFrame();
  nsISVGChildFrame* svgFrame = do_QueryFrame(kid);
  if (svgFrame) {
    
    gfxPoint point = aPoint;
    gfxMatrix m =
      static_cast<const nsSVGElement*>(mContent)->
        PrependLocalTransformsTo(gfxMatrix(), nsSVGElement::eChildToUserSpace);
    m = static_cast<const nsSVGElement*>(kid->GetContent())->
          PrependLocalTransformsTo(m, nsSVGElement::eUserSpaceToParent);
    if (!m.IsIdentity()) {
      if (!m.Invert()) {
        return nullptr;
      }
      point = m.Transform(point);
    }
    return svgFrame->GetFrameForPoint(point);
  }

  return nullptr;
}

nsRect
nsSVGSwitchFrame::GetCoveredRegion()
{
  nsRect rect;

  nsIFrame *kid = GetActiveChildFrame();
  nsISVGChildFrame* child = do_QueryFrame(kid);
  if (child) {
    rect = child->GetCoveredRegion();
  }
  return rect;
}

void
nsSVGSwitchFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                    "ReflowSVG mechanism not designed for this");

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

  nsIFrame *child = GetActiveChildFrame();
  nsISVGChildFrame* svgChild = do_QueryFrame(child);
  if (svgChild) {
    NS_ABORT_IF_FALSE(!(child->GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                      "Check for this explicitly in the |if|, then");
    svgChild->ReflowSVG();

    
    
    
    ConsiderChildOverflow(overflowRects, child);
  }

  if (isFirstReflow) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  FinishAndStoreOverflow(overflowRects, mRect.Size());

  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
}

SVGBBox
nsSVGSwitchFrame::GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags)
{
  nsIFrame* kid = GetActiveChildFrame();
  nsISVGChildFrame* svgKid = do_QueryFrame(kid);
  if (svgKid) {
    nsIContent *content = kid->GetContent();
    gfxMatrix transform = ThebesMatrix(aToBBoxUserspace);
    if (content->IsSVG()) {
      transform = static_cast<nsSVGElement*>(content)->
                    PrependLocalTransformsTo(transform);
    }
    return svgKid->GetBBoxContribution(ToMatrix(transform), aFlags);
  }
  return SVGBBox();
}

nsIFrame *
nsSVGSwitchFrame::GetActiveChildFrame()
{
  nsIContent *activeChild =
    static_cast<mozilla::dom::SVGSwitchElement*>(mContent)->GetActiveChild();

  if (activeChild) {
    for (nsIFrame* kid = mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {

      if (activeChild == kid->GetContent()) {
        return kid;
      }
    }
  }
  return nullptr;
}

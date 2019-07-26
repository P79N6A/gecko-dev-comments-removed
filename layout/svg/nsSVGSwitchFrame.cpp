





#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsSVGEffects.h"
#include "nsSVGGFrame.h"
#include "mozilla/dom/SVGSwitchElement.h"
#include "nsSVGUtils.h"

class nsRenderingContext;

typedef nsSVGGFrame nsSVGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGSwitchFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGSwitch"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext, const nsIntRect *aDirtyRect);
  NS_IMETHODIMP_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHODIMP_(nsRect) GetCoveredRegion();
  virtual void ReflowSVG();
  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags);

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
NS_IMETHODIMP
nsSVGSwitchFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::svgSwitch),
               "Content is not an SVG switch\n");

  return nsSVGSwitchFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

NS_IMETHODIMP
nsSVGSwitchFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  nsIFrame* kid = GetActiveChildFrame();
  if (kid) {
    return BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGSwitchFrame::PaintSVG(nsRenderingContext* aContext,
                           const nsIntRect *aDirtyRect)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsSVGUtils::PaintFrameWithEffects(aContext, aDirtyRect, kid);
  }
  return NS_OK;
}


NS_IMETHODIMP_(nsIFrame*)
nsSVGSwitchFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only hit-testing of non-display "
               "SVG should take this code path");

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame = do_QueryFrame(kid);
    if (svgFrame) {
      return svgFrame->GetFrameForPoint(aPoint);
    }
  }

  return nullptr;
}

NS_IMETHODIMP_(nsRect)
nsSVGSwitchFrame::GetCoveredRegion()
{
  nsRect rect;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* child = do_QueryFrame(kid);
    if (child) {
      rect = child->GetCoveredRegion();
    }
  }
  return rect;
}

void
nsSVGSwitchFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  
  
  
  
  
  

  bool outerSVGHasHadFirstReflow =
    (GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) == 0;

  if (outerSVGHasHadFirstReflow) {
    mState &= ~NS_FRAME_FIRST_REFLOW; 
  }

  nsOverflowAreas overflowRects;

  nsIFrame *child = GetActiveChildFrame();
  if (child) {
    nsISVGChildFrame* svgChild = do_QueryFrame(child);
    if (svgChild) {
      NS_ABORT_IF_FALSE(!(child->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                        "Check for this explicitly in the |if|, then");
      svgChild->ReflowSVG();

      
      
      
      ConsiderChildOverflow(overflowRects, child);
    }
  }

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  FinishAndStoreOverflow(overflowRects, mRect.Size());

  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
}

SVGBBox
nsSVGSwitchFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags)
{
  nsIFrame* kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgKid = do_QueryFrame(kid);
    if (svgKid) {
      nsIContent *content = kid->GetContent();
      gfxMatrix transform = aToBBoxUserspace;
      if (content->IsSVG()) {
        transform = static_cast<nsSVGElement*>(content)->
                      PrependLocalTransformsTo(aToBBoxUserspace);
      }
      return svgKid->GetBBoxContribution(transform, aFlags);
    }
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

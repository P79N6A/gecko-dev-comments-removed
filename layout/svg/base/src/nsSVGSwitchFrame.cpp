





































#include "nsSVGGFrame.h"
#include "nsSVGSwitchElement.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

typedef nsSVGGFrame nsSVGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGSwitchFrameBase(aContext) {}

public:
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

  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, const nsIntRect *aDirtyRect);
  NS_IMETHODIMP_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHODIMP_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyRedrawUnsuspended();
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace);

private:
  nsIFrame *GetActiveChildFrame();
};




nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGSwitchFrame(aContext);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGSwitchFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGSwitchElement> svgSwitch = do_QueryInterface(aContent);
  NS_ASSERTION(svgSwitch, "Content is not an SVG switch\n");

  return nsSVGSwitchFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

NS_IMETHODIMP
nsSVGSwitchFrame::PaintSVG(nsSVGRenderState* aContext,
                           const nsIntRect *aDirtyRect)
{
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
  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame = do_QueryFrame(kid);
    if (svgFrame) {
      return svgFrame->GetFrameForPoint(aPoint);
    }
  }

  return nsnull;
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

NS_IMETHODIMP
nsSVGSwitchFrame::UpdateCoveredRegion()
{
  static_cast<nsSVGSwitchElement*>(mContent)->UpdateActiveChild();

  return nsSVGSwitchFrameBase::UpdateCoveredRegion();
}

NS_IMETHODIMP
nsSVGSwitchFrame::InitialUpdate()
{
  nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::InitialUpdate();
}

NS_IMETHODIMP
nsSVGSwitchFrame::NotifyRedrawUnsuspended()
{
  if (GetStateBits() & NS_STATE_SVG_DIRTY)
    nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::NotifyRedrawUnsuspended();
}

gfxRect
nsSVGSwitchFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace)
{
  nsIFrame* kid = GetActiveChildFrame();
  nsISVGChildFrame* svgKid = do_QueryFrame(kid);
  if (svgKid) {
    nsIContent *content = kid->GetContent();
    gfxMatrix transform = aToBBoxUserspace;
    if (content->IsSVG()) {
      transform = static_cast<nsSVGElement*>(content)->
                    PrependLocalTransformTo(aToBBoxUserspace);
    }
    return svgKid->GetBBoxContribution(transform);
  }
  return gfxRect(0.0, 0.0, 0.0, 0.0);
}

nsIFrame *
nsSVGSwitchFrame::GetActiveChildFrame()
{
  nsIContent *activeChild =
    static_cast<nsSVGSwitchElement*>(mContent)->GetActiveChild();

  if (activeChild) {
    for (nsIFrame* kid = mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {

      if (activeChild == kid->GetContent()) {
        return kid;
      }
    }
  }
  return nsnull;
}

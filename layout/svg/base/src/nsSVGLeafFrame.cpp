



































#include "nsFrame.h"
#include "nsSVGEffects.h"
#include "nsImageLoadingContent.h"

class nsSVGLeafFrame : public nsFrame
{
  friend nsIFrame*
  NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGLeafFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        asPrevInFlow);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLeaf"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
};

nsIFrame*
NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGLeafFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGLeafFrame)

NS_IMETHODIMP
nsSVGLeafFrame::Init(nsIContent* aContent, nsIFrame* aParent, nsIFrame* asPrevInFlow)
{
  nsFrame::Init(aContent, aParent, asPrevInFlow);
  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(nsFrame::mContent);

  if (imageLoader) {
    imageLoader->FrameCreated(this);
  }

  return NS_OK;
}

 void
nsSVGLeafFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(nsFrame::mContent);

  if (imageLoader) {
    imageLoader->FrameDestroyed(this);
  }

  nsFrame::DestroyFrom(aDestructRoot);
}

 void
nsSVGLeafFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

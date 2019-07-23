



































#include "nsFrame.h"
#include "nsSVGEffects.h"

class nsSVGLeafFrame : public nsFrame
{
  friend nsIFrame*
  NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGLeafFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

public:
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
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

 void
nsSVGLeafFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

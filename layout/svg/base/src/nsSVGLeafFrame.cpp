



































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

  NS_IMETHOD DidSetStyleContext();
};

nsIFrame*
NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGLeafFrame(aContext);
}

NS_IMETHODIMP
nsSVGLeafFrame::DidSetStyleContext()
{
  nsresult rv = nsFrame::DidSetStyleContext();
  nsSVGEffects::InvalidateRenderingObservers(this);
  return rv;
}

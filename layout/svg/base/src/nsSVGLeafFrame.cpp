



































#include "nsFrame.h"

class nsSVGLeafFrame : public nsFrame
{
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

public:
  nsSVGLeafFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
};

nsIFrame*
NS_NewSVGLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGLeafFrame(aContext);
}

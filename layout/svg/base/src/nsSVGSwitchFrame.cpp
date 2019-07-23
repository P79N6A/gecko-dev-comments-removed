





































#include "nsSVGGFrame.h"
#include "nsIDOMSVGSwitchElement.h"

typedef nsSVGGFrame nsSVGGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGGSwitchFrameBase(aContext) {}

public:
  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGSwitch"), aResult);
  }
#endif
};




nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{  
  nsCOMPtr<nsIDOMSVGSwitchElement> svgSwitch = do_QueryInterface(aContent);
  if (!svgSwitch) {
    NS_ERROR("Can't create frame. Content is not an SVG switch\n");
    return nsnull;
  }

  return new (aPresShell) nsSVGSwitchFrame(aContext);
}

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

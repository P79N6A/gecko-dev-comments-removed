





































#ifdef IBMBIDI

#include "nsBidiFrames.h"
#include "nsGkAtoms.h"


nsDirectionalFrame::nsDirectionalFrame(nsStyleContext* aContext)
  : nsFrame(aContext)
{
}

nsDirectionalFrame::~nsDirectionalFrame()
{
}

nsIAtom*
nsDirectionalFrame::GetType() const
{ 
  return nsGkAtoms::directionalFrame;
}
  
#ifdef NS_DEBUG
NS_IMETHODIMP
nsDirectionalFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Directional"), aResult);
}
#endif

nsIFrame*
NS_NewDirectionalFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsDirectionalFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsDirectionalFrame)

#endif 

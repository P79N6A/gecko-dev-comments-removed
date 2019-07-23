





































#ifdef IBMBIDI

#include "nsBidiFrames.h"
#include "nsGkAtoms.h"


nsDirectionalFrame::nsDirectionalFrame(nsStyleContext* aContext, PRUnichar aChar)
  : nsFrame(aContext), mChar(aChar)
{
}

nsDirectionalFrame::~nsDirectionalFrame()
{
}

PRUnichar
nsDirectionalFrame::GetChar(void) const
{
  return mChar;
}






nsIAtom*
nsDirectionalFrame::GetType() const
{ 
  return nsGkAtoms::directionalFrame; 
}
  
NS_IMETHODIMP
nsDirectionalFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  nsresult rv = NS_NOINTERFACE;

  if (!aInstancePtr) {
    rv = NS_ERROR_NULL_POINTER;
  }
  else if (aIID.Equals(NS_GET_IID(nsDirectionalFrame) ) ) {
    *aInstancePtr = (void*) this;
    rv = NS_OK;
  }
  return rv;
}

nsIFrame*
NS_NewDirectionalFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUnichar aChar)
{
  return new (aPresShell) nsDirectionalFrame(aContext, aChar);
}

#endif 

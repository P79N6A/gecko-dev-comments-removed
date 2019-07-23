





































#ifdef IBMBIDI

#ifndef nsBidiFrames_h___
#define nsBidiFrames_h___

#include "nsFrame.h"




#define NS_DIRECTIONAL_FRAME_IID \
{ 0x90d69900, 0x67af, 0x11d4, { 0xba, 0x59, 0x00, 0x60, 0x08, 0xcd, 0x37, 0x17 } }

class nsDirectionalFrame : public nsFrame
{
protected:
  virtual ~nsDirectionalFrame();

public:
  nsDirectionalFrame(nsStyleContext* aContext, PRUnichar aChar);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DIRECTIONAL_FRAME_IID)

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  virtual nsIAtom* GetType() const;

  PRUnichar GetChar(void) const;

private:
  PRUnichar mChar;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDirectionalFrame, NS_DIRECTIONAL_FRAME_IID)

#endif 
#endif 








































#ifdef IBMBIDI

#ifndef nsBidiFrames_h___
#define nsBidiFrames_h___

#include "nsFrame.h"


class nsDirectionalFrame : public nsFrame
{
protected:
  virtual ~nsDirectionalFrame();

public:
  nsDirectionalFrame(nsStyleContext* aContext, PRUnichar aChar);

  




  virtual nsIAtom* GetType() const;

  PRUnichar GetChar() const { return mChar; }

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

private:
  PRUnichar mChar;
};


#endif 
#endif 







































#ifdef IBMBIDI

#ifndef nsBidiFrames_h___
#define nsBidiFrames_h___

#include "nsFrame.h"


class nsDirectionalFrame : public nsFrame
{
protected:
  virtual ~nsDirectionalFrame();

public:
  NS_DECL_FRAMEARENA_HELPERS

  nsDirectionalFrame(nsStyleContext* aContext);

  




  virtual nsIAtom* GetType() const;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
};


#endif 
#endif 






#ifndef nsIFormControlFrame_h___
#define nsIFormControlFrame_h___

#include "nsQueryFrame.h"
class nsAString;
class nsIContent;
class nsIAtom;
struct nsSize;






class nsIFormControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIFormControlFrame)

  




  virtual void SetFocus(bool aOn = true, bool aRepaint = false) = 0;

  






  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) = 0;
};

#endif


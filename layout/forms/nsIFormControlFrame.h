




































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
  NS_DECLARE_FRAME_ACCESSOR(nsIFormControlFrame)

  




  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE) = 0;

  






  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) = 0;
  
  







  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const = 0;
};

#endif


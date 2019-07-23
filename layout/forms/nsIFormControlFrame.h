




































#ifndef nsIFormControlFrame_h___
#define nsIFormControlFrame_h___

#include "nsISupports.h"
class nsAString;
class nsIContent;
class nsIAtom;
struct nsSize;


#define NS_IFORMCONTROLFRAME_IID    \
  { 0x189e1565, 0x44f, 0x11da, \
      { 0x94, 0xfc, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f } }






class nsIFormControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMCONTROLFRAME_IID)

  




  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE) = 0;

  






  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) = 0;
  
  







  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormControlFrame, NS_IFORMCONTROLFRAME_IID)

#endif


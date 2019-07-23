





































#ifndef nsIRadioVisitor_h___
#define nsIRadioVisitor_h___

#include "nsISupports.h"
class nsIFormControl;


#define NS_IRADIOVISITOR_IID \
{ 0xd3494bd2, 0x1dd1, 0x11b2, \
  { 0xbe, 0x86, 0xb5, 0x08, 0xc8, 0x71, 0xd7, 0xc5 } }





class nsIRadioVisitor : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOVISITOR_IID)

  











  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioVisitor, NS_IRADIOVISITOR_IID)







nsresult
NS_GetRadioSetCheckedChangedVisitor(PRBool aCheckedChanged,
                                    nsIRadioVisitor** aVisitor);










nsresult
NS_GetRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                    nsIFormControl* aExcludeElement,
                                    nsIRadioVisitor** aVisitor);

#endif 

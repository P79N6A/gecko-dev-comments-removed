





































#ifndef nsIRadioVisitor_h___
#define nsIRadioVisitor_h___

#include "nsISupports.h"
class nsIFormControl;
class nsIDocument;


#define NS_IRADIOVISITOR_IID \
{ 0xc6bed232, 0x1181, 0x4ab2, \
  { 0xa1, 0xda, 0x55, 0xc2, 0x13, 0x6d, 0xea, 0x3d } }





class nsIRadioVisitor : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOVISITOR_IID)

  










  virtual PRBool Visit(nsIFormControl* aRadio) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioVisitor, NS_IRADIOVISITOR_IID)







nsresult
NS_GetRadioSetCheckedChangedVisitor(bool aCheckedChanged,
                                    nsIRadioVisitor** aVisitor);










nsresult
NS_GetRadioGetCheckedChangedVisitor(bool* aCheckedChanged,
                                    nsIFormControl* aExcludeElement,
                                    nsIRadioVisitor** aVisitor);










nsIRadioVisitor*
NS_SetRadioValueMissingState(nsIFormControl* aExcludeElement,
                             nsIDocument* aDocument,
                             bool aValidity, bool aNotify);

#endif 

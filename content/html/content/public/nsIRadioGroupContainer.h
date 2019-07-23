



































#ifndef nsIRadioGroupContainer_h___
#define nsIRadioGroupContainer_h___

#include "nsISupports.h"

class nsIDOMHTMLInputElement;
class nsString;
class nsIRadioVisitor;
class nsIFormControl;

#define NS_IRADIOGROUPCONTAINER_IID   \
{ 0x06de7839, 0xd0db, 0x47d3, \
  { 0x82, 0x90, 0x3c, 0xb8, 0x62, 0x2e, 0xd9, 0x66 } }




class nsIRadioGroupContainer : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOGROUPCONTAINER_IID)

  






  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            PRBool aFlushContent) = 0;

  




  NS_IMETHOD SetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement* aRadio) = 0;

  




  NS_IMETHOD GetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement** aRadio) = 0;

  






  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const PRBool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadio) = 0;

  








  NS_IMETHOD AddToRadioGroup(const nsAString& aName,
                             nsIFormControl* aRadio) = 0;

  








  NS_IMETHOD RemoveFromRadioGroup(const nsAString& aName,
                                  nsIFormControl* aRadio) = 0;

  








  NS_IMETHOD GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                                PRInt32 *aPositionIndex,
                                PRInt32 *aItemsInGroup) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioGroupContainer,
                              NS_IRADIOGROUPCONTAINER_IID)

#endif 

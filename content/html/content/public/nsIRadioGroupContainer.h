



































#ifndef nsIRadioGroupContainer_h___
#define nsIRadioGroupContainer_h___

#include "nsISupports.h"

class nsIDOMHTMLInputElement;
class nsString;
class nsIRadioVisitor;
class nsIFormControl;

#define NS_IRADIOGROUPCONTAINER_IID   \
{ 0x22924a01, 0x4360, 0x401b, \
  { 0xb1, 0xd1, 0x56, 0x8d, 0xf5, 0xa3, 0xda, 0x71 } }




class nsIRadioGroupContainer : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOGROUPCONTAINER_IID)

  






  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            bool aFlushContent) = 0;

  




  NS_IMETHOD SetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement* aRadio) = 0;

  




  NS_IMETHOD GetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement** aRadio) = 0;

  






  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const bool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadio) = 0;

  








  NS_IMETHOD AddToRadioGroup(const nsAString& aName,
                             nsIFormControl* aRadio) = 0;

  








  NS_IMETHOD RemoveFromRadioGroup(const nsAString& aName,
                                  nsIFormControl* aRadio) = 0;

  








  NS_IMETHOD GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                                PRInt32 *aPositionIndex,
                                PRInt32 *aItemsInGroup) = 0;

  virtual PRUint32 GetRequiredRadioCount(const nsAString& aName) const = 0;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio) = 0;
  virtual bool GetValueMissingState(const nsAString& aName) const = 0;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioGroupContainer,
                              NS_IRADIOGROUPCONTAINER_IID)

#endif 

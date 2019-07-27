



#ifndef nsIRadioGroupContainer_h___
#define nsIRadioGroupContainer_h___

#include "nsISupports.h"

class nsIRadioVisitor;
class nsIFormControl;

namespace mozilla {
namespace dom {
class HTMLInputElement;
}
}

#define NS_IRADIOGROUPCONTAINER_IID   \
{ 0x800320a0, 0x733f, 0x11e4, \
  { 0x82, 0xf8, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }




class nsIRadioGroupContainer : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOGROUPCONTAINER_IID)

  






  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            bool aFlushContent) = 0;

  




  virtual void SetCurrentRadioButton(const nsAString& aName,
                                     mozilla::dom::HTMLInputElement* aRadio) = 0;

  




  virtual mozilla::dom::HTMLInputElement* GetCurrentRadioButton(const nsAString& aName) = 0;

  






  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const bool aPrevious,
                                mozilla::dom::HTMLInputElement*  aFocusedRadio,
                                mozilla::dom::HTMLInputElement** aRadio) = 0;

  








  virtual void AddToRadioGroup(const nsAString& aName, nsIFormControl* aRadio) = 0;

  








  virtual void RemoveFromRadioGroup(const nsAString& aName, nsIFormControl* aRadio) = 0;

  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const = 0;
  virtual void RadioRequiredWillChange(const nsAString& aName,
                                       bool aRequiredAdded) = 0;
  virtual bool GetValueMissingState(const nsAString& aName) const = 0;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioGroupContainer,
                              NS_IRADIOGROUPCONTAINER_IID)

#endif 

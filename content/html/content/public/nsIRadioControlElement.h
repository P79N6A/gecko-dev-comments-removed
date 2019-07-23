





































#ifndef nsIRadioControlElement_h___
#define nsIRadioControlElement_h___

#include "nsISupports.h"
#include "nsIRadioGroupContainer.h"
class nsAString;
class nsIForm;


#define NS_IRADIOCONTROLELEMENT_IID \
{ 0x647dff84, 0x1dd2, 0x11b2, \
    { 0x95, 0xcf, 0xe5, 0x01, 0xa8, 0x54, 0xa6, 0x40 } }





class nsIRadioControlElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOCONTROLELEMENT_IID)

  




  NS_IMETHOD RadioSetChecked(PRBool aNotify) = 0;

  









  NS_IMETHOD SetCheckedChangedInternal(PRBool aCheckedChanged) = 0;

  






  NS_IMETHOD SetCheckedChanged(PRBool aCheckedChanged) = 0;

  






  NS_IMETHOD GetCheckedChanged(PRBool* aCheckedChanged) = 0;

  



  NS_IMETHOD AddedToRadioGroup(PRBool aNotify = PR_TRUE) = 0;

  




  NS_IMETHOD WillRemoveFromRadioGroup() = 0;

  



  virtual already_AddRefed<nsIRadioGroupContainer> GetRadioGroupContainer() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioControlElement,
                              NS_IRADIOCONTROLELEMENT_IID)

#endif 

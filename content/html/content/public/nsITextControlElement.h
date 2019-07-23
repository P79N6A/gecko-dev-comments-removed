





































#ifndef nsITextControlElement_h___
#define nsITextControlElement_h___

#include "nsISupports.h"
class nsAString;
class nsITextControlFrame;


#define NS_ITEXTCONTROLELEMENT_IID    \
{ 0x8c22af1e, 0x1dd2, 0x11b2,    \
  { 0x9d, 0x72, 0xb4, 0xc1, 0x53, 0x68, 0xdc, 0xa1 } }





class nsITextControlElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTCONTROLELEMENT_IID)

  


  NS_IMETHOD TakeTextFrameValue(const nsAString& aValue) = 0;

  


  NS_IMETHOD SetValueChanged(PRBool changed) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextControlElement,
                              NS_ITEXTCONTROLELEMENT_IID)

#endif 


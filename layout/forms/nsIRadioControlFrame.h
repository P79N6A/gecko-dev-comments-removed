




































#ifndef nsIRadioControlFrame_h___
#define nsIRadioControlFrame_h___

#include "nsISupports.h"
class nsStyleContext;



#define NS_IRADIOCONTROLFRAME_IID    \
{ 0x6450e00, 0x24d9, 0x11d3,  \
  { 0x96, 0x6b, 0x0, 0x10, 0x5a, 0x1b, 0x1b, 0x76 } }






class nsIRadioControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRADIOCONTROLFRAME_IID)

  



   NS_IMETHOD SetRadioButtonFaceStyleContext(nsStyleContext *aRadioButtonFaceStyleContext) = 0;

   


   NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRadioControlFrame, NS_IRADIOCONTROLFRAME_IID)

#endif


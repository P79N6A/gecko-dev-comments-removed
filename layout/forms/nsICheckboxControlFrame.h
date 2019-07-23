




































#ifndef nsICheckControlFrame_h___
#define nsICheckControlFrame_h___

#include "nsISupports.h"
class nsStyleContext;
class nsPresContext;



#define NS_ICHECKBOXCONTROLFRAME_IID    \
{ 0x401347ed, 0x101, 0x11d4,  \
  { 0x97, 0x6, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 } }





class nsICheckboxControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHECKBOXCONTROLFRAME_IID)

  


  NS_IMETHOD SetCheckboxFaceStyleContext(
                 nsStyleContext* aCheckboxFaceStyleContext) = 0;
  
  


  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICheckboxControlFrame,
                              NS_ICHECKBOXCONTROLFRAME_IID)

#endif


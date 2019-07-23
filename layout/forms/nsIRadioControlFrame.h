




































#ifndef nsIRadioControlFrame_h___
#define nsIRadioControlFrame_h___

#include "nsQueryFrame.h"
class nsStyleContext;





class nsIRadioControlFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIRadioControlFrame)

  



   NS_IMETHOD SetRadioButtonFaceStyleContext(nsStyleContext *aRadioButtonFaceStyleContext) = 0;

   


   NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked) = 0;
};

#endif


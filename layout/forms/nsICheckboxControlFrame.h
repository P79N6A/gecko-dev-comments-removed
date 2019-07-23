




































#ifndef nsICheckControlFrame_h___
#define nsICheckControlFrame_h___

#include "nsQueryFrame.h"
class nsStyleContext;
class nsPresContext;





class nsICheckboxControlFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsICheckboxControlFrame)
  
  


  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked) = 0;
};

#endif


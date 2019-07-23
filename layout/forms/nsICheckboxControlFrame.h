




































#ifndef nsICheckControlFrame_h___
#define nsICheckControlFrame_h___

#include "nsQueryFrame.h"
class nsStyleContext;
class nsPresContext;





class nsICheckboxControlFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsICheckboxControlFrame)
  
  


  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked) = 0;
};

#endif


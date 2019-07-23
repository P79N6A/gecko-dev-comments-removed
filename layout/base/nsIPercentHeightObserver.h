




































#ifndef nsIPercentHeightObserver_h___
#define nsIPercentHeightObserver_h___

#include "nsQueryFrame.h"

struct nsHTMLReflowState;
class  nsPresContext;






class nsIPercentHeightObserver
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIPercentHeightObserver)

  
  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState) = 0;

  
  virtual PRBool NeedsToObserve(const nsHTMLReflowState& aReflowState) = 0;
};

#endif 






































#ifndef nsIPercentHeightObserver_h___
#define nsIPercentHeightObserver_h___

#include "nsQueryFrame.h"

struct nsHTMLReflowState;






class nsIPercentHeightObserver
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIPercentHeightObserver)

  
  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState) = 0;

  
  virtual bool NeedsToObserve(const nsHTMLReflowState& aReflowState) = 0;
};

#endif 

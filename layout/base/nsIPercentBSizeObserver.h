




#ifndef nsIPercentBSizeObserver_h___
#define nsIPercentBSizeObserver_h___

#include "nsQueryFrame.h"

struct nsHTMLReflowState;






class nsIPercentBSizeObserver
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIPercentBSizeObserver)

  
  
  virtual void NotifyPercentBSize(const nsHTMLReflowState& aReflowState) = 0;

  
  virtual bool NeedsToObserve(const nsHTMLReflowState& aReflowState) = 0;
};

#endif 

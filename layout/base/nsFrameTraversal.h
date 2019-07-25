



































#ifndef NSFRAMETRAVERSAL_H
#define NSFRAMETRAVERSAL_H

#include "nsIFrame.h"
#include "nsIFrameTraversal.h"

nsresult NS_NewFrameTraversal(nsIFrameEnumerator **aEnumerator,
                              nsPresContext* aPresContext,
                              nsIFrame *aStart,
                              nsIteratorType aType,
                              PRBool aVisual,
                              PRBool aLockInScrollView,
                              PRBool aFollowOOFs);

nsresult NS_CreateFrameTraversal(nsIFrameTraversal** aResult);

class nsFrameTraversal : public nsIFrameTraversal
{
public:
  nsFrameTraversal();
  virtual ~nsFrameTraversal();

  NS_DECL_ISUPPORTS

  NS_IMETHOD NewFrameTraversal(nsIFrameEnumerator **aEnumerator,
                               nsPresContext* aPresContext,
                               nsIFrame *aStart,
                               PRInt32 aType,
                               PRBool aVisual,
                               PRBool aLockInScrollView,
                               PRBool aFollowOOFs);
};

#endif 

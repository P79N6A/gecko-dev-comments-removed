



#ifndef NSFRAMETRAVERSAL_H
#define NSFRAMETRAVERSAL_H

#include "mozilla/Attributes.h"
#include "nsIFrameTraversal.h"

class nsIFrame;

nsresult NS_NewFrameTraversal(nsIFrameEnumerator **aEnumerator,
                              nsPresContext* aPresContext,
                              nsIFrame *aStart,
                              nsIteratorType aType,
                              bool aVisual,
                              bool aLockInScrollView,
                              bool aFollowOOFs,
                              bool aSkipPopupChecks);

nsresult NS_CreateFrameTraversal(nsIFrameTraversal** aResult);

class nsFrameTraversal : public nsIFrameTraversal
{
public:
  nsFrameTraversal();

  NS_DECL_ISUPPORTS

  NS_IMETHOD NewFrameTraversal(nsIFrameEnumerator **aEnumerator,
                               nsPresContext* aPresContext,
                               nsIFrame *aStart,
                               int32_t aType,
                               bool aVisual,
                               bool aLockInScrollView,
                               bool aFollowOOFs,
                               bool aSkipPopupChecks) override;

protected:
  virtual ~nsFrameTraversal();
};

#endif 

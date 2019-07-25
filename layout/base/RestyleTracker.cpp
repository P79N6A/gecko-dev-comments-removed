





































#include "RestyleTracker.h"
#include "nsCSSFrameConstructor.h"

namespace mozilla {
namespace css {

#define RESTYLE_ARRAY_STACKSIZE 128

static PLDHashOperator
CollectRestyles(nsISupports* aElement,
                RestyleTracker::RestyleData& aData,
                void* aRestyleArrayPtr)
{
  RestyleTracker::RestyleEnumerateData** restyleArrayPtr =
    static_cast<RestyleTracker::RestyleEnumerateData**>
               (aRestyleArrayPtr);
  RestyleTracker::RestyleEnumerateData* currentRestyle =
    *restyleArrayPtr;
  currentRestyle->mElement = static_cast<RestyleTracker::Element*>(aElement);
  currentRestyle->mRestyleHint = aData.mRestyleHint;
  currentRestyle->mChangeHint = aData.mChangeHint;

  
  *restyleArrayPtr = currentRestyle + 1;

  return PL_DHASH_NEXT;
}

void
RestyleTracker::ProcessRestyles()
{
  PRUint32 count = mPendingRestyles.Count();

  
  
  mFrameConstructor->BeginUpdate();

  
  while (count) {
    
    nsAutoTArray<RestyleEnumerateData, RESTYLE_ARRAY_STACKSIZE> restyleArr;
    RestyleEnumerateData* restylesToProcess = restyleArr.AppendElements(count);

    if (!restylesToProcess) {
      return;
    }

    RestyleEnumerateData* lastRestyle = restylesToProcess;
    mPendingRestyles.Enumerate(CollectRestyles, &lastRestyle);

    NS_ASSERTION(lastRestyle - restylesToProcess == PRInt32(count),
                 "Enumeration screwed up somehow");

    
    
    mPendingRestyles.Clear();

    for (RestyleEnumerateData* currentRestyle = restylesToProcess;
         currentRestyle != lastRestyle;
         ++currentRestyle) {
      mFrameConstructor->ProcessOneRestyle(currentRestyle->mElement,
                                           currentRestyle->mRestyleHint,
                                           currentRestyle->mChangeHint);
    }

    count = mPendingRestyles.Count();
  }

  
  
  mFrameConstructor->mInStyleRefresh = PR_FALSE;

  mFrameConstructor->EndUpdate();

#ifdef DEBUG
  mFrameConstructor->mPresShell->VerifyStyleTree();
#endif

}

} 
} 

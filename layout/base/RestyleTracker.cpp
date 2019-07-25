





































#include "RestyleTracker.h"
#include "nsCSSFrameConstructor.h"
#include "nsStyleChangeList.h"

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

inline void
RestyleTracker::ProcessOneRestyle(Element* aElement,
                                  nsRestyleHint aRestyleHint,
                                  nsChangeHint aChangeHint)
{
  if (aElement->GetCurrentDoc() != mFrameConstructor->mDocument) {
    
    
    return;
  }

  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();
  if (aRestyleHint & eRestyle_Self) {
    mFrameConstructor->RestyleElement(aElement, primaryFrame, aChangeHint);
  } else if (aChangeHint &&
             (primaryFrame ||
              (aChangeHint & nsChangeHint_ReconstructFrame))) {
    
    nsStyleChangeList changeList;
    changeList.AppendChange(primaryFrame, aElement, aChangeHint);
    mFrameConstructor->ProcessRestyledFrames(changeList);
  }

  if (aRestyleHint & eRestyle_LaterSiblings) {
    for (nsIContent* sibling = aElement->GetNextSibling();
         sibling;
         sibling = sibling->GetNextSibling()) {
      if (!sibling->IsElement())
        continue;

      mFrameConstructor->RestyleElement(sibling->AsElement(),
                                        sibling->GetPrimaryFrame(),
                                        NS_STYLE_HINT_NONE);
    }
  }
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
      ProcessOneRestyle(currentRestyle->mElement,
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

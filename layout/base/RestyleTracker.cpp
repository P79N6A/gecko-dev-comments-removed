









#include "RestyleTracker.h"
#include "nsCSSFrameConstructor.h"
#include "nsStyleChangeList.h"
#include "sampler.h"

namespace mozilla {
namespace css {

inline nsIDocument*
RestyleTracker::Document() const {
  return mFrameConstructor->mDocument;
}

#define RESTYLE_ARRAY_STACKSIZE 128

struct LaterSiblingCollector {
  RestyleTracker* tracker;
  nsTArray< nsRefPtr<dom::Element> >* elements;
};

static PLDHashOperator
CollectLaterSiblings(nsISupports* aElement,
                     RestyleTracker::RestyleData& aData,
                     void* aSiblingCollector)
{
  dom::Element* element =
    static_cast<dom::Element*>(aElement);
  LaterSiblingCollector* collector =
    static_cast<LaterSiblingCollector*>(aSiblingCollector);
  
  
  
  
  if (element->GetCurrentDoc() == collector->tracker->Document() &&
      element->HasFlag(collector->tracker->RestyleBit()) &&
      (aData.mRestyleHint & eRestyle_LaterSiblings)) {
    collector->elements->AppendElement(element);
  }

  return PL_DHASH_NEXT;
}

struct RestyleCollector {
  RestyleTracker* tracker;
  RestyleTracker::RestyleEnumerateData** restyleArrayPtr;
};

static PLDHashOperator
CollectRestyles(nsISupports* aElement,
                RestyleTracker::RestyleData& aData,
                void* aRestyleCollector)
{
  dom::Element* element =
    static_cast<dom::Element*>(aElement);
  RestyleCollector* collector =
    static_cast<RestyleCollector*>(aRestyleCollector);
  
  
  
  
  if (element->GetCurrentDoc() != collector->tracker->Document() ||
      !element->HasFlag(collector->tracker->RestyleBit())) {
    return PL_DHASH_NEXT;
  }

  NS_ASSERTION(!element->HasFlag(collector->tracker->RootBit()) ||
               
               (element->GetFlattenedTreeParent() &&
                (!element->GetFlattenedTreeParent()->GetPrimaryFrame()||
                 element->GetFlattenedTreeParent()->GetPrimaryFrame()->IsLeaf())) ||
               
               
               
               
               
               
               (aData.mChangeHint & nsChangeHint_ReconstructFrame),
               "Why did this not get handled while processing mRestyleRoots?");

  
  
  element->UnsetFlags(collector->tracker->RestyleBit() |
                      collector->tracker->RootBit());

  RestyleTracker::RestyleEnumerateData** restyleArrayPtr =
    collector->restyleArrayPtr;
  RestyleTracker::RestyleEnumerateData* currentRestyle =
    *restyleArrayPtr;
  currentRestyle->mElement = element;
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
  NS_PRECONDITION((aRestyleHint & eRestyle_LaterSiblings) == 0,
                  "Someone should have handled this before calling us");
  NS_PRECONDITION(Document(), "Must have a document");
  NS_PRECONDITION(aElement->GetCurrentDoc() == Document(),
                  "Element has unexpected document");

  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();
  if (aRestyleHint & (eRestyle_Self | eRestyle_Subtree)) {
    mFrameConstructor->RestyleElement(aElement, primaryFrame, aChangeHint,
                                      *this,
                                      (aRestyleHint & eRestyle_Subtree) != 0);
  } else if (aChangeHint &&
             (primaryFrame ||
              (aChangeHint & nsChangeHint_ReconstructFrame))) {
    
    nsStyleChangeList changeList;
    changeList.AppendChange(primaryFrame, aElement, aChangeHint);
    mFrameConstructor->ProcessRestyledFrames(changeList);
  }
}

void
RestyleTracker::DoProcessRestyles()
{
  PROFILER_LABEL("CSS", "ProcessRestyles");
  
  
  mFrameConstructor->BeginUpdate();

  mFrameConstructor->mInStyleRefresh = true;

  
  while (mPendingRestyles.Count()) {
    if (mHaveLaterSiblingRestyles) {
      
      nsAutoTArray<nsRefPtr<Element>, RESTYLE_ARRAY_STACKSIZE> laterSiblingArr;
      LaterSiblingCollector siblingCollector = { this, &laterSiblingArr };
      mPendingRestyles.Enumerate(CollectLaterSiblings, &siblingCollector);
      for (uint32_t i = 0; i < laterSiblingArr.Length(); ++i) {
        Element* element = laterSiblingArr[i];
        for (nsIContent* sibling = element->GetNextSibling();
             sibling;
             sibling = sibling->GetNextSibling()) {
          if (sibling->IsElement() &&
              AddPendingRestyle(sibling->AsElement(), eRestyle_Subtree,
                                NS_STYLE_HINT_NONE)) {
              
              
            break;
          }
        }
      }

      
      for (uint32_t i = 0; i < laterSiblingArr.Length(); ++i) {
        Element* element = laterSiblingArr[i];
        NS_ASSERTION(element->HasFlag(RestyleBit()), "How did that happen?");
        RestyleData data;
#ifdef DEBUG
        bool found =
#endif
          mPendingRestyles.Get(element, &data);
        NS_ASSERTION(found, "Where did our entry go?");
        data.mRestyleHint =
          nsRestyleHint(data.mRestyleHint & ~eRestyle_LaterSiblings);

        mPendingRestyles.Put(element, data);
      }

      mHaveLaterSiblingRestyles = false;
    }

    uint32_t rootCount;
    while ((rootCount = mRestyleRoots.Length())) {
      
      
      
      nsRefPtr<Element> element;
      element.swap(mRestyleRoots[rootCount - 1]);
      mRestyleRoots.RemoveElementAt(rootCount - 1);

      
      
      
      if (element->GetCurrentDoc() != Document()) {
        
        
        continue;
      }

      RestyleData data;
      if (!GetRestyleData(element, &data)) {
        continue;
      }

      ProcessOneRestyle(element, data.mRestyleHint, data.mChangeHint);
    }

    if (mHaveLaterSiblingRestyles) {
      
      continue;
    }

    
    
    
    
    
    nsAutoTArray<RestyleEnumerateData, RESTYLE_ARRAY_STACKSIZE> restyleArr;
    RestyleEnumerateData* restylesToProcess =
      restyleArr.AppendElements(mPendingRestyles.Count());
    if (restylesToProcess) {
      RestyleEnumerateData* lastRestyle = restylesToProcess;
      RestyleCollector collector = { this, &lastRestyle };
      mPendingRestyles.Enumerate(CollectRestyles, &collector);

      
      mPendingRestyles.Clear();

      for (RestyleEnumerateData* currentRestyle = restylesToProcess;
           currentRestyle != lastRestyle;
           ++currentRestyle) {
        ProcessOneRestyle(currentRestyle->mElement,
                          currentRestyle->mRestyleHint,
                          currentRestyle->mChangeHint);
      }
    }
  }

  mFrameConstructor->FlushOverflowChangedTracker();

  
  
  mFrameConstructor->mInStyleRefresh = false;

  mFrameConstructor->EndUpdate();

#ifdef DEBUG
  mFrameConstructor->mPresShell->VerifyStyleTree();
#endif
}

bool
RestyleTracker::GetRestyleData(Element* aElement, RestyleData* aData)
{
  NS_PRECONDITION(aElement->GetCurrentDoc() == Document(),
                  "Unexpected document; this will lead to incorrect behavior!");

  if (!aElement->HasFlag(RestyleBit())) {
    NS_ASSERTION(!aElement->HasFlag(RootBit()), "Bogus root bit?");
    return false;
  }

#ifdef DEBUG
  bool gotData =
#endif
  mPendingRestyles.Get(aElement, aData);
  NS_ASSERTION(gotData, "Must have data if restyle bit is set");

  if (aData->mRestyleHint & eRestyle_LaterSiblings) {
    
    
    
    
    RestyleData newData;
    newData.mChangeHint = nsChangeHint(0);
    newData.mRestyleHint = eRestyle_LaterSiblings;
    mPendingRestyles.Put(aElement, newData);
    aElement->UnsetFlags(RootBit());
    aData->mRestyleHint =
      nsRestyleHint(aData->mRestyleHint & ~eRestyle_LaterSiblings);
  } else {
    mPendingRestyles.Remove(aElement);
    aElement->UnsetFlags(mRestyleBits);
  }

  return true;
}

} 
} 


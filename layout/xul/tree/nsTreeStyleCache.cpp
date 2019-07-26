




#include "nsTreeStyleCache.h"
#include "nsStyleSet.h"
#include "mozilla/dom/Element.h"


nsStyleContext*
nsTreeStyleCache::GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext,
                                  nsIContent* aContent, 
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  const AtomArray & aInputWord)
{
  uint32_t count = aInputWord.Length();
  nsDFAState startState(0);
  nsDFAState* currState = &startState;

  
  if (!mTransitionTable) {
    
    mTransitionTable =
      new nsObjectHashtable(nullptr, nullptr, DeleteDFAState, nullptr);
  }

  
  nsTransitionKey key(currState->GetStateID(), aPseudoElement);
  currState = static_cast<nsDFAState*>(mTransitionTable->Get(&key));

  if (!currState) {
    
    currState = new nsDFAState(mNextState);
    mNextState++;
    mTransitionTable->Put(&key, currState);
  }

  for (uint32_t i = 0; i < count; i++) {
    nsTransitionKey key(currState->GetStateID(), aInputWord[i]);
    currState = static_cast<nsDFAState*>(mTransitionTable->Get(&key));

    if (!currState) {
      
      currState = new nsDFAState(mNextState);
      mNextState++;
      mTransitionTable->Put(&key, currState);
    }
  }

  
  
  nsStyleContext* result = nullptr;
  if (mCache)
    result = static_cast<nsStyleContext*>(mCache->Get(currState));
  if (!result) {
    
    result = aPresContext->StyleSet()->
      ResolveXULTreePseudoStyle(aContent->AsElement(), aPseudoElement,
                                aContext, aComparator).get();

    
    if (!mCache) {
      mCache = new nsObjectHashtable(nullptr, nullptr, ReleaseStyleContext, nullptr);
    }
    mCache->Put(currState, result);
  }

  return result;
}

bool
nsTreeStyleCache::DeleteDFAState(nsHashKey *aKey,
                                 void *aData,
                                 void *closure)
{
  nsDFAState* entry = static_cast<nsDFAState*>(aData);
  delete entry;
  return true;
}

bool
nsTreeStyleCache::ReleaseStyleContext(nsHashKey *aKey,
                                      void *aData,
                                      void *closure)
{
  nsStyleContext* context = static_cast<nsStyleContext*>(aData);
  context->Release();
  return true;
}

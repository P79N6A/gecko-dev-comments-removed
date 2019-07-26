




#include "nsTreeStyleCache.h"
#include "nsStyleSet.h"
#include "mozilla/dom/Element.h"

nsTreeStyleCache::Transition::Transition(DFAState aState, nsIAtom* aSymbol)
  : mState(aState), mInputSymbol(aSymbol)
{
}

bool
nsTreeStyleCache::Transition::operator==(const Transition& aOther) const
{
  return aOther.mState == mState && aOther.mInputSymbol == mInputSymbol;
}

uint32_t
nsTreeStyleCache::Transition::Hash() const
{
  
  uint32_t hb = mState << 16;
  uint32_t lb = (NS_PTR_TO_UINT32(mInputSymbol.get()) << 16) >> 16;
  return hb+lb;
}



nsStyleContext*
nsTreeStyleCache::GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext,
                                  nsIContent* aContent,
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  const AtomArray & aInputWord)
{
  uint32_t count = aInputWord.Length();

  
  if (!mTransitionTable) {
    
    mTransitionTable = new TransitionTable();
  }

  
  Transition transition(0, aPseudoElement);
  DFAState currState = mTransitionTable->Get(transition);

  if (!currState) {
    
    currState = mNextState;
    mNextState++;
    mTransitionTable->Put(transition, currState);
  }

  for (uint32_t i = 0; i < count; i++) {
    Transition transition(currState, aInputWord[i]);
    currState = mTransitionTable->Get(transition);

    if (!currState) {
      
      currState = mNextState;
      mNextState++;
      mTransitionTable->Put(transition, currState);
    }
  }

  
  
  nsStyleContext* result = nullptr;
  if (mCache) {
    result = mCache->GetWeak(currState);
  }
  if (!result) {
    
    nsRefPtr<nsStyleContext> newResult = aPresContext->StyleSet()->
      ResolveXULTreePseudoStyle(aContent->AsElement(), aPseudoElement,
                                aContext, aComparator);

    
    if (!mCache) {
      mCache = new StyleContextCache();
    }
    result = newResult.get();
    mCache->Put(currState, newResult.forget());
  }

  return result;
}


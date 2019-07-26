




#ifndef nsTreeStyleCache_h__
#define nsTreeStyleCache_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsCOMArray.h"
#include "nsICSSPseudoComparator.h"
#include "nsRefPtrHashtable.h"
#include "nsStyleContext.h"

typedef nsCOMArray<nsIAtom> AtomArray;

class nsTreeStyleCache
{
public:
  nsTreeStyleCache()
    : mNextState(0)
  {
  }

  ~nsTreeStyleCache()
  {
    Clear();
  }

  void Clear()
  {
    mTransitionTable = nullptr;
    mCache = nullptr;
    mNextState = 0;
  }

  nsStyleContext* GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext,
                                  nsIContent* aContent,
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  const AtomArray & aInputWord);

protected:
  typedef uint32_t DFAState;

  class Transition MOZ_FINAL
  {
  public:
    Transition(DFAState aState, nsIAtom* aSymbol);
    bool operator==(const Transition& aOther) const;
    uint32_t Hash() const;

  private:
    DFAState mState;
    nsCOMPtr<nsIAtom> mInputSymbol;
  };

  typedef nsDataHashtable<nsGenericHashKey<Transition>, DFAState> TransitionTable;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsAutoPtr<TransitionTable> mTransitionTable;

  
  
  typedef nsRefPtrHashtable<nsUint32HashKey, nsStyleContext> StyleContextCache;
  nsAutoPtr<StyleContextCache> mCache;

  
  
  DFAState mNextState;
};

#endif 

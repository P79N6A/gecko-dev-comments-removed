




#ifndef nsTreeStyleCache_h__
#define nsTreeStyleCache_h__

#include "mozilla/Attributes.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsCOMArray.h"
#include "nsICSSPseudoComparator.h"
#include "nsRefPtrHashtable.h"
#include "nsStyleContext.h"

typedef nsCOMArray<nsIAtom> AtomArray;

class nsDFAState : public nsHashKey
{
public:
  uint32_t mStateID;

  nsDFAState(uint32_t aID) :mStateID(aID) {}

  uint32_t GetStateID() { return mStateID; }

  uint32_t HashCode(void) const MOZ_OVERRIDE {
    return mStateID;
  }

  bool Equals(const nsHashKey *aKey) const MOZ_OVERRIDE {
    nsDFAState* key = (nsDFAState*)aKey;
    return key->mStateID == mStateID;
  }

  nsHashKey *Clone(void) const MOZ_OVERRIDE {
    return new nsDFAState(mStateID);
  }
};

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

  class Transition MOZ_FINAL
  {
  public:
    Transition(uint32_t aState, nsIAtom* aSymbol);
    bool operator==(const Transition& aOther) const;
    uint32_t Hash() const;

  private:
    uint32_t mState;
    nsCOMPtr<nsIAtom> mInputSymbol;
  };

  typedef nsClassHashtable<nsGenericHashKey<Transition>, nsDFAState> TransitionTable;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsAutoPtr<TransitionTable> mTransitionTable;

  
  
  typedef nsRefPtrHashtable<nsUint32HashKey, nsStyleContext> StyleContextCache;
  nsAutoPtr<StyleContextCache> mCache;

  
  
  uint32_t mNextState;
};

#endif 

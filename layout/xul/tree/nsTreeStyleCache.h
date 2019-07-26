




#ifndef nsTreeStyleCache_h__
#define nsTreeStyleCache_h__

#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsCOMArray.h"
#include "nsICSSPseudoComparator.h"
#include "nsStyleContext.h"

typedef nsCOMArray<nsIAtom> AtomArray;

class nsDFAState : public nsHashKey
{
public:
  uint32_t mStateID;

  nsDFAState(uint32_t aID) :mStateID(aID) {}

  uint32_t GetStateID() { return mStateID; }

  uint32_t HashCode(void) const {
    return mStateID;
  }

  bool Equals(const nsHashKey *aKey) const {
    nsDFAState* key = (nsDFAState*)aKey;
    return key->mStateID == mStateID;
  }

  nsHashKey *Clone(void) const {
    return new nsDFAState(mStateID);
  }
};

class nsTransitionKey : public nsHashKey
{
public:
  uint32_t mState;
  nsCOMPtr<nsIAtom> mInputSymbol;

  nsTransitionKey(uint32_t aState, nsIAtom* aSymbol) :mState(aState), mInputSymbol(aSymbol) {}

  uint32_t HashCode(void) const {
    
    int32_t hb = mState << 16;
    int32_t lb = (NS_PTR_TO_INT32(mInputSymbol.get()) << 16) >> 16;
    return hb+lb;
  }

  bool Equals(const nsHashKey *aKey) const {
    nsTransitionKey* key = (nsTransitionKey*)aKey;
    return key->mState == mState && key->mInputSymbol == mInputSymbol;
  }

  nsHashKey *Clone(void) const {
    return new nsTransitionKey(mState, mInputSymbol);
  }
};

class nsTreeStyleCache 
{
public:
  nsTreeStyleCache() :mTransitionTable(nullptr), mCache(nullptr), mNextState(0) {}
  ~nsTreeStyleCache() { Clear(); }

  void Clear() { delete mTransitionTable; mTransitionTable = nullptr; delete mCache; mCache = nullptr; mNextState = 0; }

  nsStyleContext* GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext, 
                                  nsIContent* aContent, 
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  const AtomArray & aInputWord);

  static bool DeleteDFAState(nsHashKey *aKey, void *aData, void *closure);

  static bool ReleaseStyleContext(nsHashKey *aKey, void *aData, void *closure);

protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsObjectHashtable* mTransitionTable;

  
  
  nsObjectHashtable* mCache;

  
  
  uint32_t mNextState;
};

#endif 

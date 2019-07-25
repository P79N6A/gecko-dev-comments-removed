






































#ifndef nsTreeStyleCache_h__
#define nsTreeStyleCache_h__

#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsICSSPseudoComparator.h"
#include "nsStyleContext.h"

class nsISupportsArray;

class nsDFAState : public nsHashKey
{
public:
  PRUint32 mStateID;

  nsDFAState(PRUint32 aID) :mStateID(aID) {}

  PRUint32 GetStateID() { return mStateID; }

  PRUint32 HashCode(void) const {
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
  PRUint32 mState;
  nsCOMPtr<nsIAtom> mInputSymbol;

  nsTransitionKey(PRUint32 aState, nsIAtom* aSymbol) :mState(aState), mInputSymbol(aSymbol) {}

  PRUint32 HashCode(void) const {
    
    PRInt32 hb = mState << 16;
    PRInt32 lb = (NS_PTR_TO_INT32(mInputSymbol.get()) << 16) >> 16;
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
  nsTreeStyleCache() :mTransitionTable(nsnull), mCache(nsnull), mNextState(0) {}
  ~nsTreeStyleCache() { Clear(); }

  void Clear() { delete mTransitionTable; mTransitionTable = nsnull; delete mCache; mCache = nsnull; mNextState = 0; }

  nsStyleContext* GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext, 
                                  nsIContent* aContent, 
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  nsISupportsArray* aInputWord);

  static bool DeleteDFAState(nsHashKey *aKey, void *aData, void *closure);

  static bool ReleaseStyleContext(nsHashKey *aKey, void *aData, void *closure);

protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsObjectHashtable* mTransitionTable;

  
  
  nsObjectHashtable* mCache;

  
  
  PRUint32 mNextState;
};

#endif 








































#include "nsTreeStyleCache.h"
#include "nsISupportsArray.h"
#include "nsStyleSet.h"


nsStyleContext*
nsTreeStyleCache::GetStyleContext(nsICSSPseudoComparator* aComparator,
                                  nsPresContext* aPresContext,
                                  nsIContent* aContent, 
                                  nsStyleContext* aContext,
                                  nsIAtom* aPseudoElement,
                                  nsISupportsArray* aInputWord)
{
  PRUint32 count;
  aInputWord->Count(&count);
  nsDFAState startState(0);
  nsDFAState* currState = &startState;

  
  if (!mTransitionTable) {
    
    mTransitionTable =
      new nsObjectHashtable(nsnull, nsnull, DeleteDFAState, nsnull);
    if (!mTransitionTable)
      return nsnull;
  }

  
  nsTransitionKey key(currState->GetStateID(), aPseudoElement);
  currState = static_cast<nsDFAState*>(mTransitionTable->Get(&key));

  if (!currState) {
    
    currState = new nsDFAState(mNextState);
    if (!currState)
      return nsnull;
    mNextState++;
    mTransitionTable->Put(&key, currState);
  }

  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> pseudo = getter_AddRefs(static_cast<nsIAtom*>(aInputWord->ElementAt(i)));
    nsTransitionKey key(currState->GetStateID(), pseudo);
    currState = static_cast<nsDFAState*>(mTransitionTable->Get(&key));

    if (!currState) {
      
      currState = new nsDFAState(mNextState);
      if (!currState)
        return nsnull;

      mNextState++;
      mTransitionTable->Put(&key, currState);
    }
  }

  
  
  nsStyleContext* result = nsnull;
  if (mCache)
    result = static_cast<nsStyleContext*>(mCache->Get(currState));
  if (!result) {
    
    result = aPresContext->StyleSet()->
      ResolvePseudoStyleFor(aContent, aPseudoElement,
                            aContext, aComparator).get();

    
    if (!mCache) {
      mCache = new nsObjectHashtable(nsnull, nsnull, ReleaseStyleContext, nsnull);
      if (!mCache)
        return nsnull;
    }
    mCache->Put(currState, result);
  }

  return result;
}

PRBool
nsTreeStyleCache::DeleteDFAState(nsHashKey *aKey,
                                 void *aData,
                                 void *closure)
{
  nsDFAState* entry = static_cast<nsDFAState*>(aData);
  delete entry;
  return PR_TRUE;
}

PRBool
nsTreeStyleCache::ReleaseStyleContext(nsHashKey *aKey,
                                      void *aData,
                                      void *closure)
{
  nsStyleContext* context = static_cast<nsStyleContext*>(aData);
  context->Release();
  return PR_TRUE;
}

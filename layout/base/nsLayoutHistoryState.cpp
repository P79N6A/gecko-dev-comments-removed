










































#include "nsILayoutHistoryState.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsPresState.h"

class nsLayoutHistoryState : public nsILayoutHistoryState,
                             public nsSupportsWeakReference
{
public:
  NS_HIDDEN_(nsresult) Init();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD AddState(const nsCString& aKey, nsPresState* aState);
  NS_IMETHOD GetState(const nsCString& aKey, nsPresState** aState);
  NS_IMETHOD RemoveState(const nsCString& aKey);
  NS_IMETHOD_(bool) HasStates() const;
  NS_IMETHOD SetScrollPositionOnly(const bool aFlag);


private:
  ~nsLayoutHistoryState() {}
  bool mScrollPositionOnly;

  nsClassHashtable<nsCStringHashKey,nsPresState> mStates;
};


nsresult
NS_NewLayoutHistoryState(nsILayoutHistoryState** aState)
{
  nsLayoutHistoryState *state;

  *aState = nsnull;
  state = new nsLayoutHistoryState();

  NS_ADDREF(state);
  nsresult rv = state->Init();
  if (NS_SUCCEEDED(rv))
    *aState = state;
  else
    NS_RELEASE(state);

  return rv;
}

NS_IMPL_ISUPPORTS2(nsLayoutHistoryState,
                   nsILayoutHistoryState,
                   nsISupportsWeakReference)

nsresult
nsLayoutHistoryState::Init()
{
  mScrollPositionOnly = false;
  mStates.Init();
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutHistoryState::AddState(const nsCString& aStateKey, nsPresState* aState)
{
  mStates.Put(aStateKey, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutHistoryState::GetState(const nsCString& aKey, nsPresState** aState)
{
  bool entryExists = mStates.Get(aKey, aState);

  if (entryExists && mScrollPositionOnly) {
    
    (*aState)->ClearNonScrollState();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLayoutHistoryState::RemoveState(const nsCString& aKey)
{
  mStates.Remove(aKey);
  return NS_OK;
}

NS_IMETHODIMP_(bool)
nsLayoutHistoryState::HasStates() const
{
  return mStates.Count() != 0;
}

NS_IMETHODIMP
nsLayoutHistoryState::SetScrollPositionOnly(const bool aFlag)
{
  mScrollPositionOnly = aFlag;
  return NS_OK;
}

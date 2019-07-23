










































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


private:
  ~nsLayoutHistoryState() {}

  nsClassHashtable<nsCStringHashKey,nsPresState> mStates;
};


nsresult
NS_NewLayoutHistoryState(nsILayoutHistoryState** aState)
{
  nsLayoutHistoryState *state;

  *aState = nsnull;
  state = new nsLayoutHistoryState();
  if (!state)
    return NS_ERROR_OUT_OF_MEMORY;

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
  return mStates.Init() ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLayoutHistoryState::AddState(const nsCString& aStateKey, nsPresState* aState)
{
  return mStates.Put(aStateKey, aState) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsLayoutHistoryState::GetState(const nsCString& aKey, nsPresState** aState)
{
  mStates.Get(aKey, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsLayoutHistoryState::RemoveState(const nsCString& aKey)
{
  mStates.Remove(aKey);
  return NS_OK;
}

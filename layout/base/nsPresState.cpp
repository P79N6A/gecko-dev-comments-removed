









































#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsLayoutErrors.h"
#include "nsPresState.h"
#include "nsString.h"


nsresult
nsPresState::Init()
{
  return mPropertyTable.Init(8) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsPresState::GetStateProperty(const nsAString& aName, nsAString& aResult)
{
  nsresult rv = NS_STATE_PROPERTY_NOT_THERE;
  aResult.Truncate();

  
  nsISupports *data = mPropertyTable.GetWeak(aName);

  
  

  nsCOMPtr<nsISupportsCString> supportsStr = do_QueryInterface(data);
  if (supportsStr) {
    nsCAutoString data;
    supportsStr->GetData(data);

    CopyUTF8toUTF16(data, aResult);
    aResult.SetIsVoid(data.IsVoid());
    rv = NS_STATE_PROPERTY_EXISTS;
  }

  return rv;
}

nsresult
nsPresState::SetStateProperty(const nsAString& aName, const nsAString& aValue)
{
  
  nsCOMPtr<nsISupportsCString> supportsStr(do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID));
  NS_ENSURE_TRUE(supportsStr, NS_ERROR_OUT_OF_MEMORY);
  NS_ConvertUTF16toUTF8 data(aValue);
  data.SetIsVoid(aValue.IsVoid());
  supportsStr->SetData(data);

  mPropertyTable.Put(aName, supportsStr);
  return NS_OK;
}

nsresult
nsPresState::RemoveStateProperty(const nsAString& aName)
{
  mPropertyTable.Remove(aName);
  return NS_OK;
}

nsresult
nsPresState::GetStatePropertyAsSupports(const nsAString& aName,
                                        nsISupports** aResult)
{
  
  mPropertyTable.Get(aName, aResult);
  return NS_OK;
}

nsresult
nsPresState::SetStatePropertyAsSupports(const nsAString& aName,
                                        nsISupports* aValue)
{
  mPropertyTable.Put(aName, aValue);
  return NS_OK;
}

nsresult
nsPresState::SetScrollState(const nsRect& aRect)
{
  if (!mScrollState) {
    mScrollState = new nsRect();
    if (!mScrollState)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  *mScrollState = aRect;
  return NS_OK;
}

nsRect
nsPresState::GetScrollState()
{
  if (!mScrollState) {
    nsRect empty(0,0,0,0);
    return empty;  
  }

  return *mScrollState;
}

nsresult
NS_NewPresState(nsPresState** aState)
{
  nsPresState *state;

  *aState = nsnull;
  state = new nsPresState();
  if (!state)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = state->Init();
  if (NS_SUCCEEDED(rv))
    *aState = state;
  else
    delete state;

  return rv;
}

  











































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
  NS_ENSURE_ARG_POINTER(aState);

  nsPresState *state = new nsPresState();

  if (!state)
    return NS_ERROR_OUT_OF_MEMORY;


  *aState = state;
  return NS_OK;
}

  

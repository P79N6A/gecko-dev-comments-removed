









































#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsLayoutErrors.h"
#include "nsPresState.h"


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

void
nsPresState::ClearNonScrollState()
{
  mContentData = nsnull;
  mDisabledSet = PR_FALSE;
}







#include "xpcAccessibleTable.h"

#include "nsAccessible.h"
#include "TableAccessible.h"

nsresult
xpcAccessibleTable::GetCaption(nsIAccessible** aCaption)
{
  NS_ENSURE_ARG_POINTER(aCaption);
  *aCaption = nsnull;
  if (!mTable)
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aCaption = mTable->Caption());
  return NS_OK;
}

nsresult
xpcAccessibleTable::IsProbablyForLayout(bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = false;
  if (!mTable)
    return NS_ERROR_FAILURE;

  *aResult = mTable->IsProbablyLayoutTable();
  return NS_OK;
}







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
xpcAccessibleTable::GetColumnCount(PRInt32* aColumnCount)
{
  NS_ENSURE_ARG_POINTER(aColumnCount);
  *aColumnCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aColumnCount = mTable->ColCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetRowCount(PRInt32* aRowCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);
  *aRowCount = 0;

  if (!mTable)
    return NS_ERROR_FAILURE;

  *aRowCount = mTable->RowCount();
  return NS_OK;
}

nsresult
xpcAccessibleTable::GetSummary(nsAString& aSummary)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  nsAutoString summary;
  mTable->Summary(summary);
  aSummary.Assign(summary);

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

nsresult
xpcAccessibleTable::UnselectColumn(PRInt32 aColIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aColIdx < 0 || aColIdx >= mTable->ColCount())
    return NS_ERROR_INVALID_ARG;

  mTable->UnselectCol(aColIdx);
  return NS_OK;
}

nsresult
xpcAccessibleTable::UnselectRow(PRInt32 aRowIdx)
{
  if (!mTable)
    return NS_ERROR_FAILURE;

  if (aRowIdx < 0 || aRowIdx >= mTable->RowCount())
    return NS_ERROR_INVALID_ARG;

  mTable->UnselectRow(aRowIdx);
  return NS_OK;
}



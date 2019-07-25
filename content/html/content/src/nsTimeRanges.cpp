





































#include "nsTimeRanges.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF(nsTimeRanges)
NS_IMPL_RELEASE(nsTimeRanges)

DOMCI_DATA(TimeRanges, nsTimeRanges)

NS_INTERFACE_MAP_BEGIN(nsTimeRanges)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTimeRanges)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TimeRanges)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsTimeRanges::GetLength(PRUint32* aLength) {
  *aLength = mRanges.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsTimeRanges::Start(PRUint32 aIndex, float* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mStart;
  return NS_OK;
}

NS_IMETHODIMP
nsTimeRanges::End(PRUint32 aIndex, float* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mEnd;
  return NS_OK;
}

void
nsTimeRanges::Add(float aStart, float aEnd) {
  mRanges.AppendElement(TimeRange(aStart,aEnd));
}

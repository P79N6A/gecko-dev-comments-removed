





































#include "nsHTMLTimeRanges.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF(nsHTMLTimeRanges)
NS_IMPL_RELEASE(nsHTMLTimeRanges)

DOMCI_DATA(HTMLTimeRanges, nsHTMLTimeRanges)

NS_INTERFACE_MAP_BEGIN(nsHTMLTimeRanges)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTimeRanges)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLTimeRanges)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsHTMLTimeRanges::GetLength(PRUint32* aLength) {
  *aLength = mRanges.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTimeRanges::Start(PRUint32 aIndex, float* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mStart;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTimeRanges::End(PRUint32 aIndex, float* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mEnd;
  return NS_OK;
}

void
nsHTMLTimeRanges::Add(float aStart, float aEnd) {
  mRanges.AppendElement(TimeRange(aStart,aEnd));
}







































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

nsTimeRanges::nsTimeRanges() {
  MOZ_COUNT_CTOR(nsTimeRanges);
}

nsTimeRanges::~nsTimeRanges() {
  MOZ_COUNT_DTOR(nsTimeRanges);
}

NS_IMETHODIMP
nsTimeRanges::GetLength(PRUint32* aLength) {
  *aLength = mRanges.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsTimeRanges::Start(PRUint32 aIndex, double* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mStart;
  return NS_OK;
}

NS_IMETHODIMP
nsTimeRanges::End(PRUint32 aIndex, double* aTime) {
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mEnd;
  return NS_OK;
}

void
nsTimeRanges::Add(double aStart, double aEnd) {
  mRanges.AppendElement(TimeRange(aStart,aEnd));
}

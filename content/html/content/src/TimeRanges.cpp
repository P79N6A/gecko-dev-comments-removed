





#include "mozilla/dom/TimeRanges.h"
#include "nsDOMClassInfoID.h"
#include "nsError.h"

DOMCI_DATA(TimeRanges, mozilla::dom::TimeRanges)

namespace mozilla {
namespace dom {

NS_IMPL_ADDREF(TimeRanges)
NS_IMPL_RELEASE(TimeRanges)

NS_INTERFACE_MAP_BEGIN(TimeRanges)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTimeRanges)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TimeRanges)
NS_INTERFACE_MAP_END

TimeRanges::TimeRanges()
{
  MOZ_COUNT_CTOR(TimeRanges);
}

TimeRanges::~TimeRanges()
{
  MOZ_COUNT_DTOR(TimeRanges);
}

NS_IMETHODIMP
TimeRanges::GetLength(uint32_t* aLength)
{
  *aLength = mRanges.Length();
  return NS_OK;
}

NS_IMETHODIMP
TimeRanges::Start(uint32_t aIndex, double* aTime)
{
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mStart;
  return NS_OK;
}

NS_IMETHODIMP
TimeRanges::End(uint32_t aIndex, double* aTime)
{
  if (aIndex >= mRanges.Length())
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  *aTime = mRanges[aIndex].mEnd;
  return NS_OK;
}

void
TimeRanges::Add(double aStart, double aEnd)
{
  if (aStart > aEnd) {
    NS_WARNING("Can't add a range if the end is older that the start.");
    return;
  }
  mRanges.AppendElement(TimeRange(aStart,aEnd));
}

double
TimeRanges::GetFinalEndTime()
{
  if (mRanges.IsEmpty()) {
    return -1.0;
  }
  uint32_t finalIndex = mRanges.Length() - 1;
  return mRanges[finalIndex].mEnd;
}

void
TimeRanges::Normalize()
{
  if (mRanges.Length() >= 2) {
    nsAutoTArray<TimeRange,4> normalized;

    mRanges.Sort(CompareTimeRanges());

    
    TimeRange current(mRanges[0]);
    for (uint32_t i = 1; i < mRanges.Length(); i++) {
      if (current.mStart <= mRanges[i].mStart &&
          current.mEnd >= mRanges[i].mEnd) {
        continue;
      }
      if (current.mEnd >= mRanges[i].mStart) {
        current.mEnd = mRanges[i].mEnd;
      } else {
        normalized.AppendElement(current);
        current = mRanges[i];
      }
    }

    normalized.AppendElement(current);

    mRanges = normalized;
  }
}

} 
} 

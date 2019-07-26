





#include "mozilla/dom/TimeRanges.h"
#include "mozilla/dom/TimeRangesBinding.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsError.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS1(TimeRanges, nsIDOMTimeRanges)

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
  *aLength = Length();
  return NS_OK;
}

double
TimeRanges::Start(uint32_t aIndex, ErrorResult& aRv)
{
  if (aIndex >= mRanges.Length()) {
    aRv = NS_ERROR_DOM_INDEX_SIZE_ERR;
    return 0;
  }

  return mRanges[aIndex].mStart;
}

NS_IMETHODIMP
TimeRanges::Start(uint32_t aIndex, double* aTime)
{
  ErrorResult rv;
  *aTime = Start(aIndex, rv);
  return rv.ErrorCode();
}

double
TimeRanges::End(uint32_t aIndex, ErrorResult& aRv)
{
  if (aIndex >= mRanges.Length()) {
    aRv = NS_ERROR_DOM_INDEX_SIZE_ERR;
    return 0;
  }

  return mRanges[aIndex].mEnd;
}

NS_IMETHODIMP
TimeRanges::End(uint32_t aIndex, double* aTime)
{
  ErrorResult rv;
  *aTime = End(aIndex, rv);
  return rv.ErrorCode();
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

JSObject*
TimeRanges::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return TimeRangesBinding::Wrap(aCx, aScope, this);
}

} 
} 










































#include "nsString.h"
#include "SegmentBase.h"


namespace mozilla {
namespace net {

void
SegmentBase::GetIndexRange(int64_t* aStartBytes, int64_t* aEndBytes) const
{
  NS_ENSURE_TRUE(aStartBytes, );
  NS_ENSURE_TRUE(aEndBytes, );
  *aStartBytes = mIndexRangeStart;
  *aEndBytes = mIndexRangeEnd;
}

void
SegmentBase::GetInitRange(int64_t* aStartBytes, int64_t* aEndBytes) const
{
  NS_ENSURE_TRUE(aStartBytes, );
  NS_ENSURE_TRUE(aEndBytes, );
  *aStartBytes = mInitRangeStart;
  *aEndBytes = mInitRangeEnd;
}

void
SegmentBase::SetIndexRange(nsAString const &aRangeStr)
{
  SetRange(aRangeStr, mIndexRangeStart, mIndexRangeEnd);
}

void
SegmentBase::SetInitRange(nsAString const &aRangeStr)
{
  SetRange(aRangeStr, mInitRangeStart, mInitRangeEnd);
}

void
SegmentBase::SetRange(nsAString const &aRangeStr,
                      int64_t &aStart,
                      int64_t &aEnd)
{
  NS_ENSURE_TRUE(!aRangeStr.IsEmpty(), );

  nsAString::const_iterator start, end, dashStart, dashEnd;

  aRangeStr.BeginReading(start);
  aRangeStr.EndReading(end);
  dashStart = start;
  dashEnd = end;

  if (FindInReadable(NS_LITERAL_STRING("-"), dashStart, dashEnd)) {
    nsAutoString temp(Substring(start, dashStart));
    nsresult rv;
    aStart = temp.ToInteger64(&rv);
    NS_ENSURE_SUCCESS(rv, );

    temp = Substring(dashEnd, end);
    aEnd = temp.ToInteger64(&rv);
    NS_ENSURE_SUCCESS(rv, );
  }
}

}
}

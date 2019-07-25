





































#include "nsIDOMTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"



class nsTimeRanges : public nsIDOMTimeRanges {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  void Add(float aStart, float aEnd);

private:

  struct TimeRange {
    TimeRange(float aStart, float aEnd)
      : mStart(aStart),
        mEnd(aEnd) {}
    float mStart;
    float mEnd;
  };

  nsAutoTArray<TimeRange,4> mRanges;
};

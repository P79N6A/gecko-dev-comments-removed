





































#include "nsIDOMTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"



class nsTimeRanges : public nsIDOMTimeRanges {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  void Add(double aStart, double aEnd);

private:

  struct TimeRange {
    TimeRange(double aStart, double aEnd)
      : mStart(aStart),
        mEnd(aEnd) {}
    double mStart;
    double mEnd;
  };

  nsAutoTArray<TimeRange,4> mRanges;
};

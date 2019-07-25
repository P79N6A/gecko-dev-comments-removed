





































#include "nsIDOMHTMLTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"



class nsHTMLTimeRanges : public nsIDOMHTMLTimeRanges {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMHTMLTIMERANGES

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

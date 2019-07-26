





#ifndef nsTimeRanges_h__
#define nsTimeRanges_h__

#include "nsIDOMTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"



class nsTimeRanges MOZ_FINAL : public nsIDOMTimeRanges
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  nsTimeRanges();
  ~nsTimeRanges();

  void Add(double aStart, double aEnd);

  
  double GetFinalEndTime();

  
  void Normalize();

private:

  
  struct TimeRange
  {
    TimeRange(double aStart, double aEnd)
      : mStart(aStart),
        mEnd(aEnd) {}
    double mStart;
    double mEnd;
  };

  struct CompareTimeRanges
  {
    bool Equals(const TimeRange& aTr1, const TimeRange& aTr2) const {
      return aTr1.mStart == aTr2.mStart && aTr1.mEnd == aTr2.mEnd;
    }

    bool LessThan(const TimeRange& aTr1, const TimeRange& aTr2) const {
      return aTr1.mStart < aTr2.mStart;
    }
  };

  nsAutoTArray<TimeRange,4> mRanges;
};

#endif 

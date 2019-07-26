





#ifndef mozilla_dom_TimeRanges_h_
#define mozilla_dom_TimeRanges_h_

#include "nsIDOMTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {



class TimeRanges MOZ_FINAL : public nsIDOMTimeRanges
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  TimeRanges();
  ~TimeRanges();

  void Add(double aStart, double aEnd);

  
  double GetFinalEndTime();

  
  void Normalize();

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  uint32_t Length() const
  {
    return mRanges.Length();
  }

  virtual double Start(uint32_t aIndex, ErrorResult& aRv);

  virtual double End(uint32_t aIndex, ErrorResult& aRv);

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

} 
} 

#endif 

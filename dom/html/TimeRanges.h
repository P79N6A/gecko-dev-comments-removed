





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

class TimeRanges;

}

namespace dom {



class TimeRanges final : public nsIDOMTimeRanges
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  TimeRanges();

  void Add(double aStart, double aEnd);

  
  double GetStartTime();

  
  double GetEndTime();

  
  void Normalize(double aTolerance = 0.0);

  
  void Union(const TimeRanges* aOtherRanges, double aTolerance);

  
  void Intersection(const TimeRanges* aOtherRanges);

  bool WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector);

  uint32_t Length() const
  {
    return mRanges.Length();
  }

  virtual double Start(uint32_t aIndex, ErrorResult& aRv);

  virtual double End(uint32_t aIndex, ErrorResult& aRv);

  
  void Shift(double aOffset);

private:
  ~TimeRanges();

  
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

public:
  typedef nsTArray<TimeRange>::index_type index_type;
  static const index_type NoIndex = index_type(-1);

  index_type Find(double aTime, double aTolerance = 0);

  bool Contains(double aStart, double aEnd) {
    index_type target = Find(aStart);
    if (target == NoIndex) {
      return false;
    }

    return mRanges[target].mEnd >= aEnd;
  }
};

} 
} 

#endif 

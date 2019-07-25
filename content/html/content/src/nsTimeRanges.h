





































#ifndef nsTimeRanges_h__
#define nsTimeRanges_h__

#include "nsIDOMTimeRanges.h"
#include "nsISupports.h"
#include "nsTArray.h"



class nsTimeRanges : public nsIDOMTimeRanges {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMTIMERANGES

  nsTimeRanges();
  ~nsTimeRanges();

  void Add(double aStart, double aEnd);

  
  void Normalize();

private:

  struct TimeRange {
    TimeRange(double aStart, double aEnd)
      : mStart(aStart),
        mEnd(aEnd) {}
    double mStart;
    double mEnd;
  };

  struct CompareTimeRanges
  {
    PRBool Equals(const TimeRange& tr1, const TimeRange& tr2) const
    {
      return tr1.mStart == tr2.mStart && tr1.mEnd == tr2.mEnd;
    }

    
    
    PRBool LessThan(const TimeRange& tr1, const TimeRange& tr2) const
    {
      return tr1.mStart < tr2.mStart;
    }
  };

  nsAutoTArray<TimeRange,4> mRanges;
};

#endif 

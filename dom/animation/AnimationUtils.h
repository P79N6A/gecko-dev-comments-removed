




#ifndef mozilla_dom_AnimationUtils_h
#define mozilla_dom_AnimationUtils_h

#include "mozilla/TimeStamp.h"
#include "mozilla/dom/Nullable.h"

namespace mozilla {
namespace dom {

class AnimationUtils
{
public:
  static Nullable<double>
    TimeDurationToDouble(const Nullable<TimeDuration>& aTime)
  {
    Nullable<double> result;

    if (!aTime.IsNull()) {
      result.SetValue(aTime.Value().ToMilliseconds());
    }

    return result;
  }

  static Nullable<TimeDuration>
    DoubleToTimeDuration(const Nullable<double>& aTime)
  {
    Nullable<TimeDuration> result;

    if (!aTime.IsNull()) {
      result.SetValue(TimeDuration::FromMilliseconds(aTime.Value()));
    }

    return result;
  }
};

} 
} 

#endif

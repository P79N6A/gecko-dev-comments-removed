





#ifndef mozilla_dom_performancemeasure_h___
#define mozilla_dom_performancemeasure_h___

#include "mozilla/dom/PerformanceEntry.h"

namespace mozilla {
namespace dom {


class PerformanceMeasure final : public PerformanceEntry
{
public:
  PerformanceMeasure(nsISupports* aParent,
                     const nsAString& aName,
                     DOMHighResTimeStamp aStartTime,
                     DOMHighResTimeStamp aEndTime);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual DOMHighResTimeStamp StartTime() const override
  {
    return mStartTime;
  }

  virtual DOMHighResTimeStamp Duration() const override
  {
    return mDuration;
  }

protected:
  virtual ~PerformanceMeasure();
  DOMHighResTimeStamp mStartTime;
  DOMHighResTimeStamp mDuration;
};

} 
} 

#endif 

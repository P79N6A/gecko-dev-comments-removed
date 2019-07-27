





#ifndef mozilla_dom_performancemark_h___
#define mozilla_dom_performancemark_h___

#include "mozilla/dom/PerformanceEntry.h"

namespace mozilla {
namespace dom {


class PerformanceMark final : public PerformanceEntry
{
public:
  PerformanceMark(nsISupports* aParent,
                  const nsAString& aName,
                  DOMHighResTimeStamp aStartTime);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual DOMHighResTimeStamp StartTime() const override
  {
    return mStartTime;
  }

protected:
  virtual ~PerformanceMark();
  DOMHighResTimeStamp mStartTime;
};

} 
} 

#endif 

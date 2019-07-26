




#ifndef mozilla_dom_PerformanceResourceTiming_h___
#define mozilla_dom_PerformanceResourceTiming_h___

#include "nsCOMPtr.h"
#include "nsPerformance.h"
#include "nsIChannel.h"
#include "nsITimedChannel.h"
#include "nsDOMNavigationTiming.h"
#include "PerformanceEntry.h"

namespace mozilla {
namespace dom {


class PerformanceResourceTiming MOZ_FINAL : public PerformanceEntry
{
public:
  typedef mozilla::TimeStamp TimeStamp;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
      PerformanceResourceTiming,
      PerformanceEntry)

  PerformanceResourceTiming(nsPerformanceTiming* aPerformanceTiming,
                            nsPerformance* aPerformance);
  virtual ~PerformanceResourceTiming();

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;


  virtual DOMHighResTimeStamp StartTime() const;

  virtual DOMHighResTimeStamp Duration() const
  {
    return ResponseEnd() - StartTime();
  }

  void GetInitiatorType(nsAString& aInitiatorType) const
  {
    aInitiatorType = mInitiatorType;
  }

  void SetInitiatorType(const nsAString& aInitiatorType)
  {
    mInitiatorType = aInitiatorType;
  }

  DOMHighResTimeStamp FetchStart() const {
    return mTiming
        ? mTiming->FetchStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp RedirectStart() const {
    
    
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->RedirectStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp RedirectEnd() const {
    
    
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->RedirectEndHighRes()
        : 0;
  }

  DOMHighResTimeStamp DomainLookupStart() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->DomainLookupStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp DomainLookupEnd() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->DomainLookupEndHighRes()
        : 0;
  }

  DOMHighResTimeStamp ConnectStart() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->ConnectStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp ConnectEnd() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->ConnectEndHighRes()
        : 0;
  }

  DOMHighResTimeStamp RequestStart() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->RequestStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp ResponseStart() const {
    return mTiming && mTiming->IsSameOriginAsReferral()
        ? mTiming->ResponseStartHighRes()
        : 0;
  }

  DOMHighResTimeStamp ResponseEnd() const {
    return mTiming
        ? mTiming->ResponseEndHighRes()
        : 0;
  }

  DOMHighResTimeStamp SecureConnectionStart() const
  {
    
    
    return 0;
  }

protected:
  nsString mInitiatorType;
  nsRefPtr<nsPerformanceTiming> mTiming;
};

} 
} 

#endif 

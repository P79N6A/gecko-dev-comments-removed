




#ifndef nsCCUncollectableMarker_h_
#define nsCCUncollectableMarker_h_

#include "js/TracingAPI.h"
#include "mozilla/Attributes.h"
#include "nsIObserver.h"

class nsCCUncollectableMarker MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  static nsresult Init();

  


  static bool InGeneration(uint32_t aGeneration)
  {
    return aGeneration && aGeneration == sGeneration;
  }

  template <class CCCallback>
  static bool InGeneration(CCCallback& aCb, uint32_t aGeneration)
  {
    return InGeneration(aGeneration) && !aCb.WantAllTraces();
  }

  static uint32_t sGeneration;

private:
  nsCCUncollectableMarker() {}
  ~nsCCUncollectableMarker() {}
};

namespace mozilla {
namespace dom {
void TraceBlackJS(JSTracer* aTrc, uint32_t aGCNumber, bool aIsShutdownGC);
}
}

#endif

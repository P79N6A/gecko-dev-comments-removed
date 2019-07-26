




#ifndef nsCCUncollectableMarker_h_
#define nsCCUncollectableMarker_h_

#include "nsIObserver.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

struct JSTracer;

class nsCCUncollectableMarker MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  static nsresult Init();

  


  static bool InGeneration(uint32_t aGeneration)
  {
    return aGeneration && aGeneration == sGeneration;
  }

  static bool InGeneration(nsCycleCollectionTraversalCallback& aCb,
                           uint32_t aGeneration)
  {
    return InGeneration(aGeneration) && !aCb.WantAllTraces();
  }

  static uint32_t sGeneration;

private:
  nsCCUncollectableMarker() {}

};

namespace mozilla {
namespace dom {
void TraceBlackJS(JSTracer* aTrc, uint32_t aGCNumber, bool aIsShutdownGC);
}
}

#endif

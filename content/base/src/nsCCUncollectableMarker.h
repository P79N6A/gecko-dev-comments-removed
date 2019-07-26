




#include "nsIObserver.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

struct JSTracer;

class nsCCUncollectableMarker MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  static nsresult Init();

  


  static bool InGeneration(PRUint32 aGeneration)
  {
    return aGeneration && aGeneration == sGeneration;
  }

  static bool InGeneration(nsCycleCollectionTraversalCallback& aCb,
                           PRUint32 aGeneration)
  {
    return InGeneration(aGeneration) && !aCb.WantAllTraces();
  }

  static PRUint32 sGeneration;

private:
  nsCCUncollectableMarker() {}

};

namespace mozilla {
namespace dom {
void TraceBlackJS(JSTracer* aTrc);
}
}

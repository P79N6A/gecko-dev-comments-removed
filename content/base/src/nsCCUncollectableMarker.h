




































#include "nsIObserver.h"
#include "nsCycleCollectionParticipant.h"

class nsCCUncollectableMarker : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  static nsresult Init();

  


  static bool InGeneration(nsCycleCollectionTraversalCallback &cb,
                             PRUint32 aGeneration) {
    return !cb.WantAllTraces() &&
           aGeneration &&
           aGeneration == sGeneration;
  }

  static PRUint32 sGeneration;

private:
  nsCCUncollectableMarker() {}

};

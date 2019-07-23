




































#include "nsIObserver.h"

class nsCCUncollectableMarker : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  static nsresult Init();

  


  static PRBool InGeneration(PRUint32 aGeneration) {
    return aGeneration && aGeneration == sGeneration;
  }

  static PRUint32 sGeneration;

private:
  nsCCUncollectableMarker() {};

};

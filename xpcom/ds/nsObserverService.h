




































#ifndef nsObserverService_h___
#define nsObserverService_h___

#include "nsIObserverService.h"
#include "nsObserverList.h"
#include "nsTHashtable.h"

#define NS_OBSERVERSERVICE_CLASSNAME "Observer Service"


#define NS_OBSERVERSERVICE_CID \
    { 0xd07f5195, 0xe3d1, 0x11d2, { 0x8a, 0xcd, 0x0, 0x10, 0x5a, 0x1b, 0x88, 0x60 } }

class nsObserverService : public nsIObserverService {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_OBSERVERSERVICE_CID)

  nsObserverService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVERSERVICE
  
  void Shutdown();

  static NS_METHOD
  Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

private:
  ~nsObserverService(void);

  PRBool mShuttingDown;
  nsTHashtable<nsObserverList> mObserverTopicTable;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsObserverService, NS_OBSERVERSERVICE_CID)

#endif 

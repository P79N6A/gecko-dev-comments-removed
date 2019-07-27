





#ifndef nsObserverService_h___
#define nsObserverService_h___

#include "nsIObserverService.h"
#include "nsObserverList.h"
#include "nsIMemoryReporter.h"
#include "nsTHashtable.h"
#include "mozilla/Attributes.h"


#define NS_OBSERVERSERVICE_CID \
    { 0xd07f5195, 0xe3d1, 0x11d2, { 0x8a, 0xcd, 0x0, 0x10, 0x5a, 0x1b, 0x88, 0x60 } }

class nsIMemoryReporter;

class nsObserverService final
  : public nsIObserverService
  , public nsIMemoryReporter
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_OBSERVERSERVICE_CID)

  nsObserverService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVERSERVICE
  NS_DECL_NSIMEMORYREPORTER

  void Shutdown();

  static nsresult Create(nsISupports* aOuter, const nsIID& aIID,
                         void** aInstancePtr);

  
  
  NS_IMETHOD UnmarkGrayStrongObservers();

private:
  ~nsObserverService(void);
  void RegisterReporter();

  static const size_t kSuspectReferentCount = 100;
  static PLDHashOperator CountReferents(nsObserverList* aObserverList,
                                        void* aClosure);
  bool mShuttingDown;
  nsTHashtable<nsObserverList> mObserverTopicTable;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsObserverService, NS_OBSERVERSERVICE_CID)

#endif 






#include "nsIRDFDataSource.h"
#include "nsIWindowMediatorListener.h"
#include "nsIWindowDataSource.h"
#include "nsIObserver.h"

#include "nsHashKeys.h"
#include "nsIRDFService.h"
#include "nsIRDFContainer.h"
#include "nsInterfaceHashtable.h"
#include "nsCycleCollectionParticipant.h"


#define NS_WINDOWDATASOURCE_CID \
{ 0xc744ca3d, 0x840b, 0x460a, \
 { 0x8d, 0x70, 0x7c, 0xe6, 0x3c, 0x51, 0xc9, 0x58 } }


class nsWindowDataSource final : public nsIRDFDataSource,
                                 public nsIObserver,
                                 public nsIWindowMediatorListener,
                                 public nsIWindowDataSource
{
 public:
    nsWindowDataSource() { }

    nsresult Init();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsWindowDataSource,
                                             nsIRDFDataSource)
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIWINDOWMEDIATORLISTENER
    NS_DECL_NSIWINDOWDATASOURCE
    NS_DECL_NSIRDFDATASOURCE

 protected:
    virtual ~nsWindowDataSource();

 private:

    
    nsInterfaceHashtable<nsPtrHashKey<nsIXULWindow>, nsIRDFResource> mWindowResources;

    static uint32_t windowCount;
    static uint32_t gRefCnt;

    nsCOMPtr<nsIRDFDataSource> mInner;
    nsCOMPtr<nsIRDFContainer> mContainer;

    static nsIRDFResource* kNC_Name;
    static nsIRDFResource* kNC_KeyIndex;
    static nsIRDFResource* kNC_WindowRoot;
    static nsIRDFService* gRDFService;
};

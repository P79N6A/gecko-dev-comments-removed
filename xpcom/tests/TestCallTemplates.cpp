












































#include "nsISupportsUtils.h"
#include "nsIWeakReference.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#define NS_ITESTSERVICE_IID \
  {0x127b5253, 0x37b1, 0x43c7, \
    { 0x96, 0x2b, 0xab, 0xf1, 0x2d, 0x22, 0x56, 0xae }}

class NS_NO_VTABLE nsITestService : public nsISupports {
  public: 
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITESTSERVICE_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITestService, NS_ITESTSERVICE_IID)

class nsTestService : public nsITestService, public nsSupportsWeakReference
{
  public:
    NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS2(nsTestService, nsITestService, nsISupportsWeakReference)

#define NS_TEST_SERVICE_CONTRACTID "@mozilla.org/test/testservice;1"
#define NS_TEST_SERVICE_CID \
  {0xa00c1406, 0x283a, 0x45c9, \
    {0xae, 0xd2, 0x1a, 0xb6, 0xdd, 0xba, 0xfe, 0x53}}
static NS_DEFINE_CID(kTestServiceCID, NS_TEST_SERVICE_CID);

int main()
{
    







    NS_NOTREACHED("This test is not intended to run, only to compile!");

    

    nsISupports *mySupportsPtr = NS_REINTERPRET_CAST(nsISupports*, 0x1000);

    nsITestService *myITestService = nsnull;
    CallQueryInterface(mySupportsPtr, &myITestService);

    nsTestService *myTestService =
        NS_REINTERPRET_CAST(nsTestService*, mySupportsPtr);
    nsISupportsWeakReference *mySupportsWeakRef;
    CallQueryInterface(myTestService, &mySupportsWeakRef);

    

    nsIWeakReference *myWeakRef =
        NS_STATIC_CAST(nsIWeakReference*, mySupportsPtr);
    CallQueryReferent(myWeakRef, &myITestService);

    

    CallCreateInstance(kTestServiceCID, mySupportsPtr, &myITestService);
    CallCreateInstance(kTestServiceCID, &myITestService);
    CallCreateInstance(NS_TEST_SERVICE_CONTRACTID, mySupportsPtr,
                       &myITestService);
    CallCreateInstance(NS_TEST_SERVICE_CONTRACTID, &myITestService);

    
    CallGetService(kTestServiceCID, &myITestService);
    CallGetService(NS_TEST_SERVICE_CONTRACTID, &myITestService);

    
    nsIInterfaceRequestor *myInterfaceRequestor =
        NS_STATIC_CAST(nsIInterfaceRequestor*, mySupportsPtr);
    CallGetInterface(myInterfaceRequestor, &myITestService);

    return 0;
}

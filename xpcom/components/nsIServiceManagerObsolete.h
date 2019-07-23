




































#ifndef nsIServiceManagerObsolete_h___
#define nsIServiceManagerObsolete_h___












#include "nsIComponentManager.h"
#include "nsID.h"

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

class nsIServiceManager;
class nsIShutdownListener;
class nsIDirectoryServiceProvider;

class nsServiceManagerObsolete;

#define NS_ISERVICEMANAGER_OBSOLETE_IID              \
{ /* cf0df3b0-3401-11d2-8163-006008119d7a */         \
    0xcf0df3b0,                                      \
    0x3401,                                          \
    0x11d2,                                          \
    {0x81, 0x63, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}





class nsIServiceManagerObsolete : public nsISupports {
public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISERVICEMANAGER_OBSOLETE_IID)

    





    NS_IMETHOD
    RegisterService(const nsCID& aClass, nsISupports* aService) = 0;

    









    NS_IMETHOD
    UnregisterService(const nsCID& aClass) = 0;

    NS_IMETHOD
    GetService(const nsCID& aClass, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = nsnull) = 0;

    
    NS_IMETHOD
    ReleaseService(const nsCID& aClass, nsISupports* service,
                   nsIShutdownListener* shutdownListener = nsnull) = 0;

    
    

    NS_IMETHOD
    RegisterService(const char* aContractID, nsISupports* aService) = 0;

    NS_IMETHOD
    UnregisterService(const char* aContractID) = 0;

    NS_IMETHOD
    GetService(const char* aContractID, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = nsnull) = 0;

    
    NS_IMETHOD
    ReleaseService(const char* aContractID, nsISupports* service,
                   nsIShutdownListener* shutdownListener = nsnull) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIServiceManagerObsolete,
                              NS_ISERVICEMANAGER_OBSOLETE_IID)


class NS_COM nsServiceManager {
public:

    static nsresult
    RegisterService(const nsCID& aClass, nsISupports* aService);

    static nsresult
    UnregisterService(const nsCID& aClass);

    static nsresult
    GetService(const nsCID& aClass, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = nsnull);

    
    static nsresult
    ReleaseService(const nsCID& aClass, nsISupports* service,
                   nsIShutdownListener* shutdownListener = nsnull);

    
    

    static nsresult
    RegisterService(const char* aContractID, nsISupports* aService);

    static nsresult
    UnregisterService(const char* aContractID);

    static nsresult
    GetService(const char* aContractID, const nsIID& aIID,
               nsISupports* *result,
               nsIShutdownListener* shutdownListener = nsnull);

    
    static nsresult
    ReleaseService(const char* aContractID, nsISupports* service,
                   nsIShutdownListener* shutdownListener = nsnull);


    
    
    
    static nsresult GetGlobalServiceManager(nsIServiceManager* *result);
    static nsresult ShutdownGlobalServiceManager(nsIServiceManager* *result);
};


#define NS_DECL_NSISERVICEMANAGEROBSOLETE \
    NS_IMETHOD RegisterService(const nsCID& aClass, nsISupports* aService); \
    NS_IMETHOD UnregisterService(const nsCID& aClass);\
    NS_IMETHOD GetService(const nsCID& aClass, const nsIID& aIID, nsISupports* *result, nsIShutdownListener* shutdownListener);\
    NS_IMETHOD ReleaseService(const nsCID& aClass, nsISupports* service, nsIShutdownListener* shutdownListener);\
    NS_IMETHOD RegisterService(const char* aContractID, nsISupports* aService);\
    NS_IMETHOD UnregisterService(const char* aContractID);\
    NS_IMETHOD GetService(const char* aContractID, const nsIID& aIID, nsISupports* *result, nsIShutdownListener* shutdownListener);\
    NS_IMETHOD ReleaseService(const char* aContractID, nsISupports* service, nsIShutdownListener* shutdownListener);



#define NS_ISHUTDOWNLISTENER_IID                     \
{ /* 56decae0-3406-11d2-8163-006008119d7a */         \
    0x56decae0,                                      \
    0x3406,                                          \
    0x11d2,                                          \
    {0x81, 0x63, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}




#endif 

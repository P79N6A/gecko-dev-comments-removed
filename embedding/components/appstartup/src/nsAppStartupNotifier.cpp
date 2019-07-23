




































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsAppStartupNotifier.h"

NS_IMPL_ISUPPORTS1(nsAppStartupNotifier, nsIObserver)

nsAppStartupNotifier::nsAppStartupNotifier()
{
}

nsAppStartupNotifier::~nsAppStartupNotifier()
{
}

NS_IMETHODIMP nsAppStartupNotifier::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    NS_ENSURE_ARG(aTopic);
    nsresult rv;

    
    nsCOMPtr<nsICategoryManager> categoryManager =
                    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = categoryManager->EnumerateCategory(aTopic,
                               getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> entry;
    while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
        nsCOMPtr<nsISupportsCString> category = do_QueryInterface(entry, &rv);

        if (NS_SUCCEEDED(rv)) {
            nsCAutoString categoryEntry;
            rv = category->GetData(categoryEntry);

            nsXPIDLCString contractId;
            categoryManager->GetCategoryEntry(aTopic, 
                                              categoryEntry.get(),
                                              getter_Copies(contractId));

            if (NS_SUCCEEDED(rv)) {

                
                
                

                const char *pServicePrefix = "service,";
                
                nsCAutoString cid(contractId);
                PRInt32 serviceIdx = cid.Find(pServicePrefix);

                nsCOMPtr<nsIObserver> startupObserver;
                if (serviceIdx == 0)
                    startupObserver = do_GetService(cid.get() + strlen(pServicePrefix), &rv);
                else
                    startupObserver = do_CreateInstance(contractId, &rv);

                if (NS_SUCCEEDED(rv)) {
                    rv = startupObserver->Observe(nsnull, aTopic, nsnull);
 
                    
                    NS_ASSERTION(NS_SUCCEEDED(rv), "Startup Observer failed!\n");
                }
                else {
                  #ifdef NS_DEBUG
                    nsCAutoString warnStr("Cannot create startup observer : ");
                    warnStr += contractId.get();
                    NS_WARNING(warnStr.get());
                  #endif
                }
            }
        }
    }

    return NS_OK;
}

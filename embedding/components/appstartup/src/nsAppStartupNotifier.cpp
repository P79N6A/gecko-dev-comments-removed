




#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsAppStartupNotifier.h"

#include "mozilla/FunctionTimer.h"

NS_IMPL_ISUPPORTS1(nsAppStartupNotifier, nsIObserver)

nsAppStartupNotifier::nsAppStartupNotifier()
{
}

nsAppStartupNotifier::~nsAppStartupNotifier()
{
}

NS_IMETHODIMP nsAppStartupNotifier::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    NS_TIME_FUNCTION;

    NS_ENSURE_ARG(aTopic);
    nsresult rv;

    
    nsCOMPtr<nsICategoryManager> categoryManager =
                    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = categoryManager->EnumerateCategory(aTopic,
                               getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    NS_TIME_FUNCTION_MARK("EnumerateCategory");

    nsCOMPtr<nsISupports> entry;
    while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
        nsCOMPtr<nsISupportsCString> category = do_QueryInterface(entry, &rv);

        if (NS_SUCCEEDED(rv)) {
            nsAutoCString categoryEntry;
            rv = category->GetData(categoryEntry);

            nsXPIDLCString contractId;
            categoryManager->GetCategoryEntry(aTopic, 
                                              categoryEntry.get(),
                                              getter_Copies(contractId));

            if (NS_SUCCEEDED(rv)) {

                
                
                

                nsCOMPtr<nsISupports> startupInstance;
                if (Substring(contractId, 0, 8).EqualsLiteral("service,"))
                    startupInstance = do_GetService(contractId.get() + 8, &rv);
                else
                    startupInstance = do_CreateInstance(contractId, &rv);

                if (NS_SUCCEEDED(rv)) {
                    
                    nsCOMPtr<nsIObserver> startupObserver =
                        do_QueryInterface(startupInstance, &rv);
                    if (NS_SUCCEEDED(rv)) {
                        rv = startupObserver->Observe(nullptr, aTopic, nullptr);
     
                        
                        NS_ASSERTION(NS_SUCCEEDED(rv), "Startup Observer failed!\n");
                    }
                }
                else {
                  #ifdef DEBUG
                    nsAutoCString warnStr("Cannot create startup observer : ");
                    warnStr += contractId.get();
                    NS_WARNING(warnStr.get());
                  #endif
                }

                NS_TIME_FUNCTION_MARK("observer: category: %s cid: %s", categoryEntry.get(), nsPromiseFlatCString(contractId).get());

            }
        }
    }

    return NS_OK;
}

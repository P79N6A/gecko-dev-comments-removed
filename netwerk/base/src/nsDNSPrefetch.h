





































#ifndef nsDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDNSListener.h"

class nsIURI;
class nsIDNSService;

class nsDNSPrefetch : public nsIDNSListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSLISTENER
  
    nsDNSPrefetch(nsIURI *aURI);

    static nsresult Initialize(nsIDNSService *aDNSService);
    static nsresult Shutdown();

    
    nsresult PrefetchHigh();
    nsresult PrefetchMedium();
    nsresult PrefetchLow();
  
private:
    nsCString  mHostname;
    
    nsresult Prefetch(PRUint16 flags);
};

#endif 

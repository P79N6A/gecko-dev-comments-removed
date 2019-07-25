





































#ifndef nsDNSPrefetch_h___
#define nsDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/TimeStamp.h"

#include "nsIDNSListener.h"

class nsIURI;
class nsIDNSService;

class nsDNSPrefetch : public nsIDNSListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSLISTENER
  
    nsDNSPrefetch(nsIURI *aURI, PRBool storeTiming);
    bool TimingsValid() const {
        return !mStartTimestamp.IsNull() && !mEndTimestamp.IsNull();
    }
    
    const mozilla::TimeStamp& StartTimestamp() const { return mStartTimestamp; }
    const mozilla::TimeStamp& EndTimestamp() const { return mEndTimestamp; }

    static nsresult Initialize(nsIDNSService *aDNSService);
    static nsresult Shutdown();

    
    nsresult PrefetchHigh();
    nsresult PrefetchMedium();
    nsresult PrefetchLow();
  
private:
    nsCString mHostname;
    PRBool mStoreTiming;
    mozilla::TimeStamp mStartTimestamp;
    mozilla::TimeStamp mEndTimestamp;

    nsresult Prefetch(PRUint16 flags);
};

#endif 

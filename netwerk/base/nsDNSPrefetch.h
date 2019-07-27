




#ifndef nsDNSPrefetch_h___
#define nsDNSPrefetch_h___

#include "nsWeakReference.h"
#include "nsString.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

#include "nsIDNSListener.h"

class nsIURI;
class nsIDNSService;

class nsDNSPrefetch final : public nsIDNSListener
{
    ~nsDNSPrefetch() {}

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIDNSLISTENER
  
    nsDNSPrefetch(nsIURI *aURI, nsIDNSListener *aListener, bool storeTiming);
    bool TimingsValid() const {
        return !mStartTimestamp.IsNull() && !mEndTimestamp.IsNull();
    }
    
    const mozilla::TimeStamp& StartTimestamp() const { return mStartTimestamp; }
    const mozilla::TimeStamp& EndTimestamp() const { return mEndTimestamp; }

    static nsresult Initialize(nsIDNSService *aDNSService);
    static nsresult Shutdown();

    
    nsresult PrefetchHigh(bool refreshDNS = false);
    nsresult PrefetchMedium(bool refreshDNS = false);
    nsresult PrefetchLow(bool refreshDNS = false);
  
private:
    nsCString mHostname;
    bool mStoreTiming;
    mozilla::TimeStamp mStartTimestamp;
    mozilla::TimeStamp mEndTimestamp;
    nsWeakPtr mListener;

    nsresult Prefetch(uint16_t flags);
};

#endif 






#ifndef nsDNSPrefetch_h___
#define nsDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

#include "nsIDNSListener.h"

class nsIURI;
class nsIDNSService;

class nsDNSPrefetch MOZ_FINAL : public nsIDNSListener
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIDNSLISTENER
  
    nsDNSPrefetch(nsIURI *aURI, bool storeTiming);
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
    bool mStoreTiming;
    mozilla::TimeStamp mStartTimestamp;
    mozilla::TimeStamp mEndTimestamp;

    nsresult Prefetch(uint16_t flags);
};

#endif 






#ifndef nsHTMLDNSPrefetch_h___
#define nsHTMLDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsIDNSListener.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"

class nsIDocument;
class nsITimer;
namespace mozilla {
namespace dom {
class Link;
} 
} 

namespace mozilla {
namespace net {
class NeckoParent;
} 
} 

class nsHTMLDNSPrefetch 
{
public:
  
  
  
  static bool     IsAllowed(nsIDocument *aDocument);
 
  static nsresult Initialize();
  static nsresult Shutdown();
  
  
  
  
  
  
  
  

  static nsresult PrefetchHigh(mozilla::dom::Link *aElement);
  static nsresult PrefetchMedium(mozilla::dom::Link *aElement);
  static nsresult PrefetchLow(mozilla::dom::Link *aElement);
  static nsresult PrefetchHigh(const nsAString &host);
  static nsresult PrefetchMedium(const nsAString &host);
  static nsresult PrefetchLow(const nsAString &host);
  static nsresult CancelPrefetchLow(const nsAString &host, nsresult aReason);
  static nsresult CancelPrefetchLow(mozilla::dom::Link *aElement,
                                    nsresult aReason);

private:
  static nsresult Prefetch(const nsAString &host, uint16_t flags);
  static nsresult Prefetch(mozilla::dom::Link *aElement, uint16_t flags);
  static nsresult CancelPrefetch(const nsAString &hostname,
                                 uint16_t flags,
                                 nsresult aReason);
  static nsresult CancelPrefetch(mozilla::dom::Link *aElement,
                                 uint16_t flags,
                                 nsresult aReason);
  
public:
  class nsListener MOZ_FINAL : public nsIDNSListener
  {
    
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSLISTENER

    nsListener()  {}
  private:
    ~nsListener() {}
  };
  
  class nsDeferrals MOZ_FINAL: public nsIWebProgressListener
                             , public nsSupportsWeakReference
                             , public nsIObserver
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIOBSERVER

    nsDeferrals();
    
    void Activate();
    nsresult Add(uint16_t flags, mozilla::dom::Link *aElement);
    
  private:
    ~nsDeferrals();
    void Flush();
    
    void SubmitQueue();
    
    uint16_t                  mHead;
    uint16_t                  mTail;
    uint32_t                  mActiveLoaderCount;

    nsCOMPtr<nsITimer>        mTimer;
    bool                      mTimerArmed;
    static void Tick(nsITimer *aTimer, void *aClosure);
    
    static const int          sMaxDeferred = 512;  
    static const int          sMaxDeferredMask = (sMaxDeferred - 1);
    
    struct deferred_entry
    {
      uint16_t                         mFlags;
      nsWeakPtr                        mElement;
    } mEntries[sMaxDeferred];
  };

  friend class mozilla::net::NeckoParent;
};

#endif 

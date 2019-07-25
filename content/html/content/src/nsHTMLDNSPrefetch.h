





































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
  static nsresult PrefetchHigh(nsAString &host);
  static nsresult PrefetchMedium(nsAString &host);
  static nsresult PrefetchLow(nsAString &host);
  static nsresult CancelPrefetchLow(nsAString &host, nsresult aReason);
  static nsresult CancelPrefetchLow(mozilla::dom::Link *aElement, nsresult aReason);

private:
  static nsresult Prefetch(nsAString &host, PRUint16 flags);
  static nsresult Prefetch(mozilla::dom::Link *aElement, PRUint16 flags);
  static nsresult CancelPrefetch(nsAString &hostname,
                                 PRUint16 flags,
                                 nsresult aReason);
  static nsresult CancelPrefetch(mozilla::dom::Link *aElement,
                                 PRUint16 flags,
                                 nsresult aReason);
  
public:
  class nsListener : public nsIDNSListener
  {
    
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSLISTENER

    nsListener()  {}
  private:
    ~nsListener() {}
  };
  
  class nsDeferrals : public nsIWebProgressListener
                    , public nsSupportsWeakReference
                    , public nsIObserver
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIOBSERVER

    nsDeferrals();
    
    void Activate();
    nsresult Add(PRUint16 flags, mozilla::dom::Link *aElement);
    nsresult Remove(PRUint16 aFlags, mozilla::dom::Link *aElement, bool *aFound);
    
  private:
    ~nsDeferrals();
    void Flush();
    
    void SubmitQueue();
    
    PRUint16                  mHead;
    PRUint16                  mTail;
    PRUint32                  mActiveLoaderCount;

    nsCOMPtr<nsITimer>        mTimer;
    bool                      mTimerArmed;
    static void Tick(nsITimer *aTimer, void *aClosure);
    
    static const int          sMaxDeferred = 512;  
    static const int          sMaxDeferredMask = (sMaxDeferred - 1);
    
    struct deferred_entry
    {
      PRUint16                         mFlags;
      nsWeakPtr                        mElement;
    } mEntries[sMaxDeferred];
  };

  friend class mozilla::net::NeckoParent;
};

#endif 

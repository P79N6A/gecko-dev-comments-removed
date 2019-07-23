





































#ifndef nsHTMLDNSPrefetch_h___
#define nsHTMLDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDNSListener.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsIURI;
class nsIDocument;

class nsHTMLDNSPrefetch 
{
public:
  
  
  
  static PRBool   IsAllowed(nsIDocument *aDocument);
 
  static nsresult Initialize();
  static nsresult Shutdown();
  
  
  
  
  
  
  
  

  static nsresult PrefetchHigh(nsIURI *aURI);
  static nsresult PrefetchMedium(nsIURI *aURI);
  static nsresult PrefetchLow(nsIURI *aURI);
  static nsresult PrefetchHigh(nsAString &host);
  static nsresult PrefetchMedium(nsAString &host);
  static nsresult PrefetchLow(nsAString &host);

private:
  static nsresult Prefetch(nsAString &host, PRUint16 flags);
  static nsresult Prefetch(nsIURI *aURI, PRUint16 flags);
  static PRBool   IsSecureBaseContext(nsIDocument *aDocument);
  
public:
  class nsDeferrals : public nsIDNSListener
                    , public nsIWebProgressListener
                    , public nsSupportsWeakReference
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER
    
    nsDeferrals();
    
    void Activate();
    nsresult Add(PRUint16 flags, nsIURI *aURI);
    
  private:
    ~nsDeferrals() {}
    
    void SubmitQueue();
    
    PRUint16                  mHead;
    PRUint16                  mTail;
    PRUint32                  mActiveLoaderCount;
    
    static const int          sMaxDeferred = 512;  
    static const int          sMaxDeferredMask = (sMaxDeferred - 1);
    
    struct deferred_entry
    {
      PRUint16                 mFlags;
      nsCOMPtr<nsIURI>         mURI;
    } mEntries[sMaxDeferred];
  };
};

#endif 

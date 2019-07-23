





































#ifndef nsHMTLDNSPrefetch_h___

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIDNSListener.h"

class nsIURI;
class nsIDocument;

class nsHTMLDNSPrefetch : public nsIDNSListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSLISTENER
  
  
  
  

  nsHTMLDNSPrefetch(nsAString &aHostname, nsIDocument *aDocument);
  nsHTMLDNSPrefetch(nsIURI *aURI,         nsIDocument *aDocument);
  
  static nsresult Initialize();
  static nsresult Shutdown();
  
  
  nsresult PrefetchHigh();
  nsresult PrefetchMedium();
  nsresult PrefetchLow();
  
private:
  nsCString  mHostname;
  PRBool     mAllowed;
    
  nsresult Prefetch(PRUint16 flags);
  PRBool   IsSecureBaseContext(nsIDocument *aDocument);
  PRBool   IsAllowed(nsIDocument *aDocument);
};

#endif 

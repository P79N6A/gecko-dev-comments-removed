



































#ifndef nsXMLEncodingObserverFactory_h__
#define nsXMLEncodingObserverFactory_h__

#include "nsIFactory.h"
#include "nsIXMLEncodingService.h"
#include "nsIElementObserver.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsObserverBase.h"
#include "nsWeakReference.h"

class nsXMLEncodingObserver: public nsIElementObserver, 
                             public nsIObserver, 
                             public nsObserverBase,
                             public nsIXMLEncodingService,
                             public nsSupportsWeakReference {
public:
  nsXMLEncodingObserver();
  virtual ~nsXMLEncodingObserver();

  

  







  NS_IMETHOD Notify(PRUint32 aDocumentID, eHTMLTags aTag, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);
  NS_IMETHOD Notify(PRUint32 aDocumentID, const PRUnichar* aTag, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);
  NS_IMETHOD Notify(nsISupports* aWebShell,
                    nsISupports* aChannel,
                    const PRUnichar* aTag, 
                    const nsStringArray* keys, 
                    const nsStringArray* values,
                    const PRUint32 aFlags)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD Start();
  NS_IMETHOD End();

private:
  NS_IMETHOD Notify(PRUint32 aDocumentID, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);

  PRBool bXMLEncodingObserverStarted;
};

#endif 

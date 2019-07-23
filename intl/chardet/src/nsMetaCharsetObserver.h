



































#ifndef nsMetaCharsetObserverFactory_h__
#define nsMetaCharsetObserverFactory_h__

#include "nsIFactory.h"
#include "nsIMetaCharsetService.h"
#include "nsIElementObserver.h"
#include "nsIObserver.h"
#include "nsObserverBase.h"
#include "nsWeakReference.h"
#include "nsTArray.h"






class nsMetaCharsetObserver: public nsIElementObserver, 
                             public nsIObserver, 
                             public nsObserverBase,
                             public nsIMetaCharsetService,
                             public nsSupportsWeakReference {
public:
  nsMetaCharsetObserver();
  virtual ~nsMetaCharsetObserver();

  

  







  NS_IMETHOD Notify(PRUint32 aDocumentID, eHTMLTags aTag, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);
  NS_IMETHOD Notify(PRUint32 aDocumentID, const PRUnichar* aTag, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);

  NS_IMETHOD Notify(nsISupports* aWebShell, 
                    nsISupports* aChannel,
                    const PRUnichar* aTag, 
                    const nsTArray<nsString>* keys, 
                    const nsTArray<nsString>* values,
                    const PRUint32 aFlags);

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  
  NS_IMETHOD Start();
  NS_IMETHOD End();
 
private:

  NS_IMETHOD Notify(PRUint32 aDocumentID, PRUint32 numOfAttributes, 
                    const PRUnichar* nameArray[], const PRUnichar* valueArray[]);

  NS_IMETHOD Notify(nsISupports* aWebShell, 
                    nsISupports* aChannel,
                    const nsTArray<nsString>* keys, 
                    const nsTArray<nsString>* values);

  NS_IMETHOD GetCharsetFromCompatibilityTag(const nsTArray<nsString>* keys, 
                                            const nsTArray<nsString>* values, 
                                            nsAString& aCharset);

  nsCOMPtr<nsICharsetAlias> mAlias;

  PRBool bMetaCharsetObserverStarted;
};

#endif 

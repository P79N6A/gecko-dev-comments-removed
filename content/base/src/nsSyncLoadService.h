










































#ifndef nsSyncLoadService_h__
#define nsSyncLoadService_h__

#include "nsISyncLoadDOMService.h"

class nsIInputStream;
class nsILoadGroup;
class nsIStreamListener;

class nsSyncLoadService : public nsISyncLoadDOMService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISYNCLOADDOMSERVICE

    











    static nsresult LoadDocument(nsIURI *aURI, nsIPrincipal *aLoaderPrincipal,
                                 nsILoadGroup *aLoadGroup, PRBool aForceToXML,
                                 nsIDOMDocument** aResult);

    








    static nsresult PushSyncStreamToListener(nsIInputStream* aIn,
                                             nsIStreamListener* aListener,
                                             nsIChannel* aChannel);
};

#endif 

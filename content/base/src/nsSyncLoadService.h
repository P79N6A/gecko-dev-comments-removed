










































#ifndef nsSyncLoadService_h__
#define nsSyncLoadService_h__

#include "nscore.h"

class nsIInputStream;
class nsILoadGroup;
class nsIStreamListener;
class nsIURI;
class nsIPrincipal;
class nsIDOMDocument;
class nsIChannel;

class nsSyncLoadService
{
public:
    











    static nsresult LoadDocument(nsIURI *aURI, nsIPrincipal *aLoaderPrincipal,
                                 nsILoadGroup *aLoadGroup, PRBool aForceToXML,
                                 nsIDOMDocument** aResult);

    








    static nsresult PushSyncStreamToListener(nsIInputStream* aIn,
                                             nsIStreamListener* aListener,
                                             nsIChannel* aChannel);
};

#endif 

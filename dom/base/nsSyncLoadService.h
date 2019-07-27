








#ifndef nsSyncLoadService_h__
#define nsSyncLoadService_h__

#include "nscore.h"
#include "mozilla/net/ReferrerPolicy.h"

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
                                 nsILoadGroup *aLoadGroup, bool aForceToXML,
                                 mozilla::net::ReferrerPolicy aReferrerPolicy,
                                 nsIDOMDocument** aResult);

    








    static nsresult PushSyncStreamToListener(nsIInputStream* aIn,
                                             nsIStreamListener* aListener,
                                             nsIChannel* aChannel);
};

#endif 

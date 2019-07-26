



#ifndef _nsApplicationCacheService_h_
#define _nsApplicationCacheService_h_

#include "mozilla/Attributes.h"

class nsApplicationCacheService MOZ_FINAL : public nsIApplicationCacheService
{
public:
    nsApplicationCacheService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAPPLICATIONCACHESERVICE
private:
    nsresult GetJARIdentifier(nsIURI *aURI,
                              nsILoadContext *aLoadContext,
                              nsACString &_result);

    nsRefPtr<nsCacheService> mCacheService;
};

#endif 

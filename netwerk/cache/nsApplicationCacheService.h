



#ifndef _nsApplicationCacheService_h_
#define _nsApplicationCacheService_h_

#include "nsIApplicationCacheService.h"
#include "mozilla/Attributes.h"

class nsCacheService;

class nsApplicationCacheService MOZ_FINAL : public nsIApplicationCacheService
{
public:
    nsApplicationCacheService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAPPLICATIONCACHESERVICE

    static void AppClearDataObserverInit();

private:
    nsresult GetJARIdentifier(nsIURI *aURI,
                              nsILoadContext *aLoadContext,
                              nsACString &_result);

    nsRefPtr<nsCacheService> mCacheService;
};

#endif 





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
    nsRefPtr<nsCacheService> mCacheService;
};

#endif 

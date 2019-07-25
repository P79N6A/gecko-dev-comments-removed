



#ifndef _nsApplicationCacheService_h_
#define _nsApplicationCacheService_h_

class nsApplicationCacheService : public nsIApplicationCacheService
{
public:
    nsApplicationCacheService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAPPLICATIONCACHESERVICE
private:
    nsRefPtr<nsCacheService> mCacheService;
};

#endif 

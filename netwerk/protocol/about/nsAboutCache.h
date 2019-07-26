




#ifndef nsAboutCache_h__
#define nsAboutCache_h__

#include "nsIAboutModule.h"
#include "nsICacheStorageVisitor.h"
#include "nsICacheStorage.h"

#include "nsString.h"
#include "nsIOutputStream.h"
#include "nsILoadContextInfo.h"

#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsAboutCache : public nsIAboutModule 
                   , public nsICacheStorageVisitor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIABOUTMODULE
    NS_DECL_NSICACHESTORAGEVISITOR

    nsAboutCache() {}

    static nsresult
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    static nsresult
    GetStorage(nsACString const & storageName, nsILoadContextInfo* loadInfo,
               nsICacheStorage **storage);

protected:
    virtual ~nsAboutCache() {}

    nsresult ParseURI(nsIURI * uri, nsACString & storage);

    
    
    
    nsresult VisitNextStorage();
    
    
    
    void FireVisitStorage();
    
    
    
    nsresult VisitStorage(nsACString const & storageName);

    
    
    void FlushBuffer();

    
    
    bool mOverview;

    
    
    bool mEntriesHeaderAdded;

    
    nsCOMPtr<nsILoadContextInfo> mLoadInfo;
    nsCString mContextString;

    
    nsTArray<nsCString> mStorageList;
    nsCString mStorageName;
    nsCOMPtr<nsICacheStorage> mStorage;

    
    nsCString mBuffer;
    nsCOMPtr<nsIOutputStream> mStream;
};

#define NS_ABOUT_CACHE_MODULE_CID                    \
{ /* 9158c470-86e4-11d4-9be2-00e09872a416 */         \
    0x9158c470,                                      \
    0x86e4,                                          \
    0x11d4,                                          \
    {0x9b, 0xe2, 0x00, 0xe0, 0x98, 0x72, 0xa4, 0x16} \
}

#endif 

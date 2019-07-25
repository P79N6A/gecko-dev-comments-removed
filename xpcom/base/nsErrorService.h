




#ifndef nsErrorService_h__
#define nsErrorService_h__

#include "mozilla/Attributes.h"

#include "nsIErrorService.h"
#include "nsHashtable.h"

class nsInt2StrHashtable
{
public:
    nsInt2StrHashtable();

    nsresult  Put(uint32_t key, const char* aData);
    char*     Get(uint32_t key);
    nsresult  Remove(uint32_t key);

protected:
    nsObjectHashtable mHashtable;
};

class nsErrorService MOZ_FINAL : public nsIErrorService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIERRORSERVICE

    nsErrorService() {}

    static nsresult
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

private:
    ~nsErrorService() {}

protected:
    nsInt2StrHashtable mErrorStringBundleURLMap;
    nsInt2StrHashtable mErrorStringBundleKeyMap;
};

#endif 

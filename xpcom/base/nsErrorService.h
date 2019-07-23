




































#ifndef nsErrorService_h__
#define nsErrorService_h__

#include "nsIErrorService.h"
#include "nsHashtable.h"

class nsInt2StrHashtable
{
public:
    nsInt2StrHashtable();

    nsresult  Put(PRUint32 key, const char* aData);
    char*     Get(PRUint32 key);
    nsresult  Remove(PRUint32 key);

protected:
    nsObjectHashtable mHashtable;
};

class nsErrorService : public nsIErrorService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIERRORSERVICE

    nsErrorService() {}

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

private:
    ~nsErrorService() {}

protected:
    nsInt2StrHashtable mErrorStringBundleURLMap;
    nsInt2StrHashtable mErrorStringBundleKeyMap;
};

#endif 











































#include "xpctest_private.h"

class xpcstringtest : public nsIXPCTestString
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTSTRING

    xpcstringtest();
    virtual ~xpcstringtest();
};

xpcstringtest::xpcstringtest()
{
    NS_ADDREF_THIS();
}

xpcstringtest::~xpcstringtest()
{
}

NS_IMPL_ISUPPORTS1(xpcstringtest, nsIXPCTestString)


NS_IMETHODIMP
xpcstringtest::GetStringA(char **_retval)
{
    const char myResult[] = "result of xpcstringtest::GetStringA";

    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = (char*) nsMemory::Clone(myResult,
                                          sizeof(char)*(strlen(myResult)+1));
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
xpcstringtest::GetStringB(char **s)
{
    const char myResult[] = "result of xpcstringtest::GetStringB";

    if(!s)
        return NS_ERROR_NULL_POINTER;

    *s = (char*) nsMemory::Clone(myResult,
                                    sizeof(char)*(strlen(myResult)+1));

    return *s ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}



NS_IMETHODIMP
xpcstringtest::GetStringC(const char **s)
{
    static const char myResult[] = "result of xpcstringtest::GetStringC";
    if(!s)
        return NS_ERROR_NULL_POINTER;
    *s = myResult;
    return NS_OK;
}


static PRUnichar* GetTestWString(int* size)
{
    static PRUnichar* sWStr;            
    static char str[] = "This is part of a long string... ";
    static const int slen = (sizeof(str)-1)/sizeof(char);
    static const int rep = 1;
    static const int space = (slen*rep*sizeof(PRUnichar))+sizeof(PRUnichar);

    if(!sWStr)
    {
        sWStr = (PRUnichar*) nsMemory::Alloc(space);
        if(sWStr)
        {
            PRUnichar* p = sWStr;
            for(int k = 0; k < rep; k++)
                for (int i = 0; i < slen; i++)
                    *(p++) = (PRUnichar) str[i];
        *p = 0;        
        }
    }
    if(size)
        *size = space;
    return sWStr;
}        


NS_IMETHODIMP xpcstringtest::GetWStringCopied(PRUnichar **s)
{
    if(!s)
        return NS_ERROR_NULL_POINTER;

    int size;
    PRUnichar* str = GetTestWString(&size);
    if(!str)
        return NS_ERROR_OUT_OF_MEMORY;

    *s = (PRUnichar*) nsMemory::Clone(str, size);
    return *s ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}        


NS_IMETHODIMP xpcstringtest::GetWStringShared(const PRUnichar **s)
{
    if(!s)
        return NS_ERROR_NULL_POINTER;
    *s = GetTestWString(nsnull);
    return NS_OK;
}        




NS_IMETHODIMP
xpctest::ConstructStringTest(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcstringtest* obj = new xpcstringtest();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}






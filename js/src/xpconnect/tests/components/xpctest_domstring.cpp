







































#include "nsISupports.h"
#include "nsString.h"
#include "xpctest_domstring.h"
#include "xpctest_private.h"

class xpcTestDOMString : public nsIXPCTestDOMString {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTDOMSTRING
    xpcTestDOMString();
    virtual ~xpcTestDOMString();
private:
    nsString mStr;
};

NS_IMPL_ISUPPORTS1(xpcTestDOMString, nsIXPCTestDOMString)

xpcTestDOMString::xpcTestDOMString()
{
    NS_ADDREF_THIS();
}

xpcTestDOMString::~xpcTestDOMString()
{
}

NS_IMETHODIMP
xpcTestDOMString::HereHaveADOMString(const nsAString &str)
{
    
    mStr = str;
    return NS_OK;
}

NS_IMETHODIMP
xpcTestDOMString::DontKeepThisOne(const nsAString &str)
{
    nsCString c; c.AssignWithConversion(str);
    fprintf(stderr, "xpcTestDOMString::DontKeepThisOne: \"%s\"\n", c.get());
    return NS_OK;
}

NS_IMETHODIMP
xpcTestDOMString::GiveDOMStringTo(nsIXPCTestDOMString *recv)
{
    NS_NAMED_LITERAL_STRING(myString, "A DOM String, Just For You");
    return recv->HereHaveADOMString(myString);
}

NS_IMETHODIMP
xpcTestDOMString::PassDOMStringThroughTo(const nsAString &str,
                                         nsIXPCTestDOMString *recv)
{
    return recv->HereHaveADOMString(str);
}


NS_IMETHODIMP
xpctest::ConstructXPCTestDOMString(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult)
{
    nsresult rv;
    NS_ASSERTION(!aOuter, "no aggregation");
    xpcTestDOMString *obj = new xpcTestDOMString();
    if (obj)
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










































#include "nsISupports.h"
#include "xpctest_const.h"
#include "xpctest_private.h"

class xpcTestConst : public nsIXPCTestConst {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTCONST
  xpcTestConst();
};

NS_IMPL_ISUPPORTS1(xpcTestConst, nsIXPCTestConst)

xpcTestConst :: xpcTestConst() {
    NS_ADDREF_THIS();
}

NS_IMETHODIMP
xpctest::ConstructXPCTestConst(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestConst *obj = new xpcTestConst();
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

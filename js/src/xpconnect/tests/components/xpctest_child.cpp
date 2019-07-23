










































#include "xpctest_private.h"

#define USE_MI 0

#if USE_MI


class xpctestOther : public nsIXPCTestOther
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTOTHER

    xpctestOther();
};

class xpctestChild : public nsIXPCTestChild, public xpctestOther
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIXPCTESTPARENT
    NS_DECL_NSIXPCTESTCHILD

    xpctestChild();
};

NS_IMPL_ISUPPORTS1(xpctestOther, nsIXPCTestOther)

xpctestOther::xpctestOther()
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpctestOther::Method3(PRInt16 i, PRInt16 j, PRInt16 k)
{
    printf("Method3 called on inherited other\n");
    return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED2(xpctestChild,
                             xpctestOther,
                             nsIXPCTestChild,
                             nsIXPCTestParent)

xpctestChild::xpctestChild()
{
}

NS_IMETHODIMP xpctestChild::Method1(PRInt16 i)
{
    printf("Method1 called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::Method1a(nsIXPCTestParent *foo)
{
    printf("Method1a called on child\n");
    return NS_OK;
}


NS_IMETHODIMP xpctestChild::Method2(PRInt16 i, PRInt16 j)
{
    printf("Method2 called on child\n");
    return NS_OK;
}

#if 0
class xpctestParent : public nsIXPCTestParent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTPARENT

    xpctestParent();
};


class xpctestChild : public xpctestParent, public nsIXPCTestChild
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTCHILD

    xpctestChild();
};

NS_IMETHODIMP xpctestParent::Method1(PRInt16 i)
{
    printf("Method1 called on parent via child\n");
    return NS_OK;
}
NS_IMETHODIMP xpctestParent::Method1a(nsIXPCTestParent *foo)
{
    printf("Method1a called on parent via child\n");
    return NS_OK;
}

#endif


#else

class xpctestChild : public nsIXPCTestChild
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTPARENT
    NS_DECL_NSIXPCTESTCHILD

    xpctestChild();
};

NS_IMPL_ADDREF(xpctestChild)
NS_IMPL_RELEASE(xpctestChild)

NS_IMETHODIMP
xpctestChild::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(NS_GET_IID(nsIXPCTestChild)) ||
        iid.Equals(NS_GET_IID(nsIXPCTestParent)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
        *result = static_cast<nsIXPCTestChild*>(this);
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }
}

xpctestChild::xpctestChild()
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpctestChild::Method1(PRInt16 i)
{
    printf("Method1 called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::Method1a(nsIXPCTestParent *foo)
{
    printf("Method1a called on child\n");
    return NS_OK;
}

NS_IMETHODIMP xpctestChild::Method2(PRInt16 i, PRInt16 j)
{
    printf("Method2 called on child\n");
    return NS_OK;
}
#endif



NS_IMETHODIMP
xpctest::ConstructChild(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpctestChild* obj = new xpctestChild();
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






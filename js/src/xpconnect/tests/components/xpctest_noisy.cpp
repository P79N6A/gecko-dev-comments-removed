










































#include "xpctest_private.h"

class xpctestNoisy : public nsIXPCTestNoisy
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTNOISY

    xpctestNoisy();
    virtual ~xpctestNoisy();
private:
    static int sID;
    static int sCount;
    int        mID;
};

int xpctestNoisy::sID = 0;
int xpctestNoisy::sCount = 0;


NS_IMETHODIMP_(nsrefcnt) xpctestNoisy::AddRef(void)
{
  NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "xpctestNoisy", sizeof(*this));
  printf("Noisy %d - incremented refcount to %d\n", mID, mRefCnt.get());
  return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt) xpctestNoisy::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  --mRefCnt;
  printf("Noisy %d - decremented refcount to %d\n", mID, mRefCnt.get());
  NS_LOG_RELEASE(this, mRefCnt, "xpctestNoisy");
  if (mRefCnt == 0) {
    NS_DELETEXPCOM(this);
    return 0;
  }
  return mRefCnt;
}

NS_IMETHODIMP
xpctestNoisy::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(NS_GET_IID(nsIXPCTestNoisy)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
        *result = static_cast<nsIXPCTestNoisy*>(this);
        printf("Noisy %d - QueryInterface called and succeeding\n", mID);
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *result = nsnull;
        printf("Noisy %d - QueryInterface for interface I don't do\n", mID);
        return NS_NOINTERFACE;
    }
}

xpctestNoisy::xpctestNoisy()
    : mID(++sID)
{
    sCount++;
    printf("Noisy %d - Created, %d total\n", mID, sCount);
    NS_ADDREF_THIS();
}

xpctestNoisy::~xpctestNoisy()
{
    sCount--;
    printf("Noisy %d - Destroyed, %d total\n", mID, sCount);
}

NS_IMETHODIMP xpctestNoisy::Squawk()
{
    printf("Noisy %d - Squawk called\n", mID);
    return NS_OK;
}




NS_IMETHODIMP
xpctest::ConstructNoisy(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpctestNoisy* obj = new xpctestNoisy();

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






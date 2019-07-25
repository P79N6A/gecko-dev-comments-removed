






































#include "xptcprivate.h"
#include "xptiprivate.h"

NS_IMETHODIMP
nsXPTCStubBase::QueryInterface(REFNSIID aIID,
                               void **aInstancePtr)
{
    if (aIID.Equals(mEntry->IID())) {
        NS_ADDREF_THIS();
        *aInstancePtr = static_cast<nsISupports*>(this);
        return NS_OK;
    }

    return mOuter->QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP_(nsrefcnt)
nsXPTCStubBase::AddRef()
{
    return mOuter->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
nsXPTCStubBase::Release()
{
    return mOuter->Release();
}

EXPORT_XPCOM_API(nsresult)
NS_GetXPTCallStub(REFNSIID aIID, nsIXPTCProxy* aOuter,
                  nsISomeInterface* *aResult)
{
    NS_ENSURE_ARG(aOuter && aResult);

    xptiInterfaceInfoManager *iim =
        xptiInterfaceInfoManager::GetSingleton();
    NS_ENSURE_TRUE(iim, NS_ERROR_NOT_INITIALIZED);

    xptiInterfaceEntry *iie = iim->GetInterfaceEntryForIID(&aIID);
    if (!iie || !iie->EnsureResolved())
        return NS_ERROR_FAILURE;

    nsXPTCStubBase* newbase = new nsXPTCStubBase(aOuter, iie);
    if (!newbase)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = newbase;
    return NS_OK;
}

EXPORT_XPCOM_API(void)
NS_DestroyXPTCallStub(nsISomeInterface* aStub)
{
    nsXPTCStubBase* stub = static_cast<nsXPTCStubBase*>(aStub);
    delete(stub);
}








#include "xptcprivate.h"
#include "xptiprivate.h"
#include "mozilla/XPTInterfaceInfoManager.h"
#include "nsPrintfCString.h"

using namespace mozilla;

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

NS_IMETHODIMP_(MozExternalRefCountType)
nsXPTCStubBase::AddRef()
{
    return mOuter->AddRef();
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsXPTCStubBase::Release()
{
    return mOuter->Release();
}

EXPORT_XPCOM_API(nsresult)
NS_GetXPTCallStub(REFNSIID aIID, nsIXPTCProxy* aOuter,
                  nsISomeInterface* *aResult)
{
    if (NS_WARN_IF(!aOuter) || NS_WARN_IF(!aResult))
        return NS_ERROR_INVALID_ARG;

    XPTInterfaceInfoManager *iim =
        XPTInterfaceInfoManager::GetSingleton();
    if (NS_WARN_IF(!iim))
        return NS_ERROR_NOT_INITIALIZED;

    xptiInterfaceEntry *iie = iim->GetInterfaceEntryForIID(&aIID);
    if (!iie || !iie->EnsureResolved() || iie->GetBuiltinClassFlag())
        return NS_ERROR_FAILURE;

    if (iie->GetHasNotXPCOMFlag()) {
#ifdef DEBUG
        nsPrintfCString msg("XPTCall will not implement interface %s because of [notxpcom] members.", iie->GetTheName());
        NS_WARNING(msg.get());
#endif
        return NS_ERROR_FAILURE;
    }

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

EXPORT_XPCOM_API(size_t)
NS_SizeOfIncludingThisXPTCallStub(const nsISomeInterface* aStub,
                                  mozilla::MallocSizeOf aMallocSizeOf)
{
    
    
    return aMallocSizeOf(aStub);
}

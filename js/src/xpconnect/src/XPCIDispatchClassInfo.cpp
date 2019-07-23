




































#include "xpcprivate.h"
#include "nsCRT.h"

NS_IMPL_ISUPPORTS1(XPCIDispatchClassInfo, nsIClassInfo)

XPCIDispatchClassInfo* XPCIDispatchClassInfo::sInstance = nsnull;

XPCIDispatchClassInfo* XPCIDispatchClassInfo::GetSingleton()
{
    if(!sInstance)
    {
        sInstance = new XPCIDispatchClassInfo;
        NS_IF_ADDREF(sInstance);
    }
    NS_IF_ADDREF(sInstance);
    return sInstance;
}

void XPCIDispatchClassInfo::FreeSingleton()
{
    NS_IF_RELEASE(sInstance);
    sInstance = nsnull;
}



NS_IMETHODIMP 
XPCIDispatchClassInfo::GetInterfaces(PRUint32 *count, nsIID * **array)
{
    *count = 0;
    *array = NS_STATIC_CAST(nsIID**, nsMemory::Alloc(sizeof(nsIID*)));
    NS_ENSURE_TRUE(*array, NS_ERROR_OUT_OF_MEMORY);

    **array = NS_STATIC_CAST(nsIID *, nsMemory::Clone(&NSID_IDISPATCH,
                                                      sizeof(NSID_IDISPATCH)));
    if(!**array)
    {
        nsMemory::Free(*array);
        *array = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    *count = 1;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetHelperForLanguage(PRUint32 language, 
                                            nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nsCRT::strdup("IDispatch");
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetImplementationLanguage(
    PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::DOM_OBJECT;
    return NS_OK;
}


NS_IMETHODIMP 
XPCIDispatchClassInfo::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}










































#include "xpcprivate.h"

NS_INTERFACE_MAP_BEGIN(BackstagePass)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
#ifndef XPCONNECT_STANDALONE
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
#endif
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCScriptable)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(BackstagePass)
NS_IMPL_THREADSAFE_RELEASE(BackstagePass)


#define XPC_MAP_CLASSNAME           BackstagePass
#define XPC_MAP_QUOTED_CLASSNAME   "BackstagePass"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY   | \
                            nsIXPCScriptable::DONT_ENUM_STATIC_PROPS       | \
                            nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE    | \
                            nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES
#include "xpc_map_end.h" 


NS_IMETHODIMP
BackstagePass::NewResolve(nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx, JSObject * obj,
                          jsval id, PRUint32 flags, 
                          JSObject * *objp, PRBool *_retval)
{
    JSBool resolved;

    *_retval = JS_ResolveStandardClass(cx, obj, id, &resolved);
    if(*_retval && resolved)
        *objp = obj;
    return NS_OK;
}




NS_IMETHODIMP 
BackstagePass::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    nsresult rv = NS_OK;
    PRUint32 count = 1;
#ifndef XPCONNECT_STANDALONE
    ++count;
#endif
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if(!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id) \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),   \
                                                    sizeof(nsIID)));  \
    if (!clone)                                                       \
        goto oom;                                                     \
    array[index++] = clone;

    PUSH_IID(nsIXPCScriptable)
#ifndef XPCONNECT_STANDALONE
    PUSH_IID(nsIScriptObjectPrincipal)
#endif
#undef PUSH_IID

    return NS_OK;
oom:
    while (index)
        nsMemory::Free(array[--index]);
    nsMemory::Free(array);
    *aArray = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP 
BackstagePass::GetHelperForLanguage(PRUint32 language, 
                                      nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP 
BackstagePass::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP 
BackstagePass::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "BackstagePass";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP 
BackstagePass::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP 
BackstagePass::GetImplementationLanguage(
    PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP 
BackstagePass::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP 
BackstagePass::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}









nsJSRuntimeServiceImpl::nsJSRuntimeServiceImpl() :
    mRuntime(0)
{
}

nsJSRuntimeServiceImpl::~nsJSRuntimeServiceImpl() {
    if(mRuntime)
    {
        JS_DestroyRuntime(mRuntime);
        JS_ShutDown();
#ifdef DEBUG_shaver_off
        fprintf(stderr, "nJRSI: destroyed runtime %p\n", (void *)mRuntime);
#endif
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsJSRuntimeServiceImpl,
                              nsIJSRuntimeService,
                              nsISupportsWeakReference)

nsJSRuntimeServiceImpl*
nsJSRuntimeServiceImpl::gJSRuntimeService = nsnull;

nsJSRuntimeServiceImpl*
nsJSRuntimeServiceImpl::GetSingleton()
{
    if(!gJSRuntimeService)
    {
        gJSRuntimeService = new nsJSRuntimeServiceImpl();
        
        NS_IF_ADDREF(gJSRuntimeService);

    }
    NS_IF_ADDREF(gJSRuntimeService);

    return gJSRuntimeService;
}

void
nsJSRuntimeServiceImpl::FreeSingleton()
{
    NS_IF_RELEASE(gJSRuntimeService);
}

const uint32 gGCSize = 32L * 1024L * 1024L; 


NS_IMETHODIMP
nsJSRuntimeServiceImpl::GetRuntime(JSRuntime **runtime)
{
    if(!runtime)
        return NS_ERROR_NULL_POINTER;

    if(!mRuntime)
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        XPCPerThreadData::GetData();
        
        mRuntime = JS_NewRuntime(gGCSize);
        if(!mRuntime)
            return NS_ERROR_OUT_OF_MEMORY;

        
        
        
        
        
        
        JS_SetGCParameter(mRuntime, JSGC_MAX_BYTES, 0xffffffff);
    }
    *runtime = mRuntime;
    return NS_OK;
}


NS_IMETHODIMP
nsJSRuntimeServiceImpl::GetBackstagePass(nsIXPCScriptable **bsp)
{
    if(!mBackstagePass) {
#ifndef XPCONNECT_STANDALONE
        nsCOMPtr<nsIPrincipal> sysprin;
        nsCOMPtr<nsIScriptSecurityManager> secman = 
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if(!secman)
            return NS_ERROR_NOT_AVAILABLE;
        if(NS_FAILED(secman->GetSystemPrincipal(getter_AddRefs(sysprin))))
            return NS_ERROR_NOT_AVAILABLE;
        
        mBackstagePass = new BackstagePass(sysprin);
#else
        mBackstagePass = new BackstagePass();
#endif
        if(!mBackstagePass)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*bsp = mBackstagePass);
    return NS_OK;
}







#include "xpcprivate.h"

#include "mozilla/dom/workers/Workers.h"
using mozilla::dom::workers::ResolveWorkerClasses;

NS_INTERFACE_MAP_BEGIN(BackstagePass)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCScriptable)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(BackstagePass)
NS_IMPL_THREADSAFE_RELEASE(BackstagePass)


#define XPC_MAP_CLASSNAME           BackstagePass
#define XPC_MAP_QUOTED_CLASSNAME   "BackstagePass"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   |  \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   |  \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY   |  \
                            nsIXPCScriptable::DONT_ENUM_STATIC_PROPS       |  \
                            nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE    |  \
                            nsIXPCScriptable::IS_GLOBAL_OBJECT             |  \
                            nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES
#include "xpc_map_end.h" 


NS_IMETHODIMP
BackstagePass::NewResolve(nsIXPConnectWrappedNative *wrapper,
                          JSContext * cx, JSObject * obj_,
                          jsid id_, uint32_t flags,
                          JSObject * *objp_, bool *_retval)
{
    JS::RootedObject obj(cx, obj_);
    JS::RootedId id(cx, id_);

    JSBool resolved;

    *_retval = !!JS_ResolveStandardClass(cx, obj, id, &resolved);
    if (!*_retval) {
        *objp_ = nullptr;
        return NS_OK;
    }

    if (resolved) {
        *objp_ = obj;
        return NS_OK;
    }

    JS::RootedObject objp(cx, *objp_);
    *_retval = !!ResolveWorkerClasses(cx, obj, id, flags, &objp);
    *objp_ = objp;
    return NS_OK;
}




NS_IMETHODIMP
BackstagePass::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
{
    const uint32_t count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    uint32_t index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCScriptable)
    PUSH_IID(nsIScriptObjectPrincipal)
#undef PUSH_IID

    return NS_OK;
oom:
    while (index)
        nsMemory::Free(array[--index]);
    nsMemory::Free(array);
    *aArray = nullptr;
    return NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
BackstagePass::GetHelperForLanguage(uint32_t language,
                                    nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
BackstagePass::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
BackstagePass::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
BackstagePass::GetFlags(uint32_t *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
BackstagePass::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

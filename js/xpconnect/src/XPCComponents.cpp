








#include "mozilla/unused.h"

#include "xpcprivate.h"
#include "XPCQuickStubs.h"
#include "nsReadableUtils.h"
#include "xpcIJSModuleLoader.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMWindow.h"
#include "XPCJSWeakReference.h"
#include "XPCWrapper.h"
#include "jsproxy.h"
#include "WrapperFactory.h"
#include "XrayWrapper.h"
#include "nsNullPrincipal.h"
#include "nsJSUtils.h"
#include "mozJSComponentLoader.h"
#include "nsContentUtils.h"
#include "jsgc.h"
#include "jsfriendapi.h"
#include "AccessCheck.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Preferences.h"
#include "nsPrincipal.h"
#include "mozilla/Attributes.h"
#include "nsIScriptContext.h"

using namespace mozilla;
using namespace js;
using namespace xpc;

using mozilla::dom::DestroyProtoOrIfaceCache;




static nsresult ThrowAndFail(unsigned errNum, JSContext* cx, bool* retval)
{
    XPCThrower::Throw(errNum, cx);
    *retval = false;
    return NS_OK;
}

static JSBool
JSValIsInterfaceOfType(JSContext *cx, jsval v, REFNSIID iid)
{
    nsCOMPtr<nsIXPConnect> xpc;
    nsCOMPtr<nsIXPConnectWrappedNative> wn;
    nsCOMPtr<nsISupports> sup;
    nsISupports* iface;
    if (!JSVAL_IS_PRIMITIVE(v) &&
        nsnull != (xpc = nsXPConnect::GetXPConnect()) &&
        NS_SUCCEEDED(xpc->GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT(v),
                                                     getter_AddRefs(wn))) && wn &&
        NS_SUCCEEDED(wn->Native()->QueryInterface(iid, (void**)&iface)) && iface) {
        NS_RELEASE(iface);
        return true;
    }
    return false;
}

char* xpc_CloneAllAccess()
{
    static const char allAccess[] = "AllAccess";
    return (char*)nsMemory::Clone(allAccess, sizeof(allAccess));
}

char * xpc_CheckAccessList(const PRUnichar* wideName, const char* list[])
{
    nsCAutoString asciiName;
    CopyUTF16toUTF8(nsDependentString(wideName), asciiName);

    for (const char** p = list; *p; p++)
        if (!strcmp(*p, asciiName.get()))
            return xpc_CloneAllAccess();

    return nsnull;
}







class nsXPCComponents_Interfaces :
            public nsIXPCComponents_Interfaces,
            public nsIXPCScriptable,
            public nsIClassInfo,
            public nsISecurityCheckedComponent
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_INTERFACES
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

public:
    nsXPCComponents_Interfaces();
    virtual ~nsXPCComponents_Interfaces();

private:
    nsCOMPtr<nsIInterfaceInfoManager> mManager;
};



NS_IMETHODIMP
nsXPCComponents_Interfaces::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 3;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_Interfaces)
    PUSH_IID(nsIXPCScriptable)
    PUSH_IID(nsISecurityCheckedComponent)
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
nsXPCComponents_Interfaces::GetHelperForLanguage(PRUint32 language,
                                                 nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_Interfaces";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Interfaces::nsXPCComponents_Interfaces() :
    mManager(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID))
{
}

nsXPCComponents_Interfaces::~nsXPCComponents_Interfaces()
{
    
}


NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Interfaces)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Interfaces)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Interfaces)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Interfaces)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Interfaces)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Interfaces
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Interfaces"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Interfaces::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                         JSContext * cx, JSObject * obj,
                                         PRUint32 enum_op, jsval * statep,
                                         jsid * idp, bool *_retval)
{
    nsIEnumerator* e;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            if (!mManager ||
                NS_FAILED(mManager->EnumerateInterfaces(&e)) || !e ||
                NS_FAILED(e->First()))

            {
                *statep = JSVAL_NULL;
                return NS_ERROR_UNEXPECTED;
            }

            *statep = PRIVATE_TO_JSVAL(e);
            if (idp)
                *idp = INT_TO_JSID(0); 
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            nsCOMPtr<nsISupports> isup;

            e = (nsIEnumerator*) JSVAL_TO_PRIVATE(*statep);

            while (1) {
                if (NS_ENUMERATOR_FALSE == e->IsDone() &&
                    NS_SUCCEEDED(e->CurrentItem(getter_AddRefs(isup))) && isup) {
                    e->Next();
                    nsCOMPtr<nsIInterfaceInfo> iface(do_QueryInterface(isup));
                    if (iface) {
                        JSString* idstr;
                        const char* name;
                        bool scriptable;

                        if (NS_SUCCEEDED(iface->IsScriptable(&scriptable)) &&
                            !scriptable) {
                            continue;
                        }

                        if (NS_SUCCEEDED(iface->GetNameShared(&name)) && name &&
                            nsnull != (idstr = JS_NewStringCopyZ(cx, name)) &&
                            JS_ValueToId(cx, STRING_TO_JSVAL(idstr), idp)) {
                            return NS_OK;
                        }
                    }
                }
                
                break;
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsIEnumerator*) JSVAL_TO_PRIVATE(*statep);
            NS_IF_RELEASE(e);
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                       JSContext * cx, JSObject * obj,
                                       jsid id, PRUint32 flags,
                                       JSObject * *objp, bool *_retval)
{
    JSAutoByteString name;
    if (mManager &&
        JSID_IS_STRING(id) &&
        name.encode(cx, JSID_TO_STRING(id)) &&
        name.ptr()[0] != '{') { 
        nsCOMPtr<nsIInterfaceInfo> info;
        mManager->GetInfoForName(name.ptr(), getter_AddRefs(info));
        if (!info)
            return NS_OK;

        nsCOMPtr<nsIJSIID> nsid =
            dont_AddRef(static_cast<nsIJSIID*>(nsJSIID::NewID(info)));

        if (nsid) {
            nsCOMPtr<nsIXPConnect> xpc;
            wrapper->GetXPConnect(getter_AddRefs(xpc));
            if (xpc) {
                nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
                if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                                 static_cast<nsIJSIID*>(nsid),
                                                 NS_GET_IID(nsIJSIID),
                                                 getter_AddRefs(holder)))) {
                    JSObject* idobj;
                    if (holder && NS_SUCCEEDED(holder->GetJSObject(&idobj))) {
                        *objp = obj;
                        *_retval = JS_DefinePropertyById(cx, obj, id,
                                                         OBJECT_TO_JSVAL(idobj),
                                                         nsnull, nsnull,
                                                         JSPROP_ENUMERATE |
                                                         JSPROP_READONLY |
                                                         JSPROP_PERMANENT);
                    }
                }
            }
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::CanCreateWrapper(const nsIID * iid, char **_retval)
{
    
    *_retval = xpc_CloneAllAccess();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}





class nsXPCComponents_InterfacesByID :
            public nsIXPCComponents_InterfacesByID,
            public nsIXPCScriptable,
            public nsIClassInfo,
            public nsISecurityCheckedComponent
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_INTERFACESBYID
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

public:
    nsXPCComponents_InterfacesByID();
    virtual ~nsXPCComponents_InterfacesByID();

private:
    nsCOMPtr<nsIInterfaceInfoManager> mManager;
};




NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 3;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_InterfacesByID)
    PUSH_IID(nsIXPCScriptable)
    PUSH_IID(nsISecurityCheckedComponent)
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
nsXPCComponents_InterfacesByID::GetHelperForLanguage(PRUint32 language,
                                                     nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_InterfacesByID";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_InterfacesByID::nsXPCComponents_InterfacesByID() :
    mManager(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID))
{
}

nsXPCComponents_InterfacesByID::~nsXPCComponents_InterfacesByID()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_InterfacesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_InterfacesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_InterfacesByID)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_InterfacesByID)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_InterfacesByID)


#define XPC_MAP_CLASSNAME           nsXPCComponents_InterfacesByID
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_InterfacesByID"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                             JSContext * cx, JSObject * obj,
                                             PRUint32 enum_op, jsval * statep,
                                             jsid * idp, bool *_retval)
{
    nsIEnumerator* e;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            if (!mManager ||
                NS_FAILED(mManager->EnumerateInterfaces(&e)) || !e ||
                NS_FAILED(e->First()))

            {
                *statep = JSVAL_NULL;
                return NS_ERROR_UNEXPECTED;
            }

            *statep = PRIVATE_TO_JSVAL(e);
            if (idp)
                *idp = INT_TO_JSID(0); 
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            nsCOMPtr<nsISupports> isup;

            e = (nsIEnumerator*) JSVAL_TO_PRIVATE(*statep);

            while (1) {
                if (NS_ENUMERATOR_FALSE == e->IsDone() &&
                    NS_SUCCEEDED(e->CurrentItem(getter_AddRefs(isup))) && isup) {
                    e->Next();
                    nsCOMPtr<nsIInterfaceInfo> iface(do_QueryInterface(isup));
                    if (iface) {
                        nsIID const *iid;
                        char idstr[NSID_LENGTH];
                        JSString* jsstr;
                        bool scriptable;

                        if (NS_SUCCEEDED(iface->IsScriptable(&scriptable)) &&
                            !scriptable) {
                            continue;
                        }

                        if (NS_SUCCEEDED(iface->GetIIDShared(&iid))) {
                            iid->ToProvidedString(idstr);
                            jsstr = JS_NewStringCopyZ(cx, idstr);
                            if (jsstr &&
                                JS_ValueToId(cx, STRING_TO_JSVAL(jsstr), idp)) {
                                return NS_OK;
                            }
                        }
                    }
                }
                
                break;
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsIEnumerator*) JSVAL_TO_PRIVATE(*statep);
            NS_IF_RELEASE(e);
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                           JSContext * cx, JSObject * obj,
                                           jsid id, PRUint32 flags,
                                           JSObject * *objp, bool *_retval)
{
    const jschar* name = nsnull;

    if (mManager &&
        JSID_IS_STRING(id) &&
        38 == JS_GetStringLength(JSID_TO_STRING(id)) &&
        nsnull != (name = JS_GetInternedStringChars(JSID_TO_STRING(id)))) {
        nsID iid;
        if (!iid.Parse(NS_ConvertUTF16toUTF8(name).get()))
            return NS_OK;

        nsCOMPtr<nsIInterfaceInfo> info;
        mManager->GetInfoForIID(&iid, getter_AddRefs(info));
        if (!info)
            return NS_OK;

        nsCOMPtr<nsIJSIID> nsid =
            dont_AddRef(static_cast<nsIJSIID*>(nsJSIID::NewID(info)));

        if (!nsid)
            return NS_ERROR_OUT_OF_MEMORY;

        nsCOMPtr<nsIXPConnect> xpc;
        wrapper->GetXPConnect(getter_AddRefs(xpc));
        if (xpc) {
            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                             static_cast<nsIJSIID*>(nsid),
                                             NS_GET_IID(nsIJSIID),
                                             getter_AddRefs(holder)))) {
                JSObject* idobj;
                if (holder && NS_SUCCEEDED(holder->GetJSObject(&idobj))) {
                    *objp = obj;
                    *_retval =
                        JS_DefinePropertyById(cx, obj, id,
                                              OBJECT_TO_JSVAL(idobj),
                                              nsnull, nsnull,
                                              JSPROP_ENUMERATE |
                                              JSPROP_READONLY |
                                              JSPROP_PERMANENT);
                }
            }
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::CanCreateWrapper(const nsIID * iid, char **_retval)
{
    
    *_retval = xpc_CloneAllAccess();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}







class nsXPCComponents_Classes :
  public nsIXPCComponents_Classes,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_CLASSES
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_Classes();
    virtual ~nsXPCComponents_Classes();
};




NS_IMETHODIMP
nsXPCComponents_Classes::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_Classes)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Classes::GetHelperForLanguage(PRUint32 language,
                                              nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_Classes";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Classes::nsXPCComponents_Classes()
{
}

nsXPCComponents_Classes::~nsXPCComponents_Classes()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Classes)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Classes)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Classes)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Classes)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Classes)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Classes
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Classes"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Classes::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                      JSContext * cx, JSObject * obj,
                                      PRUint32 enum_op, jsval * statep,
                                      jsid * idp, bool *_retval)
{
    nsISimpleEnumerator* e;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            nsCOMPtr<nsIComponentRegistrar> compMgr;
            if (NS_FAILED(NS_GetComponentRegistrar(getter_AddRefs(compMgr))) || !compMgr ||
                NS_FAILED(compMgr->EnumerateContractIDs(&e)) || !e ) {
                *statep = JSVAL_NULL;
                return NS_ERROR_UNEXPECTED;
            }

            *statep = PRIVATE_TO_JSVAL(e);
            if (idp)
                *idp = INT_TO_JSID(0); 
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            nsCOMPtr<nsISupports> isup;
            bool hasMore;
            e = (nsISimpleEnumerator*) JSVAL_TO_PRIVATE(*statep);

            if (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore &&
                NS_SUCCEEDED(e->GetNext(getter_AddRefs(isup))) && isup) {
                nsCOMPtr<nsISupportsCString> holder(do_QueryInterface(isup));
                if (holder) {
                    nsCAutoString name;
                    if (NS_SUCCEEDED(holder->GetData(name))) {
                        JSString* idstr = JS_NewStringCopyN(cx, name.get(), name.Length());
                        if (idstr &&
                            JS_ValueToId(cx, STRING_TO_JSVAL(idstr), idp)) {
                            return NS_OK;
                        }
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsISimpleEnumerator*) JSVAL_TO_PRIVATE(*statep);
            NS_IF_RELEASE(e);
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_Classes::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    jsid id, PRUint32 flags,
                                    JSObject * *objp, bool *_retval)

{
    JSAutoByteString name;

    if (JSID_IS_STRING(id) &&
        name.encode(cx, JSID_TO_STRING(id)) &&
        name.ptr()[0] != '{') { 
        nsCOMPtr<nsIJSCID> nsid =
            dont_AddRef(static_cast<nsIJSCID*>(nsJSCID::NewID(name.ptr())));
        if (nsid) {
            nsCOMPtr<nsIXPConnect> xpc;
            wrapper->GetXPConnect(getter_AddRefs(xpc));
            if (xpc) {
                nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
                if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                                 static_cast<nsIJSCID*>(nsid),
                                                 NS_GET_IID(nsIJSCID),
                                                 getter_AddRefs(holder)))) {
                    JSObject* idobj;
                    if (holder && NS_SUCCEEDED(holder->GetJSObject(&idobj))) {
                        *objp = obj;
                        *_retval = JS_DefinePropertyById(cx, obj, id,
                                                         OBJECT_TO_JSVAL(idobj),
                                                         nsnull, nsnull,
                                                         JSPROP_ENUMERATE |
                                                         JSPROP_READONLY |
                                                         JSPROP_PERMANENT);
                    }
                }
            }
        }
    }
    return NS_OK;
}





class nsXPCComponents_ClassesByID :
  public nsIXPCComponents_ClassesByID,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_CLASSESBYID
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_ClassesByID();
    virtual ~nsXPCComponents_ClassesByID();
};




NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_ClassesByID)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_ClassesByID::GetHelperForLanguage(PRUint32 language,
                                                  nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_ClassesByID";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_ClassesByID::nsXPCComponents_ClassesByID()
{
}

nsXPCComponents_ClassesByID::~nsXPCComponents_ClassesByID()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_ClassesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_ClassesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_ClassesByID)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_ClassesByID)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_ClassesByID)


#define XPC_MAP_CLASSNAME           nsXPCComponents_ClassesByID
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_ClassesByID"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 


NS_IMETHODIMP
nsXPCComponents_ClassesByID::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                          JSContext * cx, JSObject * obj,
                                          PRUint32 enum_op, jsval * statep,
                                          jsid * idp, bool *_retval)
{
    nsISimpleEnumerator* e;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            nsCOMPtr<nsIComponentRegistrar> compMgr;
            if (NS_FAILED(NS_GetComponentRegistrar(getter_AddRefs(compMgr))) || !compMgr ||
                NS_FAILED(compMgr->EnumerateCIDs(&e)) || !e ) {
                *statep = JSVAL_NULL;
                return NS_ERROR_UNEXPECTED;
            }

            *statep = PRIVATE_TO_JSVAL(e);
            if (idp)
                *idp = INT_TO_JSID(0); 
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            nsCOMPtr<nsISupports> isup;
            bool hasMore;
            e = (nsISimpleEnumerator*) JSVAL_TO_PRIVATE(*statep);

            if (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore &&
                NS_SUCCEEDED(e->GetNext(getter_AddRefs(isup))) && isup) {
                nsCOMPtr<nsISupportsID> holder(do_QueryInterface(isup));
                if (holder) {
                    char* name;
                    if (NS_SUCCEEDED(holder->ToString(&name)) && name) {
                        JSString* idstr = JS_NewStringCopyZ(cx, name);
                        nsMemory::Free(name);
                        if (idstr &&
                            JS_ValueToId(cx, STRING_TO_JSVAL(idstr), idp)) {
                            return NS_OK;
                        }
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsISimpleEnumerator*) JSVAL_TO_PRIVATE(*statep);
            NS_IF_RELEASE(e);
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}

static bool
IsRegisteredCLSID(const char* str)
{
    bool registered;
    nsID id;

    if (!id.Parse(str))
        return false;

    nsCOMPtr<nsIComponentRegistrar> compMgr;
    if (NS_FAILED(NS_GetComponentRegistrar(getter_AddRefs(compMgr))) || !compMgr ||
        NS_FAILED(compMgr->IsCIDRegistered(id, &registered)))
        return false;

    return registered;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                        JSContext * cx, JSObject * obj,
                                        jsid id, PRUint32 flags,
                                        JSObject * *objp, bool *_retval)
{
    JSAutoByteString name;

    if (JSID_IS_STRING(id) &&
        name.encode(cx, JSID_TO_STRING(id)) &&
        name.ptr()[0] == '{' &&
        IsRegisteredCLSID(name.ptr())) { 
        nsCOMPtr<nsIJSCID> nsid =
            dont_AddRef(static_cast<nsIJSCID*>(nsJSCID::NewID(name.ptr())));
        if (nsid) {
            nsCOMPtr<nsIXPConnect> xpc;
            wrapper->GetXPConnect(getter_AddRefs(xpc));
            if (xpc) {
                nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
                if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                                 static_cast<nsIJSCID*>(nsid),
                                                 NS_GET_IID(nsIJSCID),
                                                 getter_AddRefs(holder)))) {
                    JSObject* idobj;
                    if (holder && NS_SUCCEEDED(holder->GetJSObject(&idobj))) {
                        *objp = obj;
                        *_retval = JS_DefinePropertyById(cx, obj, id,
                                                         OBJECT_TO_JSVAL(idobj),
                                                         nsnull, nsnull,
                                                         JSPROP_ENUMERATE |
                                                         JSPROP_READONLY |
                                                         JSPROP_PERMANENT);
                    }
                }
            }
        }
    }
    return NS_OK;
}







class nsXPCComponents_Results :
  public nsIXPCComponents_Results,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_RESULTS
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_Results();
    virtual ~nsXPCComponents_Results();
};




NS_IMETHODIMP
nsXPCComponents_Results::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_Results)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Results::GetHelperForLanguage(PRUint32 language,
                                              nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_Results";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Results::nsXPCComponents_Results()
{
}

nsXPCComponents_Results::~nsXPCComponents_Results()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Results)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Results)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Results)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Results)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Results)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Results
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Results"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_NEWENUMERATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |\
                                    nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 


NS_IMETHODIMP
nsXPCComponents_Results::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                      JSContext * cx, JSObject * obj,
                                      PRUint32 enum_op, jsval * statep,
                                      jsid * idp, bool *_retval)
{
    void** iter;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            if (idp)
                *idp = INT_TO_JSID(nsXPCException::GetNSResultCount());

            void** space = (void**) new char[sizeof(void*)];
            *space = nsnull;
            *statep = PRIVATE_TO_JSVAL(space);
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            const char* name;
            iter = (void**) JSVAL_TO_PRIVATE(*statep);
            if (nsXPCException::IterateNSResults(nsnull, &name, nsnull, iter)) {
                JSString* idstr = JS_NewStringCopyZ(cx, name);
                if (idstr && JS_ValueToId(cx, STRING_TO_JSVAL(idstr), idp))
                    return NS_OK;
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            iter = (void**) JSVAL_TO_PRIVATE(*statep);
            delete [] (char*) iter;
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}



NS_IMETHODIMP
nsXPCComponents_Results::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    jsid id, PRUint32 flags,
                                    JSObject * *objp, bool *_retval)
{
    JSAutoByteString name;

    if (JSID_IS_STRING(id) && name.encode(cx, JSID_TO_STRING(id))) {
        const char* rv_name;
        void* iter = nsnull;
        nsresult rv;
        while (nsXPCException::IterateNSResults(&rv, &rv_name, nsnull, &iter)) {
            if (!strcmp(name.ptr(), rv_name)) {
                jsval val;

                *objp = obj;
                if (!JS_NewNumberValue(cx, (double)rv, &val) ||
                    !JS_DefinePropertyById(cx, obj, id, val,
                                           nsnull, nsnull,
                                           JSPROP_ENUMERATE |
                                           JSPROP_READONLY |
                                           JSPROP_PERMANENT)) {
                    return NS_ERROR_UNEXPECTED;
                }
            }
        }
    }
    return NS_OK;
}




class nsXPCComponents_ID :
  public nsIXPCComponents_ID,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_ID
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO


public:
    nsXPCComponents_ID();
    virtual ~nsXPCComponents_ID();

private:
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    PRUint32 argc, jsval * argv,
                                    jsval * vp, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_ID::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_ID)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_ID::GetHelperForLanguage(PRUint32 language,
                                         nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_ID";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_ID::nsXPCComponents_ID()
{
}

nsXPCComponents_ID::~nsXPCComponents_ID()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_ID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_ID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_ID)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_ID)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_ID)


#define XPC_MAP_CLASSNAME           nsXPCComponents_ID
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_ID"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_ID::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);

}


NS_IMETHODIMP
nsXPCComponents_ID::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}


nsresult
nsXPCComponents_ID::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    PRUint32 argc, jsval * argv,
                                    jsval * vp, bool *_retval)
{
    

    if (!argc)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    XPCContext* xpcc = ccx.GetXPCContext();

    

    nsIXPCSecurityManager* sm =
            xpcc->GetAppropriateSecurityManager(nsIXPCSecurityManager::HOOK_CREATE_INSTANCE);
    if (sm && NS_FAILED(sm->CanCreateInstance(cx, nsJSID::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    

    JSString* jsstr;
    JSAutoByteString bytes;
    nsID id;

    if (!(jsstr = JS_ValueToString(cx, argv[0])) ||
        !bytes.encode(cx, jsstr) ||
        !id.Parse(bytes.ptr())) {
        return ThrowAndFail(NS_ERROR_XPC_BAD_ID_STRING, cx, _retval);
    }

    

    JSObject* newobj = xpc_NewIDObject(cx, obj, id);

    if (vp)
        *vp = OBJECT_TO_JSVAL(newobj);

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                JSContext * cx, JSObject * obj,
                                const jsval &val, bool *bp, bool *_retval)
{
    if (bp)
        *bp = JSValIsInterfaceOfType(cx, val, NS_GET_IID(nsIJSID));
    return NS_OK;
}




class nsXPCComponents_Exception :
  public nsIXPCComponents_Exception,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_EXCEPTION
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO


public:
    nsXPCComponents_Exception();
    virtual ~nsXPCComponents_Exception();

private:
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    PRUint32 argc, jsval * argv,
                                    jsval * vp, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_Exception::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_Exception)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Exception::GetHelperForLanguage(PRUint32 language,
                                                nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_Exception";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Exception::nsXPCComponents_Exception()
{
}

nsXPCComponents_Exception::~nsXPCComponents_Exception()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Exception)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Exception)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Exception)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Exception)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Exception)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Exception
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Exception"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Exception::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);

}


NS_IMETHODIMP
nsXPCComponents_Exception::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}

struct NS_STACK_CLASS ExceptionArgParser
{
    ExceptionArgParser(JSContext *context,
                       nsXPConnect *xpconnect)
        : eMsg("exception")
        , eResult(NS_ERROR_FAILURE)
        , cx(context)
        , xpc(xpconnect)
    {}

    
    
    const char*             eMsg;
    nsresult                eResult;
    nsCOMPtr<nsIStackFrame> eStack;
    nsCOMPtr<nsISupports>   eData;

    
    bool parse(uint32_t argc, JS::Value *argv) {
        




















        if (argc > 0 && !parseMessage(argv[0]))
            return false;
        if (argc > 1) {
            if (argv[1].isObject())
                return parseOptionsObject(argv[1].toObject());
            if (!parseResult(argv[1]))
                return false;
        }
        if (argc > 2 && !parseStack(argv[2]))
            return false;
        if (argc > 3 && !parseData(argv[3]))
            return false;
        return true;
    }

  protected:

    



    bool parseMessage(JS::Value &v) {
        JSString *str = JS_ValueToString(cx, v);
        if (!str)
           return false;
        eMsg = messageBytes.encode(cx, str);
        return !!eMsg;
    }

    bool parseResult(JS::Value &v) {
        return JS_ValueToECMAInt32(cx, v, (int32_t*) &eResult);
    }

    bool parseStack(JS::Value &v) {
        if (!v.isObject()) {
            
            
            return true;
        }

        return NS_SUCCEEDED(xpc->WrapJS(cx, JSVAL_TO_OBJECT(v),
                                        NS_GET_IID(nsIStackFrame),
                                        getter_AddRefs(eStack)));
    }

    bool parseData(JS::Value &v) {
        if (!v.isObject()) {
            
            
            return true;
        }

        return NS_SUCCEEDED(xpc->WrapJS(cx, &v.toObject(),
                                        NS_GET_IID(nsISupports),
                                        getter_AddRefs(eData)));
    }

    bool parseOptionsObject(JSObject &obj) {
        JS::Value v;

        if (!getOption(obj, "result", &v) ||
            (!v.isUndefined() && !parseResult(v)))
            return false;

        if (!getOption(obj, "stack", &v) ||
            (!v.isUndefined() && !parseStack(v)))
            return false;

        if (!getOption(obj, "data", &v) ||
            (!v.isUndefined() && !parseData(v)))
            return false;

        return true;
    }

    bool getOption(JSObject &obj, const char *name, JS::Value *rv) {
        
        JSBool found;
        if (!JS_HasProperty(cx, &obj, name, &found))
            return false;

        
        if (!found) {
            *rv = JSVAL_VOID;
            return true;
        }

        
        return JS_GetProperty(cx, &obj, name, rv);
    }

    



    
    JSAutoByteString messageBytes;

    
    JSContext *cx;
    nsXPConnect *xpc;
};


nsresult
nsXPCComponents_Exception::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                           JSContext * cx, JSObject * obj,
                                           PRUint32 argc, jsval * argv,
                                           jsval * vp, bool *_retval)
{
    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    nsXPConnect* xpc = ccx.GetXPConnect();
    XPCContext* xpcc = ccx.GetXPCContext();

    

    nsIXPCSecurityManager* sm =
            xpcc->GetAppropriateSecurityManager(nsIXPCSecurityManager::HOOK_CREATE_INSTANCE);
    if (sm && NS_FAILED(sm->CanCreateInstance(cx, nsXPCException::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    
    ExceptionArgParser args(cx, xpc);
    if (!args.parse(argc, argv))
        return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

    nsCOMPtr<nsIException> e;
    nsXPCException::NewException(args.eMsg, args.eResult, args.eStack,
                                 args.eData, getter_AddRefs(e));
    if (!e)
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    JSObject* newObj = nsnull;

    if (NS_FAILED(xpc->WrapNative(cx, obj, e, NS_GET_IID(nsIXPCException),
                                  getter_AddRefs(holder))) || !holder ||
        NS_FAILED(holder->GetJSObject(&newObj)) || !newObj) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    if (vp)
        *vp = OBJECT_TO_JSVAL(newObj);

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                       JSContext * cx, JSObject * obj,
                                       const jsval &val, bool *bp,
                                       bool *_retval)
{
    if (bp)
        *bp = JSValIsInterfaceOfType(cx, val, NS_GET_IID(nsIException));
    return NS_OK;
}









#define NS_XPCCONSTRUCTOR_CID                                                 \
{ 0xb4a95150, 0xe25a, 0x11d3,                                                 \
    { 0x8f, 0x61, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a } }

class nsXPCConstructor :
  public nsIXPCConstructor,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_XPCCONSTRUCTOR_CID)
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCONSTRUCTOR
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCConstructor(); 
    nsXPCConstructor(nsIJSCID* aClassID,
                     nsIJSIID* aInterfaceID,
                     const char* aInitializer);
    virtual ~nsXPCConstructor();

private:
    nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                             JSContext * cx, JSObject * obj,
                             PRUint32 argc, jsval * argv,
                             jsval * vp, bool *_retval);
private:
    nsIJSCID* mClassID;
    nsIJSIID* mInterfaceID;
    char*     mInitializer;
};




NS_IMETHODIMP
nsXPCConstructor::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCConstructor)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCConstructor::GetHelperForLanguage(PRUint32 language,
                                       nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCConstructor::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCConstructor";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCConstructor::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCConstructor::nsXPCConstructor(nsIJSCID* aClassID,
                                   nsIJSIID* aInterfaceID,
                                   const char* aInitializer)
{
    NS_IF_ADDREF(mClassID = aClassID);
    NS_IF_ADDREF(mInterfaceID = aInterfaceID);
    mInitializer = aInitializer ?
        (char*) nsMemory::Clone(aInitializer, strlen(aInitializer)+1) :
        nsnull;
}

nsXPCConstructor::~nsXPCConstructor()
{
    NS_IF_RELEASE(mClassID);
    NS_IF_RELEASE(mInterfaceID);
    if (mInitializer)
        nsMemory::Free(mInitializer);
}


NS_IMETHODIMP
nsXPCConstructor::GetClassID(nsIJSCID * *aClassID)
{
    NS_IF_ADDREF(*aClassID = mClassID);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetInterfaceID(nsIJSIID * *aInterfaceID)
{
    NS_IF_ADDREF(*aInterfaceID = mInterfaceID);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetInitializer(char * *aInitializer)
{
    XPC_STRING_GETTER_BODY(aInitializer, mInitializer);
}

NS_INTERFACE_MAP_BEGIN(nsXPCConstructor)
  NS_INTERFACE_MAP_ENTRY(nsIXPCConstructor)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCConstructor)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCConstructor)
NS_IMPL_THREADSAFE_RELEASE(nsXPCConstructor)


#define XPC_MAP_CLASSNAME           nsXPCConstructor
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCConstructor"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCConstructor::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);

}


NS_IMETHODIMP
nsXPCConstructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}


nsresult
nsXPCConstructor::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                  JSContext * cx, JSObject * obj,
                                  PRUint32 argc, jsval * argv,
                                  jsval * vp, bool *_retval)
{
    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    nsXPConnect* xpc = ccx.GetXPConnect();

    
    

    nsCOMPtr<nsIXPConnectJSObjectHolder> cidHolder;
    nsCOMPtr<nsIXPConnectJSObjectHolder> iidHolder;
    JSObject* cidObj;
    JSObject* iidObj;

    if (NS_FAILED(xpc->WrapNative(cx, obj, mClassID, NS_GET_IID(nsIJSCID),
                                  getter_AddRefs(cidHolder))) || !cidHolder ||
        NS_FAILED(cidHolder->GetJSObject(&cidObj)) || !cidObj ||
        NS_FAILED(xpc->WrapNative(cx, obj, mInterfaceID, NS_GET_IID(nsIJSIID),
                                  getter_AddRefs(iidHolder))) || !iidHolder ||
        NS_FAILED(iidHolder->GetJSObject(&iidObj)) || !iidObj) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    jsval ctorArgs[1] = {OBJECT_TO_JSVAL(iidObj)};
    jsval val;

    if (!JS_CallFunctionName(cx, cidObj, "createInstance", 1, ctorArgs, &val) ||
        JSVAL_IS_PRIMITIVE(val)) {
        
        *_retval = false;
        return NS_OK;
    }

    
    if (vp)
        *vp = val;

    
    if (mInitializer) {
        JSObject* newObj = JSVAL_TO_OBJECT(val);
        jsval fun;
        jsval ignored;

        
        if (!JS_GetProperty(cx, newObj, mInitializer, &fun) ||
            JSVAL_IS_PRIMITIVE(fun)) {
            return ThrowAndFail(NS_ERROR_XPC_BAD_INITIALIZER_NAME, cx, _retval);
        }

        if (!JS_CallFunctionValue(cx, newObj, fun, argc, argv, &ignored)) {
            
            *_retval = false;
            return NS_OK;
        }
    }

    return NS_OK;
}




class nsXPCComponents_Constructor :
  public nsIXPCComponents_Constructor,
  public nsIXPCScriptable,
  public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_CONSTRUCTOR
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_Constructor();
    virtual ~nsXPCComponents_Constructor();

private:
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    PRUint32 argc, jsval * argv,
                                    jsval * vp, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_Constructor::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 2;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents_Constructor)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Constructor::GetHelperForLanguage(PRUint32 language,
                                                  nsISupports **retval)
{
    *retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents_Constructor";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Constructor::nsXPCComponents_Constructor()
{
}

nsXPCComponents_Constructor::~nsXPCComponents_Constructor()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Constructor)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Constructor)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Constructor)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Constructor)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Constructor)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Constructor
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Constructor"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Constructor::Call(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}


NS_IMETHODIMP
nsXPCComponents_Constructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, PRUint32 argc, jsval * argv, jsval * vp, bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}


nsresult
nsXPCComponents_Constructor::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                             JSContext * cx, JSObject * obj,
                                             PRUint32 argc, jsval * argv,
                                             jsval * vp, bool *_retval)
{
    

    if (!argc)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    

    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid())
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    nsXPConnect* xpc = ccx.GetXPConnect();
    XPCContext* xpcc = ccx.GetXPCContext();
    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, obj);
    nsXPCComponents* comp;

    if (!xpc || !xpcc || !scope || !(comp = scope->GetComponents()))
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    

    nsIXPCSecurityManager* sm =
            xpcc->GetAppropriateSecurityManager(nsIXPCSecurityManager::HOOK_CREATE_INSTANCE);
    if (sm && NS_FAILED(sm->CanCreateInstance(cx, nsXPCConstructor::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    
    nsCOMPtr<nsIJSCID> cClassID;
    nsCOMPtr<nsIJSIID> cInterfaceID;
    const char*        cInitializer = nsnull;
    JSAutoByteString  cInitializerBytes;

    if (argc >= 3) {
        
        JSString* str = JS_ValueToString(cx, argv[2]);
        if (!str || !(cInitializer = cInitializerBytes.encode(cx, str)))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);
    }

    if (argc >= 2) {
        
        

        nsCOMPtr<nsIXPCComponents_Interfaces> ifaces;
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        JSObject* ifacesObj = nsnull;

        
        
        

        if (NS_FAILED(comp->GetInterfaces(getter_AddRefs(ifaces))) ||
            NS_FAILED(xpc->WrapNative(cx, obj, ifaces,
                                      NS_GET_IID(nsIXPCComponents_Interfaces),
                                      getter_AddRefs(holder))) || !holder ||
            NS_FAILED(holder->GetJSObject(&ifacesObj)) || !ifacesObj) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }

        JSString* str = JS_ValueToString(cx, argv[1]);
        jsid id;
        if (!str || !JS_ValueToId(cx, STRING_TO_JSVAL(str), &id))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

        jsval val;
        if (!JS_GetPropertyById(cx, ifacesObj, id, &val) || JSVAL_IS_PRIMITIVE(val))
            return ThrowAndFail(NS_ERROR_XPC_BAD_IID, cx, _retval);

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        if (NS_FAILED(xpc->GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT(val),
                                                      getter_AddRefs(wn))) || !wn ||
            !(cInterfaceID = do_QueryWrappedNative(wn))) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }
    } else {
        nsCOMPtr<nsIInterfaceInfo> info;
        xpc->GetInfoForIID(&NS_GET_IID(nsISupports), getter_AddRefs(info));

        if (info) {
            cInterfaceID =
                dont_AddRef(static_cast<nsIJSIID*>(nsJSIID::NewID(info)));
        }
        if (!cInterfaceID)
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
    }

    
    {
        
        

        
        
        

        nsCOMPtr<nsIXPCComponents_Classes> classes;
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        JSObject* classesObj = nsnull;

        if (NS_FAILED(comp->GetClasses(getter_AddRefs(classes))) ||
            NS_FAILED(xpc->WrapNative(cx, obj, classes,
                                      NS_GET_IID(nsIXPCComponents_Classes),
                                      getter_AddRefs(holder))) || !holder ||
            NS_FAILED(holder->GetJSObject(&classesObj)) || !classesObj) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }

        JSString* str = JS_ValueToString(cx, argv[0]);
        jsid id;
        if (!str || !JS_ValueToId(cx, STRING_TO_JSVAL(str), &id))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

        jsval val;
        if (!JS_GetPropertyById(cx, classesObj, id, &val) || JSVAL_IS_PRIMITIVE(val))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CID, cx, _retval);

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        if (NS_FAILED(xpc->GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT(val),
                                                      getter_AddRefs(wn))) || !wn ||
            !(cClassID = do_QueryWrappedNative(wn))) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }
    }

    nsCOMPtr<nsIXPCConstructor> ctor =
        static_cast<nsIXPCConstructor*>
                   (new nsXPCConstructor(cClassID, cInterfaceID, cInitializer));
    if (!ctor)
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder2;
    JSObject* newObj = nsnull;

    if (NS_FAILED(xpc->WrapNative(cx, obj, ctor, NS_GET_IID(nsIXPCConstructor),
                                  getter_AddRefs(holder2))) || !holder2 ||
        NS_FAILED(holder2->GetJSObject(&newObj)) || !newObj) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    if (vp)
        *vp = OBJECT_TO_JSVAL(newObj);

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                         JSContext * cx, JSObject * obj,
                                         const jsval &val, bool *bp,
                                         bool *_retval)
{
    if (bp)
        *bp = JSValIsInterfaceOfType(cx, val, NS_GET_IID(nsIXPCConstructor));
    return NS_OK;
}



class nsXPCComponents_utils_Sandbox : public nsIXPCComponents_utils_Sandbox,
                                      public nsIXPCScriptable
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_UTILS_SANDBOX
    NS_DECL_NSIXPCSCRIPTABLE

public:
    nsXPCComponents_utils_Sandbox();
    virtual ~nsXPCComponents_utils_Sandbox();

private:
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx, JSObject * obj,
                                    PRUint32 argc, jsval * argv,
                                    jsval * vp, bool *_retval);
};

class nsXPCComponents_Utils :
            public nsIXPCComponents_Utils,
            public nsIXPCScriptable,
            public nsISecurityCheckedComponent
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
    NS_DECL_NSIXPCCOMPONENTS_UTILS

public:
    nsXPCComponents_Utils() { }
    virtual ~nsXPCComponents_Utils() { }

private:
    nsCOMPtr<nsIXPCComponents_utils_Sandbox> mSandbox;
};

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Utils)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Utils)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Utils)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_Utils)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_Utils)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Utils
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Utils"
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 

NS_IMETHODIMP
nsXPCComponents_Utils::GetSandbox(nsIXPCComponents_utils_Sandbox **aSandbox)
{
    NS_ENSURE_ARG_POINTER(aSandbox);
    if (!mSandbox && !(mSandbox = new nsXPCComponents_utils_Sandbox())) {
        *aSandbox = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*aSandbox = mSandbox);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::LookupMethod(const JS::Value& object,
                                    const JS::Value& name,
                                    JSContext *cx,
                                    JS::Value *retval)
{
    JSAutoRequest ar(cx);

    
    if (!object.isObject())
        return NS_ERROR_XPC_BAD_CONVERT_JS;
    JSObject *obj = &object.toObject();

    
    if (!JSVAL_IS_STRING(name))
        return NS_ERROR_XPC_BAD_CONVERT_JS;
    JSString *methodName = name.toString();
    jsid methodId = INTERNED_STRING_TO_JSID(cx, JS_InternJSString(cx, methodName));

    
    
    
    if (js::IsCrossCompartmentWrapper(obj)) {
        obj = js::UnwrapOneChecked(cx, obj);
        if (!obj)
            return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    {
        
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, obj))
            return NS_ERROR_FAILURE;

        
        
        JSObject *xray = WrapperFactory::WrapForSameCompartmentXray(cx, obj);
        if (!xray)
            return NS_ERROR_XPC_BAD_CONVERT_JS;

        
        *retval = JSVAL_VOID;
        JSPropertyDescriptor desc;
        if (!JS_GetPropertyDescriptorById(cx, xray, methodId, 0, &desc))
            return NS_ERROR_FAILURE;

        
        
        JSObject *methodObj = desc.value.isObject() ? &desc.value.toObject() : NULL;
        if (!methodObj && (desc.attrs & JSPROP_GETTER))
            methodObj = JS_FUNC_TO_DATA_PTR(JSObject *, desc.getter);

        
        
        if (methodObj && JS_ObjectIsCallable(cx, methodObj))
            methodObj = JS_BindCallable(cx, methodObj, obj);

        
        *retval = methodObj ? ObjectValue(*methodObj) : JSVAL_VOID;
    }

    
    if (!JS_WrapValue(cx, retval))
        return NS_ERROR_FAILURE;;

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ReportError(const JS::Value &error, JSContext *cx)
{
    

    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));

    nsCOMPtr<nsIScriptError> scripterr(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));

    if (!scripterr || !console)
        return NS_OK;

    JSAutoRequest ar(cx);

    const PRUint64 innerWindowID = nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx);

    JSErrorReport *err = JS_ErrorFromException(cx, error);
    if (err) {
        
        nsAutoString fileUni;
        CopyUTF8toUTF16(err->filename, fileUni);

        PRUint32 column = err->uctokenptr - err->uclinebuf;

        nsresult rv = scripterr->InitWithWindowID(
                static_cast<const PRUnichar*>(err->ucmessage), fileUni.get(),
                static_cast<const PRUnichar*>(err->uclinebuf), err->lineno,
                column, err->flags, "XPConnect JavaScript", innerWindowID);
        NS_ENSURE_SUCCESS(rv, NS_OK);

        console->LogMessage(scripterr);
        return NS_OK;
    }

    
    JSString *msgstr = JS_ValueToString(cx, error);
    if (!msgstr) {
        return NS_OK;
    }

    nsCOMPtr<nsIStackFrame> frame;
    nsXPConnect *xpc = nsXPConnect::GetXPConnect();
    if (xpc)
        xpc->GetCurrentJSStack(getter_AddRefs(frame));

    nsXPIDLCString fileName;
    PRInt32 lineNo = 0;
    if (frame) {
        frame->GetFilename(getter_Copies(fileName));
        frame->GetLineNumber(&lineNo);
    }

    const jschar *msgchars = JS_GetStringCharsZ(cx, msgstr);
    if (!msgchars)
        return NS_OK;

    nsresult rv = scripterr->InitWithWindowID(
            reinterpret_cast<const PRUnichar *>(msgchars),
            NS_ConvertUTF8toUTF16(fileName).get(),
            nsnull, lineNo, 0, 0, "XPConnect JavaScript", innerWindowID);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    console->LogMessage(scripterr);
    return NS_OK;
}

#include "nsIScriptSecurityManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
const char kScriptSecurityManagerContractID[] = NS_SCRIPTSECURITYMANAGER_CONTRACTID;

NS_IMPL_THREADSAFE_ISUPPORTS1(PrincipalHolder, nsIScriptObjectPrincipal)

nsIPrincipal *
PrincipalHolder::GetPrincipal()
{
    return mHoldee;
}

static JSBool
SandboxDump(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *str;
    if (!argc)
        return true;

    str = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
    if (!str)
        return false;

    size_t length;
    const jschar *chars = JS_GetStringCharsZAndLength(cx, str, &length);
    if (!chars)
        return false;

    nsDependentString wstr(chars, length);
    char *cstr = ToNewUTF8String(wstr);
    if (!cstr)
        return false;

#if defined(XP_MACOSX)
    
    char *c = cstr, *cEnd = cstr + strlen(cstr);
    while (c < cEnd) {
        if (*c == '\r')
            *c = '\n';
        c++;
    }
#endif

    fputs(cstr, stdout);
    fflush(stdout);
    NS_Free(cstr);
    JS_SET_RVAL(cx, vp, JSVAL_TRUE);
    return true;
}

static JSBool
SandboxDebug(JSContext *cx, unsigned argc, jsval *vp)
{
#ifdef DEBUG
    return SandboxDump(cx, argc, vp);
#else
    return true;
#endif
}

static JSBool
SandboxImport(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *thisobj = JS_THIS_OBJECT(cx, vp);
    if (!thisobj)
        return false;

    jsval *argv = JS_ARGV(cx, vp);
    if (argc < 1 || JSVAL_IS_PRIMITIVE(argv[0])) {
        XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
        return false;
    }

    JSString *funname;
    if (argc > 1) {
        
        funname = JS_ValueToString(cx, argv[1]);
        if (!funname)
            return false;
        argv[1] = STRING_TO_JSVAL(funname);
    } else {
        
        JSObject *funobj = JSVAL_TO_OBJECT(argv[0]);
        if (js::IsProxy(funobj)) {
            funobj = XPCWrapper::UnsafeUnwrapSecurityWrapper(funobj);
        }

        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, funobj)) {
            return false;
        }

        JSFunction *fun = JS_ValueToFunction(cx, OBJECT_TO_JSVAL(funobj));
        if (!fun) {
            XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
            return false;
        }

        
        funname = JS_GetFunctionId(fun);
        if (!funname) {
            XPCThrower::Throw(NS_ERROR_INVALID_ARG, cx);
            return false;
        }
    }

    jsid id;
    if (!JS_ValueToId(cx, STRING_TO_JSVAL(funname), &id))
        return false;

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_SetPropertyById(cx, thisobj, id, &argv[0]);
}

static JSBool
CreateXMLHttpRequest(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *global = JS_GetGlobalForScopeChain(cx);
    MOZ_ASSERT(global);

    nsCOMPtr<nsISupports> inst;
    nsresult rv;
    inst = do_CreateInstance("@mozilla.org/xmlextras/xmlhttprequest;1", &rv);
    if (NS_FAILED(rv))
        return false;

    rv = nsContentUtils::WrapNative(cx, global, inst, vp);
    if (NS_FAILED(rv))
        return false;

    return true;
}

static JSBool
sandbox_enumerate(JSContext *cx, JSHandleObject obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static JSBool
sandbox_resolve(JSContext *cx, JSHandleObject obj, JSHandleId id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static void
sandbox_finalize(JSFreeOp *fop, JSObject *obj)
{
    nsIScriptObjectPrincipal *sop =
        (nsIScriptObjectPrincipal *)xpc_GetJSPrivate(obj);
    NS_IF_RELEASE(sop);
    DestroyProtoOrIfaceCache(obj);
}

static JSBool
sandbox_convert(JSContext *cx, JSHandleObject obj, JSType type, jsval *vp)
{
    if (type == JSTYPE_OBJECT) {
        *vp = OBJECT_TO_JSVAL(obj);
        return true;
    }

    return JS_ConvertStub(cx, obj, type, vp);
}

static JSClass SandboxClass = {
    "Sandbox",
    XPCONNECT_GLOBAL_FLAGS,
    JS_PropertyStub,   JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    sandbox_enumerate, sandbox_resolve, sandbox_convert,  sandbox_finalize,
    NULL, NULL, NULL, NULL, TraceXPCGlobal
};

static JSFunctionSpec SandboxFunctions[] = {
    {"dump",    SandboxDump,    1,0},
    {"debug",   SandboxDebug,   1,0},
    {"importFunction", SandboxImport, 1,0},
    {nsnull,nsnull,0,0}
};


nsXPCComponents_utils_Sandbox::nsXPCComponents_utils_Sandbox()
{
}

nsXPCComponents_utils_Sandbox::~nsXPCComponents_utils_Sandbox()
{
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_utils_Sandbox)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_utils_Sandbox)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_utils_Sandbox)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents_utils_Sandbox)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents_utils_Sandbox)


#define XPC_MAP_CLASSNAME           nsXPCComponents_utils_Sandbox
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_utils_Sandbox"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 

static bool
WrapForSandbox(JSContext *cx, bool wantXrays, jsval *vp)
{
    return wantXrays
           ? JS_WrapValue(cx, vp)
           : xpc::WrapperFactory::WaiveXrayAndWrap(cx, vp);
}



class Identity MOZ_FINAL : public nsISupports
{
    NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS0(Identity)

xpc::SandboxProxyHandler xpc::sandboxProxyHandler;




class SandboxCallableProxyHandler : public js::DirectWrapper {
public:
    SandboxCallableProxyHandler() : js::DirectWrapper(0)
    {
    }

    virtual bool call(JSContext *cx, JSObject *proxy, unsigned argc,
                      Value *vp);
};

bool
SandboxCallableProxyHandler::call(JSContext *cx, JSObject *proxy, unsigned argc,
                                  Value *vp)
{
    
    

    
    JSObject *sandboxProxy = JS_GetParent(proxy);
    MOZ_ASSERT(js::IsProxy(sandboxProxy) &&
               js::GetProxyHandler(sandboxProxy) == &xpc::sandboxProxyHandler);

    
    
    JSObject *sandboxGlobal = JS_GetParent(sandboxProxy);
    MOZ_ASSERT(js::GetObjectJSClass(sandboxGlobal) == &SandboxClass);

    
    
    
    
    
    
    JS::Value thisVal = JS_THIS_VALUE(cx, vp);
    if (thisVal == ObjectValue(*sandboxGlobal)) {
        thisVal = ObjectValue(*js::GetProxyTargetObject(sandboxProxy));
    }

    return JS::Call(cx, thisVal, js::GetProxyCall(proxy), argc,
                    JS_ARGV(cx, vp), vp);
}

static SandboxCallableProxyHandler sandboxCallableProxyHandler;



static JSObject*
WrapCallable(JSContext *cx, JSObject *callable, JSObject *sandboxProtoProxy)
{
    MOZ_ASSERT(JS_ObjectIsCallable(cx, callable));
    
    
    
    MOZ_ASSERT(js::IsProxy(sandboxProtoProxy) &&
               js::GetProxyHandler(sandboxProtoProxy) ==
                 &xpc::sandboxProxyHandler);

    
    
    return js::NewProxyObject(cx, &sandboxCallableProxyHandler,
                              ObjectValue(*callable), nsnull,
                              sandboxProtoProxy, callable, callable);
}

template<typename Op>
bool BindPropertyOp(JSContext *cx, Op& op, PropertyDescriptor *desc, jsid id,
                    unsigned attrFlag, JSObject *sandboxProtoProxy)
{
    if (!op) {
        return true;
    }

    JSObject *func;
    if (desc->attrs & attrFlag) {
        
        func = JS_FUNC_TO_DATA_PTR(JSObject *, op);
    } else {
        
        
        uint32_t args = (attrFlag == JSPROP_GETTER) ? 0 : 1;
        func = GeneratePropertyOp(cx, desc->obj, id, args, op);
        if (!func)
            return false;
    }
    func = WrapCallable(cx, func, sandboxProtoProxy);
    if (!func)
        return false;
    op = JS_DATA_TO_FUNC_PTR(Op, func);
    desc->attrs |= attrFlag;
    return true;
}

extern JSBool
XPC_WN_Helper_GetProperty(JSContext *cx, JSHandleObject obj, JSHandleId id, jsval *vp);
extern JSBool
XPC_WN_Helper_SetProperty(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, jsval *vp);

bool
xpc::SandboxProxyHandler::getPropertyDescriptor(JSContext *cx, JSObject *proxy,
                                                jsid id_, bool set,
                                                PropertyDescriptor *desc)
{
    JS::RootedObject obj(cx, wrappedObject(proxy));
    JS::RootedId id(cx, id_);

    JS_ASSERT(js::GetObjectCompartment(obj) == js::GetObjectCompartment(proxy));
    
    
    if (!JS_GetPropertyDescriptorById(cx, obj, id,
                                      (set ? JSRESOLVE_ASSIGNING : 0) | JSRESOLVE_QUALIFIED,
                                      desc))
        return false;

    if (!desc->obj)
        return true; 

    
    
    
    
    
    
    
    
    
    
    if (desc->getter != xpc::holder_get &&
        desc->getter != XPC_WN_Helper_GetProperty &&
        !BindPropertyOp(cx, desc->getter, desc, id, JSPROP_GETTER, proxy))
        return false;
    if (desc->setter != xpc::holder_set &&
        desc->setter != XPC_WN_Helper_SetProperty &&
        !BindPropertyOp(cx, desc->setter, desc, id, JSPROP_SETTER, proxy))
        return false;
    if (desc->value.isObject()) {
        JSObject* val = &desc->value.toObject();
        if (JS_ObjectIsCallable(cx, val)) {
            val = WrapCallable(cx, val, proxy);
            if (!val)
                return false;
            desc->value = ObjectValue(*val);
        }
    }

    return true;
}

bool
xpc::SandboxProxyHandler::getOwnPropertyDescriptor(JSContext *cx,
                                                   JSObject *proxy,
                                                   jsid id, bool set,
                                                   PropertyDescriptor *desc)
{
    if (!getPropertyDescriptor(cx, proxy, id, set, desc))
        return false;

    if (desc->obj != wrappedObject(proxy))
        desc->obj = nsnull;

    return true;
}

nsresult
xpc_CreateSandboxObject(JSContext *cx, jsval *vp, nsISupports *prinOrSop, SandboxOptions& options)
{
    
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if (NS_FAILED(rv))
        return NS_ERROR_XPC_UNEXPECTED;

    nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(prinOrSop));

    if (!sop) {
        nsCOMPtr<nsIPrincipal> principal(do_QueryInterface(prinOrSop));

        if (!principal) {
            principal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
            NS_ASSERTION(NS_FAILED(rv) || principal,
                         "Bad return from do_CreateInstance");

            if (!principal || NS_FAILED(rv)) {
                if (NS_SUCCEEDED(rv)) {
                    rv = NS_ERROR_FAILURE;
                }

                return rv;
            }
        }

        sop = new PrincipalHolder(principal);
        if (!sop)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsIPrincipal *principal = sop->GetPrincipal();

    JSCompartment *compartment;
    JSObject *sandbox;

    nsRefPtr<Identity> identity = new Identity();
    rv = xpc_CreateGlobalObject(cx, &SandboxClass, principal, identity,
                                options.wantXrays, &sandbox, &compartment);
    NS_ENSURE_SUCCESS(rv, rv);

    JS::AutoObjectRooter tvr(cx, sandbox);

    {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, sandbox))
            return NS_ERROR_XPC_UNEXPECTED;

        if (options.proto) {
            bool ok = JS_WrapObject(cx, &options.proto);
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;

            if (xpc::WrapperFactory::IsXrayWrapper(options.proto) && !options.wantXrays) {
                jsval v = OBJECT_TO_JSVAL(options.proto);
                if (!xpc::WrapperFactory::WaiveXrayAndWrap(cx, &v))
                    return NS_ERROR_FAILURE;
                options.proto = JSVAL_TO_OBJECT(v);
            }

            
            JSObject *unwrappedProto = js::UnwrapObject(options.proto, false);
            js::Class *unwrappedClass = js::GetObjectClass(unwrappedProto);
            if (IS_WRAPPER_CLASS(unwrappedClass) ||
                mozilla::dom::IsDOMClass(Jsvalify(unwrappedClass))) {
                
                
                options.proto = js::NewProxyObject(cx, &xpc::sandboxProxyHandler,
                                                   ObjectValue(*options.proto), nsnull,
                                                   sandbox);
                if (!options.proto)
                    return NS_ERROR_OUT_OF_MEMORY;
            }

            ok = JS_SetPrototype(cx, sandbox, options.proto);
            if (!ok)
                return NS_ERROR_XPC_UNEXPECTED;
        }

        
        JS_SetPrivate(sandbox, sop.forget().get());

        XPCCallContext ccx(NATIVE_CALLER, cx);
        if (!ccx.IsValid())
            return NS_ERROR_XPC_UNEXPECTED;

        {
          JSAutoEnterCompartment ac;
          if (!ac.enter(ccx, sandbox))
              return NS_ERROR_XPC_UNEXPECTED;
          XPCWrappedNativeScope* scope =
              XPCWrappedNativeScope::GetNewOrUsed(ccx, sandbox);

          if (!scope)
              return NS_ERROR_XPC_UNEXPECTED;

          if (options.wantComponents &&
              !nsXPCComponents::AttachComponentsObject(ccx, scope, sandbox))
              return NS_ERROR_XPC_UNEXPECTED;

          if (!XPCNativeWrapper::AttachNewConstructorObject(ccx, sandbox))
              return NS_ERROR_XPC_UNEXPECTED;
        }

        if (!JS_DefineFunctions(cx, sandbox, SandboxFunctions))
            return NS_ERROR_XPC_UNEXPECTED;

        if (options.wantXHRConstructor &&
            !JS_DefineFunction(cx, sandbox, "XMLHttpRequest", CreateXMLHttpRequest, 0, JSFUN_CONSTRUCTOR))
            return NS_ERROR_XPC_UNEXPECTED;
    }

    if (vp) {
        *vp = OBJECT_TO_JSVAL(sandbox);
        if (!WrapForSandbox(cx, options.wantXrays, vp)) {
            return NS_ERROR_UNEXPECTED;
        }
    }

    
    
    xpc::SetLocationForGlobal(sandbox, options.sandboxName);

    return NS_OK;
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Call(nsIXPConnectWrappedNative *wrapper,
                                    JSContext * cx,
                                    JSObject * obj,
                                    PRUint32 argc,
                                    jsval * argv,
                                    jsval * vp,
                                    bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}








NS_IMETHODIMP
nsXPCComponents_utils_Sandbox::Construct(nsIXPConnectWrappedNative *wrapper,
                                         JSContext * cx,
                                         JSObject * obj,
                                         PRUint32 argc,
                                         jsval * argv,
                                         jsval * vp,
                                         bool *_retval)
{
    return CallOrConstruct(wrapper, cx, obj, argc, argv, vp, _retval);
}



nsresult
GetPrincipalFromString(JSContext *cx, JSString *codebase, nsIPrincipal **principal)
{
    MOZ_ASSERT(principal);
    MOZ_ASSERT(codebase);
    nsCOMPtr<nsIURI> uri;
    nsDependentJSString codebaseStr;
    NS_ENSURE_TRUE(codebaseStr.init(cx, codebase), NS_ERROR_FAILURE);
    nsresult rv = NS_NewURI(getter_AddRefs(uri), codebaseStr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptSecurityManager> secman =
        do_GetService(kScriptSecurityManagerContractID);
    NS_ENSURE_TRUE(secman, NS_ERROR_FAILURE);

    
    
    
    rv = secman->GetNoAppCodebasePrincipal(uri, principal);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(*principal, NS_ERROR_FAILURE);

    return NS_OK;
}



nsresult
GetPrincipalOrSOP(JSContext *cx, JSObject &from, nsISupports **out)
{
    MOZ_ASSERT(out);
    *out = NULL;

    nsCOMPtr<nsIXPConnect> xpc = nsXPConnect::GetXPConnect();
    if (!xpc)
        return NS_ERROR_XPC_UNEXPECTED;
    nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
    xpc->GetWrappedNativeOfJSObject(cx, &from,
                                    getter_AddRefs(wrapper));

    NS_ENSURE_TRUE(wrapper, NS_ERROR_INVALID_ARG);

    if (nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryWrappedNative(wrapper)) {
        sop.forget(out);
        return NS_OK;
    }

    nsCOMPtr<nsIPrincipal> principal = do_QueryWrappedNative(wrapper);
    principal.forget(out);
    NS_ENSURE_TRUE(*out, NS_ERROR_INVALID_ARG);

    return NS_OK;
}



nsresult
GetExpandedPrincipal(JSContext *cx, JSObject &arrayObj, nsIExpandedPrincipal **out)
{
    MOZ_ASSERT(out);
    uint32_t length;

    if (!JS_IsArrayObject(cx, &arrayObj) ||
        !JS_GetArrayLength(cx, &arrayObj, &length) ||
        !length)
    {
        
        
        
        return NS_ERROR_INVALID_ARG;
    }

    nsTArray< nsCOMPtr<nsIPrincipal> > allowedDomains(length);
    allowedDomains.SetLength(length);
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    NS_ENSURE_TRUE(ssm, NS_ERROR_XPC_UNEXPECTED);

    for (uint32_t i = 0; i < length; ++i) {
        jsval allowed;

        if (!JS_GetElement(cx, &arrayObj, i, &allowed))
            return NS_ERROR_INVALID_ARG;

        nsresult rv;
        nsCOMPtr<nsIPrincipal> principal;
        if (allowed.isString()) {
            
            rv = GetPrincipalFromString(cx, allowed.toString(), getter_AddRefs(principal));
            NS_ENSURE_SUCCESS(rv, rv);
        } else if (allowed.isObject()) {
            
            nsCOMPtr<nsISupports> prinOrSop;
            rv = GetPrincipalOrSOP(cx, allowed.toObject(), getter_AddRefs(prinOrSop));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(prinOrSop));
            principal = do_QueryInterface(prinOrSop);
            if (sop) {
                principal = sop->GetPrincipal();
            }
        }
        NS_ENSURE_TRUE(principal, NS_ERROR_INVALID_ARG);

        
        bool isSystem;
        rv = ssm->IsSystemPrincipal(principal, &isSystem);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_FALSE(isSystem, NS_ERROR_INVALID_ARG);
        allowedDomains[i] = principal;
  }

  nsCOMPtr<nsIExpandedPrincipal> result = new nsExpandedPrincipal(allowedDomains);
  result.forget(out);
  return NS_OK;
}


nsresult
GetPropFromOptions(JSContext *cx, JSObject &from, const char *name, jsval *prop, JSBool *found)
{
    if (!JS_HasProperty(cx, &from, name, found))
        return NS_ERROR_INVALID_ARG;

    if (found && !JS_GetProperty(cx, &from, name, prop))
        return NS_ERROR_INVALID_ARG;

    return NS_OK;
}


nsresult
GetBoolPropFromOptions(JSContext *cx, JSObject &from, const char *name, bool *prop)
{
    MOZ_ASSERT(prop);
    jsval propVal;
    JSBool found;
    if (NS_FAILED(GetPropFromOptions(cx, from, name, &propVal, &found)))
        return NS_ERROR_INVALID_ARG;

    if (!found)
        return NS_OK;

    if (!propVal.isBoolean())
        return NS_ERROR_INVALID_ARG;

    *prop = propVal.toBoolean();
    return NS_OK;
}


nsresult
GetObjPropFromOptions(JSContext *cx, JSObject &from, const char *name, JSObject **prop)
{
    MOZ_ASSERT(prop);
    jsval propVal;
    JSBool found;

    if (NS_FAILED(GetPropFromOptions(cx, from, name, &propVal, &found)))
        return NS_ERROR_INVALID_ARG;

    if (!found) {
        *prop = NULL;
        return NS_OK;
    }

    if (!propVal.isObject())
        return NS_ERROR_INVALID_ARG;

    *prop = &propVal.toObject();
    return NS_OK;
}


nsresult
GetStringPropFromOptions(JSContext *cx, JSObject &from, const char *name, nsCString &prop)
{
    jsval propVal;
    JSBool found;
    nsresult rv = GetPropFromOptions(cx, from, name, &propVal, &found);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!found)
        return NS_OK;

    NS_ENSURE_TRUE(propVal.isString(), NS_ERROR_INVALID_ARG);

    char *tmp = JS_EncodeString(cx, propVal.toString());
    NS_ENSURE_TRUE(tmp, NS_ERROR_INVALID_ARG);
    prop.Adopt(tmp, strlen(tmp));
    return NS_OK;
}


nsresult
ParseOptionsObject(JSContext *cx, jsval from, SandboxOptions &options)
{
    NS_ENSURE_TRUE(from.isObject(), NS_ERROR_INVALID_ARG);
    JSObject &optionsObject = from.toObject();
    nsresult rv = GetObjPropFromOptions(cx, optionsObject,
                                        "sandboxPrototype", &options.proto);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantXrays", &options.wantXrays);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantComponents", &options.wantComponents);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetBoolPropFromOptions(cx, optionsObject,
                                "wantXHRConstructor", &options.wantXHRConstructor);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetStringPropFromOptions(cx, optionsObject,
                                  "sandboxName", options.sandboxName);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

static nsresult
AssembleSandboxMemoryReporterName(JSContext *cx, nsCString &sandboxName)
{
    
    if (sandboxName.IsEmpty())
        sandboxName = NS_LITERAL_CSTRING("[anonymous sandbox]");

    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    NS_ENSURE_TRUE(xpc, NS_ERROR_XPC_UNEXPECTED);

    
    nsAXPCNativeCallContext *cc = nsnull;
    xpc->GetCurrentNativeCallContext(&cc);
    NS_ENSURE_TRUE(cc, NS_ERROR_INVALID_ARG);

    
    nsCOMPtr<nsIStackFrame> frame;
    xpc->GetCurrentJSStack(getter_AddRefs(frame));

    
    if (frame) {
        nsCString location;
        PRInt32 lineNumber = 0;
        frame->GetFilename(getter_Copies(location));
        frame->GetLineNumber(&lineNumber);
        
        sandboxName.AppendLiteral(" (from: ");
        sandboxName.Append(location);
        sandboxName.AppendLiteral(":");
        sandboxName.AppendInt(lineNumber);
        sandboxName.AppendLiteral(")");
    }

    return NS_OK;
}


nsresult
nsXPCComponents_utils_Sandbox::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                               JSContext * cx, JSObject * obj,
                                               PRUint32 argc, JS::Value * argv,
                                               jsval * vp, bool *_retval)
{
    if (argc < 1)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    nsresult rv;

    
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIExpandedPrincipal> expanded;
    nsCOMPtr<nsISupports> prinOrSop;

    if (argv[0].isString()) {
        rv = GetPrincipalFromString(cx, argv[0].toString(), getter_AddRefs(principal));
        prinOrSop = principal;
    } else if (argv[0].isObject()) {
        if (JS_IsArrayObject(cx, &argv[0].toObject())) {
            rv = GetExpandedPrincipal(cx, argv[0].toObject(), getter_AddRefs(expanded));
            prinOrSop = expanded;
        } else {
            rv = GetPrincipalOrSOP(cx, argv[0].toObject(), getter_AddRefs(prinOrSop));
        }
    } else {
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);
    }

    if (NS_FAILED(rv))
        return ThrowAndFail(rv, cx, _retval);

    SandboxOptions options;

    if (argc > 1 && NS_FAILED(ParseOptionsObject(cx, argv[1], options)))
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    if (NS_FAILED(AssembleSandboxMemoryReporterName(cx, options.sandboxName)))
        return ThrowAndFail(NS_ERROR_INVALID_ARG, cx, _retval);

    rv = xpc_CreateSandboxObject(cx, vp, prinOrSop, options);

    if (NS_FAILED(rv))
        return ThrowAndFail(rv, cx, _retval);

    *_retval = true;

    return rv;
}

class ContextHolder : public nsIScriptObjectPrincipal
                    , public nsIScriptContextPrincipal
{
public:
    ContextHolder(JSContext *aOuterCx, JSObject *aSandbox, nsIPrincipal *aPrincipal);
    virtual ~ContextHolder();

    JSContext * GetJSContext()
    {
        return mJSContext;
    }

    nsIScriptObjectPrincipal * GetObjectPrincipal() { return this; }
    nsIPrincipal * GetPrincipal() { return mPrincipal; }

    NS_DECL_ISUPPORTS

private:
    static JSBool ContextHolderOperationCallback(JSContext *cx);

    JSContext* mJSContext;
    JSContext* mOrigCx;
    nsCOMPtr<nsIPrincipal> mPrincipal;
};

NS_IMPL_ISUPPORTS2(ContextHolder, nsIScriptObjectPrincipal, nsIScriptContextPrincipal)

ContextHolder::ContextHolder(JSContext *aOuterCx,
                             JSObject *aSandbox,
                             nsIPrincipal *aPrincipal)
    : mJSContext(JS_NewContext(JS_GetRuntime(aOuterCx), 1024)),
      mOrigCx(aOuterCx),
      mPrincipal(aPrincipal)
{
    if (mJSContext) {
        bool isChrome;
        DebugOnly<nsresult> rv = XPCWrapper::GetSecurityManager()->
                                   IsSystemPrincipal(mPrincipal, &isChrome);
        MOZ_ASSERT(NS_SUCCEEDED(rv));
        bool allowXML = Preferences::GetBool(isChrome ?
                                             "javascript.options.xml.chrome" :
                                             "javascript.options.xml.content");

        JSAutoRequest ar(mJSContext);
        JS_SetOptions(mJSContext,
                      JS_GetOptions(mJSContext) |
                      JSOPTION_DONT_REPORT_UNCAUGHT |
                      JSOPTION_PRIVATE_IS_NSISUPPORTS |
                      (allowXML ? JSOPTION_ALLOW_XML : 0));
        JS_SetGlobalObject(mJSContext, aSandbox);
        JS_SetContextPrivate(mJSContext, this);
        JS_SetOperationCallback(mJSContext, ContextHolderOperationCallback);
    }
}

ContextHolder::~ContextHolder()
{
    if (mJSContext)
        JS_DestroyContextNoGC(mJSContext);
}

JSBool
ContextHolder::ContextHolderOperationCallback(JSContext *cx)
{
    ContextHolder* thisObject =
        static_cast<ContextHolder*>(JS_GetContextPrivate(cx));
    NS_ASSERTION(thisObject, "How did that happen?");

    JSContext *origCx = thisObject->mOrigCx;
    JSOperationCallback callback = JS_GetOperationCallback(origCx);
    JSBool ok = true;
    if (callback)
        ok = callback(origCx);
    return ok;
}




NS_IMETHODIMP
nsXPCComponents_Utils::EvalInSandbox(const nsAString& source,
                                     const JS::Value& sandboxVal,
                                     const JS::Value& version,
                                     const JS::Value& filenameVal,
                                     PRInt32 lineNumber,
                                     JSContext *cx,
                                     PRUint8 optionalArgc,
                                     JS::Value *retval)
{
    JSObject *sandbox;
    if (!JS_ValueToObject(cx, sandboxVal, &sandbox) || !sandbox)
        return NS_ERROR_INVALID_ARG;

    
    JSVersion jsVersion = JSVERSION_DEFAULT;
    if (optionalArgc >= 1) {
        JSString *jsVersionStr = JS_ValueToString(cx, version);
        if (!jsVersionStr)
            return NS_ERROR_INVALID_ARG;

        JSAutoByteString bytes(cx, jsVersionStr);
        if (!bytes)
            return NS_ERROR_INVALID_ARG;

        jsVersion = JS_StringToVersion(bytes.ptr());
        if (jsVersion == JSVERSION_UNKNOWN)
            return NS_ERROR_INVALID_ARG;
    }

    
    nsXPIDLCString filename;
    PRInt32 lineNo = (optionalArgc >= 3) ? lineNumber : 0;
    if (optionalArgc >= 2) {
        JSString *filenameStr = JS_ValueToString(cx, filenameVal);
        if (!filenameStr)
            return NS_ERROR_INVALID_ARG;

        JSAutoByteString filenameBytes;
        if (!filenameBytes.encode(cx, filenameStr))
            return NS_ERROR_INVALID_ARG;
        filename = filenameBytes.ptr();
    } else {
        
        nsresult rv;
        nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIStackFrame> frame;
        xpc->GetCurrentJSStack(getter_AddRefs(frame));
        if (frame) {
            frame->GetFilename(getter_Copies(filename));
            frame->GetLineNumber(&lineNo);
        }
    }

    return xpc_EvalInSandbox(cx, sandbox, source, filename.get(), lineNo,
                             jsVersion, false, retval);
}

nsresult
xpc_EvalInSandbox(JSContext *cx, JSObject *sandbox, const nsAString& source,
                  const char *filename, PRInt32 lineNo,
                  JSVersion jsVersion, bool returnStringOnly, jsval *rval)
{
    JS_AbortIfWrongThread(JS_GetRuntime(cx));

#ifdef DEBUG
    
    {
        nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
        if (ssm) {
            JSStackFrame *fp;
            nsIPrincipal *subjectPrincipal =
                ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
            bool system;
            ssm->IsSystemPrincipal(subjectPrincipal, &system);
            if (fp && !system) {
                ssm->IsCapabilityEnabled("UniversalXPConnect", &system);
                NS_ASSERTION(system, "Bad caller!");
            }
        }
    }
#endif

    sandbox = XPCWrapper::UnsafeUnwrapSecurityWrapper(sandbox);
    if (!sandbox || js::GetObjectJSClass(sandbox) != &SandboxClass) {
        return NS_ERROR_INVALID_ARG;
    }

    nsIScriptObjectPrincipal *sop =
        (nsIScriptObjectPrincipal*)xpc_GetJSPrivate(sandbox);
    NS_ASSERTION(sop, "Invalid sandbox passed");
    nsCOMPtr<nsIPrincipal> prin = sop->GetPrincipal();

    if (!prin) {
        return NS_ERROR_FAILURE;
    }

    nsCAutoString filenameBuf;
    if (!filename) {
        
        nsJSPrincipals::get(prin)->GetScriptLocation(filenameBuf);
        filename = filenameBuf.get();
        lineNo = 1;
    }

    JSObject *callingScope;
    {
        JSAutoRequest req(cx);

        callingScope = JS_GetGlobalForScopeChain(cx);
        if (!callingScope) {
            return NS_ERROR_FAILURE;
        }
    }

    nsRefPtr<ContextHolder> sandcx = new ContextHolder(cx, sandbox, prin);
    if (!sandcx || !sandcx->GetJSContext()) {
        JS_ReportError(cx, "Can't prepare context for evalInSandbox");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (jsVersion != JSVERSION_DEFAULT)
        JS_SetVersion(sandcx->GetJSContext(), jsVersion);

    XPCJSContextStack *stack = XPCJSRuntime::Get()->GetJSContextStack();
    MOZ_ASSERT(stack);
    if (!stack->Push(sandcx->GetJSContext())) {
        JS_ReportError(cx, "Unable to initialize XPConnect with the sandbox context");
        return NS_ERROR_FAILURE;
    }

    nsresult rv = NS_OK;

    {
        JSAutoRequest req(sandcx->GetJSContext());
        JSAutoEnterCompartment ac;

        if (!ac.enter(sandcx->GetJSContext(), sandbox)) {
            if (stack)
                unused << stack->Pop();
            return NS_ERROR_FAILURE;
        }

        jsval v;
        JSString *str = nsnull;
        JSBool ok =
            JS_EvaluateUCScriptForPrincipals(sandcx->GetJSContext(), sandbox,
                                             nsJSPrincipals::get(prin),
                                             reinterpret_cast<const jschar *>
                                                             (PromiseFlatString(source).get()),
                                             source.Length(), filename, lineNo,
                                             &v);
        if (ok && returnStringOnly && !(JSVAL_IS_VOID(v))) {
            ok = !!(str = JS_ValueToString(sandcx->GetJSContext(), v));
        }

        if (!ok) {
            
            

            jsval exn;
            if (JS_GetPendingException(sandcx->GetJSContext(), &exn)) {
                JS_ClearPendingException(sandcx->GetJSContext());

                if (returnStringOnly) {
                    
                    
                    str = JS_ValueToString(sandcx->GetJSContext(), exn);

                    if (str) {
                        
                        
                        exn = STRING_TO_JSVAL(str);
                        if (JS_WrapValue(cx, &exn)) {
                            JS_SetPendingException(cx, exn);
                        } else {
                            JS_ClearPendingException(cx);
                            rv = NS_ERROR_FAILURE;
                        }
                    } else {
                        JS_ClearPendingException(cx);
                        rv = NS_ERROR_FAILURE;
                    }
                } else {
                    if (JS_WrapValue(cx, &exn)) {
                        JS_SetPendingException(cx, exn);
                    }
                }


                
                str = nsnull;
            } else {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        } else {
            
            JSAutoRequest req(cx);
            JSAutoEnterCompartment ac;
            if (str) {
                v = STRING_TO_JSVAL(str);
            }

            CompartmentPrivate *sandboxdata = GetCompartmentPrivate(sandbox);
            if (!ac.enter(cx, callingScope) ||
                !WrapForSandbox(cx, sandboxdata->wantXrays, &v)) {
                rv = NS_ERROR_FAILURE;
            }

            if (NS_SUCCEEDED(rv)) {
                *rval = v;
            }
        }
    }

    if (stack)
        unused << stack->Pop();

    return rv;
}




NS_IMETHODIMP
nsXPCComponents_Utils::Import(const nsACString& registryLocation,
                              const JS::Value& targetObj,
                              JSContext* cx,
                              PRUint8 optionalArgc,
                              JS::Value* retval)
{
    nsCOMPtr<xpcIJSModuleLoader> moduleloader =
        do_GetService(MOZJSCOMPONENTLOADER_CONTRACTID);
    if (!moduleloader)
        return NS_ERROR_FAILURE;
    return moduleloader->Import(registryLocation, targetObj, cx, optionalArgc, retval);
}



NS_IMETHODIMP
nsXPCComponents_Utils::Unload(const nsACString & registryLocation)
{
    nsCOMPtr<xpcIJSModuleLoader> moduleloader =
        do_GetService(MOZJSCOMPONENTLOADER_CONTRACTID);
    if (!moduleloader)
        return NS_ERROR_FAILURE;
    return moduleloader->Unload(registryLocation);
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetWeakReference(const JS::Value &object, JSContext *cx,
                                        xpcIJSWeakReference **_retval)
{
    nsRefPtr<xpcJSWeakReference> ref = new xpcJSWeakReference();
    nsresult rv = ref->Init(cx, object);
    NS_ENSURE_SUCCESS(rv, rv);
    ref.forget(_retval);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForceGC()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
    js::PrepareForFullGC(rt);
    js::GCForReason(rt, js::gcreason::COMPONENT_UTILS);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForceShrinkingGC()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
    js::PrepareForFullGC(rt);
    js::ShrinkingGC(rt, js::gcreason::COMPONENT_UTILS);
    return NS_OK;
}

class PreciseGCRunnable : public nsRunnable
{
  public:
    PreciseGCRunnable(ScheduledGCCallback* aCallback, bool aShrinking)
    : mCallback(aCallback), mShrinking(aShrinking) {}

    NS_IMETHOD Run()
    {
        JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();

        JSContext *cx;
        JSContext *iter = nsnull;
        while ((cx = JS_ContextIterator(rt, &iter)) != NULL) {
            if (JS_IsRunning(cx)) {
                return NS_DispatchToMainThread(this);
            }
        }

        js::PrepareForFullGC(rt);
        if (mShrinking)
            js::ShrinkingGC(rt, js::gcreason::COMPONENT_UTILS);
        else
            js::GCForReason(rt, js::gcreason::COMPONENT_UTILS);

        mCallback->Callback();
        return NS_OK;
    }

  private:
    nsRefPtr<ScheduledGCCallback> mCallback;
    bool mShrinking;
};


NS_IMETHODIMP
nsXPCComponents_Utils::SchedulePreciseGC(ScheduledGCCallback* aCallback)
{
    nsRefPtr<PreciseGCRunnable> event = new PreciseGCRunnable(aCallback, false);
    return NS_DispatchToMainThread(event);
}


NS_IMETHODIMP
nsXPCComponents_Utils::SchedulePreciseShrinkingGC(ScheduledGCCallback* aCallback)
{
    nsRefPtr<PreciseGCRunnable> event = new PreciseGCRunnable(aCallback, true);
    return NS_DispatchToMainThread(event);
}


NS_IMETHODIMP
nsXPCComponents_Utils::NondeterministicGetWeakMapKeys(const JS::Value &aMap,
                                                      JSContext *aCx,
                                                      JS::Value *aKeys)
{
    if (!aMap.isObject()) {
        aKeys->setUndefined();
        return NS_OK;
    }
    JSObject *objRet;
    if (!JS_NondeterministicGetWeakMapKeys(aCx, &aMap.toObject(), &objRet))
        return NS_ERROR_OUT_OF_MEMORY;
    *aKeys = objRet ? ObjectValue(*objRet) : UndefinedValue();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetJSTestingFunctions(JSContext *cx,
                                             JS::Value *retval)
{
    JSObject *obj = js::GetTestingFunctions(cx);
    if (!obj)
        return NS_ERROR_XPC_JAVASCRIPT_ERROR;
    *retval = OBJECT_TO_JSVAL(obj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetGlobalForObject(const JS::Value& object,
                                          JSContext *cx,
                                          JS::Value *retval)
{
  
  if (JSVAL_IS_PRIMITIVE(object))
    return NS_ERROR_XPC_BAD_CONVERT_JS;

  
  
  
  
  
  JS::Rooted<JSObject*> obj(cx, JSVAL_TO_OBJECT(object));
  obj = js::UnwrapObject(obj);
  {
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, obj))
      return NS_ERROR_FAILURE;
    obj = JS_GetGlobalForObject(cx, obj);
  }
  JS_WrapObject(cx, obj.address());
  *retval = OBJECT_TO_JSVAL(obj);

  
  if (JSObjectOp outerize = js::GetObjectClass(obj)->ext.outerObject)
      *retval = OBJECT_TO_JSVAL(outerize(cx, obj));

  return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CreateObjectIn(const jsval &vobj, JSContext *cx, jsval *rval)
{
    if (!cx)
        return NS_ERROR_FAILURE;

    
    if (JSVAL_IS_PRIMITIVE(vobj))
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    JSObject *scope = js::UnwrapObject(JSVAL_TO_OBJECT(vobj));
    JSObject *obj;
    {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, scope))
            return NS_ERROR_FAILURE;

        obj = JS_NewObject(cx, nsnull, nsnull, scope);
        if (!obj)
            return NS_ERROR_FAILURE;
    }

    if (!JS_WrapObject(cx, &obj))
        return NS_ERROR_FAILURE;
    *rval = OBJECT_TO_JSVAL(obj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CreateArrayIn(const jsval &vobj, JSContext *cx, jsval *rval)
{
    if (!cx)
        return NS_ERROR_FAILURE;

    
    if (JSVAL_IS_PRIMITIVE(vobj))
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    JSObject *scope = js::UnwrapObject(JSVAL_TO_OBJECT(vobj));
    JSObject *obj;
    {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, scope))
            return NS_ERROR_FAILURE;

        obj =  JS_NewArrayObject(cx, 0, NULL);
        if (!obj)
            return NS_ERROR_FAILURE;
    }

    if (!JS_WrapObject(cx, &obj))
        return NS_ERROR_FAILURE;
    *rval = OBJECT_TO_JSVAL(obj);
    return NS_OK;
}

JSBool
FunctionWrapper(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *callee = &JS_CALLEE(cx, vp).toObject();
    JS::Value v = js::GetFunctionNativeReserved(callee, 0);
    NS_ASSERTION(v.isObject(), "weird function");

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj) {
        return JS_FALSE;
    }
    return JS_CallFunctionValue(cx, obj, v, argc, JS_ARGV(cx, vp), vp);
}

JSBool
WrapCallable(JSContext *cx, JSObject *obj, jsid id, JSObject *propobj, jsval *vp)
{
    JSFunction *fun = js::NewFunctionByIdWithReserved(cx, FunctionWrapper, 0, 0,
                                                      JS_GetGlobalForObject(cx, obj), id);
    if (!fun)
        return false;

    JSObject *funobj = JS_GetFunctionObject(fun);
    js::SetFunctionNativeReserved(funobj, 0, OBJECT_TO_JSVAL(propobj));
    *vp = OBJECT_TO_JSVAL(funobj);
    return true;
}


NS_IMETHODIMP
nsXPCComponents_Utils::MakeObjectPropsNormal(const jsval &vobj, JSContext *cx)
{
    if (!cx)
        return NS_ERROR_FAILURE;

    
    if (JSVAL_IS_PRIMITIVE(vobj))
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    JSObject *obj = js::UnwrapObject(JSVAL_TO_OBJECT(vobj));

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, obj))
        return NS_ERROR_FAILURE;

    JS::AutoIdArray ida(cx, JS_Enumerate(cx, obj));
    if (!ida)
        return NS_ERROR_FAILURE;

    for (size_t i = 0; i < ida.length(); ++i) {
        jsid id = ida[i];
        jsval v;

        if (!JS_GetPropertyById(cx, obj, id, &v))
            return NS_ERROR_FAILURE;

        if (JSVAL_IS_PRIMITIVE(v))
            continue;

        JSObject *propobj = JSVAL_TO_OBJECT(v);
        
        if (!js::IsWrapper(propobj) || !JS_ObjectIsCallable(cx, propobj))
            continue;

        if (!WrapCallable(cx, obj, id, propobj, &v) ||
            !JS_SetPropertyById(cx, obj, id, &v))
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::IsDeadWrapper(const jsval &obj, bool *out)
{
    *out = false;
    if (JSVAL_IS_PRIMITIVE(obj))
        return NS_ERROR_INVALID_ARG;

    *out = JS_IsDeadWrapper(JSVAL_TO_OBJECT(obj));
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CanCreateWrapper(const nsIID * iid, char **_retval)
{
    
    *_retval = xpc_CloneAllAccess();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
    static const char* allowed[] = { "lookupMethod", "evalInSandbox", nsnull };
    *_retval = xpc_CheckAccessList(methodName, allowed);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}

nsresult
GetBoolOption(JSContext* cx, uint32_t aOption, bool* aValue)
{
    *aValue = !!(JS_GetOptions(cx) & aOption);
    return NS_OK;
}

nsresult
SetBoolOption(JSContext* cx, uint32_t aOption, bool aValue)
{
    uint32_t options = JS_GetOptions(cx);
    if (aValue) {
        options |= aOption;
    } else {
        options &= ~aOption;
    }
    JS_SetOptions(cx, options & JSALLOPTION_MASK);
    return NS_OK;
}

#define GENERATE_JSOPTION_GETTER_SETTER(_attr, _flag)                   \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Get## _attr(JSContext* cx, bool* aValue)     \
    {                                                                   \
        return GetBoolOption(cx, _flag, aValue);                        \
    }                                                                   \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Set## _attr(JSContext* cx, bool aValue)      \
    {                                                                   \
        return SetBoolOption(cx, _flag, aValue);                        \
    }

GENERATE_JSOPTION_GETTER_SETTER(Strict, JSOPTION_STRICT)
GENERATE_JSOPTION_GETTER_SETTER(Werror, JSOPTION_WERROR)
GENERATE_JSOPTION_GETTER_SETTER(Atline, JSOPTION_ATLINE)
GENERATE_JSOPTION_GETTER_SETTER(Xml, JSOPTION_MOAR_XML)
GENERATE_JSOPTION_GETTER_SETTER(Relimit, JSOPTION_RELIMIT)
GENERATE_JSOPTION_GETTER_SETTER(Methodjit, JSOPTION_METHODJIT)
GENERATE_JSOPTION_GETTER_SETTER(Methodjit_always, JSOPTION_METHODJIT_ALWAYS)
GENERATE_JSOPTION_GETTER_SETTER(Strict_mode, JSOPTION_STRICT_MODE)

#undef GENERATE_JSOPTION_GETTER_SETTER

NS_IMETHODIMP
nsXPCComponents_Utils::SetGCZeal(PRInt32 aValue, JSContext* cx)
{
#ifdef JS_GC_ZEAL
    JS_SetGCZeal(cx, PRUint8(aValue), JS_DEFAULT_ZEAL_FREQ);
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::NukeSandbox(const JS::Value &obj, JSContext *cx)
{
    NS_ENSURE_TRUE(obj.isObject(), NS_ERROR_INVALID_ARG);
    JSObject *wrapper = &obj.toObject();
    NS_ENSURE_TRUE(IsWrapper(wrapper), NS_ERROR_INVALID_ARG);
    JSObject *sb = UnwrapObject(wrapper);
    NS_ENSURE_TRUE(GetObjectJSClass(sb) == &SandboxClass, NS_ERROR_INVALID_ARG);
    NukeCrossCompartmentWrappers(cx, AllCompartments(), 
                                 SingleCompartment(GetObjectCompartment(sb)),
                                 NukeWindowReferences);
    return NS_OK;
}








NS_INTERFACE_MAP_BEGIN(nsXPCComponents)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(nsXPCComponents)
NS_IMPL_THREADSAFE_RELEASE(nsXPCComponents)



NS_IMETHODIMP
nsXPCComponents::GetInterfaces(PRUint32 *aCount, nsIID * **aArray)
{
    const PRUint32 count = 3;
    *aCount = count;
    nsIID **array;
    *aArray = array = static_cast<nsIID**>(nsMemory::Alloc(count * sizeof(nsIID*)));
    if (!array)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 index = 0;
    nsIID* clone;
#define PUSH_IID(id)                                                          \
    clone = static_cast<nsIID *>(nsMemory::Clone(&NS_GET_IID( id ),           \
                                                 sizeof(nsIID)));             \
    if (!clone)                                                               \
        goto oom;                                                             \
    array[index++] = clone;

    PUSH_IID(nsIXPCComponents)
    PUSH_IID(nsIXPCScriptable)
    PUSH_IID(nsISecurityCheckedComponent)
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
nsXPCComponents::GetHelperForLanguage(PRUint32 language,
                                      nsISupports **retval)
{
    *retval = static_cast<nsIXPCComponents*>(this);
    NS_ADDREF(this);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsXPCComponents::GetClassDescription(char * *aClassDescription)
{
    static const char classDescription[] = "XPCComponents";
    *aClassDescription = (char*)nsMemory::Clone(classDescription, sizeof(classDescription));
    return *aClassDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXPCComponents::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents::nsXPCComponents(XPCWrappedNativeScope* aScope)
    :   mScope(aScope),
        mInterfaces(nsnull),
        mInterfacesByID(nsnull),
        mClasses(nsnull),
        mClassesByID(nsnull),
        mResults(nsnull),
        mID(nsnull),
        mException(nsnull),
        mConstructor(nsnull),
        mUtils(nsnull)
{
    MOZ_ASSERT(aScope, "aScope must not be null");
}

nsXPCComponents::~nsXPCComponents()
{
    ClearMembers();
}

void
nsXPCComponents::ClearMembers()
{
    NS_IF_RELEASE(mInterfaces);
    NS_IF_RELEASE(mInterfacesByID);
    NS_IF_RELEASE(mClasses);
    NS_IF_RELEASE(mClassesByID);
    NS_IF_RELEASE(mResults);
    NS_IF_RELEASE(mID);
    NS_IF_RELEASE(mException);
    NS_IF_RELEASE(mConstructor);
    NS_IF_RELEASE(mUtils);
}


#define XPC_IMPL_GET_OBJ_METHOD(_n)                                           \
NS_IMETHODIMP nsXPCComponents::Get##_n(nsIXPCComponents_##_n * *a##_n) {      \
    NS_ENSURE_ARG_POINTER(a##_n);                                             \
    if (!m##_n) {                                                             \
        if (!(m##_n = new nsXPCComponents_##_n())) {                          \
            *a##_n = nsnull;                                                  \
            return NS_ERROR_OUT_OF_MEMORY;                                    \
        }                                                                     \
        NS_ADDREF(m##_n);                                                     \
    }                                                                         \
    NS_ADDREF(m##_n);                                                         \
    *a##_n = m##_n;                                                           \
    return NS_OK;                                                             \
}

XPC_IMPL_GET_OBJ_METHOD(Interfaces)
XPC_IMPL_GET_OBJ_METHOD(InterfacesByID)
XPC_IMPL_GET_OBJ_METHOD(Classes)
XPC_IMPL_GET_OBJ_METHOD(ClassesByID)
XPC_IMPL_GET_OBJ_METHOD(Results)
XPC_IMPL_GET_OBJ_METHOD(ID)
XPC_IMPL_GET_OBJ_METHOD(Exception)
XPC_IMPL_GET_OBJ_METHOD(Constructor)
XPC_IMPL_GET_OBJ_METHOD(Utils)

#undef XPC_IMPL_GET_OBJ_METHOD


NS_IMETHODIMP
nsXPCComponents::IsSuccessCode(nsresult result, bool *out)
{
    *out = NS_SUCCEEDED(result);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents::GetStack(nsIStackFrame * *aStack)
{
    nsresult rv;
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc)
        return NS_ERROR_FAILURE;
    rv = xpc->GetCurrentJSStack(aStack);
    return rv;
}

NS_IMETHODIMP
nsXPCComponents::GetManager(nsIComponentManager * *aManager)
{
    NS_ASSERTION(aManager, "bad param");
    return NS_GetComponentManager(aManager);
}




#define XPC_MAP_CLASSNAME           nsXPCComponents
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define                             XPC_MAP_WANT_GETPROPERTY
#define                             XPC_MAP_WANT_SETPROPERTY
#define                             XPC_MAP_WANT_PRECREATE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 


NS_IMETHODIMP
nsXPCComponents::NewResolve(nsIXPConnectWrappedNative *wrapper,
                            JSContext * cx, JSObject * obj,
                            jsid id, PRUint32 flags,
                            JSObject * *objp, bool *_retval)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    if (!rt)
        return NS_ERROR_FAILURE;

    unsigned attrs = 0;

    if (id == rt->GetStringID(XPCJSRuntime::IDX_LAST_RESULT))
        attrs = JSPROP_READONLY;
    else if (id != rt->GetStringID(XPCJSRuntime::IDX_RETURN_CODE))
        return NS_OK;

    *objp = obj;
    *_retval = JS_DefinePropertyById(cx, obj, id, JSVAL_VOID, nsnull, nsnull,
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                     attrs);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::GetProperty(nsIXPConnectWrappedNative *wrapper,
                             JSContext * cx, JSObject * obj,
                             jsid id, jsval * vp, bool *_retval)
{
    XPCContext* xpcc = XPCContext::GetXPCContext(cx);
    if (!xpcc)
        return NS_ERROR_FAILURE;

    bool doResult = false;
    nsresult res;
    XPCJSRuntime* rt = xpcc->GetRuntime();
    if (id == rt->GetStringID(XPCJSRuntime::IDX_LAST_RESULT)) {
        res = xpcc->GetLastResult();
        doResult = true;
    } else if (id == rt->GetStringID(XPCJSRuntime::IDX_RETURN_CODE)) {
        res = xpcc->GetPendingResult();
        doResult = true;
    }

    nsresult rv = NS_OK;
    if (doResult) {
        if (!JS_NewNumberValue(cx, (double) res, vp))
            return NS_ERROR_OUT_OF_MEMORY;
        rv = NS_SUCCESS_I_DID_SOMETHING;
    }

    return rv;
}


NS_IMETHODIMP
nsXPCComponents::SetProperty(nsIXPConnectWrappedNative *wrapper,
                             JSContext * cx, JSObject * obj, jsid id,
                             jsval * vp, bool *_retval)
{
    XPCContext* xpcc = XPCContext::GetXPCContext(cx);
    if (!xpcc)
        return NS_ERROR_FAILURE;

    XPCJSRuntime* rt = xpcc->GetRuntime();
    if (!rt)
        return NS_ERROR_FAILURE;

    if (id == rt->GetStringID(XPCJSRuntime::IDX_RETURN_CODE)) {
        nsresult rv;
        if (JS_ValueToECMAUint32(cx, *vp, (uint32_t*)&rv)) {
            xpcc->SetPendingResult(rv);
            xpcc->SetLastResult(rv);
            return NS_SUCCESS_I_DID_SOMETHING;
        }
        return NS_ERROR_FAILURE;
    }

    return NS_ERROR_XPC_CANT_MODIFY_PROP_ON_WN;
}


JSBool
nsXPCComponents::AttachComponentsObject(XPCCallContext& ccx,
                                        XPCWrappedNativeScope* aScope,
                                        JSObject* aGlobal)
{
    if (!aGlobal)
        return false;

    nsXPCComponents* components = aScope->GetComponents();
    if (!components) {
        components = new nsXPCComponents(aScope);
        if (!components)
            return false;
        aScope->SetComponents(components);
    }

    nsCOMPtr<nsIXPCComponents> cholder(components);

    AutoMarkingNativeInterfacePtr iface(ccx);
    iface = XPCNativeInterface::GetNewOrUsed(ccx, &NS_GET_IID(nsIXPCComponents));

    if (!iface)
        return false;

    nsCOMPtr<XPCWrappedNative> wrapper;
    xpcObjectHelper helper(cholder);
    XPCWrappedNative::GetNewOrUsed(ccx, helper, aScope, iface, getter_AddRefs(wrapper));
    if (!wrapper)
        return false;

    
    
    js::Value v = ObjectValue(*wrapper->GetFlatJSObject());
    if (!JS_WrapValue(ccx, &v))
        return false;

    jsid id = ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_COMPONENTS);
    return JS_DefinePropertyById(ccx, aGlobal, id, v, nsnull, nsnull,
                                 JSPROP_PERMANENT | JSPROP_READONLY);
}


NS_IMETHODIMP
nsXPCComponents::LookupMethod(const JS::Value& object,
                              const JS::Value& name,
                              JSContext *cx,
                              JS::Value *retval)
{
    NS_WARNING("Components.lookupMethod deprecated, use Components.utils.lookupMethod");

    nsCOMPtr<nsIXPCComponents_Utils> utils;
    nsresult rv = GetUtils(getter_AddRefs(utils));
    if (NS_FAILED(rv))
        return rv;

    return utils->LookupMethod(object, name, cx, retval);
}


NS_IMETHODIMP nsXPCComponents::ReportError(const JS::Value &error, JSContext *cx)
{
    NS_WARNING("Components.reportError deprecated, use Components.utils.reportError");

    nsCOMPtr<nsIXPCComponents_Utils> utils;
    nsresult rv = GetUtils(getter_AddRefs(utils));
    if (NS_FAILED(rv))
        return rv;

    return utils->ReportError(error, cx);
}


NS_IMETHODIMP
nsXPCComponents::CanCreateWrapper(const nsIID * iid, char **_retval)
{
    
    *_retval = xpc_CloneAllAccess();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
    static const char* allowed[] = { "isSuccessCode", "lookupMethod", nsnull };
    *_retval = xpc_CheckAccessList(methodName, allowed);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    static const char* allowed[] = { "interfaces", "interfacesByID", "results", nsnull};
    *_retval = xpc_CheckAccessList(propertyName, allowed);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
    
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents::PreCreate(nsISupports *nativeObj, JSContext *cx, JSObject *globalObj, JSObject **parentObj)
{
  
  if (!mScope) {
      NS_WARNING("mScope must not be null when nsXPCComponents::PreCreate is called");
      return NS_ERROR_FAILURE;
  }
  *parentObj = mScope->GetGlobalJSObject();
  return NS_OK;
}

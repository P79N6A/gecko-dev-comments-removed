







#include "xpcprivate.h"
#include "xpcIJSModuleLoader.h"
#include "XPCJSWeakReference.h"
#include "WrapperFactory.h"
#include "nsJSUtils.h"
#include "mozJSComponentLoader.h"
#include "nsContentUtils.h"
#include "JavaScriptParent.h"
#include "jsfriendapi.h"
#include "js/StructuredClone.h"
#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"
#include "nsJSEnvironment.h"
#include "mozilla/XPTInterfaceInfoManager.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/DOMExceptionBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/StructuredCloneTags.h"
#include "nsZipArchive.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsWindowMemoryReporter.h"
#include "nsDOMClassInfo.h"
#include "ShimInterfaceInfo.h"

using namespace mozilla;
using namespace JS;
using namespace js;
using namespace xpc;
using mozilla::dom::Exception;




nsresult
xpc::ThrowAndFail(nsresult errNum, JSContext *cx, bool *retval)
{
    XPCThrower::Throw(errNum, cx);
    *retval = false;
    return NS_OK;
}

static bool
JSValIsInterfaceOfType(JSContext *cx, HandleValue v, REFNSIID iid)
{

    nsCOMPtr<nsIXPConnectWrappedNative> wn;
    nsCOMPtr<nsISupports> sup;
    nsISupports* iface;

    if (v.isPrimitive())
        return false;

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    RootedObject obj(cx, &v.toObject());
    if (NS_SUCCEEDED(xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wn))) && wn &&
        NS_SUCCEEDED(wn->Native()->QueryInterface(iid, (void**)&iface)) && iface)
    {
        NS_RELEASE(iface);
        return true;
    }
    return false;
}

char *
xpc::CloneAllAccess()
{
    static const char allAccess[] = "AllAccess";
    return (char*)nsMemory::Clone(allAccess, sizeof(allAccess));
}

char *
xpc::CheckAccessList(const char16_t *wideName, const char *const list[])
{
    nsAutoCString asciiName;
    CopyUTF16toUTF8(nsDependentString(wideName), asciiName);

    for (const char* const* p = list; *p; p++)
        if (!strcmp(*p, asciiName.get()))
            return CloneAllAccess();

    return nullptr;
}






class nsXPCComponents_Interfaces MOZ_FINAL :
            public nsIXPCComponents_Interfaces,
            public nsIXPCScriptable,
            public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_INTERFACES
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_Interfaces();

private:
    virtual ~nsXPCComponents_Interfaces();

    nsCOMArray<nsIInterfaceInfo> mInterfaces;
};



NS_IMETHODIMP
nsXPCComponents_Interfaces::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_Interfaces)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Interfaces::GetHelperForLanguage(uint32_t language,
                                                 nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetFlags(uint32_t *aFlags)
{
    
    
    *aFlags = nsIClassInfo::DOM_OBJECT;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_Interfaces::nsXPCComponents_Interfaces()
{
}

nsXPCComponents_Interfaces::~nsXPCComponents_Interfaces()
{
    
}


NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Interfaces)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Interfaces)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Interfaces)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Interfaces)
NS_IMPL_RELEASE(nsXPCComponents_Interfaces)


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
                                         uint32_t enum_op, jsval * statep,
                                         jsid * idp, bool *_retval)
{
    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            
            
            if (mInterfaces.IsEmpty()) {
                XPTInterfaceInfoManager::GetSingleton()->
                    GetScriptableInterfaces(mInterfaces);
            }

            *statep = JSVAL_ZERO;
            if (idp)
                *idp = INT_TO_JSID(mInterfaces.Length());
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            uint32_t idx = statep->toInt32();
            nsIInterfaceInfo* interface = mInterfaces.SafeElementAt(idx);
            *statep = UINT_TO_JSVAL(idx + 1);

            if (interface) {
                const char* name;

                RootedId id(cx);
                if (NS_SUCCEEDED(interface->GetNameShared(&name)) && name) {
                    RootedString idstr(cx, JS_NewStringCopyZ(cx, name));
                    if (idstr && JS_StringToId(cx, idstr, &id)) {
                        *idp = id;
                        return NS_OK;
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_Interfaces::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                       JSContext *cx, JSObject *objArg,
                                       jsid idArg, JSObject **objp,
                                       bool *_retval)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);

    if (!JSID_IS_STRING(id))
        return NS_OK;

    JSAutoByteString name;
    RootedString str(cx, JSID_TO_STRING(id));

    
    if (name.encodeLatin1(cx, str) && name.ptr()[0] != '{') {
        nsCOMPtr<nsIInterfaceInfo> info =
            ShimInterfaceInfo::MaybeConstruct(name.ptr(), cx);
        if (!info) {
            XPTInterfaceInfoManager::GetSingleton()->
                GetInfoForName(name.ptr(), getter_AddRefs(info));
        }
        if (!info)
            return NS_OK;

        nsCOMPtr<nsIJSIID> nsid = nsJSIID::NewID(info);

        if (nsid) {
            nsXPConnect* xpc = nsXPConnect::XPConnect();
            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                             static_cast<nsIJSIID*>(nsid),
                                             NS_GET_IID(nsIJSIID),
                                             getter_AddRefs(holder)))) {
                RootedObject idobj(cx);
                if (holder &&
                    
                    (idobj = holder->GetJSObject())) {
                    *objp = obj;
                    *_retval = JS_DefinePropertyById(cx, obj, id, idobj,
                                                     JSPROP_ENUMERATE |
                                                     JSPROP_READONLY |
                                                     JSPROP_PERMANENT);
                }
            }
        }
    }
    return NS_OK;
}





class nsXPCComponents_InterfacesByID MOZ_FINAL :
            public nsIXPCComponents_InterfacesByID,
            public nsIXPCScriptable,
            public nsIClassInfo
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCCOMPONENTS_INTERFACESBYID
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSICLASSINFO

public:
    nsXPCComponents_InterfacesByID();

private:
    virtual ~nsXPCComponents_InterfacesByID();

    nsCOMArray<nsIInterfaceInfo> mInterfaces;
};




NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_InterfacesByID)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_InterfacesByID::GetHelperForLanguage(uint32_t language,
                                                     nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetFlags(uint32_t *aFlags)
{
    
    
    *aFlags = nsIClassInfo::DOM_OBJECT;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}

nsXPCComponents_InterfacesByID::nsXPCComponents_InterfacesByID()
{
}

nsXPCComponents_InterfacesByID::~nsXPCComponents_InterfacesByID()
{
    
}

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_InterfacesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_InterfacesByID)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_InterfacesByID)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_InterfacesByID)
NS_IMPL_RELEASE(nsXPCComponents_InterfacesByID)


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
                                             uint32_t enum_op, jsval * statep,
                                             jsid * idp, bool *_retval)
{
    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            
            
            if (mInterfaces.IsEmpty()) {
                XPTInterfaceInfoManager::GetSingleton()->
                    GetScriptableInterfaces(mInterfaces);
            }

            *statep = JSVAL_ZERO;
            if (idp)
                *idp = INT_TO_JSID(mInterfaces.Length());
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            uint32_t idx = statep->toInt32();
            nsIInterfaceInfo* interface = mInterfaces.SafeElementAt(idx);
            *statep = UINT_TO_JSVAL(idx + 1);
            if (interface) {
                nsIID const *iid;
                char idstr[NSID_LENGTH];

                if (NS_SUCCEEDED(interface->GetIIDShared(&iid))) {
                    iid->ToProvidedString(idstr);
                    RootedString jsstr(cx, JS_NewStringCopyZ(cx, idstr));
                    RootedId id(cx);
                    if (jsstr && JS_StringToId(cx, jsstr, &id)) {
                        *idp = id;
                        return NS_OK;
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_InterfacesByID::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                           JSContext *cx, JSObject *objArg,
                                           jsid idArg, JSObject **objp,
                                           bool *_retval)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);

    if (!JSID_IS_STRING(id))
        return NS_OK;

    RootedString str(cx, JSID_TO_STRING(id));
    if (38 != JS_GetStringLength(str))
        return NS_OK;

    JSAutoByteString utf8str;
    if (utf8str.encodeUtf8(cx, str)) {
        nsID iid;
        if (!iid.Parse(utf8str.ptr()))
            return NS_OK;

        nsCOMPtr<nsIInterfaceInfo> info;
        XPTInterfaceInfoManager::GetSingleton()->
            GetInfoForIID(&iid, getter_AddRefs(info));
        if (!info)
            return NS_OK;

        nsCOMPtr<nsIJSIID> nsid = nsJSIID::NewID(info);

        if (!nsid)
            return NS_ERROR_OUT_OF_MEMORY;

        nsXPConnect* xpc = nsXPConnect::XPConnect();
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                         static_cast<nsIJSIID*>(nsid),
                                            NS_GET_IID(nsIJSIID),
                                            getter_AddRefs(holder)))) {
            RootedObject idobj(cx);
            if (holder &&
                
                (idobj = holder->GetJSObject())) {
                *objp = obj;
                *_retval =
                    JS_DefinePropertyById(cx, obj, id, idobj,
                                          JSPROP_ENUMERATE |
                                          JSPROP_READONLY |
                                          JSPROP_PERMANENT);
            }
        }
    }
    return NS_OK;
}







class nsXPCComponents_Classes MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_Classes();
};




NS_IMETHODIMP
nsXPCComponents_Classes::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_Classes)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Classes::GetHelperForLanguage(uint32_t language,
                                              nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Classes::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Classes)
NS_IMPL_RELEASE(nsXPCComponents_Classes)


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
                                      uint32_t enum_op, jsval * statep,
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
            e = (nsISimpleEnumerator*) statep->toPrivate();

            if (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore &&
                NS_SUCCEEDED(e->GetNext(getter_AddRefs(isup))) && isup) {
                nsCOMPtr<nsISupportsCString> holder(do_QueryInterface(isup));
                if (holder) {
                    nsAutoCString name;
                    if (NS_SUCCEEDED(holder->GetData(name))) {
                        RootedString idstr(cx, JS_NewStringCopyN(cx, name.get(), name.Length()));
                        RootedId id(cx);
                        if (idstr && JS_StringToId(cx, idstr, &id)) {
                            *idp = id;
                            return NS_OK;
                        }
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsISimpleEnumerator*) statep->toPrivate();
            NS_IF_RELEASE(e);
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}


NS_IMETHODIMP
nsXPCComponents_Classes::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *objArg,
                                    jsid idArg, JSObject **objp,
                                    bool *_retval)

{
    RootedId id(cx, idArg);
    RootedObject obj(cx, objArg);

    JSAutoByteString name;
    if (JSID_IS_STRING(id) &&
        name.encodeLatin1(cx, JSID_TO_STRING(id)) &&
        name.ptr()[0] != '{') { 
        nsCOMPtr<nsIJSCID> nsid = nsJSCID::NewID(name.ptr());
        if (nsid) {
            nsXPConnect* xpc = nsXPConnect::XPConnect();
            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                             static_cast<nsIJSCID*>(nsid),
                                             NS_GET_IID(nsIJSCID),
                                             getter_AddRefs(holder)))) {
                RootedObject idobj(cx);
                if (holder &&
                    
                        (idobj = holder->GetJSObject())) {
                    *objp = obj;
                    *_retval = JS_DefinePropertyById(cx, obj, id, idobj,
                                                     JSPROP_ENUMERATE |
                                                     JSPROP_READONLY |
                                                     JSPROP_PERMANENT);
                }
            }
        }
    }
    return NS_OK;
}





class nsXPCComponents_ClassesByID MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_ClassesByID();
};




NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_ClassesByID)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_ClassesByID::GetHelperForLanguage(uint32_t language,
                                                  nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ClassesByID::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_ClassesByID)
NS_IMPL_RELEASE(nsXPCComponents_ClassesByID)


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
                                          uint32_t enum_op, jsval * statep,
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
            e = (nsISimpleEnumerator*) statep->toPrivate();

            if (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore &&
                NS_SUCCEEDED(e->GetNext(getter_AddRefs(isup))) && isup) {
                nsCOMPtr<nsISupportsID> holder(do_QueryInterface(isup));
                if (holder) {
                    char* name;
                    if (NS_SUCCEEDED(holder->ToString(&name)) && name) {
                        RootedString idstr(cx, JS_NewStringCopyZ(cx, name));
                        nsMemory::Free(name);
                        RootedId id(cx);
                        if (idstr && JS_StringToId(cx, idstr, &id)) {
                            *idp = id;
                            return NS_OK;
                        }
                    }
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            e = (nsISimpleEnumerator*) statep->toPrivate();
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
                                        JSContext *cx, JSObject *objArg,
                                        jsid idArg, JSObject **objp,
                                        bool *_retval)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);

    if (!JSID_IS_STRING(id))
        return NS_OK;

    JSAutoByteString name;
    RootedString str(cx, JSID_TO_STRING(id));
    if (name.encodeLatin1(cx, str) && name.ptr()[0] == '{' &&
        IsRegisteredCLSID(name.ptr())) 
    {
        nsCOMPtr<nsIJSCID> nsid = nsJSCID::NewID(name.ptr());
        if (nsid) {
            nsXPConnect* xpc = nsXPConnect::XPConnect();
            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            if (NS_SUCCEEDED(xpc->WrapNative(cx, obj,
                                             static_cast<nsIJSCID*>(nsid),
                                             NS_GET_IID(nsIJSCID),
                                             getter_AddRefs(holder)))) {
                RootedObject idobj(cx);
                if (holder &&
                    
                    (idobj = holder->GetJSObject())) {
                    *objp = obj;
                    *_retval = JS_DefinePropertyById(cx, obj, id, idobj,
                                                     JSPROP_ENUMERATE |
                                                     JSPROP_READONLY |
                                                     JSPROP_PERMANENT);
                }
            }
        }
    }
    return NS_OK;
}







class nsXPCComponents_Results MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_Results();
};




NS_IMETHODIMP
nsXPCComponents_Results::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_Results)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Results::GetHelperForLanguage(uint32_t language,
                                              nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Results::GetFlags(uint32_t *aFlags)
{
    
    
    *aFlags = nsIClassInfo::DOM_OBJECT;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Results)
NS_IMPL_RELEASE(nsXPCComponents_Results)


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
                                      uint32_t enum_op, jsval * statep,
                                      jsid * idp, bool *_retval)
{
    const void** iter;

    switch (enum_op) {
        case JSENUMERATE_INIT:
        case JSENUMERATE_INIT_ALL:
        {
            if (idp)
                *idp = INT_TO_JSID(nsXPCException::GetNSResultCount());

            void** space = (void**) new char[sizeof(void*)];
            *space = nullptr;
            *statep = PRIVATE_TO_JSVAL(space);
            return NS_OK;
        }
        case JSENUMERATE_NEXT:
        {
            const char* name;
            iter = (const void**) statep->toPrivate();
            if (nsXPCException::IterateNSResults(nullptr, &name, nullptr, iter)) {
                RootedString idstr(cx, JS_NewStringCopyZ(cx, name));
                JS::RootedId id(cx);
                if (idstr && JS_StringToId(cx, idstr, &id)) {
                    *idp = id;
                    return NS_OK;
                }
            }
            
        }

        case JSENUMERATE_DESTROY:
        default:
            iter = (const void**) statep->toPrivate();
            delete [] (char*) iter;
            *statep = JSVAL_NULL;
            return NS_OK;
    }
}



NS_IMETHODIMP
nsXPCComponents_Results::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, JSObject *objArg,
                                    jsid idArg, JSObject * *objp,
                                    bool *_retval)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    JSAutoByteString name;

    if (JSID_IS_STRING(id) && name.encodeLatin1(cx, JSID_TO_STRING(id))) {
        const char* rv_name;
        const void* iter = nullptr;
        nsresult rv;
        while (nsXPCException::IterateNSResults(&rv, &rv_name, nullptr, &iter)) {
            if (!strcmp(name.ptr(), rv_name)) {
                *objp = obj;
                if (!JS_DefinePropertyById(cx, obj, id, (uint32_t)rv,
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




class nsXPCComponents_ID MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_ID();
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, HandleObject obj,
                                    const CallArgs &args, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_ID::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_ID)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_ID::GetHelperForLanguage(uint32_t language,
                                         nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_ID)
NS_IMPL_RELEASE(nsXPCComponents_ID)


#define XPC_MAP_CLASSNAME           nsXPCComponents_ID
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_ID"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_ID::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx, JSObject *objArg,
                         const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


NS_IMETHODIMP
nsXPCComponents_ID::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx, JSObject *objArg,
                              const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


nsresult
nsXPCComponents_ID::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, HandleObject obj,
                                    const CallArgs &args, bool *_retval)
{
    

    if (args.length() < 1)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    

    if (NS_FAILED(nsXPConnect::SecurityManager()->CanCreateInstance(cx, nsJSID::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    

    JSString* jsstr;
    JSAutoByteString bytes;
    nsID id;

    if (!(jsstr = ToString(cx, args[0])) ||
        !bytes.encodeLatin1(cx, jsstr) ||
        !id.Parse(bytes.ptr())) {
        return ThrowAndFail(NS_ERROR_XPC_BAD_ID_STRING, cx, _retval);
    }

    

    JSObject* newobj = xpc_NewIDObject(cx, obj, id);
    if (!newobj)
        return NS_ERROR_UNEXPECTED;

    args.rval().setObject(*newobj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_ID::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                JSContext *cx, JSObject *obj,
                                HandleValue val, bool *bp, bool *_retval)
{
    if (bp)
        *bp = JSValIsInterfaceOfType(cx, val, NS_GET_IID(nsIJSID));
    return NS_OK;
}




class nsXPCComponents_Exception MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_Exception();
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, HandleObject obj,
                                    const CallArgs &args, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_Exception::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_Exception)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Exception::GetHelperForLanguage(uint32_t language,
                                                nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Exception)
NS_IMPL_RELEASE(nsXPCComponents_Exception)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Exception
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Exception"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Exception::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx, JSObject *objArg,
                                const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


NS_IMETHODIMP
nsXPCComponents_Exception::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                                     JSObject *objArg, const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}

struct MOZ_STACK_CLASS ExceptionArgParser
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

    
    bool parse(const CallArgs &args) {
        




















        if (args.length() > 0 && !parseMessage(args[0]))
            return false;
        if (args.length() > 1) {
            if (args[1].isObject()) {
                RootedObject obj(cx, &args[1].toObject());
                return parseOptionsObject(obj);
            }
            if (!parseResult(args[1]))
                return false;
        }
        if (args.length() > 2) {
            if (!parseStack(args[2]))
                return false;
        }
        if (args.length() > 3) {
            if (!parseData(args[3]))
                return false;
        }
        return true;
    }

  protected:

    



    bool parseMessage(HandleValue v) {
        JSString *str = ToString(cx, v);
        if (!str)
           return false;
        eMsg = messageBytes.encodeLatin1(cx, str);
        return !!eMsg;
    }

    bool parseResult(HandleValue v) {
        return JS::ToUint32(cx, v, (uint32_t*) &eResult);
    }

    bool parseStack(HandleValue v) {
        if (!v.isObject()) {
            
            
            return true;
        }

        return NS_SUCCEEDED(xpc->WrapJS(cx, &v.toObject(),
                                        NS_GET_IID(nsIStackFrame),
                                        getter_AddRefs(eStack)));
    }

    bool parseData(HandleValue v) {
        if (!v.isObject()) {
            
            
            return true;
        }

        return NS_SUCCEEDED(xpc->WrapJS(cx, &v.toObject(),
                                        NS_GET_IID(nsISupports),
                                        getter_AddRefs(eData)));
    }

    bool parseOptionsObject(HandleObject obj) {
        RootedValue v(cx);

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

    bool getOption(HandleObject obj, const char *name, MutableHandleValue rv) {
        
        bool found;
        if (!JS_HasProperty(cx, obj, name, &found))
            return false;

        
        if (!found) {
            rv.setUndefined();
            return true;
        }

        
        return JS_GetProperty(cx, obj, name, rv);
    }

    



    
    JSAutoByteString messageBytes;

    
    JSContext *cx;
    nsXPConnect *xpc;
};


nsresult
nsXPCComponents_Exception::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                           JSContext *cx, HandleObject obj,
                                           const CallArgs &args, bool *_retval)
{
    nsXPConnect* xpc = nsXPConnect::XPConnect();

    

    if (NS_FAILED(nsXPConnect::SecurityManager()->CanCreateInstance(cx, Exception::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    
    ExceptionArgParser parser(cx, xpc);
    if (!parser.parse(args))
        return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

    nsCOMPtr<nsIException> e = new Exception(nsCString(parser.eMsg),
                                             parser.eResult,
                                             EmptyCString(),
                                             parser.eStack,
                                             parser.eData);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    RootedObject newObj(cx);

    if (NS_FAILED(xpc->WrapNative(cx, obj, e, NS_GET_IID(nsIXPCException),
                                  getter_AddRefs(holder))) || !holder ||
        
        !(newObj = holder->GetJSObject())) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    args.rval().setObject(*newObj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Exception::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                       JSContext * cx, JSObject * obj,
                                       HandleValue val, bool *bp,
                                       bool *_retval)
{
    using namespace mozilla::dom;

    RootedValue v(cx, val);
    if (bp) {
        Exception* e;
        *bp = NS_SUCCEEDED(UNWRAP_OBJECT(Exception, v.toObjectOrNull(), e)) ||
              JSValIsInterfaceOfType(cx, v, NS_GET_IID(nsIException));
    }
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

private:
    virtual ~nsXPCConstructor();
    nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                             JSContext *cx, HandleObject obj,
                             const CallArgs &args, bool *_retval);
private:
    nsRefPtr<nsIJSCID> mClassID;
    nsRefPtr<nsIJSIID> mInterfaceID;
    char*              mInitializer;
};




NS_IMETHODIMP
nsXPCConstructor::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCConstructor)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCConstructor::GetHelperForLanguage(uint32_t language,
                                       nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
    : mClassID(aClassID),
      mInterfaceID(aInterfaceID)
{
    mInitializer = aInitializer ?
        (char*) nsMemory::Clone(aInitializer, strlen(aInitializer)+1) :
        nullptr;
}

nsXPCConstructor::~nsXPCConstructor()
{
    if (mInitializer)
        nsMemory::Free(mInitializer);
}


NS_IMETHODIMP
nsXPCConstructor::GetClassID(nsIJSCID * *aClassID)
{
    nsRefPtr<nsIJSCID> rval = mClassID;
    rval.forget(aClassID);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCConstructor::GetInterfaceID(nsIJSIID * *aInterfaceID)
{
    nsRefPtr<nsIJSIID> rval = mInterfaceID;
    rval.forget(aInterfaceID);
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCConstructor)
NS_IMPL_RELEASE(nsXPCConstructor)


#define XPC_MAP_CLASSNAME           nsXPCConstructor
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCConstructor"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCConstructor::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx, JSObject *objArg,
                       const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);

}


NS_IMETHODIMP
nsXPCConstructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx, JSObject *objArg,
                            const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


nsresult
nsXPCConstructor::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,JSContext *cx,
                                  HandleObject obj, const CallArgs &args, bool *_retval)
{
    nsXPConnect* xpc = nsXPConnect::XPConnect();

    
    

    nsCOMPtr<nsIXPConnectJSObjectHolder> cidHolder;
    nsCOMPtr<nsIXPConnectJSObjectHolder> iidHolder;
    RootedObject cidObj(cx);
    RootedObject iidObj(cx);

    if (NS_FAILED(xpc->WrapNative(cx, obj, mClassID, NS_GET_IID(nsIJSCID),
                                  getter_AddRefs(cidHolder))) || !cidHolder ||
        
        !(cidObj = cidHolder->GetJSObject()) ||
        NS_FAILED(xpc->WrapNative(cx, obj, mInterfaceID, NS_GET_IID(nsIJSIID),
                                  getter_AddRefs(iidHolder))) || !iidHolder ||
        
        !(iidObj = iidHolder->GetJSObject())) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    JS::Rooted<JS::Value> arg(cx, ObjectValue(*iidObj));
    RootedValue rval(cx);
    if (!JS_CallFunctionName(cx, cidObj, "createInstance", JS::HandleValueArray(arg), &rval) ||
        rval.isPrimitive()) {
        
        *_retval = false;
        return NS_OK;
    }

    args.rval().set(rval);

    
    if (mInitializer) {
        RootedObject newObj(cx, &rval.toObject());
        
        RootedValue fun(cx);
        if (!JS_GetProperty(cx, newObj, mInitializer, &fun) ||
            fun.isPrimitive()) {
            return ThrowAndFail(NS_ERROR_XPC_BAD_INITIALIZER_NAME, cx, _retval);
        }

        RootedValue dummy(cx);
        if (!JS_CallFunctionValue(cx, newObj, fun, args, &dummy)) {
            
            *_retval = false;
            return NS_OK;
        }
    }

    return NS_OK;
}




class nsXPCComponents_Constructor MOZ_FINAL :
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

private:
    virtual ~nsXPCComponents_Constructor();
    static nsresult CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                    JSContext *cx, HandleObject obj,
                                    const CallArgs &args, bool *_retval);
};




NS_IMETHODIMP
nsXPCComponents_Constructor::GetInterfaces(uint32_t *aCount, nsIID * **aArray)
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

    PUSH_IID(nsIXPCComponents_Constructor)
    PUSH_IID(nsIXPCScriptable)
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
nsXPCComponents_Constructor::GetHelperForLanguage(uint32_t language,
                                                  nsISupports **retval)
{
    *retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
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
    *aClassID = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::GetFlags(uint32_t *aFlags)
{
    *aFlags = 0;
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
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Constructor)
NS_IMPL_RELEASE(nsXPCComponents_Constructor)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Constructor
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Constructor"
#define                             XPC_MAP_WANT_CALL
#define                             XPC_MAP_WANT_CONSTRUCT
#define                             XPC_MAP_WANT_HASINSTANCE
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 



NS_IMETHODIMP
nsXPCComponents_Constructor::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                                  JSObject *objArg, const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


NS_IMETHODIMP
nsXPCComponents_Constructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                                       JSObject *objArg, const CallArgs &args, bool *_retval)
{
    RootedObject obj(cx, objArg);
    return CallOrConstruct(wrapper, cx, obj, args, _retval);
}


nsresult
nsXPCComponents_Constructor::CallOrConstruct(nsIXPConnectWrappedNative *wrapper,
                                             JSContext *cx, HandleObject obj,
                                             const CallArgs &args, bool *_retval)
{
    

    if (args.length() < 1)
        return ThrowAndFail(NS_ERROR_XPC_NOT_ENOUGH_ARGS, cx, _retval);

    

    nsXPConnect* xpc = nsXPConnect::XPConnect();
    XPCWrappedNativeScope* scope = ObjectScope(obj);
    nsCOMPtr<nsIXPCComponents> comp;

    if (!xpc || !scope || !(comp = do_QueryInterface(scope->GetComponents())))
        return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);

    

    if (NS_FAILED(nsXPConnect::SecurityManager()->CanCreateInstance(cx, nsXPCConstructor::GetCID()))) {
        
        *_retval = false;
        return NS_OK;
    }

    
    nsCOMPtr<nsIJSCID> cClassID;
    nsCOMPtr<nsIJSIID> cInterfaceID;
    const char*        cInitializer = nullptr;
    JSAutoByteString  cInitializerBytes;

    if (args.length() >= 3) {
        
        RootedString str(cx, ToString(cx, args[2]));
        if (!str || !(cInitializer = cInitializerBytes.encodeLatin1(cx, str)))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);
    }

    if (args.length() >= 2) {
        
        

        nsCOMPtr<nsIXPCComponents_Interfaces> ifaces;
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        RootedObject ifacesObj(cx);

        
        
        

        if (NS_FAILED(comp->GetInterfaces(getter_AddRefs(ifaces))) ||
            NS_FAILED(xpc->WrapNative(cx, obj, ifaces,
                                      NS_GET_IID(nsIXPCComponents_Interfaces),
                                      getter_AddRefs(holder))) || !holder ||
            
            !(ifacesObj = holder->GetJSObject())) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }

        RootedString str(cx, ToString(cx, args[1]));
        RootedId id(cx);
        if (!str || !JS_StringToId(cx, str, &id))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

        RootedValue val(cx);
        if (!JS_GetPropertyById(cx, ifacesObj, id, &val) || val.isPrimitive())
            return ThrowAndFail(NS_ERROR_XPC_BAD_IID, cx, _retval);

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        if (NS_FAILED(xpc->GetWrappedNativeOfJSObject(cx, &val.toObject(),
                                                      getter_AddRefs(wn))) || !wn ||
            !(cInterfaceID = do_QueryWrappedNative(wn))) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }
    } else {
        nsCOMPtr<nsIInterfaceInfo> info;
        xpc->GetInfoForIID(&NS_GET_IID(nsISupports), getter_AddRefs(info));

        if (info) {
            cInterfaceID = nsJSIID::NewID(info);
        }
        if (!cInterfaceID)
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
    }

    
    {
        
        

        
        
        

        nsCOMPtr<nsIXPCComponents_Classes> classes;
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        RootedObject classesObj(cx);

        if (NS_FAILED(comp->GetClasses(getter_AddRefs(classes))) ||
            NS_FAILED(xpc->WrapNative(cx, obj, classes,
                                      NS_GET_IID(nsIXPCComponents_Classes),
                                      getter_AddRefs(holder))) || !holder ||
            
            !(classesObj = holder->GetJSObject())) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }

        RootedString str(cx, ToString(cx, args[0]));
        RootedId id(cx);
        if (!str || !JS_StringToId(cx, str, &id))
            return ThrowAndFail(NS_ERROR_XPC_BAD_CONVERT_JS, cx, _retval);

        RootedValue val(cx);
        if (!JS_GetPropertyById(cx, classesObj, id, &val) || val.isPrimitive())
            return ThrowAndFail(NS_ERROR_XPC_BAD_CID, cx, _retval);

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        if (NS_FAILED(xpc->GetWrappedNativeOfJSObject(cx, val.toObjectOrNull(),
                                                      getter_AddRefs(wn))) || !wn ||
            !(cClassID = do_QueryWrappedNative(wn))) {
            return ThrowAndFail(NS_ERROR_XPC_UNEXPECTED, cx, _retval);
        }
    }

    nsCOMPtr<nsIXPCConstructor> ctor = new nsXPCConstructor(cClassID, cInterfaceID, cInitializer);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder2;
    RootedObject newObj(cx);

    if (NS_FAILED(xpc->WrapNative(cx, obj, ctor, NS_GET_IID(nsIXPCConstructor),
                                  getter_AddRefs(holder2))) || !holder2 ||
        
        !(newObj = holder2->GetJSObject())) {
        return ThrowAndFail(NS_ERROR_XPC_CANT_CREATE_WN, cx, _retval);
    }

    args.rval().setObject(*newObj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Constructor::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                         JSContext * cx, JSObject * obj,
                                         HandleValue val, bool *bp,
                                         bool *_retval)
{
    if (bp)
        *bp = JSValIsInterfaceOfType(cx, val, NS_GET_IID(nsIXPCConstructor));
    return NS_OK;
}

class nsXPCComponents_Utils MOZ_FINAL :
            public nsIXPCComponents_Utils,
            public nsIXPCScriptable
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSCRIPTABLE
    NS_DECL_NSIXPCCOMPONENTS_UTILS

public:
    nsXPCComponents_Utils() { }

private:
    virtual ~nsXPCComponents_Utils() { }
    nsCOMPtr<nsIXPCComponents_utils_Sandbox> mSandbox;
};

NS_INTERFACE_MAP_BEGIN(nsXPCComponents_Utils)
  NS_INTERFACE_MAP_ENTRY(nsIXPCComponents_Utils)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCComponents_Utils)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXPCComponents_Utils)
NS_IMPL_RELEASE(nsXPCComponents_Utils)


#define XPC_MAP_CLASSNAME           nsXPCComponents_Utils
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents_Utils"
#define XPC_MAP_FLAGS               nsIXPCScriptable::ALLOW_PROP_MODS_DURING_RESOLVE
#include "xpc_map_end.h" 

NS_IMETHODIMP
nsXPCComponents_Utils::GetSandbox(nsIXPCComponents_utils_Sandbox **aSandbox)
{
    NS_ENSURE_ARG_POINTER(aSandbox);
    if (!mSandbox)
        mSandbox = NewSandboxConstructor();

    nsCOMPtr<nsIXPCComponents_utils_Sandbox> rval = mSandbox;
    rval.forget(aSandbox);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ReportError(HandleValue error, JSContext *cx)
{
    

    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));

    nsCOMPtr<nsIScriptError> scripterr(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));

    if (!scripterr || !console)
        return NS_OK;

    const uint64_t innerWindowID = nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx);

    RootedObject errorObj(cx, error.isObject() ? &error.toObject() : nullptr);
    JSErrorReport *err = errorObj ? JS_ErrorFromException(cx, errorObj) : nullptr;
    if (err) {
        
        nsAutoString fileUni;
        CopyUTF8toUTF16(err->filename, fileUni);

        uint32_t column = err->uctokenptr - err->uclinebuf;

        const char16_t* ucmessage =
            static_cast<const char16_t*>(err->ucmessage);
        const char16_t* uclinebuf =
            static_cast<const char16_t*>(err->uclinebuf);

        nsresult rv = scripterr->InitWithWindowID(
                ucmessage ? nsDependentString(ucmessage) : EmptyString(),
                fileUni,
                uclinebuf ? nsDependentString(uclinebuf) : EmptyString(),
                err->lineno,
                column, err->flags, "XPConnect JavaScript", innerWindowID);
        NS_ENSURE_SUCCESS(rv, NS_OK);

        console->LogMessage(scripterr);
        return NS_OK;
    }

    
    RootedString msgstr(cx, ToString(cx, error));
    if (!msgstr)
        return NS_OK;

    nsCOMPtr<nsIStackFrame> frame;
    nsXPConnect *xpc = nsXPConnect::XPConnect();
    xpc->GetCurrentJSStack(getter_AddRefs(frame));

    nsString fileName;
    int32_t lineNo = 0;
    if (frame) {
        frame->GetFilename(fileName);
        frame->GetLineNumber(&lineNo);
    }

    nsAutoJSString msg;
    if (!msg.init(cx, msgstr))
        return NS_OK;

    nsresult rv = scripterr->InitWithWindowID(
            msg, fileName, EmptyString(), lineNo, 0, 0,
            "XPConnect JavaScript", innerWindowID);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    console->LogMessage(scripterr);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::EvalInSandbox(const nsAString& source,
                                     HandleValue sandboxVal,
                                     HandleValue version,
                                     const nsACString& filenameArg,
                                     int32_t lineNumber,
                                     JSContext *cx,
                                     uint8_t optionalArgc,
                                     MutableHandleValue retval)
{
    RootedObject sandbox(cx);
    if (!JS_ValueToObject(cx, sandboxVal, &sandbox) || !sandbox)
        return NS_ERROR_INVALID_ARG;

    
    JSVersion jsVersion = JSVERSION_DEFAULT;
    if (optionalArgc >= 1) {
        JSString *jsVersionStr = ToString(cx, version);
        if (!jsVersionStr)
            return NS_ERROR_INVALID_ARG;

        JSAutoByteString bytes(cx, jsVersionStr);
        if (!bytes)
            return NS_ERROR_INVALID_ARG;

        jsVersion = JS_StringToVersion(bytes.ptr());
        
        
        if (jsVersion == JSVERSION_UNKNOWN &&
            !strcmp(bytes.ptr(), "latest"))
        {
            jsVersion = JSVERSION_LATEST;
        }
        if (jsVersion == JSVERSION_UNKNOWN)
            return NS_ERROR_INVALID_ARG;
    }

    
    int32_t lineNo = (optionalArgc >= 3) ? lineNumber : 1;
    nsCString filename;
    if (!filenameArg.IsVoid()) {
        filename.Assign(filenameArg);
    } else {
        
        nsresult rv;
        nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIStackFrame> frame;
        xpc->GetCurrentJSStack(getter_AddRefs(frame));
        if (frame) {
            nsString frameFile;
            frame->GetFilename(frameFile);
            CopyUTF16toUTF8(frameFile, filename);
            frame->GetLineNumber(&lineNo);
        }
    }

    return xpc::EvalInSandbox(cx, sandbox, source, filename, lineNo,
                              jsVersion, retval);
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetSandboxAddonId(HandleValue sandboxVal,
                                         JSContext *cx, MutableHandleValue rval)
{
    if (!sandboxVal.isObject())
        return NS_ERROR_INVALID_ARG;

    RootedObject sandbox(cx, &sandboxVal.toObject());
    sandbox = js::CheckedUnwrap(sandbox);
    if (!sandbox || !xpc::IsSandbox(sandbox))
        return NS_ERROR_INVALID_ARG;

    return xpc::GetSandboxAddonId(cx, sandbox, rval);
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetSandboxMetadata(HandleValue sandboxVal,
                                          JSContext *cx, MutableHandleValue rval)
{
    if (!sandboxVal.isObject())
        return NS_ERROR_INVALID_ARG;

    RootedObject sandbox(cx, &sandboxVal.toObject());
    sandbox = js::CheckedUnwrap(sandbox);
    if (!sandbox || !xpc::IsSandbox(sandbox))
        return NS_ERROR_INVALID_ARG;

    return xpc::GetSandboxMetadata(cx, sandbox, rval);
}

NS_IMETHODIMP
nsXPCComponents_Utils::SetSandboxMetadata(HandleValue sandboxVal,
                                          HandleValue metadataVal,
                                          JSContext *cx)
{
    if (!sandboxVal.isObject())
        return NS_ERROR_INVALID_ARG;

    RootedObject sandbox(cx, &sandboxVal.toObject());
    sandbox = js::CheckedUnwrap(sandbox);
    if (!sandbox || !xpc::IsSandbox(sandbox))
        return NS_ERROR_INVALID_ARG;

    nsresult rv = xpc::SetSandboxMetadata(cx, sandbox, metadataVal);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}




NS_IMETHODIMP
nsXPCComponents_Utils::Import(const nsACString& registryLocation,
                              HandleValue targetObj,
                              JSContext* cx,
                              uint8_t optionalArgc,
                              MutableHandleValue retval)
{
    nsCOMPtr<xpcIJSModuleLoader> moduleloader =
        do_GetService(MOZJSCOMPONENTLOADER_CONTRACTID);
    if (!moduleloader)
        return NS_ERROR_FAILURE;
    return moduleloader->Import(registryLocation, targetObj, cx, optionalArgc, retval);
}



NS_IMETHODIMP
nsXPCComponents_Utils::IsModuleLoaded(const nsACString& registryLocation, bool *retval)
{
    nsCOMPtr<xpcIJSModuleLoader> moduleloader =
        do_GetService(MOZJSCOMPONENTLOADER_CONTRACTID);
    if (!moduleloader)
        return NS_ERROR_FAILURE;
    return moduleloader->IsModuleLoaded(registryLocation, retval);
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
nsXPCComponents_Utils::ImportGlobalProperties(HandleValue aPropertyList,
                                              JSContext* cx)
{
    RootedObject global(cx, CurrentGlobalOrNull(cx));
    MOZ_ASSERT(global);
    GlobalProperties options;
    NS_ENSURE_TRUE(aPropertyList.isObject(), NS_ERROR_INVALID_ARG);
    RootedObject propertyList(cx, &aPropertyList.toObject());
    NS_ENSURE_TRUE(JS_IsArrayObject(cx, propertyList), NS_ERROR_INVALID_ARG);
    if (!options.Parse(cx, propertyList) ||
        !options.Define(cx, global))
    {
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetWeakReference(HandleValue object, JSContext *cx,
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
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    PrepareForFullGC(rt);
    GCForReason(rt, gcreason::COMPONENT_UTILS);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForceCC()
{
    nsJSContext::CycleCollectNow();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::FinishCC()
{
    nsCycleCollector_finishAnyCurrentCollection();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CcSlice(int64_t budget)
{
    nsJSContext::RunCycleCollectorWorkSlice(budget);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetMaxCCSliceTimeSinceClear(int32_t *out)
{
    *out = nsJSContext::GetMaxCCSliceTimeSinceClear();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ClearMaxCCTime()
{
    nsJSContext::ClearMaxCCSliceTime();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForceShrinkingGC()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    PrepareForFullGC(rt);
    ShrinkingGC(rt, gcreason::COMPONENT_UTILS);
    return NS_OK;
}

class PreciseGCRunnable : public nsRunnable
{
  public:
    PreciseGCRunnable(ScheduledGCCallback* aCallback, bool aShrinking)
    : mCallback(aCallback), mShrinking(aShrinking) {}

    NS_IMETHOD Run()
    {
        JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();

        JSContext *cx;
        JSContext *iter = nullptr;
        while ((cx = JS_ContextIterator(rt, &iter)) != nullptr) {
            if (JS_IsRunning(cx)) {
                return NS_DispatchToMainThread(this);
            }
        }

        PrepareForFullGC(rt);
        if (mShrinking)
            ShrinkingGC(rt, gcreason::COMPONENT_UTILS);
        else
            GCForReason(rt, gcreason::COMPONENT_UTILS);

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
nsXPCComponents_Utils::UnlinkGhostWindows()
{
#ifdef DEBUG
    nsWindowMemoryReporter::UnlinkGhostWindows();
    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


NS_IMETHODIMP
nsXPCComponents_Utils::NondeterministicGetWeakMapKeys(HandleValue aMap,
                                                      JSContext *aCx,
                                                      MutableHandleValue aKeys)
{
    if (!aMap.isObject()) {
        aKeys.setUndefined();
        return NS_OK;
    }
    RootedObject objRet(aCx);
    RootedObject mapObj(aCx, &aMap.toObject());
    if (!JS_NondeterministicGetWeakMapKeys(aCx, mapObj, &objRet))
        return NS_ERROR_OUT_OF_MEMORY;
     aKeys.set(objRet ? ObjectValue(*objRet) : UndefinedValue());
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetJSTestingFunctions(JSContext *cx,
                                             MutableHandleValue retval)
{
    JSObject *obj = js::GetTestingFunctions(cx);
    if (!obj)
        return NS_ERROR_XPC_JAVASCRIPT_ERROR;
    retval.setObject(*obj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetGlobalForObject(HandleValue object,
                                          JSContext *cx,
                                          MutableHandleValue retval)
{
    
    if (object.isPrimitive())
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    
    
    
    
    
    Rooted<JSObject*> obj(cx, &object.toObject());
    obj = js::UncheckedUnwrap(obj);
    {
        JSAutoCompartment ac(cx, obj);
        obj = JS_GetGlobalForObject(cx, obj);
    }

    if (!JS_WrapObject(cx, &obj))
        return NS_ERROR_FAILURE;

    
    if (js::ObjectOp outerize = js::GetObjectClass(obj)->ext.outerObject)
      obj = outerize(cx, obj);

    retval.setObject(*obj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::IsProxy(HandleValue vobj, JSContext *cx, bool *rval)
{
    if (!vobj.isObject()) {
        *rval = false;
        return NS_OK;
    }

    RootedObject obj(cx, &vobj.toObject());
    obj = js::CheckedUnwrap(obj,  false);
    NS_ENSURE_TRUE(obj, NS_ERROR_FAILURE);

    *rval = js::IsScriptedProxy(obj);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ExportFunction(HandleValue vfunction, HandleValue vscope,
                                      HandleValue voptions, JSContext *cx,
                                      MutableHandleValue rval)
{
    if (!xpc::ExportFunction(cx, vfunction, vscope, voptions, rval))
        return NS_ERROR_FAILURE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::CreateObjectIn(HandleValue vobj, HandleValue voptions,
                                      JSContext *cx, MutableHandleValue rval)
{
    RootedObject optionsObject(cx, voptions.isObject() ? &voptions.toObject()
                                                       : nullptr);
    CreateObjectInOptions options(cx, optionsObject);
    if (voptions.isObject() &&
        !options.Parse())
    {
        return NS_ERROR_FAILURE;
    }

    if (!xpc::CreateObjectIn(cx, vobj, options, rval))
        return NS_ERROR_FAILURE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::MakeObjectPropsNormal(HandleValue vobj, JSContext *cx)
{
    if (!cx)
        return NS_ERROR_FAILURE;

    
    if (vobj.isPrimitive())
        return NS_ERROR_XPC_BAD_CONVERT_JS;

    RootedObject obj(cx, js::UncheckedUnwrap(&vobj.toObject()));
    JSAutoCompartment ac(cx, obj);
    AutoIdArray ida(cx, JS_Enumerate(cx, obj));
    if (!ida)
        return NS_ERROR_FAILURE;

    RootedId id(cx);
    RootedValue v(cx);
    for (size_t i = 0; i < ida.length(); ++i) {
        id = ida[i];

        if (!JS_GetPropertyById(cx, obj, id, &v))
            return NS_ERROR_FAILURE;

        if (v.isPrimitive())
            continue;

        RootedObject propobj(cx, &v.toObject());
        
        if (!js::IsWrapper(propobj) || !JS_ObjectIsCallable(cx, propobj))
            continue;

        if (!NewFunctionForwarder(cx, id, propobj, &v) ||
            !JS_SetPropertyById(cx, obj, id, v))
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::IsDeadWrapper(HandleValue obj, bool *out)
{
    *out = false;
    if (obj.isPrimitive())
        return NS_ERROR_INVALID_ARG;

    
    
    
    *out = JS_IsDeadWrapper(js::CheckedUnwrap(&obj.toObject()));
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::IsCrossProcessWrapper(HandleValue obj, bool *out)
{
    *out = false;
    if (obj.isPrimitive())
        return NS_ERROR_INVALID_ARG;

    *out = jsipc::IsWrappedCPOW(&obj.toObject());
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::RecomputeWrappers(HandleValue vobj, JSContext *cx)
{
    
    JSCompartment *c = vobj.isObject()
                       ? js::GetObjectCompartment(js::UncheckedUnwrap(&vobj.toObject()))
                       : nullptr;

    
    if (!c)
        js::RecomputeWrappers(cx, js::AllCompartments(), js::AllCompartments());
    
    else
        js::RecomputeWrappers(cx, js::SingleCompartment(c), js::AllCompartments()) &&
        js::RecomputeWrappers(cx, js::AllCompartments(), js::SingleCompartment(c));

    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::SetWantXrays(HandleValue vscope, JSContext *cx)
{
    if (!vscope.isObject())
        return NS_ERROR_INVALID_ARG;
    JSObject *scopeObj = js::UncheckedUnwrap(&vscope.toObject());
    JSCompartment *compartment = js::GetObjectCompartment(scopeObj);
    CompartmentPrivate::Get(scopeObj)->wantXrays = true;
    bool ok = js::RecomputeWrappers(cx, js::SingleCompartment(compartment),
                                    js::AllCompartments());
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForcePermissiveCOWs(JSContext *cx)
{
    CrashIfNotInAutomation();
    CompartmentPrivate::Get(CurrentGlobalOrNull(cx))->forcePermissiveCOWs = true;
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::ForcePrivilegedComponentsForScope(HandleValue vscope,
                                                         JSContext *cx)
{
    if (!vscope.isObject())
        return NS_ERROR_INVALID_ARG;
    CrashIfNotInAutomation();
    JSObject *scopeObj = js::UncheckedUnwrap(&vscope.toObject());
    XPCWrappedNativeScope *scope = ObjectScope(scopeObj);
    scope->ForcePrivilegedComponents();
    return NS_OK;
}


NS_IMETHODIMP
nsXPCComponents_Utils::GetComponentsForScope(HandleValue vscope, JSContext *cx,
                                             MutableHandleValue rval)
{
    if (!vscope.isObject())
        return NS_ERROR_INVALID_ARG;
    JSObject *scopeObj = js::UncheckedUnwrap(&vscope.toObject());
    XPCWrappedNativeScope *scope = ObjectScope(scopeObj);
    RootedObject components(cx);
    if (!scope->GetComponentsJSObject(&components))
        return NS_ERROR_FAILURE;
    if (!JS_WrapObject(cx, &components))
        return NS_ERROR_FAILURE;
    rval.setObject(*components);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::Dispatch(HandleValue runnableArg, HandleValue scope,
                                JSContext *cx)
{
    RootedValue runnable(cx, runnableArg);
    
    Maybe<JSAutoCompartment> ac;
    if (scope.isObject()) {
        JSObject *scopeObj = js::UncheckedUnwrap(&scope.toObject());
        if (!scopeObj)
            return NS_ERROR_FAILURE;
        ac.emplace(cx, scopeObj);
        if (!JS_WrapValue(cx, &runnable))
            return NS_ERROR_FAILURE;
    }

    
    if (!runnable.isObject())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIRunnable> run;
    nsresult rv = nsXPConnect::XPConnect()->WrapJS(cx, &runnable.toObject(),
                                                   NS_GET_IID(nsIRunnable),
                                                   getter_AddRefs(run));
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(run);

    
    return NS_DispatchToMainThread(run);
}

#define GENERATE_JSCONTEXTOPTION_GETTER_SETTER(_attr, _getter, _setter) \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Get## _attr(JSContext* cx, bool* aValue)     \
    {                                                                   \
        *aValue = ContextOptionsRef(cx)._getter();                      \
        return NS_OK;                                                   \
    }                                                                   \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Set## _attr(JSContext* cx, bool aValue)      \
    {                                                                   \
        ContextOptionsRef(cx)._setter(aValue);                          \
        return NS_OK;                                                   \
    }

#define GENERATE_JSRUNTIMEOPTION_GETTER_SETTER(_attr, _getter, _setter) \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Get## _attr(JSContext* cx, bool* aValue)     \
    {                                                                   \
        *aValue = RuntimeOptionsRef(cx)._getter();                      \
        return NS_OK;                                                   \
    }                                                                   \
    NS_IMETHODIMP                                                       \
    nsXPCComponents_Utils::Set## _attr(JSContext* cx, bool aValue)      \
    {                                                                   \
        RuntimeOptionsRef(cx)._setter(aValue);                          \
        return NS_OK;                                                   \
    }

GENERATE_JSRUNTIMEOPTION_GETTER_SETTER(Strict, extraWarnings, setExtraWarnings)
GENERATE_JSRUNTIMEOPTION_GETTER_SETTER(Werror, werror, setWerror)
GENERATE_JSRUNTIMEOPTION_GETTER_SETTER(Strict_mode, strictMode, setStrictMode)
GENERATE_JSRUNTIMEOPTION_GETTER_SETTER(Ion, ion, setIon)

#undef GENERATE_JSCONTEXTOPTION_GETTER_SETTER
#undef GENERATE_JSRUNTIMEOPTION_GETTER_SETTER

NS_IMETHODIMP
nsXPCComponents_Utils::SetGCZeal(int32_t aValue, JSContext* cx)
{
#ifdef JS_GC_ZEAL
    JS_SetGCZeal(cx, uint8_t(aValue), JS_DEFAULT_ZEAL_FREQ);
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::NukeSandbox(HandleValue obj, JSContext *cx)
{
    NS_ENSURE_TRUE(obj.isObject(), NS_ERROR_INVALID_ARG);
    JSObject *wrapper = &obj.toObject();
    NS_ENSURE_TRUE(IsWrapper(wrapper), NS_ERROR_INVALID_ARG);
    JSObject *sb = UncheckedUnwrap(wrapper);
    NS_ENSURE_TRUE(IsSandbox(sb), NS_ERROR_INVALID_ARG);
    NukeCrossCompartmentWrappers(cx, AllCompartments(),
                                 SingleCompartment(GetObjectCompartment(sb)),
                                 NukeWindowReferences);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::BlockScriptForGlobal(HandleValue globalArg,
                                            JSContext *cx)
{
    NS_ENSURE_TRUE(globalArg.isObject(), NS_ERROR_INVALID_ARG);
    RootedObject global(cx, UncheckedUnwrap(&globalArg.toObject(),
                                             false));
    NS_ENSURE_TRUE(JS_IsGlobalObject(global), NS_ERROR_INVALID_ARG);
    if (nsContentUtils::IsSystemPrincipal(xpc::GetObjectPrincipal(global))) {
        JS_ReportError(cx, "Script may not be disabled for system globals");
        return NS_ERROR_FAILURE;
    }
    Scriptability::Get(global).Block();
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::UnblockScriptForGlobal(HandleValue globalArg,
                                              JSContext *cx)
{
    NS_ENSURE_TRUE(globalArg.isObject(), NS_ERROR_INVALID_ARG);
    RootedObject global(cx, UncheckedUnwrap(&globalArg.toObject(),
                                             false));
    NS_ENSURE_TRUE(JS_IsGlobalObject(global), NS_ERROR_INVALID_ARG);
    if (nsContentUtils::IsSystemPrincipal(xpc::GetObjectPrincipal(global))) {
        JS_ReportError(cx, "Script may not be disabled for system globals");
        return NS_ERROR_FAILURE;
    }
    Scriptability::Get(global).Unblock();
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::IsXrayWrapper(HandleValue obj, bool* aRetval)
{
    *aRetval =
        obj.isObject() && xpc::WrapperFactory::IsXrayWrapper(&obj.toObject());
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::WaiveXrays(HandleValue aVal, JSContext *aCx, MutableHandleValue aRetval)
{
    RootedValue value(aCx, aVal);
    if (!xpc::WrapperFactory::WaiveXrayAndWrap(aCx, &value))
        return NS_ERROR_FAILURE;
    aRetval.set(value);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::UnwaiveXrays(HandleValue aVal, JSContext *aCx, MutableHandleValue aRetval)
{
    if (!aVal.isObject()) {
        aRetval.set(aVal);
        return NS_OK;
    }

    RootedObject obj(aCx, js::UncheckedUnwrap(&aVal.toObject()));
    if (!JS_WrapObject(aCx, &obj))
        return NS_ERROR_FAILURE;
    aRetval.setObject(*obj);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetClassName(HandleValue aObj, bool aUnwrap, JSContext *aCx, char **aRv)
{
    if (!aObj.isObject())
        return NS_ERROR_INVALID_ARG;
    RootedObject obj(aCx, &aObj.toObject());
    if (aUnwrap)
        obj = js::UncheckedUnwrap(obj,  false);
    *aRv = NS_strdup(js::GetObjectClass(obj)->name);
    NS_ENSURE_TRUE(*aRv, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetDOMClassInfo(const nsAString& aClassName,
                                       nsIClassInfo** aClassInfo)
{
    *aClassInfo = nullptr;
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetIncumbentGlobal(HandleValue aCallback,
                                          JSContext *aCx, MutableHandleValue aOut)
{
    nsCOMPtr<nsIGlobalObject> global = mozilla::dom::GetIncumbentGlobal();
    RootedValue globalVal(aCx);

    if (!global) {
        globalVal = NullValue();
    } else {
        
        globalVal = ObjectValue(*global->GetGlobalJSObject());
        if (!JS_WrapValue(aCx, &globalVal))
            return NS_ERROR_FAILURE;
    }

    
    if (aCallback.isObject()) {
        RootedValue ignored(aCx);
        if (!JS_CallFunctionValue(aCx, JS::NullPtr(), aCallback, JS::HandleValueArray(globalVal), &ignored))
            return NS_ERROR_FAILURE;
    }

    aOut.set(globalVal);
    return NS_OK;
}











class WrappedJSHolder : public nsISupports
{
    NS_DECL_ISUPPORTS
    WrappedJSHolder() {}

    nsRefPtr<nsXPCWrappedJS> mWrappedJS;

private:
    virtual ~WrappedJSHolder() {}
};
NS_IMPL_ISUPPORTS0(WrappedJSHolder);

NS_IMETHODIMP
nsXPCComponents_Utils::GenerateXPCWrappedJS(HandleValue aObj, HandleValue aScope,
                                            JSContext *aCx, nsISupports **aOut)
{
    if (!aObj.isObject())
        return NS_ERROR_INVALID_ARG;
    RootedObject obj(aCx, &aObj.toObject());
    RootedObject scope(aCx, aScope.isObject() ? js::UncheckedUnwrap(&aScope.toObject())
                                              : CurrentGlobalOrNull(aCx));
    JSAutoCompartment ac(aCx, scope);
    if (!JS_WrapObject(aCx, &obj))
        return NS_ERROR_FAILURE;

    nsRefPtr<WrappedJSHolder> holder = new WrappedJSHolder();
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(obj, NS_GET_IID(nsISupports),
                                               getter_AddRefs(holder->mWrappedJS));
    holder.forget(aOut);
    return rv;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetWatchdogTimestamp(const nsAString& aCategory, PRTime *aOut)
{
    WatchdogTimestampCategory category;
    if (aCategory.EqualsLiteral("RuntimeStateChange"))
        category = TimestampRuntimeStateChange;
    else if (aCategory.EqualsLiteral("WatchdogWakeup"))
        category = TimestampWatchdogWakeup;
    else if (aCategory.EqualsLiteral("WatchdogHibernateStart"))
        category = TimestampWatchdogHibernateStart;
    else if (aCategory.EqualsLiteral("WatchdogHibernateStop"))
        category = TimestampWatchdogHibernateStop;
    else
        return NS_ERROR_INVALID_ARG;
    *aOut = XPCJSRuntime::Get()->GetWatchdogTimestamp(category);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetJSEngineTelemetryValue(JSContext *cx, MutableHandleValue rval)
{
    RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    if (!obj)
        return NS_ERROR_OUT_OF_MEMORY;

    unsigned attrs = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

    size_t i = JS_SetProtoCalled(cx);
    RootedValue v(cx, DoubleValue(i));
    if (!JS_DefineProperty(cx, obj, "setProto", v, attrs))
        return NS_ERROR_OUT_OF_MEMORY;

    i = JS_GetCustomIteratorCount(cx);
    v.setDouble(i);
    if (!JS_DefineProperty(cx, obj, "customIter", v, attrs))
        return NS_ERROR_OUT_OF_MEMORY;

    rval.setObject(*obj);
    return NS_OK;
}

bool
xpc::CloneInto(JSContext *aCx, HandleValue aValue, HandleValue aScope,
               HandleValue aOptions, MutableHandleValue aCloned)
{
    if (!aScope.isObject())
        return false;

    RootedObject scope(aCx, &aScope.toObject());
    scope = js::CheckedUnwrap(scope);
    if(!scope) {
        JS_ReportError(aCx, "Permission denied to clone object into scope");
        return false;
    }

    if (!aOptions.isUndefined() && !aOptions.isObject()) {
        JS_ReportError(aCx, "Invalid argument");
        return false;
    }

    RootedObject optionsObject(aCx, aOptions.isObject() ? &aOptions.toObject()
                                                        : nullptr);
    StackScopedCloneOptions options(aCx, optionsObject);
    if (aOptions.isObject() && !options.Parse())
        return false;

    {
        JSAutoCompartment ac(aCx, scope);
        aCloned.set(aValue);
        if (!StackScopedClone(aCx, options, aCloned))
            return false;
    }

    return JS_WrapValue(aCx, aCloned);
}

NS_IMETHODIMP
nsXPCComponents_Utils::CloneInto(HandleValue aValue, HandleValue aScope,
                                 HandleValue aOptions, JSContext *aCx,
                                 MutableHandleValue aCloned)
{
    return xpc::CloneInto(aCx, aValue, aScope, aOptions, aCloned) ?
           NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetWebIDLCallerPrincipal(nsIPrincipal **aResult)
{
    
    
    
    nsCOMPtr<nsIPrincipal> callerPrin = mozilla::dom::GetWebIDLCallerPrincipal();
    if (!callerPrin)
        return NS_ERROR_NOT_AVAILABLE;
    callerPrin.forget(aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::GetObjectPrincipal(HandleValue val, JSContext *cx,
                                          nsIPrincipal **result)
{
    if (!val.isObject())
        return NS_ERROR_INVALID_ARG;
    RootedObject obj(cx, &val.toObject());
    obj = js::CheckedUnwrap(obj);
    MOZ_ASSERT(obj);

    nsCOMPtr<nsIPrincipal> prin = nsContentUtils::ObjectPrincipal(obj);
    prin.forget(result);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents_Utils::SetAddonInterposition(const nsACString &addonIdStr,
                                             nsIAddonInterposition *interposition,
                                             JSContext *cx)
{
    JSAddonId *addonId = xpc::NewAddonId(cx, addonIdStr);
    if (!addonId)
        return NS_ERROR_FAILURE;
    if (!XPCWrappedNativeScope::SetAddonInterposition(addonId, interposition))
        return NS_ERROR_FAILURE;
    return NS_OK;
}






nsXPCComponentsBase::nsXPCComponentsBase(XPCWrappedNativeScope* aScope)
    :   mScope(aScope)
{
    MOZ_ASSERT(aScope, "aScope must not be null");
}

nsXPCComponents::nsXPCComponents(XPCWrappedNativeScope* aScope)
    :   nsXPCComponentsBase(aScope)
{
}

nsXPCComponentsBase::~nsXPCComponentsBase()
{
}

nsXPCComponents::~nsXPCComponents()
{
}

void
nsXPCComponentsBase::ClearMembers()
{
    mInterfaces = nullptr;
    mInterfacesByID = nullptr;
    mResults = nullptr;
}

void
nsXPCComponents::ClearMembers()
{
    mClasses = nullptr;
    mClassesByID = nullptr;
    mID = nullptr;
    mException = nullptr;
    mConstructor = nullptr;
    mUtils = nullptr;

    nsXPCComponentsBase::ClearMembers();
}


#define XPC_IMPL_GET_OBJ_METHOD(_class, _n)                                   \
NS_IMETHODIMP _class::Get##_n(nsIXPCComponents_##_n * *a##_n) {               \
    NS_ENSURE_ARG_POINTER(a##_n);                                             \
    if (!m##_n)                                                               \
        m##_n = new nsXPCComponents_##_n();                                   \
    nsRefPtr<nsXPCComponents_##_n> ret = m##_n;                               \
    ret.forget(a##_n);                                                        \
    return NS_OK;                                                             \
}

XPC_IMPL_GET_OBJ_METHOD(nsXPCComponentsBase, Interfaces)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponentsBase, InterfacesByID)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, Classes)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, ClassesByID)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponentsBase, Results)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, ID)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, Exception)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, Constructor)
XPC_IMPL_GET_OBJ_METHOD(nsXPCComponents, Utils)

#undef XPC_IMPL_GET_OBJ_METHOD


NS_IMETHODIMP
nsXPCComponentsBase::IsSuccessCode(nsresult result, bool *out)
{
    *out = NS_SUCCEEDED(result);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents::GetStack(nsIStackFrame * *aStack)
{
    nsresult rv;
    nsXPConnect* xpc = nsXPConnect::XPConnect();
    rv = xpc->GetCurrentJSStack(aStack);
    return rv;
}

NS_IMETHODIMP
nsXPCComponents::GetManager(nsIComponentManager * *aManager)
{
    MOZ_ASSERT(aManager, "bad param");
    return NS_GetComponentManager(aManager);
}

NS_IMETHODIMP
nsXPCComponents::GetLastResult(JSContext *aCx, MutableHandleValue aOut)
{
    XPCContext* xpcc = XPCContext::GetXPCContext(aCx);
    if (!xpcc)
        return NS_ERROR_FAILURE;
    nsresult res = xpcc->GetLastResult();
    aOut.setNumber(static_cast<uint32_t>(res));
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents::GetReturnCode(JSContext *aCx, MutableHandleValue aOut)
{
    XPCContext* xpcc = XPCContext::GetXPCContext(aCx);
    if (!xpcc)
        return NS_ERROR_FAILURE;
    nsresult res = xpcc->GetPendingResult();
    aOut.setNumber(static_cast<uint32_t>(res));
    return NS_OK;
}

NS_IMETHODIMP
nsXPCComponents::SetReturnCode(JSContext *aCx, HandleValue aCode)
{
    XPCContext* xpcc = XPCContext::GetXPCContext(aCx);
    if (!xpcc)
        return NS_ERROR_FAILURE;
    nsresult rv;
    if (!ToUint32(aCx, aCode, (uint32_t*)&rv))
        return NS_ERROR_FAILURE;
    xpcc->SetPendingResult(rv);
    xpcc->SetLastResult(rv);
    return NS_OK;
}



NS_IMETHODIMP nsXPCComponents::ReportError(HandleValue error, JSContext *cx)
{
    NS_WARNING("Components.reportError deprecated, use Components.utils.reportError");

    nsCOMPtr<nsIXPCComponents_Utils> utils;
    nsresult rv = GetUtils(getter_AddRefs(utils));
    if (NS_FAILED(rv))
        return rv;

    return utils->ReportError(error, cx);
}



class ComponentsSH : public nsIXPCScriptable
{
public:
    explicit ComponentsSH(unsigned dummy)
    {
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSCRIPTABLE
    
    
    
    static NS_IMETHODIMP Get(uint32_t aLangId, nsISupports **helper)
    {
        *helper = &singleton;
        return NS_OK;
    }

private:
    static ComponentsSH singleton;
};

ComponentsSH ComponentsSH::singleton(0);


NS_IMETHODIMP_(MozExternalRefCountType) ComponentsSH::AddRef(void) { return 1; }
NS_IMETHODIMP_(MozExternalRefCountType) ComponentsSH::Release(void) { return 1; }

NS_INTERFACE_MAP_BEGIN(ComponentsSH)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

#define NSXPCCOMPONENTSBASE_CID \
{ 0xc62998e5, 0x95f1, 0x4058, \
  { 0xa5, 0x09, 0xec, 0x21, 0x66, 0x18, 0x92, 0xb9 } }

#define NSXPCCOMPONENTS_CID \
{ 0x3649f405, 0xf0ec, 0x4c28, \
    { 0xae, 0xb0, 0xaf, 0x9a, 0x51, 0xe4, 0x4c, 0x81 } }

NS_IMPL_CLASSINFO(nsXPCComponentsBase, &ComponentsSH::Get, nsIClassInfo::DOM_OBJECT, NSXPCCOMPONENTSBASE_CID)
NS_IMPL_ISUPPORTS_CI(nsXPCComponentsBase, nsIXPCComponentsBase)

NS_IMPL_CLASSINFO(nsXPCComponents, &ComponentsSH::Get, nsIClassInfo::DOM_OBJECT, NSXPCCOMPONENTS_CID)


NS_IMPL_ADDREF_INHERITED(nsXPCComponents, nsXPCComponentsBase)
NS_IMPL_RELEASE_INHERITED(nsXPCComponents, nsXPCComponentsBase)
NS_INTERFACE_MAP_BEGIN(nsXPCComponents)
    NS_INTERFACE_MAP_ENTRY(nsIXPCComponents)
    NS_IMPL_QUERY_CLASSINFO(nsXPCComponents)
NS_INTERFACE_MAP_END_INHERITING(nsXPCComponentsBase)
NS_IMPL_CI_INTERFACE_GETTER(nsXPCComponents, nsIXPCComponents)


#define XPC_MAP_CLASSNAME           ComponentsSH
#define XPC_MAP_QUOTED_CLASSNAME   "nsXPCComponents"
#define                             XPC_MAP_WANT_PRECREATE
#include "xpc_map_end.h" 

NS_IMETHODIMP
ComponentsSH::PreCreate(nsISupports *nativeObj, JSContext *cx, JSObject *globalObj, JSObject **parentObj)
{
  nsXPCComponentsBase *self = static_cast<nsXPCComponentsBase*>(nativeObj);
  
  if (!self->GetScope()) {
      NS_WARNING("mScope must not be null when nsXPCComponents::PreCreate is called");
      return NS_ERROR_FAILURE;
  }
  *parentObj = self->GetScope()->GetGlobalJSObject();
  return NS_OK;
}

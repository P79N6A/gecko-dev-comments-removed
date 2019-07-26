





#include "nsCOMPtr.h"
#include "nsIScriptSecurityManager.h"
#include "nsScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsPrincipal.h"
#include "nsSystemPrincipal.h"
#include "nsNullPrincipal.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIScriptContext.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsNetCID.h"
#include "nsIClassInfoImpl.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocument.h"
#include "jsfriendapi.h"





nsSecurityNameSet::nsSecurityNameSet()
{
}

nsSecurityNameSet::~nsSecurityNameSet()
{
}

NS_IMPL_ISUPPORTS1(nsSecurityNameSet, nsIScriptExternalNameSet)

static JSString *
getStringArgument(JSContext *cx, JSObject *obj, uint16_t argNum, unsigned argc, jsval *argv)
{
    if (argc <= argNum || !JSVAL_IS_STRING(argv[argNum])) {
        JS_ReportError(cx, "String argument expected");
        return nullptr;
    }

    



    return JSVAL_TO_STRING(argv[argNum]);
}

static bool
getBytesArgument(JSContext *cx, JSObject *obj, uint16_t argNum, unsigned argc, jsval *argv,
                 JSAutoByteString *bytes)
{
    JSString *str = getStringArgument(cx, obj, argNum, argc, argv);
    return str && bytes->encode(cx, str);
}

static JSBool
netscape_security_enablePrivilege(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    JSAutoByteString cap;
    if (!getBytesArgument(cx, obj, 0, argc, JS_ARGV(cx, vp), &cap))
        return JS_FALSE;

    
    
    {
        JSAutoEnterCompartment ac;
        if (ac.enter(cx, obj)) {
            nsCOMPtr<nsPIDOMWindow> win =
                do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(cx, obj));
            if (win) {
                nsCOMPtr<nsIDocument> doc =
                    do_QueryInterface(win->GetExtantDocument());
                if (doc) {
                    doc->WarnOnceAbout(nsIDocument::eEnablePrivilege);
                }
            }
        }
    }

    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return JS_FALSE;

    

    rv = securityManager->EnableCapability(cap.ptr());
    if (NS_FAILED(rv))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSFunctionSpec PrivilegeManager_static_methods[] = {
    JS_FS("enablePrivilege", netscape_security_enablePrivilege, 1, 0),
    JS_FS_END
};





NS_IMETHODIMP 
nsSecurityNameSet::InitializeNameSet(nsIScriptContext* aScriptContext)
{
    JSContext* cx = aScriptContext->GetNativeContext();
    JSObject *global = JS_ObjectToInnerObject(cx, JS_GetGlobalObject(cx));

    



    JSObject *obj = global;
    JSObject *proto;
    JSAutoRequest ar(cx);
    while ((proto = JS_GetPrototype(obj)) != nullptr)
        obj = proto;
    JSClass *objectClass = JS_GetClass(obj);

    JS::Value v;
    if (!JS_GetProperty(cx, global, "netscape", &v))
        return NS_ERROR_FAILURE;

    JSObject *securityObj;
    if (v.isObject()) {
        



        obj = &v.toObject();
        if (!JS_GetProperty(cx, obj, "security", &v) || !v.isObject())
            return NS_ERROR_FAILURE;
        securityObj = &v.toObject();
    } else {
        
        obj = JS_DefineObject(cx, global, "netscape", objectClass, nullptr, 0);
        if (obj == nullptr)
            return NS_ERROR_FAILURE;
        securityObj = JS_DefineObject(cx, obj, "security", objectClass,
                                      nullptr, 0);
        if (securityObj == nullptr)
            return NS_ERROR_FAILURE;
    }

    
    obj = JS_DefineObject(cx, securityObj, "PrivilegeManager", objectClass,
                          nullptr, 0);
    if (obj == nullptr)
        return NS_ERROR_FAILURE;

    return JS_DefineFunctions(cx, obj, PrivilegeManager_static_methods)
           ? NS_OK
           : NS_ERROR_FAILURE;
}

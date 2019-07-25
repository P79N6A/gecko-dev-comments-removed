





































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
#include "jsobj.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocument.h"





nsSecurityNameSet::nsSecurityNameSet()
{
}

nsSecurityNameSet::~nsSecurityNameSet()
{
}

NS_IMPL_ISUPPORTS1(nsSecurityNameSet, nsIScriptExternalNameSet)

static JSString *
getStringArgument(JSContext *cx, JSObject *obj, PRUint16 argNum, uintN argc, jsval *argv)
{
    if (argc <= argNum || !JSVAL_IS_STRING(argv[argNum])) {
        JS_ReportError(cx, "String argument expected");
        return nsnull;
    }

    



    return JSVAL_TO_STRING(argv[argNum]);
}

static bool
getBytesArgument(JSContext *cx, JSObject *obj, PRUint16 argNum, uintN argc, jsval *argv,
                 JSAutoByteString *bytes)
{
    JSString *str = getStringArgument(cx, obj, argNum, argc, argv);
    return str && bytes->encode(cx, str);
}

static void
getUTF8StringArgument(JSContext *cx, JSObject *obj, PRUint16 argNum,
                      uintN argc, jsval *argv, nsCString& aRetval)
{
    aRetval.Truncate();

    if (argc <= argNum || !JSVAL_IS_STRING(argv[argNum])) {
        JS_ReportError(cx, "String argument expected");
        return;
    }

    



    JSString *str = JSVAL_TO_STRING(argv[argNum]);
    if (!str)
        return;

    const PRUnichar *data = JS_GetStringCharsZ(cx, str);
    if (!data)
        return;

    CopyUTF16toUTF8(data, aRetval);
}

static JSBool
netscape_security_isPrivilegeEnabled(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    JSBool result = JS_FALSE;
    if (JSString *str = getStringArgument(cx, obj, 0, argc, JS_ARGV(cx, vp))) {
        JSAutoByteString cap(cx, str);
        if (!cap)
            return JS_FALSE;

        nsresult rv;
        nsCOMPtr<nsIScriptSecurityManager> securityManager = 
                 do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            

            rv = securityManager->IsCapabilityEnabled(cap.ptr(), &result);
            if (NS_FAILED(rv)) 
                result = JS_FALSE;
        }
    }
    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(result));
    return JS_TRUE;
}


static JSBool
netscape_security_enablePrivilege(JSContext *cx, uintN argc, jsval *vp)
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

static JSBool
netscape_security_disablePrivilege(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    JSAutoByteString cap;
    if (!getBytesArgument(cx, obj, 0, argc, JS_ARGV(cx, vp), &cap))
        return JS_FALSE;

    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return JS_FALSE;

    

    rv = securityManager->DisableCapability(cap.ptr());
    if (NS_FAILED(rv))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
netscape_security_revertPrivilege(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    JSAutoByteString cap;
    if (!getBytesArgument(cx, obj, 0, argc, JS_ARGV(cx, vp), &cap))
        return JS_FALSE;

    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return JS_FALSE;

    

    rv = securityManager->RevertCapability(cap.ptr());
    if (NS_FAILED(rv))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
netscape_security_setCanEnablePrivilege(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    if (argc < 2) return JS_FALSE;
    nsCAutoString principalFingerprint;
    getUTF8StringArgument(cx, obj, 0, argc, JS_ARGV(cx, vp), principalFingerprint);
    JSAutoByteString cap;
    getBytesArgument(cx, obj, 1, argc, JS_ARGV(cx, vp), &cap);
    if (principalFingerprint.IsEmpty() || !cap)
        return JS_FALSE;

    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return JS_FALSE;

    

    rv = securityManager->SetCanEnableCapability(principalFingerprint,
                                                 cap.ptr(), 
                                                 nsIPrincipal::ENABLE_GRANTED);
    if (NS_FAILED(rv))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
netscape_security_invalidate(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;

    nsCAutoString principalFingerprint;
    getUTF8StringArgument(cx, obj, 0, argc, JS_ARGV(cx, vp), principalFingerprint);
    if (principalFingerprint.IsEmpty())
        return JS_FALSE;

    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return JS_FALSE;

    

    rv = securityManager->SetCanEnableCapability(principalFingerprint,
                                                 nsPrincipal::sInvalid,
                                                 nsIPrincipal::ENABLE_GRANTED);
    if (NS_FAILED(rv))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSFunctionSpec PrivilegeManager_static_methods[] = {
    { "isPrivilegeEnabled", netscape_security_isPrivilegeEnabled,   1,0},
    { "enablePrivilege",    netscape_security_enablePrivilege,      1,0},
    { "disablePrivilege",   netscape_security_disablePrivilege,     1,0},
    { "revertPrivilege",    netscape_security_revertPrivilege,      1,0},
    
    { "setCanEnablePrivilege", netscape_security_setCanEnablePrivilege,
                                                                    2,0},
    { "invalidate",            netscape_security_invalidate,        1,0},
    {nsnull,nsnull,0,0}
};





NS_IMETHODIMP 
nsSecurityNameSet::InitializeNameSet(nsIScriptContext* aScriptContext)
{
    JSContext *cx = (JSContext *) aScriptContext->GetNativeContext();
    JSObject *global = JS_GetGlobalObject(cx);
    OBJ_TO_INNER_OBJECT(cx, global);

    



    JSObject *obj = global;
    JSObject *proto;
    JSAutoRequest ar(cx);
    while ((proto = JS_GetPrototype(cx, obj)) != nsnull)
        obj = proto;
    JSClass *objectClass = JS_GET_CLASS(cx, obj);

    jsval v;
    if (!JS_GetProperty(cx, global, "netscape", &v))
        return NS_ERROR_FAILURE;
    JSObject *securityObj;
    if (JSVAL_IS_OBJECT(v)) {
        



        obj = JSVAL_TO_OBJECT(v);
        if (!JS_GetProperty(cx, obj, "security", &v) || !JSVAL_IS_OBJECT(v))
            return NS_ERROR_FAILURE;
        securityObj = JSVAL_TO_OBJECT(v);
    } else {
        
        obj = JS_DefineObject(cx, global, "netscape", objectClass, nsnull, 0);
        if (obj == nsnull)
            return NS_ERROR_FAILURE;
        securityObj = JS_DefineObject(cx, obj, "security", objectClass,
                                      nsnull, 0);
        if (securityObj == nsnull)
            return NS_ERROR_FAILURE;
    }

    
    obj = JS_DefineObject(cx, securityObj, "PrivilegeManager", objectClass,
                          nsnull, 0);
    if (obj == nsnull)
        return NS_ERROR_FAILURE;

    return JS_DefineFunctions(cx, obj, PrivilegeManager_static_methods)
           ? NS_OK
           : NS_ERROR_FAILURE;
}

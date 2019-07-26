





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
#include "xpcprivate.h"
#include "nsCxPusher.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;





nsSecurityNameSet::nsSecurityNameSet()
{
}

nsSecurityNameSet::~nsSecurityNameSet()
{
}

NS_IMPL_ISUPPORTS1(nsSecurityNameSet, nsIScriptExternalNameSet)

static bool
netscape_security_enablePrivilege(JSContext *cx, unsigned argc, JS::Value *vp)
{
    Telemetry::Accumulate(Telemetry::ENABLE_PRIVILEGE_EVER_CALLED, true);
    return xpc::EnableUniversalXPConnect(cx);
}

static const JSFunctionSpec PrivilegeManager_static_methods[] = {
    JS_FS("enablePrivilege", netscape_security_enablePrivilege, 1, 0),
    JS_FS_END
};





NS_IMETHODIMP
nsSecurityNameSet::InitializeNameSet(nsIScriptContext* aScriptContext)
{
    AutoJSContext cx;
    JS::Rooted<JSObject*> global(cx, aScriptContext->GetWindowProxy());
    JSAutoCompartment ac(cx, global);

    



    JS::Rooted<JSObject*> obj(cx, global);
    JS::Rooted<JSObject*> proto(cx);
    for (;;) {
        MOZ_ALWAYS_TRUE(JS_GetPrototype(cx, obj, &proto));
        if (!proto)
            break;
        obj = proto;
    }
    JSClass *objectClass = JS_GetClass(obj);

    JS::Rooted<JS::Value> v(cx);
    if (!JS_GetProperty(cx, global, "netscape", &v))
        return NS_ERROR_FAILURE;

    JS::Rooted<JSObject*> securityObj(cx);
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

    
    
    
    
    
    
    if (!Preferences::GetBool("security.turn_off_all_security_so_that_viruses_can_take_over_this_computer"))
        return NS_OK;

    
    obj = JS_DefineObject(cx, securityObj, "PrivilegeManager", objectClass,
                          nullptr, 0);
    if (obj == nullptr)
        return NS_ERROR_FAILURE;

    return JS_DefineFunctions(cx, obj, PrivilegeManager_static_methods)
           ? NS_OK
           : NS_ERROR_FAILURE;
}

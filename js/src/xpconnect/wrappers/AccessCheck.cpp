






































#include "AccessCheck.h"

#include "nsJSPrincipals.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsContentUtils.h"

#include "XPCWrapper.h"
#include "XrayWrapper.h"
#include "FilteringWrapper.h"
#include "WrapperFactory.h"

#include "jsfriendapi.h"

namespace xpc {

static nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return compartment->principals ? static_cast<nsJSPrincipals *>(compartment->principals)->nsIPrincipalPtr : 0;
}

bool
AccessCheck::isSameOrigin(JSCompartment *a, JSCompartment *b)
{
    nsIPrincipal *aprin = GetCompartmentPrincipal(a);
    nsIPrincipal *bprin = GetCompartmentPrincipal(b);

    
    
    
    if (!aprin || !bprin)
        return true;

    PRBool equals;
    nsresult rv = aprin->EqualsIgnoringDomain(bprin, &equals);
    if (NS_FAILED(rv)) {
        NS_ERROR("unable to ask about equality");
        return false;
    }

    return equals;
}

bool
AccessCheck::isLocationObjectSameOrigin(JSContext *cx, JSObject *wrapper)
{
    JSObject *obj = wrapper->unwrap()->getParent();
    if (!obj->getClass()->ext.innerObject) {
        obj = obj->unwrap();
        JS_ASSERT(obj->getClass()->ext.innerObject);
    }
    OBJ_TO_INNER_OBJECT(cx, obj);
    return obj &&
           (isSameOrigin(wrapper->compartment(), obj->compartment()) ||
            documentDomainMakesSameOrigin(cx, obj));
}

bool
AccessCheck::isChrome(JSCompartment *compartment)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return false;
    }

    PRBool privileged;
    nsIPrincipal *principal = GetCompartmentPrincipal(compartment);
    return NS_SUCCEEDED(ssm->IsSystemPrincipal(principal, &privileged)) && privileged;
}

nsIPrincipal *
AccessCheck::getPrincipal(JSCompartment *compartment)
{
    return GetCompartmentPrincipal(compartment);
}

#define NAME(ch, str, cases)                                                  \
    case ch: if (!strcmp(name, str)) switch (propChars[0]) { cases }; break;
#define PROP(ch, actions) case ch: { actions }; break;
#define RW(str) if (JS_FlatStringEqualsAscii(prop, str)) return true;
#define R(str) if (!set && JS_FlatStringEqualsAscii(prop, str)) return true;
#define W(str) if (set && JS_FlatStringEqualsAscii(prop, str)) return true;




static bool
IsPermitted(const char *name, JSFlatString *prop, bool set)
{
    size_t propLength;
    const jschar *propChars = JS_GetInternedStringCharsAndLength(prop, &propLength);
    if (!propLength)
        return false;
    switch(name[0]) {
        NAME('D', "DOMException",
             PROP('c', RW("code"))
             PROP('m', RW("message"))
             PROP('n', RW("name"))
             PROP('r', RW("result"))
             PROP('t', R("toString")))
        NAME('E', "Error",
             PROP('m', R("message")))
        NAME('H', "History",
             PROP('b', R("back"))
             PROP('f', R("forward"))
             PROP('g', R("go")))
        NAME('L', "Location",
             PROP('h', W("hash") W("href"))
             PROP('r', R("replace")))
        NAME('N', "Navigator",
             PROP('p', RW("preference")))
        NAME('W', "Window",
             PROP('b', R("blur"))
             PROP('c', R("close") R("closed"))
             PROP('f', R("focus") R("frames"))
             PROP('h', R("history"))
             PROP('l', RW("location") R("length"))
             PROP('o', R("opener"))
             PROP('p', R("parent") R("postMessage"))
             PROP('s', R("self"))
             PROP('t', R("top"))
             PROP('w', R("window")))
    }
    return false;
}

#undef NAME
#undef RW
#undef R
#undef W

static bool
IsFrameId(JSContext *cx, JSObject *obj, jsid id)
{
    XPCWrappedNative *wn = XPCWrappedNative::GetWrappedNativeOfJSObject(cx, obj);
    if (!wn) {
        return false;
    }

    nsCOMPtr<nsIDOMWindow> domwin(do_QueryWrappedNative(wn));
    if (!domwin) {
        return false;
    }

    nsCOMPtr<nsIDOMWindowCollection> col;
    domwin->GetFrames(getter_AddRefs(col));
    if (!col) {
        return false;
    }

    if (JSID_IS_INT(id)) {
        col->Item(JSID_TO_INT(id), getter_AddRefs(domwin));
    } else if (JSID_IS_ATOM(id)) {
        nsAutoString str(JS_GetInternedStringChars(JSID_TO_STRING(id)));
        col->NamedItem(str, getter_AddRefs(domwin));
    } else {
        return false;
    }

    return domwin != nsnull;
}

static bool
IsWindow(const char *name)
{
    return name[0] == 'W' && !strcmp(name, "Window");
}

static bool
IsLocation(const char *name)
{
    return name[0] == 'L' && !strcmp(name, "Location");
}

static nsIPrincipal *
GetPrincipal(JSObject *obj)
{
    NS_ASSERTION(!IS_SLIM_WRAPPER(obj), "global object is a slim wrapper?");
    if (!IS_WN_WRAPPER(obj)) {
        NS_ASSERTION(!(~obj->getClass()->flags &
                       (JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_HAS_PRIVATE)),
                     "bad object");
        nsCOMPtr<nsIScriptObjectPrincipal> objPrin =
            do_QueryInterface((nsISupports*)xpc_GetJSPrivate(obj));
        NS_ASSERTION(objPrin, "global isn't nsIScriptObjectPrincipal?");
        return objPrin->GetPrincipal();
    }

    nsIXPConnect *xpc = nsXPConnect::GetRuntimeInstance()->GetXPConnect();
    return xpc->GetPrincipal(obj, PR_TRUE);
}

bool
AccessCheck::documentDomainMakesSameOrigin(JSContext *cx, JSObject *obj)
{
    JSObject *scope = nsnull;
    JSStackFrame *fp = nsnull;
    JS_FrameIterator(cx, &fp);
    if (fp) {
        while (!JS_IsScriptFrame(cx, fp)) {
            if (!JS_FrameIterator(cx, &fp))
                break;
        }

        if (fp)
            scope = JS_GetFrameScopeChainRaw(fp);
    }

    if (!scope)
        scope = JS_GetScopeChain(cx);

    nsIPrincipal *subject;
    nsIPrincipal *object;

    {
        JSAutoEnterCompartment ac;

        if (!ac.enter(cx, scope))
            return false;

        subject = GetPrincipal(JS_GetGlobalForObject(cx, scope));
    }

    if (!subject)
        return false;

    {
        JSAutoEnterCompartment ac;

        if (!ac.enter(cx, obj))
            return false;

        object = GetPrincipal(JS_GetGlobalForObject(cx, obj));
    }

    PRBool subsumes;
    return NS_SUCCEEDED(subject->Subsumes(object, &subsumes)) && subsumes;
}

bool
AccessCheck::isCrossOriginAccessPermitted(JSContext *cx, JSObject *wrapper, jsid id,
                                          JSWrapper::Action act)
{
    if (!XPCWrapper::GetSecurityManager())
        return true;

    if (act == JSWrapper::CALL)
        return true;

    JSObject *obj = JSWrapper::wrappedObject(wrapper);

    const char *name;
    js::Class *clasp = obj->getClass();
    NS_ASSERTION(Jsvalify(clasp) != &XrayUtils::HolderClass, "shouldn't have a holder here");
    if (clasp->ext.innerObject)
        name = "Window";
    else
        name = clasp->name;

    if (JSID_IS_ATOM(id)) {
        if (IsPermitted(name, JSID_TO_FLAT_STRING(id), act == JSWrapper::SET))
            return true;
    }

    if (IsWindow(name) && IsFrameId(cx, obj, id))
        return true;

    
    
    if (!IsLocation(name) && documentDomainMakesSameOrigin(cx, obj))
        return true;

    return (act == JSWrapper::SET)
           ? nsContentUtils::IsCallerTrustedForWrite()
           : nsContentUtils::IsCallerTrustedForRead();
}

bool
AccessCheck::isSystemOnlyAccessPermitted(JSContext *cx)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return true;
    }

    JSStackFrame *fp;
    nsIPrincipal *principal = ssm->GetCxSubjectPrincipalAndFrame(cx, &fp);
    if (!principal) {
        return false;
    }

    if (!fp) {
        if (!JS_FrameIterator(cx, &fp)) {
            
            
            return true;
        }

        
        
        fp = NULL;
    } else if (!JS_IsScriptFrame(cx, fp)) {
        fp = NULL;
    }

    PRBool privileged;
    if (NS_SUCCEEDED(ssm->IsSystemPrincipal(principal, &privileged)) &&
        privileged) {
        return true;
    }

    
    
    static const char prefix[] = "chrome://global/";
    const char *filename;
    if (fp &&
        (filename = JS_GetFrameScript(cx, fp)->filename) &&
        !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
        return true;
    }

    return NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) && privileged;
}

bool
AccessCheck::needsSystemOnlyWrapper(JSObject *obj)
{
    if (!IS_WN_WRAPPER(obj))
        return false;

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(obj->getPrivate());
    return wn->NeedsSOW();
}

bool
AccessCheck::isScriptAccessOnly(JSContext *cx, JSObject *wrapper)
{
    JS_ASSERT(wrapper->isWrapper());

    uintN flags;
    JSObject *obj = wrapper->unwrap(&flags);

    
    if (flags & WrapperFactory::SCRIPT_ACCESS_ONLY_FLAG) {
        if (flags & WrapperFactory::SOW_FLAG)
            return !isSystemOnlyAccessPermitted(cx);

        if (flags & WrapperFactory::PARTIALLY_TRANSPARENT)
            return !XrayUtils::IsTransparent(cx, wrapper);

        nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
        if (!ssm)
            return true;

        
        PRBool privileged;
        return !NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) ||
               !privileged;
    }

    
    if (wrapper->getProxyHandler() == &FilteringWrapper<JSCrossCompartmentWrapper,
        CrossOriginAccessiblePropertiesOnly>::singleton) {
        jsid scriptOnlyId = GetRTIdByIndex(cx, XPCJSRuntime::IDX_SCRIPTONLY);
        jsval scriptOnly;
        if (JS_LookupPropertyById(cx, obj, scriptOnlyId, &scriptOnly) &&
            scriptOnly == JSVAL_TRUE)
            return true; 
    }

    
    
    return WrapperFactory::IsLocationObject(obj) && !isLocationObjectSameOrigin(cx, wrapper);
}

void
AccessCheck::deny(JSContext *cx, jsid id)
{
    if (id == JSID_VOID) {
        JS_ReportError(cx, "Permission denied to access object");
    } else {
        jsval idval;
        if (!JS_IdToValue(cx, id, &idval))
            return;
        JSString *str = JS_ValueToString(cx, idval);
        if (!str)
            return;
        const jschar *chars = JS_GetStringCharsZ(cx, str);
        if (chars)
            JS_ReportError(cx, "Permission denied to access property '%hs'", chars);
    }
}

enum Access { READ = (1<<0), WRITE = (1<<1), NO_ACCESS = 0 };

static bool
Deny(JSContext *cx, jsid id, JSWrapper::Action act)
{
    
    if (act == JSWrapper::GET)
        return true;
    
    AccessCheck::deny(cx, id);
    return false;
}

bool
PermitIfUniversalXPConnect(JSContext *cx, jsid id, JSWrapper::Action act,
                           ExposedPropertiesOnly::Permission &perm)
{
    
    
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return false;
    }
    PRBool privileged;
    if (NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) &&
        privileged) {
        perm = ExposedPropertiesOnly::PermitPropertyAccess;
        return true; 
    }

    
    return Deny(cx, id, act);
}

bool
ExposedPropertiesOnly::check(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act,
                             Permission &perm)
{
    JSObject *wrappedObject = JSWrapper::wrappedObject(wrapper);

    if (act == JSWrapper::CALL) {
        perm = PermitObjectAccess;
        return true;
    }

    perm = DenyAccess;

    jsid exposedPropsId = GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS);

    JSBool found = JS_FALSE;
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, wrappedObject) ||
        !JS_HasPropertyById(cx, wrappedObject, exposedPropsId, &found))
        return false;

    
    if (JS_IsArrayObject(cx, wrappedObject) &&
        ((JSID_IS_INT(id) && JSID_TO_INT(id) >= 0) ||
         (JSID_IS_ATOM(id) && JS_FlatStringEqualsAscii(JSID_TO_FLAT_STRING(id), "length")))) {
        perm = PermitPropertyAccess;
        return true; 
    }

    
    if (!found) {
        
        if (!JS_ObjectIsFunction(cx, wrappedObject)) {
            perm = PermitPropertyAccess;
            return true;
        }
        return PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    if (id == JSID_VOID) {
        
        perm = PermitPropertyAccess;
        return true;
    }

    jsval exposedProps;
    if (!JS_LookupPropertyById(cx, wrappedObject, exposedPropsId, &exposedProps))
        return false;

    if (JSVAL_IS_VOID(exposedProps) || JSVAL_IS_NULL(exposedProps)) {
        return PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    if (!JSVAL_IS_OBJECT(exposedProps)) {
        JS_ReportError(cx, "__exposedProps__ must be undefined, null, or an Object");
        return false;
    }

    JSObject *hallpass = JSVAL_TO_OBJECT(exposedProps);

    Access access = NO_ACCESS;

    JSPropertyDescriptor desc;
    if (!JS_GetPropertyDescriptorById(cx, hallpass, id, JSRESOLVE_QUALIFIED, &desc)) {
        return false; 
    }
    if (desc.obj == NULL || !(desc.attrs & JSPROP_ENUMERATE)) {
        return PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    if (!JSVAL_IS_STRING(desc.value)) {
        JS_ReportError(cx, "property must be a string");
        return false;
    }

    JSString *str = JSVAL_TO_STRING(desc.value);
    size_t length;
    const jschar *chars = JS_GetStringCharsAndLength(cx, str, &length);
    if (!chars)
        return false;

    for (size_t i = 0; i < length; ++i) {
        switch (chars[i]) {
        case 'r':
            if (access & READ) {
                JS_ReportError(cx, "duplicate 'readable' property flag");
                return false;
            }
            access = Access(access | READ);
            break;

        case 'w':
            if (access & WRITE) {
                JS_ReportError(cx, "duplicate 'writable' property flag");
                return false;
            }
            access = Access(access | WRITE);
            break;

        default:
            JS_ReportError(cx, "properties can only be readable or read and writable");
            return false;
        }
    }

    if (access == NO_ACCESS) {
        JS_ReportError(cx, "specified properties must have a permission bit set");
        return false;
    }

    if ((act == JSWrapper::SET && !(access & WRITE)) ||
        (act != JSWrapper::SET && !(access & READ))) {
        return PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    perm = PermitPropertyAccess;
    return true; 
}

}

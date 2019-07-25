






#include "mozilla/Util.h"

#include "AccessCheck.h"

#include "nsJSPrincipals.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"

#include "XPCWrapper.h"
#include "XrayWrapper.h"
#include "FilteringWrapper.h"

#include "jsfriendapi.h"

using namespace mozilla;
using namespace js;

namespace xpc {

nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return nsJSPrincipals::get(JS_GetCompartmentPrincipals(compartment));
}


bool
AccessCheck::subsumes(JSCompartment *a, JSCompartment *b)
{
    nsIPrincipal *aprin = GetCompartmentPrincipal(a);
    nsIPrincipal *bprin = GetCompartmentPrincipal(b);

    
    
    
    if (!aprin || !bprin)
        return true;

    bool subsumes;
    nsresult rv = aprin->Subsumes(bprin, &subsumes);
    NS_ENSURE_SUCCESS(rv, false);

    return subsumes;
}

bool
AccessCheck::isLocationObjectSameOrigin(JSContext *cx, JSObject *wrapper)
{
    
    MOZ_ASSERT(WrapperFactory::IsLocationObject(js::UnwrapObject(wrapper)));

    
    
    

    
    JSObject *obj = js::GetObjectParent(js::UnwrapObject(wrapper));
    if (!js::GetObjectClass(obj)->ext.innerObject) {
        
        obj = js::UnwrapObject(obj);
        JS_ASSERT(js::GetObjectClass(obj)->ext.innerObject);
    }

    
    obj = JS_ObjectToInnerObject(cx, obj);

    
    return obj && subsumes(js::GetObjectCompartment(wrapper),
                           js::GetObjectCompartment(obj));
}

bool
AccessCheck::isChrome(JSCompartment *compartment)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return false;
    }

    bool privileged;
    nsIPrincipal *principal = GetCompartmentPrincipal(compartment);
    return NS_SUCCEEDED(ssm->IsSystemPrincipal(principal, &privileged)) && privileged;
}

bool
AccessCheck::callerIsChrome()
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm)
        return false;
    bool subjectIsSystem;
    nsresult rv = ssm->SubjectPrincipalIsSystem(&subjectIsSystem);
    return NS_SUCCEEDED(rv) && subjectIsSystem;
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
    const jschar *propChars =
        JS_GetInternedStringCharsAndLength(JS_FORGET_STRING_FLATNESS(prop), &propLength);
    if (!propLength)
        return false;
    switch (name[0]) {
        NAME('H', "History",
             PROP('b', R("back"))
             PROP('f', R("forward"))
             PROP('g', R("go")))
        NAME('L', "Location",
             PROP('h', W("hash") W("href"))
             PROP('r', R("replace")))
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
    } else if (JSID_IS_STRING(id)) {
        nsAutoString str(JS_GetInternedStringChars(JSID_TO_STRING(id)));
        col->NamedItem(str, getter_AddRefs(domwin));
    } else {
        return false;
    }

    return domwin != nullptr;
}

static bool
IsWindow(const char *name)
{
    return name[0] == 'W' && !strcmp(name, "Window");
}

bool
AccessCheck::isCrossOriginAccessPermitted(JSContext *cx, JSObject *wrapper, jsid id,
                                          Wrapper::Action act)
{
    if (!XPCWrapper::GetSecurityManager())
        return true;

    if (act == Wrapper::CALL)
        return true;

    JSObject *obj = Wrapper::wrappedObject(wrapper);

    
    if (act == Wrapper::PUNCTURE) {
        return nsContentUtils::CallerHasUniversalXPConnect();
    }

    const char *name;
    js::Class *clasp = js::GetObjectClass(obj);
    NS_ASSERTION(Jsvalify(clasp) != &XrayUtils::HolderClass, "shouldn't have a holder here");
    if (clasp->ext.innerObject)
        name = "Window";
    else
        name = clasp->name;

    if (JSID_IS_STRING(id)) {
        if (IsPermitted(name, JSID_TO_FLAT_STRING(id), act == Wrapper::SET))
            return true;
    }

    if (IsWindow(name) && IsFrameId(cx, obj, id))
        return true;

    return (act == Wrapper::SET)
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

    JSScript *script = nullptr;
    if (!fp) {
        if (!JS_DescribeScriptedCaller(cx, &script, nullptr)) {
            
            
            return true;
        }
    } else if (JS_IsScriptFrame(cx, fp)) {
        script = JS_GetFrameScript(cx, fp);
    }

    bool privileged;
    if (NS_SUCCEEDED(ssm->IsSystemPrincipal(principal, &privileged)) &&
        privileged) {
        return true;
    }

    
    
    static const char prefix[] = "chrome://global/";
    const char *filename;
    if (script &&
        (filename = JS_GetScriptFilename(cx, script)) &&
        !strncmp(filename, prefix, ArrayLength(prefix) - 1)) {
        return true;
    }

    return NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) && privileged;
}

bool
AccessCheck::needsSystemOnlyWrapper(JSObject *obj)
{
    if (!IS_WN_WRAPPER(obj))
        return false;

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(js::GetObjectPrivate(obj));
    return wn->NeedsSOW();
}

bool
AccessCheck::isScriptAccessOnly(JSContext *cx, JSObject *wrapper)
{
    JS_ASSERT(js::IsWrapper(wrapper));

    unsigned flags;
    JSObject *obj = js::UnwrapObject(wrapper, true, &flags);

    
    if (flags & WrapperFactory::SCRIPT_ACCESS_ONLY_FLAG) {
        if (flags & WrapperFactory::SOW_FLAG)
            return !isSystemOnlyAccessPermitted(cx);

        if (flags & WrapperFactory::PARTIALLY_TRANSPARENT)
            return !XrayUtils::IsTransparent(cx, wrapper);

        nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
        if (!ssm)
            return true;

        
        bool privileged;
        return !NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) ||
               !privileged;
    }

    
    if (js::GetProxyHandler(wrapper) ==
        &FilteringWrapper<CrossCompartmentSecurityWrapper,
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
Deny(JSContext *cx, jsid id, Wrapper::Action act)
{
    
    if (act == Wrapper::GET)
        return true;
    
    AccessCheck::deny(cx, id);
    return false;
}

bool
PermitIfUniversalXPConnect(JSContext *cx, jsid id, Wrapper::Action act,
                           ExposedPropertiesOnly::Permission &perm)
{
    
    
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return false;
    }

    
    
    
    NS_ASSERTION(!AccessCheck::callerIsChrome(), "About to do a meaningless security check!");

    bool privileged;
    if (NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) &&
        privileged) {
        perm = ExposedPropertiesOnly::PermitPropertyAccess;
        return true; 
    }

    
    return Deny(cx, id, act);
}

static bool
IsInSandbox(JSContext *cx, JSObject *obj)
{
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, obj))
        return false;
    JSObject *global = JS_GetGlobalForObject(cx, obj);
    return !strcmp(js::GetObjectJSClass(global)->name, "Sandbox");
}

bool
ExposedPropertiesOnly::check(JSContext *cx, JSObject *wrapper, jsid id, Wrapper::Action act,
                             Permission &perm)
{
    JSObject *wrappedObject = Wrapper::wrappedObject(wrapper);

    if (act == Wrapper::CALL) {
        perm = PermitObjectAccess;
        return true;
    }

    perm = DenyAccess;
    if (act == Wrapper::PUNCTURE)
        return PermitIfUniversalXPConnect(cx, id, act, perm); 

    jsid exposedPropsId = GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS);

    
    
    
    
    JSAutoEnterCompartment ac;
    JSAutoEnterCompartment wrapperAC;
    if (!ac.enter(cx, wrappedObject))
        return false;

    JSBool found = false;
    if (!JS_HasPropertyById(cx, wrappedObject, exposedPropsId, &found))
        return false;

    
    if (JS_IsArrayObject(cx, wrappedObject) &&
        ((JSID_IS_INT(id) && JSID_TO_INT(id) >= 0) ||
         (JSID_IS_STRING(id) && JS_FlatStringEqualsAscii(JSID_TO_FLAT_STRING(id), "length")))) {
        perm = PermitPropertyAccess;
        return true; 
    }

    
    if (!found) {
        
        if (!wrapperAC.enter(cx, wrapper))
            return false;

        
        
        if (!JS_ObjectIsFunction(cx, wrappedObject) &&
            IsInSandbox(cx, wrappedObject))
        {
            
            nsCOMPtr<nsPIDOMWindow> win =
                do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(cx, wrapper));
            if (win) {
                nsCOMPtr<nsIDocument> doc =
                    do_QueryInterface(win->GetExtantDocument());
                if (doc) {
                    doc->WarnOnceAbout(nsIDocument::eNoExposedProps,
                                        true);
                }
            }

            perm = PermitPropertyAccess;
            return true;
        }
        return PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    if (id == JSID_VOID) {
        
        perm = PermitPropertyAccess;
        return true;
    }

    JS::Value exposedProps;
    if (!JS_LookupPropertyById(cx, wrappedObject, exposedPropsId, &exposedProps))
        return false;

    if (exposedProps.isNullOrUndefined()) {
        return wrapperAC.enter(cx, wrapper) &&
               PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    if (!exposedProps.isObject()) {
        JS_ReportError(cx, "__exposedProps__ must be undefined, null, or an Object");
        return false;
    }

    JSObject *hallpass = &exposedProps.toObject();

    Access access = NO_ACCESS;

    JSPropertyDescriptor desc;
    if (!JS_GetPropertyDescriptorById(cx, hallpass, id, JSRESOLVE_QUALIFIED, &desc)) {
        return false; 
    }
    if (desc.obj == NULL || !(desc.attrs & JSPROP_ENUMERATE)) {
        return wrapperAC.enter(cx, wrapper) &&
               PermitIfUniversalXPConnect(cx, id, act, perm); 
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

    if ((act == Wrapper::SET && !(access & WRITE)) ||
        (act != Wrapper::SET && !(access & READ))) {
        return wrapperAC.enter(cx, wrapper) &&
               PermitIfUniversalXPConnect(cx, id, act, perm); 
    }

    perm = PermitPropertyAccess;
    return true; 
}

bool
ComponentsObjectPolicy::check(JSContext *cx, JSObject *wrapper, jsid id, Wrapper::Action act,
                              Permission &perm) 
{
    perm = DenyAccess;
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, wrapper))
        return false;

    if (JSID_IS_STRING(id) && act == Wrapper::GET) {
        JSFlatString *flatId = JSID_TO_FLAT_STRING(id);
        if (JS_FlatStringEqualsAscii(flatId, "isSuccessCode") ||
            JS_FlatStringEqualsAscii(flatId, "lookupMethod") ||
            JS_FlatStringEqualsAscii(flatId, "interfaces") ||
            JS_FlatStringEqualsAscii(flatId, "interfacesByID") ||
            JS_FlatStringEqualsAscii(flatId, "results"))
        {
            perm = PermitPropertyAccess;
            return true;
        }
    }

    return PermitIfUniversalXPConnect(cx, id, act, perm);  
}

}

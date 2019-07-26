






#include "mozilla/Util.h"

#include "AccessCheck.h"

#include "nsJSPrincipals.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"

#include "XPCWrapper.h"
#include "XrayWrapper.h"
#include "FilteringWrapper.h"

#include "jsfriendapi.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace js;

namespace xpc {

nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return nsJSPrincipals::get(JS_GetCompartmentPrincipals(compartment));
}

nsIPrincipal *
GetObjectPrincipal(JSObject *obj)
{
    return GetCompartmentPrincipal(js::GetObjectCompartment(obj));
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
AccessCheck::subsumes(JSObject *a, JSObject *b)
{
    return subsumes(js::GetObjectCompartment(a), js::GetObjectCompartment(b));
}


bool
AccessCheck::subsumesIgnoringDomain(JSCompartment *a, JSCompartment *b)
{
    nsIPrincipal *aprin = GetCompartmentPrincipal(a);
    nsIPrincipal *bprin = GetCompartmentPrincipal(b);

    if (!aprin || !bprin)
        return false;

    bool subsumes;
    nsresult rv = aprin->SubsumesIgnoringDomain(bprin, &subsumes);
    NS_ENSURE_SUCCESS(rv, false);

    return subsumes;
}


bool
AccessCheck::wrapperSubsumes(JSObject *wrapper)
{
    MOZ_ASSERT(js::IsWrapper(wrapper));
    JSObject *wrapped = js::UncheckedUnwrap(wrapper);
    return AccessCheck::subsumes(js::GetObjectCompartment(wrapper),
                                 js::GetObjectCompartment(wrapped));
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
AccessCheck::isChrome(JSObject *obj)
{
    return isChrome(js::GetObjectCompartment(obj));
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
        NAME('L', "Location",
             PROP('h', W("href"))
             PROP('r', R("replace")))
        NAME('W', "Window",
             PROP('b', R("blur"))
             PROP('c', R("close") R("closed"))
             PROP('f', R("focus") R("frames"))
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
IsFrameId(JSContext *cx, JSObject *objArg, jsid idArg)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);

    obj = JS_ObjectToInnerObject(cx, obj);
    MOZ_ASSERT(!js::IsWrapper(obj));
    XPCWrappedNative *wn = IS_WN_WRAPPER(obj) ? XPCWrappedNative::Get(obj)
                                              : nullptr;
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
AccessCheck::isCrossOriginAccessPermitted(JSContext *cx, JSObject *wrapperArg, jsid idArg,
                                          Wrapper::Action act)
{
    if (!XPCWrapper::GetSecurityManager())
        return true;

    if (act == Wrapper::CALL)
        return false;

    RootedId id(cx, idArg);
    RootedObject wrapper(cx, wrapperArg);
    RootedObject obj(cx, Wrapper::wrappedObject(wrapper));

    
    
    
    
    
    if (id == JSID_VOID)
        return false;

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

    
    
    if (IsWindow(name)) {
        if (JSID_IS_STRING(id) && !XrayUtils::IsXrayResolving(cx, wrapper, id)) {
            bool wouldShadow = false;
            if (!XrayUtils::HasNativeProperty(cx, wrapper, id, &wouldShadow) ||
                wouldShadow)
            {
                return false;
            }
        }
        return IsFrameId(cx, obj, id);
    }
    return false;
}

bool
AccessCheck::needsSystemOnlyWrapper(JSObject *obj)
{
    JSObject* wrapper = obj;
    if (dom::GetSameCompartmentWrapperForDOMBinding(wrapper))
        return wrapper != obj;

    if (!IS_WN_WRAPPER(obj))
        return false;

    XPCWrappedNative *wn = static_cast<XPCWrappedNative *>(js::GetObjectPrivate(obj));
    return wn->NeedsSOW();
}

enum Access { READ = (1<<0), WRITE = (1<<1), NO_ACCESS = 0 };

static void
EnterAndThrow(JSContext *cx, JSObject *wrapper, const char *msg)
{
    JSAutoCompartment ac(cx, wrapper);
    JS_ReportError(cx, msg);
}

bool
ExposedPropertiesOnly::check(JSContext *cx, JSObject *wrapperArg, jsid idArg, Wrapper::Action act)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedId id(cx, idArg);
    RootedObject wrappedObject(cx, Wrapper::wrappedObject(wrapper));

    if (act == Wrapper::CALL)
        return true;

    RootedId exposedPropsId(cx, GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS));

    
    
    
    
    JSAutoCompartment ac(cx, wrappedObject);

    JSBool found = false;
    if (!JS_HasPropertyById(cx, wrappedObject, exposedPropsId, &found))
        return false;

    
    if ((JS_IsArrayObject(cx, wrappedObject) ||
         JS_IsTypedArrayObject(wrappedObject)) &&
        ((JSID_IS_INT(id) && JSID_TO_INT(id) >= 0) ||
         (JSID_IS_STRING(id) && JS_FlatStringEqualsAscii(JSID_TO_FLAT_STRING(id), "length")))) {
        return true; 
    }

    
    if (!found) {
        return false;
    }

    if (id == JSID_VOID)
        return true;

    RootedValue exposedProps(cx);
    if (!JS_LookupPropertyById(cx, wrappedObject, exposedPropsId, exposedProps.address()))
        return false;

    if (exposedProps.isNullOrUndefined())
        return false;

    if (!exposedProps.isObject()) {
        EnterAndThrow(cx, wrapper, "__exposedProps__ must be undefined, null, or an Object");
        return false;
    }

    RootedObject hallpass(cx, &exposedProps.toObject());

    if (!AccessCheck::subsumes(js::UncheckedUnwrap(hallpass), wrappedObject)) {
        EnterAndThrow(cx, wrapper, "Invalid __exposedProps__");
        return false;
    }

    Access access = NO_ACCESS;

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, hallpass, id, 0, desc.address())) {
        return false; 
    }
    if (!desc.object() || !desc.isEnumerable())
        return false;

    if (!desc.value().isString()) {
        EnterAndThrow(cx, wrapper, "property must be a string");
        return false;
    }

    JSString *str = desc.value().toString();
    size_t length;
    const jschar *chars = JS_GetStringCharsAndLength(cx, str, &length);
    if (!chars)
        return false;

    for (size_t i = 0; i < length; ++i) {
        switch (chars[i]) {
        case 'r':
            if (access & READ) {
                EnterAndThrow(cx, wrapper, "duplicate 'readable' property flag");
                return false;
            }
            access = Access(access | READ);
            break;

        case 'w':
            if (access & WRITE) {
                EnterAndThrow(cx, wrapper, "duplicate 'writable' property flag");
                return false;
            }
            access = Access(access | WRITE);
            break;

        default:
            EnterAndThrow(cx, wrapper, "properties can only be readable or read and writable");
            return false;
        }
    }

    if (access == NO_ACCESS) {
        EnterAndThrow(cx, wrapper, "specified properties must have a permission bit set");
        return false;
    }

    if ((act == Wrapper::SET && !(access & WRITE)) ||
        (act != Wrapper::SET && !(access & READ))) {
        return false;
    }

    return true;
}

bool
ExposedPropertiesOnly::allowNativeCall(JSContext *cx, JS::IsAcceptableThis test,
                                       JS::NativeImpl impl)
{
    return js::IsReadOnlyDateMethod(test, impl) || js::IsTypedArrayThisCheck(test);
}

bool
ComponentsObjectPolicy::check(JSContext *cx, JSObject *wrapperArg, jsid idArg, Wrapper::Action act)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedId id(cx, idArg);
    JSAutoCompartment ac(cx, wrapper);

    if (JSID_IS_STRING(id) && act == Wrapper::GET) {
        JSFlatString *flatId = JSID_TO_FLAT_STRING(id);
        if (JS_FlatStringEqualsAscii(flatId, "isSuccessCode") ||
            JS_FlatStringEqualsAscii(flatId, "lookupMethod") ||
            JS_FlatStringEqualsAscii(flatId, "interfaces") ||
            JS_FlatStringEqualsAscii(flatId, "interfacesByID") ||
            JS_FlatStringEqualsAscii(flatId, "results"))
        {
            return true;
        }
    }

    
    
    
    if (xpc::IsUniversalXPConnectEnabled(cx)) {
        return true;
    }

    return false;
}

}

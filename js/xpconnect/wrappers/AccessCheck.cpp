





#include "AccessCheck.h"

#include "nsJSPrincipals.h"
#include "nsGlobalWindow.h"

#include "XPCWrapper.h"
#include "XrayWrapper.h"

#include "jsfriendapi.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/LocationBinding.h"
#include "mozilla/dom/WindowBinding.h"
#include "nsIDOMWindowCollection.h"
#include "nsJSUtils.h"

using namespace mozilla;
using namespace JS;
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
    return aprin->Subsumes(bprin);
}

bool
AccessCheck::subsumes(JSObject *a, JSObject *b)
{
    return subsumes(js::GetObjectCompartment(a), js::GetObjectCompartment(b));
}


bool
AccessCheck::subsumesConsideringDomain(JSCompartment *a, JSCompartment *b)
{
    nsIPrincipal *aprin = GetCompartmentPrincipal(a);
    nsIPrincipal *bprin = GetCompartmentPrincipal(b);
    return aprin->SubsumesConsideringDomain(bprin);
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
    bool privileged;
    nsIPrincipal *principal = GetCompartmentPrincipal(compartment);
    return NS_SUCCEEDED(nsXPConnect::SecurityManager()->IsSystemPrincipal(principal, &privileged)) && privileged;
}

bool
AccessCheck::isChrome(JSObject *obj)
{
    return isChrome(js::GetObjectCompartment(obj));
}

nsIPrincipal *
AccessCheck::getPrincipal(JSCompartment *compartment)
{
    return GetCompartmentPrincipal(compartment);
}


static bool
IsPermitted(const char *name, JSFlatString *prop, bool set)
{
    size_t propLength = JS_GetStringLength(JS_FORGET_STRING_FLATNESS(prop));
    if (!propLength)
        return false;

    jschar propChar0 = JS_GetFlatStringCharAt(prop, 0);
    if (name[0] == 'L' && !strcmp(name, "Location"))
        return dom::LocationBinding::IsPermitted(prop, propChar0, set);
    if (name[0] == 'W' && !strcmp(name, "Window"))
        return dom::WindowBinding::IsPermitted(prop, propChar0, set);

    return false;
}

static bool
IsFrameId(JSContext *cx, JSObject *objArg, jsid idArg)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);

    obj = JS_ObjectToInnerObject(cx, obj);
    MOZ_ASSERT(!js::IsWrapper(obj));
    nsGlobalWindow* win = WindowOrNull(obj);
    if (!win) {
        return false;
    }

    nsCOMPtr<nsIDOMWindowCollection> col;
    win->GetFrames(getter_AddRefs(col));
    if (!col) {
        return false;
    }

    nsCOMPtr<nsIDOMWindow> domwin;
    if (JSID_IS_INT(id)) {
        col->Item(JSID_TO_INT(id), getter_AddRefs(domwin));
    } else if (JSID_IS_STRING(id)) {
        nsAutoJSString idAsString;
        if (!idAsString.init(cx, JSID_TO_STRING(id))) {
            return false;
        }
        col->NamedItem(idAsString, getter_AddRefs(domwin));
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
    if (act == Wrapper::CALL)
        return false;

    if (act == Wrapper::ENUMERATE)
        return true;

    
    
    if (act == Wrapper::GET_PROPERTY_DESCRIPTOR) {
        return isCrossOriginAccessPermitted(cx, wrapperArg, idArg, Wrapper::GET) ||
               isCrossOriginAccessPermitted(cx, wrapperArg, idArg, Wrapper::SET);
    }

    RootedId id(cx, idArg);
    RootedObject wrapper(cx, wrapperArg);
    RootedObject obj(cx, Wrapper::wrappedObject(wrapper));

    const char *name;
    const js::Class *clasp = js::GetObjectClass(obj);
    MOZ_ASSERT(!XrayUtils::IsXPCWNHolderClass(Jsvalify(clasp)), "shouldn't have a holder here");
    if (clasp->ext.innerObject)
        name = "Window";
    else
        name = clasp->name;

    if (JSID_IS_STRING(id)) {
        if (IsPermitted(name, JSID_TO_FLAT_STRING(id), act == Wrapper::SET))
            return true;
    }

    if (act != Wrapper::GET)
        return false;

    
    
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


    
    
    if (act == Wrapper::GET_PROPERTY_DESCRIPTOR) {
        return check(cx, wrapperArg, idArg, Wrapper::GET) ||
               check(cx, wrapperArg, idArg, Wrapper::SET);
    }

    RootedId exposedPropsId(cx, GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS));

    
    
    
    
    JSAutoCompartment ac(cx, wrappedObject);

    bool found = false;
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
    if (!JS_LookupPropertyById(cx, wrappedObject, exposedPropsId, &exposedProps))
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
    if (!JS_GetPropertyDescriptorById(cx, hallpass, id, &desc)) {
        return false; 
    }
    if (!desc.object() || !desc.isEnumerable())
        return false;

    if (!desc.value().isString()) {
        EnterAndThrow(cx, wrapper, "property must be a string");
        return false;
    }

    JSFlatString *flat = JS_FlattenString(cx, desc.value().toString());
    if (!flat)
        return false;

    size_t length = JS_GetStringLength(JS_FORGET_STRING_FLATNESS(flat));

    for (size_t i = 0; i < length; ++i) {
        jschar ch = JS_GetFlatStringCharAt(flat, i);
        switch (ch) {
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

}

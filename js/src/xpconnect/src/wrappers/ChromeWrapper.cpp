






































#include "ChromeWrapper.h"
#include "AccessCheck.h"

#include "XPCWrapper.h"

using namespace js;

namespace xpc {

ChromeWrapper ChromeWrapper::singleton;

ChromeWrapper::ChromeWrapper() : JSCrossCompartmentWrapper()
{
}

ChromeWrapper::~ChromeWrapper()
{
}

typedef enum { READ = (1<<0), WRITE = (1<<1), DENIED=0 } Permission;

static bool
Allow(JSContext *cx, bool *yesno)
{
    if(yesno) {
        *yesno = true;
    }
    return true;
}

static bool
Deny(JSContext *cx, bool *yesno, const char *error)
{
    if(yesno) {
        *yesno = false;
        return true;
    }
    JS_ReportError(cx, error);
    return false;
}

static bool
CheckAccess(JSContext *cx, JSObject *hallpass, jsid id, JSCrossCompartmentWrapper::Mode mode,
            bool *yesno = NULL)
{
    if(!hallpass) {
        return Allow(cx, yesno);
    }

    Permission perm = DENIED;

    jsval v;
    if(!JS_LookupPropertyById(cx, hallpass, id, &v)) {
        return false;
    }

    if(!JSVAL_IS_STRING(v)) {
        return Deny(cx, yesno, "property permission must be a string");
    }

    JSString *str = JSVAL_TO_STRING(v);
    const jschar *chars = JS_GetStringChars(str);
    size_t length = JS_GetStringLength(str);
    for (size_t i = 0; i < length; ++i) {
        switch (chars[i]) {
        case 'r':
            if(perm & READ) {
                return Deny(cx, yesno, "duplicate 'readable' property flag");
            }
            perm = Permission(perm | READ);
            break;

        case 'w':
            if(perm & WRITE) {
                return Deny(cx, yesno, "duplicate 'writable' property flag");
            }
            perm = Permission(perm | WRITE);
            break;

        default:
            return Deny(cx, yesno, "property permission can only be readable or read and writable");
        }
    }

    if(perm == DENIED) {
        return Deny(cx, yesno, "invalid property permission");
    }

    if((mode == JSCrossCompartmentWrapper::GET && !(perm & READ)) ||
       (mode == JSCrossCompartmentWrapper::SET && !(perm & WRITE))) {
        if(yesno) {
            *yesno = false;
            return true;
        }
        AccessCheck::deny(cx, id);
        return false;
    }

    return Allow(cx, yesno);
}

static bool
GetHallPass(JSContext *cx, JSObject *wrappedObject, jsid id, JSObject **hallpassp)
{
    jsid exposedPropsId = GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS);

    JSBool found = JS_FALSE;
    if(!JS_HasPropertyById(cx, wrappedObject, exposedPropsId, &found))
        return false;
    if(!found) {
        *hallpassp = NULL;
        return true;
    }

    jsval exposedProps;
    if(!JS_LookupPropertyById(cx, wrappedObject, exposedPropsId, &exposedProps))
        return false;

    if(JSVAL_IS_VOID(exposedProps) || JSVAL_IS_NULL(exposedProps)) {
        AccessCheck::deny(cx, id);
        return false;
    }

    if(!JSVAL_IS_OBJECT(exposedProps)) {
        JS_ReportError(cx,
                       "__exposedProps__ must be undefined, null, or an object");
        return false;
    }

    *hallpassp = JSVAL_TO_OBJECT(exposedProps);
    return true;
}

static bool
Filter(JSContext *cx, JSObject *wrappedObject, AutoValueVector &props)
{
    JSObject *hallpass;
    if(!GetHallPass(cx, wrappedObject, JSVAL_VOID, &hallpass))
        return false;
    if(!hallpass)
        return true;
    size_t w = 0;
    for (size_t n = 0; n < props.length(); ++n) {
        bool yes;
        jsid id = props[n];
        if(!CheckAccess(cx, hallpass, id, JSCrossCompartmentWrapper::GET, &yes))
            return false;
        if(yes) {
            props[w++] = id;
        }
    }
    props.resize(w);
    return true;
}

bool
ChromeWrapper::getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return JSCrossCompartmentWrapper::getOwnPropertyNames(cx, wrapper, props) &&
           Filter(cx, wrappedObject(wrapper), props);
}

bool
ChromeWrapper::enumerate(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return JSCrossCompartmentWrapper::enumerate(cx, wrapper, props) &&
           Filter(cx, wrappedObject(wrapper), props);
}

bool
ChromeWrapper::enumerateOwn(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return JSCrossCompartmentWrapper::enumerateOwn(cx, wrapper, props) &&
           Filter(cx, wrappedObject(wrapper), props);
}

bool
ChromeWrapper::iterate(JSContext *cx, JSObject *wrapper, uintN flags, jsval *vp)
{
    
    
    
    
    return JSProxyHandler::iterate(cx, wrapper, flags, vp);
}

bool
ChromeWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id, Mode mode)
{
    JSObject *hallpass;
    return GetHallPass(cx, wrappedObject(wrapper), id, &hallpass) &&
           CheckAccess(cx, hallpass, id, mode);
}

JSString *
ChromeWrapper::fun_toString(JSContext *cx, JSObject *wrapper, uintN indent)
{
    
    JSObject *ctor;
    if(!JS_GetClassObject(cx, wrapper->getGlobal(), JSProto_Function, &ctor))
        return false;
    return JS_DecompileFunction(cx, JS_ValueToConstructor(cx, OBJECT_TO_JSVAL(ctor)), indent);
}

}

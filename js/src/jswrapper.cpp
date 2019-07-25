








































#include "jsapi.h"
#include "jscntxt.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jswrapper.h"

#include "jsobjinlines.h"

using namespace js;

JSWrapper::JSWrapper(JSObject *obj) : mWrappedObject(obj)
{
}

JSWrapper::~JSWrapper()
{
}

bool
JSWrapper::getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                 PropertyDescriptor *desc)
{
    JSObject *wobj = wrappedObject(proxy);
    return JS_GetPropertyDescriptorById(cx, wobj, id, JSRESOLVE_QUALIFIED,
                                        Jsvalify(desc));
}

bool
JSWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                    PropertyDescriptor *desc)
{
    return JS_GetPropertyDescriptorById(cx, wrappedObject(proxy), id, JSRESOLVE_QUALIFIED,
                                        Jsvalify(desc));
}

bool
JSWrapper::defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                          PropertyDescriptor *desc)
{
    return JS_DefinePropertyById(cx, wrappedObject(proxy), id, Jsvalify(desc->value),
                                 Jsvalify(desc->getter), Jsvalify(desc->setter),
                                 desc->attrs);
}

bool
JSWrapper::getOwnPropertyNames(JSContext *cx, JSObject *proxy, JSIdArray **idap)
{
    return GetPropertyNames(cx, wrappedObject(proxy), JSITER_OWNONLY | JSITER_HIDDEN, idap);
}

bool
JSWrapper::delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    AutoValueRooter tvr(cx);
    if (!JS_DeletePropertyById2(cx, wrappedObject(proxy), id, tvr.jsval_addr()))
        return false;
    *bp = js_ValueToBoolean(tvr.value());
    return true;
}

bool
JSWrapper::enumerate(JSContext *cx, JSObject *proxy, JSIdArray **idap)
{
    return GetPropertyNames(cx, wrappedObject(proxy), 0, idap);
}

bool
JSWrapper::fix(JSContext *cx, JSObject *proxy, Value *vp)
{
    vp->setUndefined();
    return true;
}

bool
JSWrapper::has(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    JSBool found;
    if (!JS_HasPropertyById(cx, wrappedObject(proxy), id, &found))
        return false;
    *bp = !!found;
    return true;
}

bool
JSWrapper::hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    PropertyDescriptor desc;
    JSObject *wobj = wrappedObject(proxy);
    if (!JS_GetPropertyDescriptorById(cx, wobj, id, JSRESOLVE_QUALIFIED, Jsvalify(&desc)))
        return false;
    *bp = (desc.obj == wobj);
    return true;
}

bool
JSWrapper::get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp)
{
    return JS_GetPropertyById(cx, wrappedObject(proxy), id, Jsvalify(vp));
}

bool
JSWrapper::set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp)
{
    return JS_SetPropertyById(cx, wrappedObject(proxy), id, Jsvalify(vp));
}

bool
JSWrapper::enumerateOwn(JSContext *cx, JSObject *proxy, JSIdArray **idap)
{
    return GetPropertyNames(cx, wrappedObject(proxy), JSITER_OWNONLY, idap);
}

bool
JSWrapper::iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp)
{
    return GetIterator(cx, wrappedObject(proxy), flags, vp);
}

void
JSWrapper::finalize(JSContext *cx, JSObject *proxy)
{
    if (mWrappedObject)
        delete this;
}

void
JSWrapper::trace(JSTracer *trc, JSObject *proxy)
{
    if (mWrappedObject)
        JS_CALL_OBJECT_TRACER(trc, mWrappedObject, "wrappedObject");
}

const void *
JSWrapper::family()
{
    return &singleton;
}

JSWrapper JSWrapper::singleton(NULL);

JSObject *
JSWrapper::wrap(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent, JSString *className)
{
    JSObject *wobj;
    if (obj->isCallable()) {
        JSWrapper *handler = new JSWrapper(obj);
        if (!handler)
            return NULL;
        wobj = NewFunctionProxy(cx, PrivateTag(handler), proto, parent, obj, NULL);
        if (!wobj) {
            delete handler;
            return NULL;
        }
    } else {
        wobj = NewObjectProxy(cx, PrivateTag(&singleton), proto, parent, className);
        if (!wobj)
            return NULL;
        wobj->setProxyPrivate(ObjectTag(*obj));
    }
    return wobj;
}

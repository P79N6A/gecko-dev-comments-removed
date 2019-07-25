






































#include "dombindings.h"
#include "xpcprivate.h"
#include "xpcquickstubs.h"

#include "nsIDOMNode.h"

extern XPCNativeInterface* interfaces[];

using namespace js;

namespace xpc {
namespace dom {

static int NodeListFamily;

NodeList::NodeList() : ProxyHandler(&NodeListFamily)
{
}

NodeList NodeList::instance;

static Class NodeListProtoClass = {
    "NodeList",
    0,
    JS_PropertyStub,        
    JS_PropertyStub,        
    JS_PropertyStub,        
    JS_StrictPropertyStub,  
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

nsINodeList *
NodeList::getNodeList(JSObject *obj)
{
    JS_ASSERT(js::IsProxy(obj) && js::GetProxyHandler(obj) == &NodeList::instance);
    return static_cast<nsINodeList *>(js::GetProxyPrivate(obj).toPrivate());
}

uint32
NodeList::getProtoShape(JSObject *obj)
{
    JS_ASSERT(js::IsProxy(obj) && js::GetProxyHandler(obj) == &NodeList::instance);
    return js::GetProxyExtra(obj, 0).toPrivateUint32();
}

void
NodeList::setProtoShape(JSObject *obj, uint32 shape)
{
    JS_ASSERT(js::IsProxy(obj) && js::GetProxyHandler(obj) == &NodeList::instance);
    js::SetProxyExtra(obj, 0, PrivateUint32Value(shape));
}

JSBool
NodeList::length_getter(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!js::IsProxy(obj) || (js::GetProxyHandler(obj) != &NodeList::instance)) {
        
        JS_ReportError(cx, "type error: wrong object");
        return false;
    }
    PRUint32 length;
    getNodeList(obj)->GetLength(&length);
    JS_ASSERT(int32(length) >= 0);
    vp->setInt32(length);
    return true;
}

JSObject *
NodeList::getPrototype(JSContext *cx)
{
    
    JSObject *proto = JS_NewObject(cx, Jsvalify(&NodeListProtoClass), NULL, NULL);
    if (!proto)
        return NULL;
    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, proto))
        return NULL;
    if (!JS_DefineProperty(cx, proto, "length", JSVAL_VOID,
                           length_getter, NULL,
                           JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_SHARED))
        return NULL;
    return proto;
}

JSObject *
NodeList::create(JSContext *cx, nsINodeList *aNodeList)
{
    JSObject *proto = getPrototype(cx);
    if (!proto)
        return NULL;
    JSObject *obj = js::NewProxyObject(cx, &NodeList::instance,
                                       PrivateValue(aNodeList),
                                       proto, NULL);
    if (!obj)
        return NULL;

    NS_ADDREF(aNodeList);
    setProtoShape(obj, -1);

    return obj;
}

static bool
WrapObject(JSContext *cx, JSObject *scope, nsIContent *result, jsval *vp)
{
    nsWrapperCache *cache = xpc_qsGetWrapperCache(result);
    if (xpc_FastGetCachedWrapper(cache, scope, vp))
        return true;
    XPCLazyCallContext lccx(JS_CALLER, cx, scope);
    qsObjectHelper helper(result, cache);
    return xpc_qsXPCOMObjectToJsval(lccx, helper, &NS_GET_IID(nsIDOMNode),
                                    &interfaces[k_nsIDOMNode], vp);
}

bool
NodeList::getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                   PropertyDescriptor *desc)
{
    
    int32 index;
    if (JSID_IS_INT(id) && ((index = JSID_TO_INT(id)) >= 0)) {
        nsINodeList *nodeList = getNodeList(proxy);
        nsIContent *result = nodeList->GetNodeAt(PRUint32(index));
        if (result) {
            jsval v;
            if (!WrapObject(cx, proxy, result, &v))
                return false;
            desc->obj = proxy;
            desc->value = v;
            desc->attrs = JSPROP_READONLY | JSPROP_ENUMERATE;
            desc->getter = NULL;
            desc->setter = NULL;
            desc->shortid = 0;
            return true;
        }
    }
    desc->obj = NULL;
    return true;
}

bool
NodeList::getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                PropertyDescriptor *desc)
{
    if (!getOwnPropertyDescriptor(cx, proxy, id, set, desc))
        return false;
    if (desc->obj)
        return true;
    return JS_GetPropertyDescriptorById(cx, js::GetObjectProto(proxy), id, JSRESOLVE_QUALIFIED,
                                        desc);
}

bool
NodeList::defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                         PropertyDescriptor *desc)
{
    
    return true;
}

bool
NodeList::getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoIdVector &props)
{
    
    PRUint32 length;
    getNodeList(proxy)->GetLength(&length);
    JS_ASSERT(int32(length) >= 0);
    for (int32 i = 0; i < int32(length); ++i) {
        if (!props.append(INT_TO_JSID(i)))
            return false;
    }
    return true;
}

bool
NodeList::delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    
    return true;
}

bool
NodeList::enumerate(JSContext *cx, JSObject *proxy, js::AutoIdVector &props)
{
    
    return getOwnPropertyNames(cx, proxy, props);
}

bool
NodeList::fix(JSContext *cx, JSObject *proxy, Value *vp)
{
    vp->setUndefined();
    return true;
}

bool
NodeList::hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    
    int32 index;
    if (JSID_IS_INT(id) && ((index = JSID_TO_INT(id)) >= 0)) {
        if (getNodeList(proxy)->GetNodeAt(PRUint32(index))) {
            *bp = true;
            return true;
        }
    }
    *bp = false;
    return true;
}

bool
NodeList::has(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    if (!hasOwn(cx, proxy, id, bp))
        return false;
    if (*bp)
        return true;
    JSBool found;
    if (!JS_HasPropertyById(cx, js::GetObjectProto(proxy), id, &found))
        return false;
    *bp = !!found;
    return true;
}

bool
NodeList::get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, js::Value *vp)
{
    
    int32 index;
    if (JSID_IS_INT(id) && ((index = JSID_TO_INT(id)) >= 0)) {
        nsINodeList *nodeList = getNodeList(proxy);
        nsIContent *result = nodeList->GetNodeAt(PRUint32(index));
        if (result)
            return WrapObject(cx, proxy, result, vp);
    }
    return JS_GetPropertyById(cx, js::GetObjectProto(proxy), id, vp);
}

bool
NodeList::set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
              js::Value *vp)
{
    
    return true;
}

bool
NodeList::keys(JSContext *cx, JSObject *proxy, js::AutoIdVector &props)
{
    
    return getOwnPropertyNames(cx, proxy, props);
}

bool
NodeList::iterate(JSContext *cx, JSObject *proxy, uintN flags, js::Value *vp)
{
    JS_ReportError(cx, "FIXME");
    return false;
}

bool
NodeList::hasInstance(JSContext *cx, JSObject *proxy, const js::Value *vp, bool *bp)
{
    *bp = vp->isObject() && js::GetObjectClass(&vp->toObject()) == &NodeListProtoClass;
    return true;
}

JSString *
NodeList::obj_toString(JSContext *cx, JSObject *proxy)
{
    return JS_NewStringCopyZ(cx, "[object NodeList]");
}

void
NodeList::finalize(JSContext *cx, JSObject *proxy)
{
    nsINodeList *nodeList = getNodeList(proxy);
    NS_RELEASE(nodeList);
}

}
}

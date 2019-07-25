








































#ifndef jsweakmap_h___
#define jsweakmap_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

namespace js {

typedef js::HashMap<JSObject *, Value> ObjectValueMap;

class WeakMap {
    ObjectValueMap map;
    JSObject *next;

    static WeakMap *fromJSObject(JSObject *obj);

    static JSBool has(JSContext *cx, uintN argc, Value *vp);
    static JSBool get(JSContext *cx, uintN argc, Value *vp);
    static JSBool delete_(JSContext *cx, uintN argc, Value *vp);
    static JSBool set(JSContext *cx, uintN argc, Value *vp);


  protected:
    static void mark(JSTracer *trc, JSObject *obj);
    static void finalize(JSContext *cx, JSObject *obj);

  public:
    WeakMap(JSContext *cx);

    static JSBool construct(JSContext *cx, uintN argc, Value *vp);

    static bool markIteratively(JSTracer *trc);
    static void sweep(JSContext *cx);

    static Class jsclass;
    static JSFunctionSpec methods[];
};

}

extern JSObject *
js_InitWeakMapClass(JSContext *cx, JSObject *obj);

#endif

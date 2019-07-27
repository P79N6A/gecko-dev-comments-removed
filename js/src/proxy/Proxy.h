





#ifndef proxy_Proxy_h
#define proxy_Proxy_h

#include "NamespaceImports.h"

#include "js/Class.h"

namespace js {

class RegExpGuard;








class Proxy
{
  public:
    
    static bool preventExtensions(JSContext *cx, HandleObject proxy);
    static bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                      MutableHandle<JSPropertyDescriptor> desc);
    static bool getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                      MutableHandleValue vp);
    static bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                         MutableHandle<JSPropertyDescriptor> desc);
    static bool getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                         MutableHandleValue vp);
    static bool defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                               MutableHandle<JSPropertyDescriptor> desc);
    static bool getOwnPropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props);
    static bool delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props);

    
    static bool has(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool hasOwn(JSContext *cx, HandleObject proxy, HandleId id, bool *bp);
    static bool get(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
                    MutableHandleValue vp);
    static bool set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
                    bool strict, MutableHandleValue vp);
    static bool keys(JSContext *cx, HandleObject proxy, AutoIdVector &props);
    static bool iterate(JSContext *cx, HandleObject proxy, unsigned flags, MutableHandleValue vp);

    
    static bool isExtensible(JSContext *cx, HandleObject proxy, bool *extensible);
    static bool call(JSContext *cx, HandleObject proxy, const CallArgs &args);
    static bool construct(JSContext *cx, HandleObject proxy, const CallArgs &args);
    static bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);
    static bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    static bool objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx);
    static const char *className(JSContext *cx, HandleObject proxy);
    static JSString *fun_toString(JSContext *cx, HandleObject proxy, unsigned indent);
    static bool regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g);
    static bool boxedValue_unbox(JSContext *cx, HandleObject proxy, MutableHandleValue vp);
    static bool defaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);
    static bool getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject protop);
    static bool setPrototypeOf(JSContext *cx, HandleObject proxy, HandleObject proto, bool *bp);

    static bool watch(JSContext *cx, HandleObject proxy, HandleId id, HandleObject callable);
    static bool unwatch(JSContext *cx, HandleObject proxy, HandleId id);

    static bool slice(JSContext *cx, HandleObject obj, uint32_t begin, uint32_t end,
                      HandleObject result);

    
    static bool callProp(JSContext *cx, HandleObject proxy, HandleObject reveiver, HandleId id,
                         MutableHandleValue vp);
};

} 

#endif 

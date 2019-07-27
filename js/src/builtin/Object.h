





#ifndef builtin_Object_h
#define builtin_Object_h

#include "jsapi.h"

#include "vm/NativeObject.h"

namespace JS {
class CallArgs;
class Value;
}

namespace js {


bool
obj_construct(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_valueOf(JSContext *cx, unsigned argc, JS::Value *vp);

PlainObject *
ObjectCreateImpl(JSContext *cx, HandleObject proto, NewObjectKind newKind = GenericObject,
                 HandleObjectGroup group = js::NullPtr());

PlainObject *
ObjectCreateWithTemplate(JSContext *cx, HandlePlainObject templateObj);


bool
obj_create(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_defineProperty(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_getOwnPropertyNames(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_getOwnPropertyDescriptor(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_getPrototypeOf(JSContext *cx, unsigned argc, JS::Value *vp);

bool
obj_hasOwnProperty(JSContext *cx, unsigned argc, JS::Value *vp);


bool
GetOwnPropertyKeys(JSContext *cx, const JS::CallArgs &args, unsigned flags);






bool
IdToStringOrSymbol(JSContext *cx, JS::HandleId id, JS::MutableHandleValue result);

#if JS_HAS_TOSOURCE

JSString *
ObjectToSource(JSContext *cx, JS::HandleObject obj);
#endif 

extern bool
WatchHandler(JSContext *cx, JSObject *obj, jsid id, JS::Value old,
             JS::Value *nvp, void *closure);

} 

#endif 







#ifndef jsprvtd_h
#define jsprvtd_h





#include "jsapi.h"


typedef uint8_t     jsbytecode;
typedef uint8_t     jssrcnote;
typedef uintptr_t   jsatomid;


class JSAtom;

namespace js {

class PropertyName;
class Shape;

typedef JSNative             Native;
typedef JSParallelNative     ParallelNative;
typedef JSThreadSafeNative   ThreadSafeNative;
typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;

namespace types { struct TypeObject; }

typedef JS::Handle<Shape*>             HandleShape;
typedef JS::Handle<types::TypeObject*> HandleTypeObject;
typedef JS::Handle<JSAtom*>            HandleAtom;
typedef JS::Handle<PropertyName*>      HandlePropertyName;

typedef JS::MutableHandle<Shape*>      MutableHandleShape;
typedef JS::MutableHandle<JSAtom*>     MutableHandleAtom;

typedef JS::Rooted<Shape*>             RootedShape;
typedef JS::Rooted<types::TypeObject*> RootedTypeObject;
typedef JS::Rooted<JSAtom*>            RootedAtom;
typedef JS::Rooted<PropertyName*>      RootedPropertyName;

enum XDRMode {
    XDR_ENCODE,
    XDR_DECODE
};

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair() {}
    IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
};

} 







typedef JSObject *
(* JSObjectOp)(JSContext *cx, JS::Handle<JSObject*> obj);


typedef JSObject *
(* JSClassInitializerOp)(JSContext *cx, JS::HandleObject obj);





typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JS::HandleObject obj, bool keysonly);


#endif 

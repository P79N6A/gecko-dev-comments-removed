





#ifndef builtin_SIMD_h
#define builtin_SIMD_h

#include "jsapi.h"
#include "jsobj.h"
#include "builtin/TypedObject.h"
#include "vm/GlobalObject.h"







namespace js {

class SIMDObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);
};



struct Float32x4 {
    typedef float Elem;
    static const int32_t lanes = 4;
    static const X4TypeDescr::Type type =
        X4TypeDescr::TYPE_FLOAT32;

    static TypeDescr &GetTypeDescr(GlobalObject &global) {
        return global.float32x4TypeDescr().as<TypeDescr>();
    }
    static Elem toType(Elem a) {
        return a;
    }
    static void toType2(JSContext *cx, JS::Handle<JS::Value> v, Elem *out) {
        *out = v.toNumber();
    }
    static void setReturn(CallArgs &args, float value) {
        args.rval().setDouble(JS::CanonicalizeNaN(value));
    }
};

struct Int32x4 {
    typedef int32_t Elem;
    static const int32_t lanes = 4;
    static const X4TypeDescr::Type type =
        X4TypeDescr::TYPE_INT32;

    static TypeDescr &GetTypeDescr(GlobalObject &global) {
        return global.int32x4TypeDescr().as<TypeDescr>();
    }
    static Elem toType(Elem a) {
        return ToInt32(a);
    }
    static void toType2(JSContext *cx, JS::Handle<JS::Value> v, Elem *out) {
        ToInt32(cx,v,out);
    }
    static void setReturn(CallArgs &args, int32_t value) {
        args.rval().setInt32(value);
    }
};

template<typename V>
JSObject *Create(JSContext *cx, typename V::Elem *data);

}  

JSObject *
js_InitSIMDClass(JSContext *cx, js::HandleObject obj);

#endif 

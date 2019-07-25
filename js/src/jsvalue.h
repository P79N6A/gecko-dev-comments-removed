






































#ifndef jsvalue_h__
#define jsvalue_h__



#include "jsprvtd.h"

namespace js {

























static inline jsval *        Jsvalify(Value *v)        { return v; }
static inline const jsval *  Jsvalify(const Value *v)  { return v; }
static inline jsval &        Jsvalify(Value &v)        { return v; }
static inline const jsval &  Jsvalify(const Value &v)  { return v; }
static inline Value *        Valueify(jsval *v)        { return v; }
static inline const Value *  Valueify(const jsval *v)  { return v; }
static inline Value **       Valueify(jsval **v)       { return v; }
static inline Value &        Valueify(jsval &v)        { return v; }
static inline const Value &  Valueify(const jsval &v)  { return v; }

struct Class;

typedef JSBool
(* Native)(JSContext *cx, uintN argc, Value *vp);
typedef JSBool
(* PropertyOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp);
typedef JSBool
(* StrictPropertyOp)(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);
typedef JSBool
(* ConvertOp)(JSContext *cx, JSObject *obj, JSType type, Value *vp);
typedef JSBool
(* NewEnumerateOp)(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                   Value *statep, jsid *idp);
typedef JSBool
(* HasInstanceOp)(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp);
typedef JSBool
(* CheckAccessOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                  Value *vp);
typedef JSBool
(* EqualityOp)(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp);
typedef JSBool
(* DefinePropOp)(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                 PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* DefineElementOp)(JSContext *cx, JSObject *obj, uint32 index, const Value *value,
                    PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* PropertyIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);
typedef JSBool
(* ElementIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp);
typedef JSBool
(* StrictPropertyIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* StrictElementIdOp)(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict);
typedef JSBool
(* DeleteIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* DeleteElementOp)(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict);
typedef JSBool
(* CallOp)(JSContext *cx, uintN argc, Value *vp);
typedef JSBool
(* LookupPropOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                 JSProperty **propp);
typedef JSBool
(* LookupElementOp)(JSContext *cx, JSObject *obj, uint32 index, JSObject **objp,
                    JSProperty **propp);
typedef JSBool
(* AttributesOp)(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);
typedef JSBool
(* ElementAttributesOp)(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp);
typedef JSType
(* TypeOfOp)(JSContext *cx, JSObject *obj);
typedef JSObject *
(* ObjectOp)(JSContext *cx, JSObject *obj);
typedef void
(* FinalizeOp)(JSContext *cx, JSObject *obj);

class AutoIdVector;

class PropertyName;








typedef JSBool
(* FixOp)(JSContext *cx, JSObject *obj, bool *fixed, AutoIdVector *props);

static inline Native             Valueify(JSNative f)           { return (Native)f; }
static inline JSNative           Jsvalify(Native f)             { return (JSNative)f; }
static inline PropertyOp         Valueify(JSPropertyOp f)       { return (PropertyOp)f; }
static inline JSPropertyOp       Jsvalify(PropertyOp f)         { return (JSPropertyOp)f; }
static inline StrictPropertyOp   Valueify(JSStrictPropertyOp f) { return (StrictPropertyOp)f; }
static inline JSStrictPropertyOp Jsvalify(StrictPropertyOp f)   { return (JSStrictPropertyOp)f; }
static inline ConvertOp          Valueify(JSConvertOp f)        { return (ConvertOp)f; }
static inline JSConvertOp        Jsvalify(ConvertOp f)          { return (JSConvertOp)f; }
static inline NewEnumerateOp     Valueify(JSNewEnumerateOp f)   { return (NewEnumerateOp)f; }
static inline JSNewEnumerateOp   Jsvalify(NewEnumerateOp f)     { return (JSNewEnumerateOp)f; }
static inline HasInstanceOp      Valueify(JSHasInstanceOp f)    { return (HasInstanceOp)f; }
static inline JSHasInstanceOp    Jsvalify(HasInstanceOp f)      { return (JSHasInstanceOp)f; }
static inline CheckAccessOp      Valueify(JSCheckAccessOp f)    { return (CheckAccessOp)f; }
static inline JSCheckAccessOp    Jsvalify(CheckAccessOp f)      { return (JSCheckAccessOp)f; }
static inline EqualityOp         Valueify(JSEqualityOp f);      
static inline JSEqualityOp       Jsvalify(EqualityOp f);        

static inline PropertyName       *Valueify(JSPropertyName *n)     { return (PropertyName *)n; }
static inline JSPropertyName     *Jsvalify(PropertyName *n)       { return (JSPropertyName *)n; }

static const PropertyOp       PropertyStub       = (PropertyOp)JS_PropertyStub;
static const StrictPropertyOp StrictPropertyStub = (StrictPropertyOp)JS_StrictPropertyStub;
static const JSEnumerateOp    EnumerateStub      = JS_EnumerateStub;
static const JSResolveOp      ResolveStub        = JS_ResolveStub;
static const ConvertOp        ConvertStub        = (ConvertOp)JS_ConvertStub;
static const JSFinalizeOp     FinalizeStub       = JS_FinalizeStub;

#define JS_CLASS_MEMBERS                                                      \
    const char          *name;                                                \
    uint32              flags;                                                \
                                                                              \
    /* Mandatory non-null function pointer members. */                        \
    PropertyOp          addProperty;                                          \
    PropertyOp          delProperty;                                          \
    PropertyOp          getProperty;                                          \
    StrictPropertyOp    setProperty;                                          \
    JSEnumerateOp       enumerate;                                            \
    JSResolveOp         resolve;                                              \
    ConvertOp           convert;                                              \
    JSFinalizeOp        finalize;                                             \
                                                                              \
    /* Optionally non-null members start here. */                             \
    JSClassInternal     reserved0;                                            \
    CheckAccessOp       checkAccess;                                          \
    Native              call;                                                 \
    Native              construct;                                            \
    JSXDRObjectOp       xdrObject;                                            \
    HasInstanceOp       hasInstance;                                          \
    JSTraceOp           trace





struct ClassSizeMeasurement {
    JS_CLASS_MEMBERS;
};

struct ClassExtension {
    EqualityOp          equality;
    JSObjectOp          outerObject;
    JSObjectOp          innerObject;
    JSIteratorOp        iteratorObject;
    void               *unused;

    



    bool                isWrappedNative;
};

#define JS_NULL_CLASS_EXT   {NULL,NULL,NULL,NULL,NULL,false}

struct ObjectOps {
    js::LookupPropOp        lookupProperty;
    js::LookupElementOp     lookupElement;
    js::DefinePropOp        defineProperty;
    js::DefineElementOp     defineElement;
    js::PropertyIdOp        getProperty;
    js::ElementIdOp         getElement;
    js::StrictPropertyIdOp  setProperty;
    js::StrictElementIdOp   setElement;
    js::AttributesOp        getAttributes;
    js::ElementAttributesOp getElementAttributes;
    js::AttributesOp        setAttributes;
    js::ElementAttributesOp setElementAttributes;
    js::DeleteIdOp          deleteProperty;
    js::DeleteElementOp     deleteElement;

    js::NewEnumerateOp      enumerate;
    js::TypeOfOp            typeOf;
    js::FixOp               fix;
    js::ObjectOp            thisObject;
    js::FinalizeOp          clear;
};

#define JS_NULL_OBJECT_OPS                                                    \
    {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,   \
     NULL,NULL,NULL,NULL,NULL}

struct Class {
    JS_CLASS_MEMBERS;
    ClassExtension      ext;
    ObjectOps           ops;
    uint8               pad[sizeof(JSClass) - sizeof(ClassSizeMeasurement) -
                            sizeof(ClassExtension) - sizeof(ObjectOps)];

    
    static const uint32 NON_NATIVE = JSCLASS_INTERNAL_FLAG2;

    bool isNative() const {
        return !(flags & NON_NATIVE);
    }
};

JS_STATIC_ASSERT(offsetof(JSClass, name) == offsetof(Class, name));
JS_STATIC_ASSERT(offsetof(JSClass, flags) == offsetof(Class, flags));
JS_STATIC_ASSERT(offsetof(JSClass, addProperty) == offsetof(Class, addProperty));
JS_STATIC_ASSERT(offsetof(JSClass, delProperty) == offsetof(Class, delProperty));
JS_STATIC_ASSERT(offsetof(JSClass, getProperty) == offsetof(Class, getProperty));
JS_STATIC_ASSERT(offsetof(JSClass, setProperty) == offsetof(Class, setProperty));
JS_STATIC_ASSERT(offsetof(JSClass, enumerate) == offsetof(Class, enumerate));
JS_STATIC_ASSERT(offsetof(JSClass, resolve) == offsetof(Class, resolve));
JS_STATIC_ASSERT(offsetof(JSClass, convert) == offsetof(Class, convert));
JS_STATIC_ASSERT(offsetof(JSClass, finalize) == offsetof(Class, finalize));
JS_STATIC_ASSERT(offsetof(JSClass, reserved0) == offsetof(Class, reserved0));
JS_STATIC_ASSERT(offsetof(JSClass, checkAccess) == offsetof(Class, checkAccess));
JS_STATIC_ASSERT(offsetof(JSClass, call) == offsetof(Class, call));
JS_STATIC_ASSERT(offsetof(JSClass, construct) == offsetof(Class, construct));
JS_STATIC_ASSERT(offsetof(JSClass, xdrObject) == offsetof(Class, xdrObject));
JS_STATIC_ASSERT(offsetof(JSClass, hasInstance) == offsetof(Class, hasInstance));
JS_STATIC_ASSERT(offsetof(JSClass, trace) == offsetof(Class, trace));
JS_STATIC_ASSERT(sizeof(JSClass) == sizeof(Class));

struct PropertyDescriptor {
    JSObject           *obj;
    uintN              attrs;
    PropertyOp         getter;
    StrictPropertyOp   setter;
    Value              value;
    uintN              shortid;
};
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, obj) == offsetof(PropertyDescriptor, obj));
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, attrs) == offsetof(PropertyDescriptor, attrs));
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, getter) == offsetof(PropertyDescriptor, getter));
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, setter) == offsetof(PropertyDescriptor, setter));
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, value) == offsetof(PropertyDescriptor, value));
JS_STATIC_ASSERT(offsetof(JSPropertyDescriptor, shortid) == offsetof(PropertyDescriptor, shortid));
JS_STATIC_ASSERT(sizeof(JSPropertyDescriptor) == sizeof(PropertyDescriptor));

static JS_ALWAYS_INLINE JSClass *              Jsvalify(Class *c)                { return (JSClass *)c; }
static JS_ALWAYS_INLINE Class *                Valueify(JSClass *c)              { return (Class *)c; }
static JS_ALWAYS_INLINE JSPropertyDescriptor * Jsvalify(PropertyDescriptor *p) { return (JSPropertyDescriptor *) p; }
static JS_ALWAYS_INLINE PropertyDescriptor *   Valueify(JSPropertyDescriptor *p) { return (PropertyDescriptor *) p; }







#ifdef DEBUG

# define JS_VALUEIFY(type, v) js::Valueify(v)
# define JS_JSVALIFY(type, v) js::Jsvalify(v)

static inline JSNative JsvalifyNative(Native n)   { return n; }
static inline Native ValueifyNative(JSNative n)   { return n; }
static inline JSPropertyOp CastNativeToJSPropertyOp(Native n) { return (JSPropertyOp) n; }
static inline JSStrictPropertyOp CastNativeToJSStrictPropertyOp(Native n) {
    return (JSStrictPropertyOp) n;
}

# define JS_VALUEIFY_NATIVE(n) js::ValueifyNative(n)
# define JS_JSVALIFY_NATIVE(n) js::JsvalifyNative(n)
# define JS_CAST_NATIVE_TO_JSPROPERTYOP(n) js::CastNativeToJSPropertyOp(n)
# define JS_CAST_NATIVE_TO_JSSTRICTPROPERTYOP(n) js::CastNativeToJSStrictPropertyOp(n)

#else

# define JS_VALUEIFY(type, v) ((type) (v))
# define JS_JSVALIFY(type, v) ((type) (v))

# define JS_VALUEIFY_NATIVE(n) ((js::Native) (n))
# define JS_JSVALIFY_NATIVE(n) ((JSNative) (n))
# define JS_CAST_NATIVE_TO_JSPROPERTYOP(n) ((JSPropertyOp) (n))
# define JS_CAST_NATIVE_TO_JSSTRICTPROPERTYOP(n) ((JSStrictPropertyOp) (n))

#endif






#undef JS_FN
#define JS_FN(name,call,nargs,flags)                                          \
     {name, JS_JSVALIFY_NATIVE(call), nargs, (flags) | JSFUN_STUB_GSOPS}







#define JS_PSG(name,getter,flags)                                             \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     JS_CAST_NATIVE_TO_JSPROPERTYOP(getter),                                  \
     NULL}
#define JS_PSGS(name,getter,setter,flags)                                     \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     JS_CAST_NATIVE_TO_JSPROPERTYOP(getter),                                  \
     JS_CAST_NATIVE_TO_JSSTRICTPROPERTYOP(setter)}
#define JS_PS_END {0, 0, 0, 0, 0}









#if JS_BITS_PER_WORD == 32
typedef const js::Value *ValueArgType;

static JS_ALWAYS_INLINE const js::Value &
ValueArgToConstRef(const js::Value *arg)
{
    return *arg;
}

#elif JS_BITS_PER_WORD == 64
typedef js::Value        ValueArgType;

static JS_ALWAYS_INLINE const Value &
ValueArgToConstRef(const Value &v)
{
    return v;
}
#endif



static JS_ALWAYS_INLINE void
ClearValueRange(Value *vec, uintN len, bool useHoles)
{
    if (useHoles) {
        for (uintN i = 0; i < len; i++)
            vec[i].setMagic(JS_ARRAY_HOLE);
    } else {
        for (uintN i = 0; i < len; i++)
            vec[i].setUndefined();
    }
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *beg, jsid *end)
{
    for (jsid *id = beg; id != end; ++id)
        *id = INT_TO_JSID(0);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(const Shape **beg, const Shape **end)
{
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(const Shape **vec, size_t len)
{
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setUndefined();
}

static JS_ALWAYS_INLINE void
SetValueRangeToUndefined(Value *vec, size_t len)
{
    SetValueRangeToUndefined(vec, vec + len);
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *beg, Value *end)
{
    for (Value *v = beg; v != end; ++v)
        v->setNull();
}

static JS_ALWAYS_INLINE void
SetValueRangeToNull(Value *vec, size_t len)
{
    SetValueRangeToNull(vec, vec + len);
}







static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *beg, Value *end)
{
#ifdef DEBUG
    for (Value *v = beg; v != end; ++v)
        v->setObject(*reinterpret_cast<JSObject *>(0x42));
#endif
}

static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(vec, vec + len);
#endif
}

}      
#endif 

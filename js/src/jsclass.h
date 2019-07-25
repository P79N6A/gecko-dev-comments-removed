






































#ifndef jsclass_h__
#define jsclass_h__






#include "jsapi.h"
#include "jsprvtd.h"

namespace js {

class AutoIdVector;
class PropertyName;
class SpecialId;

static JS_ALWAYS_INLINE jsid
SPECIALID_TO_JSID(const SpecialId &sid);













class SpecialId {
    uintptr_t bits;

    
    friend JS_ALWAYS_INLINE jsid SPECIALID_TO_JSID(const SpecialId &sid);

    static const uintptr_t TYPE_VOID = JSID_TYPE_VOID;
    static const uintptr_t TYPE_OBJECT = JSID_TYPE_OBJECT;
    static const uintptr_t TYPE_DEFAULT_XML_NAMESPACE = JSID_TYPE_DEFAULT_XML_NAMESPACE;
    static const uintptr_t TYPE_MASK = JSID_TYPE_MASK;

    SpecialId(uintptr_t bits) : bits(bits) { }

  public:
    SpecialId() : bits(TYPE_VOID) { }

    

    SpecialId(JSObject &obj)
      : bits(uintptr_t(&obj) | TYPE_OBJECT)
    {
        JS_ASSERT(&obj != NULL);
        JS_ASSERT((uintptr_t(&obj) & TYPE_MASK) == 0);
    }

    bool isObject() const {
        return (bits & TYPE_MASK) == TYPE_OBJECT && bits != TYPE_OBJECT;
    }

    JSObject *toObject() const {
        JS_ASSERT(isObject());
        return reinterpret_cast<JSObject *>(bits & ~TYPE_MASK);
    }

    

    static SpecialId empty() {
        SpecialId sid(TYPE_OBJECT);
        JS_ASSERT(sid.isEmpty());
        return sid;
    }

    bool isEmpty() const {
        return bits == TYPE_OBJECT;
    }

    

    static SpecialId voidId() {
        SpecialId sid(TYPE_VOID);
        JS_ASSERT(sid.isVoid());
        return sid;
    }

    bool isVoid() const {
        return bits == TYPE_VOID;
    }

    

    static SpecialId defaultXMLNamespace() {
        SpecialId sid(TYPE_DEFAULT_XML_NAMESPACE);
        JS_ASSERT(sid.isDefaultXMLNamespace());
        return sid;
    }

    bool isDefaultXMLNamespace() const {
        return bits == TYPE_DEFAULT_XML_NAMESPACE;
    }
};

static JS_ALWAYS_INLINE jsid
SPECIALID_TO_JSID(const SpecialId &sid)
{
    jsid id;
    JSID_BITS(id) = sid.bits;
    JS_ASSERT_IF(sid.isObject(), JSID_IS_OBJECT(id) && JSID_TO_OBJECT(id) == sid.toObject());
    JS_ASSERT_IF(sid.isVoid(), JSID_IS_VOID(id));
    JS_ASSERT_IF(sid.isEmpty(), JSID_IS_EMPTY(id));
    JS_ASSERT_IF(sid.isDefaultXMLNamespace(), JSID_IS_DEFAULT_XML_NAMESPACE(id));
    return id;
}

static JS_ALWAYS_INLINE bool
JSID_IS_SPECIAL(jsid id)
{
    return JSID_IS_OBJECT(id) || JSID_IS_EMPTY(id) || JSID_IS_VOID(id) ||
           JSID_IS_DEFAULT_XML_NAMESPACE(id);
}

static JS_ALWAYS_INLINE SpecialId
JSID_TO_SPECIALID(jsid id)
{
    JS_ASSERT(JSID_IS_SPECIAL(id));
    if (JSID_IS_OBJECT(id))
        return SpecialId(*JSID_TO_OBJECT(id));
    if (JSID_IS_EMPTY(id))
        return SpecialId::empty();
    if (JSID_IS_VOID(id))
        return SpecialId::voidId();
    JS_ASSERT(JSID_IS_DEFAULT_XML_NAMESPACE(id));
    return SpecialId::defaultXMLNamespace();
}



typedef JSBool
(* LookupGenericOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                    JSProperty **propp);
typedef JSBool
(* LookupPropOp)(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                 JSProperty **propp);
typedef JSBool
(* LookupElementOp)(JSContext *cx, JSObject *obj, uint32 index, JSObject **objp,
                    JSProperty **propp);
typedef JSBool
(* LookupSpecialOp)(JSContext *cx, JSObject *obj, SpecialId sid, JSObject **objp,
                    JSProperty **propp);
typedef JSBool
(* DefineGenericOp)(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                    PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* DefinePropOp)(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                 PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* DefineElementOp)(JSContext *cx, JSObject *obj, uint32 index, const Value *value,
                    PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* DefineSpecialOp)(JSContext *cx, JSObject *obj, SpecialId sid, const Value *value,
                    PropertyOp getter, StrictPropertyOp setter, uintN attrs);
typedef JSBool
(* GenericIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);
typedef JSBool
(* PropertyIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name, Value *vp);
typedef JSBool
(* ElementIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp);
typedef JSBool
(* SpecialIdOp)(JSContext *cx, JSObject *obj, JSObject *receiver, SpecialId sid, Value *vp);
typedef JSBool
(* StrictGenericIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* StrictPropertyIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* StrictElementIdOp)(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict);
typedef JSBool
(* StrictSpecialIdOp)(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict);
typedef JSBool
(* GenericAttributesOp)(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);
typedef JSBool
(* AttributesOp)(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);
typedef JSBool
(* ElementAttributesOp)(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp);
typedef JSBool
(* SpecialAttributesOp)(JSContext *cx, JSObject *obj, SpecialId sid, uintN *attrsp);
typedef JSBool
(* DeleteGenericOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* DeleteIdOp)(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
typedef JSBool
(* DeleteElementOp)(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict);
typedef JSBool
(* DeleteSpecialOp)(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict);
typedef JSType
(* TypeOfOp)(JSContext *cx, JSObject *obj);








typedef JSBool
(* FixOp)(JSContext *cx, JSObject *obj, bool *fixed, AutoIdVector *props);

typedef JSObject *
(* ObjectOp)(JSContext *cx, JSObject *obj);
typedef void
(* FinalizeOp)(JSContext *cx, JSObject *obj);

#define JS_CLASS_MEMBERS                                                      \
    const char          *name;                                                \
    uint32              flags;                                                \
                                                                              \
    /* Mandatory non-null function pointer members. */                        \
    JSPropertyOp        addProperty;                                          \
    JSPropertyOp        delProperty;                                          \
    JSPropertyOp        getProperty;                                          \
    JSStrictPropertyOp  setProperty;                                          \
    JSEnumerateOp       enumerate;                                            \
    JSResolveOp         resolve;                                              \
    JSConvertOp         convert;                                              \
    JSFinalizeOp        finalize;                                             \
                                                                              \
    /* Optionally non-null members start here. */                             \
    JSClassInternal     reserved0;                                            \
    JSCheckAccessOp     checkAccess;                                          \
    JSNative            call;                                                 \
    JSNative            construct;                                            \
    JSXDRObjectOp       xdrObject;                                            \
    JSHasInstanceOp     hasInstance;                                          \
    JSTraceOp           trace





struct ClassSizeMeasurement
{
    JS_CLASS_MEMBERS;
};

struct ClassExtension
{
    JSEqualityOp        equality;
    JSObjectOp          outerObject;
    JSObjectOp          innerObject;
    JSIteratorOp        iteratorObject;
    void               *unused;

    



    bool                isWrappedNative;
};

#define JS_NULL_CLASS_EXT   {NULL,NULL,NULL,NULL,NULL,false}

struct ObjectOps
{
    LookupGenericOp     lookupGeneric;
    LookupPropOp        lookupProperty;
    LookupElementOp     lookupElement;
    LookupSpecialOp     lookupSpecial;
    DefineGenericOp     defineGeneric;
    DefinePropOp        defineProperty;
    DefineElementOp     defineElement;
    DefineSpecialOp     defineSpecial;
    GenericIdOp         getGeneric;
    PropertyIdOp        getProperty;
    ElementIdOp         getElement;
    SpecialIdOp         getSpecial;
    StrictGenericIdOp   setGeneric;
    StrictPropertyIdOp  setProperty;
    StrictElementIdOp   setElement;
    StrictSpecialIdOp   setSpecial;
    GenericAttributesOp getGenericAttributes;
    AttributesOp        getAttributes;
    ElementAttributesOp getElementAttributes;
    SpecialAttributesOp getSpecialAttributes;
    GenericAttributesOp setGenericAttributes;
    AttributesOp        setAttributes;
    ElementAttributesOp setElementAttributes;
    SpecialAttributesOp setSpecialAttributes;
    DeleteGenericOp     deleteGeneric;
    DeleteIdOp          deleteProperty;
    DeleteElementOp     deleteElement;
    DeleteSpecialOp     deleteSpecial;

    JSNewEnumerateOp    enumerate;
    TypeOfOp            typeOf;
    FixOp               fix;
    ObjectOp            thisObject;
    FinalizeOp          clear;
};

#define JS_NULL_OBJECT_OPS                                                    \
    {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,   \
     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,   \
     NULL,NULL,NULL,NULL,NULL}

struct Class
{
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

static JS_ALWAYS_INLINE JSClass *
Jsvalify(Class *c)
{
    return (JSClass *)c;
}

static JS_ALWAYS_INLINE Class *
Valueify(JSClass *c)
{
    return (Class *)c;
}

}  
#endif  

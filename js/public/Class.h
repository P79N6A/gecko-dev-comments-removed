







#ifndef js_Class_h
#define js_Class_h

#include "mozilla/NullPtr.h"

#include "jstypes.h"

#include "js/CallArgs.h"
#include "js/Id.h"
#include "js/TypeDecls.h"








struct JSFreeOp;
struct JSFunctionSpec;

namespace js {

struct Class;
class FreeOp;
class PropertyName;
class Shape;



extern JS_FRIEND_DATA(const js::Class* const) FunctionClassPtr;

} 






typedef bool
(* JSPropertyOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                 JS::MutableHandleValue vp);






typedef bool
(* JSStrictPropertyOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                       bool strict, JS::MutableHandleValue vp);














typedef bool
(* JSDeletePropertyOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                       bool *succeeded);































typedef bool
(* JSNewEnumerateOp)(JSContext *cx, JS::HandleObject obj, JSIterateOp enum_op,
                     JS::MutableHandleValue statep, JS::MutableHandleId idp);



typedef bool
(* JSEnumerateOp)(JSContext *cx, JS::HandleObject obj);











typedef bool
(* JSResolveOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id);








typedef bool
(* JSNewResolveOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                   JS::MutableHandleObject objp);



typedef bool
(* JSConvertOp)(JSContext *cx, JS::HandleObject obj, JSType type,
                JS::MutableHandleValue vp);




typedef void
(* JSFinalizeOp)(JSFreeOp *fop, JSObject *obj);


struct JSStringFinalizer {
    void (*finalize)(const JSStringFinalizer *fin, jschar *chars);
};




typedef bool
(* JSHasInstanceOp)(JSContext *cx, JS::HandleObject obj, JS::MutableHandleValue vp,
                    bool *bp);













typedef void
(* JSTraceOp)(JSTracer *trc, JSObject *obj);



typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JS::HandleObject obj, bool keysonly);

typedef JSObject *
(* JSWeakmapKeyDelegateOp)(JSObject *obj);

typedef void
(* JSObjectMovedOp)(JSObject *obj, const JSObject *old);



namespace js {

typedef bool
(* LookupGenericOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                    JS::MutableHandleObject objp, JS::MutableHandle<Shape*> propp);
typedef bool
(* LookupPropOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                 JS::MutableHandleObject objp, JS::MutableHandle<Shape*> propp);
typedef bool
(* LookupElementOp)(JSContext *cx, JS::HandleObject obj, uint32_t index,
                    JS::MutableHandleObject objp, JS::MutableHandle<Shape*> propp);
typedef bool
(* DefineGenericOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue value,
                    JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);
typedef bool
(* DefinePropOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                 JS::HandleValue value, JSPropertyOp getter, JSStrictPropertyOp setter,
                 unsigned attrs);
typedef bool
(* DefineElementOp)(JSContext *cx, JS::HandleObject obj, uint32_t index, JS::HandleValue value,
                    JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);
typedef bool
(* GenericIdOp)(JSContext *cx, JS::HandleObject obj, JS::HandleObject receiver, JS::HandleId id,
                JS::MutableHandleValue vp);
typedef bool
(* PropertyIdOp)(JSContext *cx, JS::HandleObject obj, JS::HandleObject receiver,
                 JS::Handle<PropertyName*> name, JS::MutableHandleValue vp);
typedef bool
(* ElementIdOp)(JSContext *cx, JS::HandleObject obj, JS::HandleObject receiver, uint32_t index,
                JS::MutableHandleValue vp);
typedef bool
(* StrictGenericIdOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                      JS::MutableHandleValue vp, bool strict);
typedef bool
(* StrictPropertyIdOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                       JS::MutableHandleValue vp, bool strict);
typedef bool
(* StrictElementIdOp)(JSContext *cx, JS::HandleObject obj, uint32_t index,
                      JS::MutableHandleValue vp, bool strict);
typedef bool
(* GenericAttributesOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id, unsigned *attrsp);
typedef bool
(* PropertyAttributesOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                         unsigned *attrsp);
typedef bool
(* DeleteGenericOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool *succeeded);

typedef bool
(* WatchOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);

typedef bool
(* UnwatchOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id);

typedef bool
(* SliceOp)(JSContext *cx, JS::HandleObject obj, uint32_t begin, uint32_t end,
            JS::HandleObject result); 



typedef JSObject *
(* ObjectOp)(JSContext *cx, JS::HandleObject obj);


typedef JSObject *
(* InnerObjectOp)(JSObject *obj);

typedef void
(* FinalizeOp)(FreeOp *fop, JSObject *obj);

#define JS_CLASS_MEMBERS(FinalizeOpType)                                      \
    const char          *name;                                                \
    uint32_t            flags;                                                \
                                                                              \
    /* Mandatory function pointer members. */                                 \
    JSPropertyOp        addProperty;                                          \
    JSDeletePropertyOp  delProperty;                                          \
    JSPropertyOp        getProperty;                                          \
    JSStrictPropertyOp  setProperty;                                          \
    JSEnumerateOp       enumerate;                                            \
    JSResolveOp         resolve;                                              \
    JSConvertOp         convert;                                              \
                                                                              \
    /* Optional members (may be null). */                                     \
    FinalizeOpType      finalize;                                             \
    JSNative            call;                                                 \
    JSHasInstanceOp     hasInstance;                                          \
    JSNative            construct;                                            \
    JSTraceOp           trace


typedef JSObject *(*ClassObjectCreationOp)(JSContext *cx, JSProtoKey key);


typedef bool (*FinishClassInitOp)(JSContext *cx, JS::HandleObject ctor,
                                  JS::HandleObject proto);

struct ClassSpec
{
    ClassObjectCreationOp createConstructor;
    ClassObjectCreationOp createPrototype;
    const JSFunctionSpec *constructorFunctions;
    const JSFunctionSpec *prototypeFunctions;
    const JSPropertySpec *prototypeProperties;
    FinishClassInitOp finishInit;
    bool defined() const { return !!createConstructor; }
};

struct ClassExtension
{
    ObjectOp            outerObject;
    InnerObjectOp       innerObject;
    JSIteratorOp        iteratorObject;

    



    bool                isWrappedNative;

    










    JSWeakmapKeyDelegateOp weakmapKeyDelegateOp;

    






    JSObjectMovedOp objectMovedOp;
};

#define JS_NULL_CLASS_SPEC  {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr}
#define JS_NULL_CLASS_EXT   {nullptr,nullptr,nullptr,false,nullptr,nullptr}

struct ObjectOps
{
    LookupGenericOp     lookupGeneric;
    LookupPropOp        lookupProperty;
    LookupElementOp     lookupElement;
    DefineGenericOp     defineGeneric;
    DefinePropOp        defineProperty;
    DefineElementOp     defineElement;
    GenericIdOp         getGeneric;
    PropertyIdOp        getProperty;
    ElementIdOp         getElement;
    StrictGenericIdOp   setGeneric;
    StrictPropertyIdOp  setProperty;
    StrictElementIdOp   setElement;
    GenericAttributesOp getGenericAttributes;
    GenericAttributesOp setGenericAttributes;
    DeleteGenericOp     deleteGeneric;
    WatchOp             watch;
    UnwatchOp           unwatch;
    SliceOp             slice; 
    JSNewEnumerateOp    enumerate;
    ObjectOp            thisObject;
};

#define JS_NULL_OBJECT_OPS                                                    \
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  \
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  \
     nullptr, nullptr, nullptr}

} 



typedef void (*JSClassInternal)();

struct JSClass {
    JS_CLASS_MEMBERS(JSFinalizeOp);

    void                *reserved[32];
};

#define JSCLASS_HAS_PRIVATE             (1<<0)  // objects have private slot
#define JSCLASS_NEW_ENUMERATE           (1<<1)  // has JSNewEnumerateOp hook
#define JSCLASS_NEW_RESOLVE             (1<<2)  // has JSNewResolveOp hook
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)  // private is (nsISupports *)
#define JSCLASS_IS_DOMJSCLASS           (1<<4)  // objects are DOM
#define JSCLASS_IMPLEMENTS_BARRIERS     (1<<5)  // Correctly implements GC read
                                                
#define JSCLASS_EMULATES_UNDEFINED      (1<<6)  // objects of this class act
                                                
                                                
#define JSCLASS_USERBIT1                (1<<7)  // Reserved for embeddings.




#define JSCLASS_RESERVED_SLOTS_SHIFT    8       // room for 8 flags below */
#define JSCLASS_RESERVED_SLOTS_WIDTH    8       // and 16 above this field */
#define JSCLASS_RESERVED_SLOTS_MASK     JS_BITMASK(JSCLASS_RESERVED_SLOTS_WIDTH)
#define JSCLASS_HAS_RESERVED_SLOTS(n)   (((n) & JSCLASS_RESERVED_SLOTS_MASK)  \
                                         << JSCLASS_RESERVED_SLOTS_SHIFT)
#define JSCLASS_RESERVED_SLOTS(clasp)   (((clasp)->flags                      \
                                          >> JSCLASS_RESERVED_SLOTS_SHIFT)    \
                                         & JSCLASS_RESERVED_SLOTS_MASK)

#define JSCLASS_HIGH_FLAGS_SHIFT        (JSCLASS_RESERVED_SLOTS_SHIFT +       \
                                         JSCLASS_RESERVED_SLOTS_WIDTH)

#define JSCLASS_IS_ANONYMOUS            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+0))
#define JSCLASS_IS_GLOBAL               (1<<(JSCLASS_HIGH_FLAGS_SHIFT+1))
#define JSCLASS_INTERNAL_FLAG2          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+2))
#define JSCLASS_INTERNAL_FLAG3          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+3))

#define JSCLASS_IS_PROXY                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+4))




#define JSCLASS_USERBIT2                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+6))
#define JSCLASS_USERBIT3                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+7))

#define JSCLASS_BACKGROUND_FINALIZE     (1<<(JSCLASS_HIGH_FLAGS_SHIFT+8))

















#define JSCLASS_GLOBAL_APPLICATION_SLOTS 3
#define JSCLASS_GLOBAL_SLOT_COUNT      (JSCLASS_GLOBAL_APPLICATION_SLOTS + JSProto_LIMIT * 3 + 30)
#define JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(n)                                    \
    (JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(JSCLASS_GLOBAL_SLOT_COUNT + (n)))
#define JSCLASS_GLOBAL_FLAGS                                                  \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0)
#define JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(clasp)                              \
  (((clasp)->flags & JSCLASS_IS_GLOBAL)                                       \
   && JSCLASS_RESERVED_SLOTS(clasp) >= JSCLASS_GLOBAL_SLOT_COUNT)


#define JSCLASS_CACHED_PROTO_SHIFT      (JSCLASS_HIGH_FLAGS_SHIFT + 10)
#define JSCLASS_CACHED_PROTO_WIDTH      6
#define JSCLASS_CACHED_PROTO_MASK       JS_BITMASK(JSCLASS_CACHED_PROTO_WIDTH)
#define JSCLASS_HAS_CACHED_PROTO(key)   (uint32_t(key) << JSCLASS_CACHED_PROTO_SHIFT)
#define JSCLASS_CACHED_PROTO_KEY(clasp) ((JSProtoKey)                         \
                                         (((clasp)->flags                     \
                                           >> JSCLASS_CACHED_PROTO_SHIFT)     \
                                          & JSCLASS_CACHED_PROTO_MASK))


#define JSCLASS_NO_INTERNAL_MEMBERS     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define JSCLASS_NO_OPTIONAL_MEMBERS     0,0,0,0,0,JSCLASS_NO_INTERNAL_MEMBERS

namespace js {

struct Class
{
    JS_CLASS_MEMBERS(FinalizeOp);
    ClassSpec          spec;
    ClassExtension      ext;
    ObjectOps           ops;

    
    static const uint32_t NON_NATIVE = JSCLASS_INTERNAL_FLAG2;

    bool isNative() const {
        return !(flags & NON_NATIVE);
    }

    bool hasPrivate() const {
        return !!(flags & JSCLASS_HAS_PRIVATE);
    }

    bool emulatesUndefined() const {
        return flags & JSCLASS_EMULATES_UNDEFINED;
    }

    bool isCallable() const {
        return this == js::FunctionClassPtr || call;
    }

    bool isProxy() const {
        return flags & JSCLASS_IS_PROXY;
    }

    bool isDOMClass() const {
        return flags & JSCLASS_IS_DOMJSCLASS;
    }

    static size_t offsetOfFlags() { return offsetof(Class, flags); }
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
JS_STATIC_ASSERT(offsetof(JSClass, call) == offsetof(Class, call));
JS_STATIC_ASSERT(offsetof(JSClass, construct) == offsetof(Class, construct));
JS_STATIC_ASSERT(offsetof(JSClass, hasInstance) == offsetof(Class, hasInstance));
JS_STATIC_ASSERT(offsetof(JSClass, trace) == offsetof(Class, trace));
JS_STATIC_ASSERT(sizeof(JSClass) == sizeof(Class));

static MOZ_ALWAYS_INLINE const JSClass *
Jsvalify(const Class *c)
{
    return (const JSClass *)c;
}

static MOZ_ALWAYS_INLINE const Class *
Valueify(const JSClass *c)
{
    return (const Class *)c;
}





enum ESClassValue {
    ESClass_Object, ESClass_Array, ESClass_Number, ESClass_String,
    ESClass_Boolean, ESClass_RegExp, ESClass_ArrayBuffer, ESClass_Date,
    ESClass_Set, ESClass_Map
};







inline bool
ObjectClassIs(JSObject &obj, ESClassValue classValue, JSContext *cx);


inline bool
IsObjectWithClass(const JS::Value &v, ESClassValue classValue, JSContext *cx);


inline bool
Unbox(JSContext *cx, JS::HandleObject obj, JS::MutableHandleValue vp);

#ifdef DEBUG
JS_FRIEND_API(bool)
HasObjectMovedOp(JSObject *obj);
#endif

}  

#endif  

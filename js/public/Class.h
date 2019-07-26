







#ifndef js_Class_h
#define js_Class_h

#include "mozilla/NullPtr.h"
 
#include "jstypes.h"

#include "js/CallArgs.h"
#include "js/Id.h"
#include "js/TypeDecls.h"








class JSFreeOp;

namespace js {

class Class;
class FreeOp;
class PropertyId;
class PropertyName;
class Shape;
class SpecialId;



extern JS_FRIEND_DATA(const js::Class* const) FunctionClassPtr;

static JS_ALWAYS_INLINE jsid
SPECIALID_TO_JSID(const SpecialId &sid);












class SpecialId
{
    uintptr_t bits_;

    
    friend JS_ALWAYS_INLINE jsid SPECIALID_TO_JSID(const SpecialId &sid);
    friend class PropertyId;

    static const uintptr_t TYPE_VOID = JSID_TYPE_VOID;
    static const uintptr_t TYPE_OBJECT = JSID_TYPE_OBJECT;
    static const uintptr_t TYPE_MASK = JSID_TYPE_MASK;

    SpecialId(uintptr_t bits) : bits_(bits) { }

  public:
    SpecialId() : bits_(TYPE_VOID) { }

    

    SpecialId(JSObject &obj)
      : bits_(uintptr_t(&obj) | TYPE_OBJECT)
    {
        JS_ASSERT(&obj != nullptr);
        JS_ASSERT((uintptr_t(&obj) & TYPE_MASK) == 0);
    }

    bool isObject() const {
        return (bits_ & TYPE_MASK) == TYPE_OBJECT && bits_ != TYPE_OBJECT;
    }

    JSObject *toObject() const {
        JS_ASSERT(isObject());
        return reinterpret_cast<JSObject *>(bits_ & ~TYPE_MASK);
    }

    

    static SpecialId empty() {
        SpecialId sid(TYPE_OBJECT);
        JS_ASSERT(sid.isEmpty());
        return sid;
    }

    bool isEmpty() const {
        return bits_ == TYPE_OBJECT;
    }

    

    static SpecialId voidId() {
        SpecialId sid(TYPE_VOID);
        JS_ASSERT(sid.isVoid());
        return sid;
    }

    bool isVoid() const {
        return bits_ == TYPE_VOID;
    }
};

static JS_ALWAYS_INLINE jsid
SPECIALID_TO_JSID(const SpecialId &sid)
{
    jsid id;
    JSID_BITS(id) = sid.bits_;
    JS_ASSERT_IF(sid.isObject(), JSID_IS_OBJECT(id) && JSID_TO_OBJECT(id) == sid.toObject());
    JS_ASSERT_IF(sid.isVoid(), JSID_IS_VOID(id));
    JS_ASSERT_IF(sid.isEmpty(), JSID_IS_EMPTY(id));
    return id;
}

static JS_ALWAYS_INLINE bool
JSID_IS_SPECIAL(jsid id)
{
    return JSID_IS_OBJECT(id) || JSID_IS_EMPTY(id) || JSID_IS_VOID(id);
}

static JS_ALWAYS_INLINE SpecialId
JSID_TO_SPECIALID(jsid id)
{
    JS_ASSERT(JSID_IS_SPECIAL(id));
    if (JSID_IS_OBJECT(id))
        return SpecialId(*JSID_TO_OBJECT(id));
    if (JSID_IS_EMPTY(id))
        return SpecialId::empty();
    JS_ASSERT(JSID_IS_VOID(id));
    return SpecialId::voidId();
}

typedef JS::Handle<SpecialId> HandleSpecialId;

} 






typedef bool
(* JSPropertyOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                 JS::MutableHandle<JS::Value> vp);






typedef bool
(* JSStrictPropertyOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                       bool strict, JS::MutableHandle<JS::Value> vp);














typedef bool
(* JSDeletePropertyOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                       bool *succeeded);































typedef bool
(* JSNewEnumerateOp)(JSContext *cx, JS::Handle<JSObject*> obj, JSIterateOp enum_op,
                     JS::MutableHandle<JS::Value> statep, JS::MutableHandle<jsid> idp);



typedef bool
(* JSEnumerateOp)(JSContext *cx, JS::Handle<JSObject*> obj);











typedef bool
(* JSResolveOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id);











typedef bool
(* JSNewResolveOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, unsigned flags,
                   JS::MutableHandle<JSObject*> objp);



typedef bool
(* JSConvertOp)(JSContext *cx, JS::Handle<JSObject*> obj, JSType type,
                JS::MutableHandle<JS::Value> vp);




typedef void
(* JSFinalizeOp)(JSFreeOp *fop, JSObject *obj);


struct JSStringFinalizer {
    void (*finalize)(const JSStringFinalizer *fin, jschar *chars);
};





typedef bool
(* JSCheckAccessOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                    JSAccessMode mode, JS::MutableHandle<JS::Value> vp);




typedef bool
(* JSHasInstanceOp)(JSContext *cx, JS::Handle<JSObject*> obj, JS::MutableHandle<JS::Value> vp,
                    bool *bp);













typedef void
(* JSTraceOp)(JSTracer *trc, JSObject *obj);



typedef JSObject *
(* JSObjectOp)(JSContext *cx, JS::Handle<JSObject*> obj);



typedef JSObject *
(* JSIteratorOp)(JSContext *cx, JS::HandleObject obj, bool keysonly);

typedef JSObject *
(* JSWeakmapKeyDelegateOp)(JSObject *obj);



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
(* LookupSpecialOp)(JSContext *cx, JS::HandleObject obj, HandleSpecialId sid,
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
(* DefineSpecialOp)(JSContext *cx, JS::HandleObject obj, HandleSpecialId sid,
                    JS::HandleValue value, JSPropertyOp getter, JSStrictPropertyOp setter,
                    unsigned attrs);
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
(* ElementIfPresentOp)(JSContext *cx, JS::HandleObject obj, JS::HandleObject receiver,
                       uint32_t index, JS::MutableHandleValue vp, bool* present);
typedef bool
(* SpecialIdOp)(JSContext *cx, JS::HandleObject obj, JS::HandleObject receiver,
                HandleSpecialId sid, JS::MutableHandleValue vp);
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
(* StrictSpecialIdOp)(JSContext *cx, JS::HandleObject obj, HandleSpecialId sid,
                      JS::MutableHandleValue vp, bool strict);
typedef bool
(* GenericAttributesOp)(JSContext *cx, JS::HandleObject obj, JS::HandleId id, unsigned *attrsp);
typedef bool
(* PropertyAttributesOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                         unsigned *attrsp);
typedef bool
(* DeletePropertyOp)(JSContext *cx, JS::HandleObject obj, JS::Handle<PropertyName*> name,
                     bool *succeeded);
typedef bool
(* DeleteElementOp)(JSContext *cx, JS::HandleObject obj, uint32_t index, bool *succeeded);
typedef bool
(* DeleteSpecialOp)(JSContext *cx, JS::HandleObject obj, HandleSpecialId sid, bool *succeeded);


typedef JSObject *
(* ObjectOp)(JSContext *cx, JS::HandleObject obj);
typedef void
(* FinalizeOp)(FreeOp *fop, JSObject *obj);

#define JS_CLASS_MEMBERS                                                      \
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
    FinalizeOp          finalize;                                             \
    JSCheckAccessOp     checkAccess;                                          \
    JSNative            call;                                                 \
    JSHasInstanceOp     hasInstance;                                          \
    JSNative            construct;                                            \
    JSTraceOp           trace





struct ClassSizeMeasurement
{
    JS_CLASS_MEMBERS;
};

struct ClassExtension
{
    JSObjectOp          outerObject;
    JSObjectOp          innerObject;
    JSIteratorOp        iteratorObject;

    



    bool                isWrappedNative;

    










    JSWeakmapKeyDelegateOp weakmapKeyDelegateOp;
};

#define JS_NULL_CLASS_EXT   {nullptr,nullptr,nullptr,false,nullptr}

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
    ElementIfPresentOp  getElementIfPresent; 
    SpecialIdOp         getSpecial;
    StrictGenericIdOp   setGeneric;
    StrictPropertyIdOp  setProperty;
    StrictElementIdOp   setElement;
    StrictSpecialIdOp   setSpecial;
    GenericAttributesOp getGenericAttributes;
    GenericAttributesOp setGenericAttributes;
    DeletePropertyOp    deleteProperty;
    DeleteElementOp     deleteElement;
    DeleteSpecialOp     deleteSpecial;

    JSNewEnumerateOp    enumerate;
    ObjectOp            thisObject;
};

#define JS_NULL_OBJECT_OPS                                                    \
    {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr, \
     nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr, \
     nullptr,nullptr,nullptr,nullptr,nullptr,nullptr}

} 



typedef void (*JSClassInternal)();

struct JSClass {
    const char          *name;
    uint32_t            flags;

    
    JSPropertyOp        addProperty;
    JSDeletePropertyOp  delProperty;
    JSPropertyOp        getProperty;
    JSStrictPropertyOp  setProperty;
    JSEnumerateOp       enumerate;
    JSResolveOp         resolve;
    JSConvertOp         convert;

    
    JSFinalizeOp        finalize;
    JSCheckAccessOp     checkAccess;
    JSNative            call;
    JSHasInstanceOp     hasInstance;
    JSNative            construct;
    JSTraceOp           trace;

    void                *reserved[40];
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


#define JSCLASS_FREEZE_PROTO            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+4))
#define JSCLASS_FREEZE_CTOR             (1<<(JSCLASS_HIGH_FLAGS_SHIFT+5))


#define JSCLASS_USERBIT2                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+6))
#define JSCLASS_USERBIT3                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+7))

#define JSCLASS_BACKGROUND_FINALIZE     (1<<(JSCLASS_HIGH_FLAGS_SHIFT+8))














#define JSCLASS_GLOBAL_SLOT_COUNT      (3 + JSProto_LIMIT * 3 + 27)
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
    JS_CLASS_MEMBERS;
    ClassExtension      ext;
    ObjectOps           ops;
    uint8_t             pad[sizeof(JSClass) - sizeof(ClassSizeMeasurement) -
                            sizeof(ClassExtension) - sizeof(ObjectOps)];

    
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
JS_STATIC_ASSERT(offsetof(JSClass, checkAccess) == offsetof(Class, checkAccess));
JS_STATIC_ASSERT(offsetof(JSClass, call) == offsetof(Class, call));
JS_STATIC_ASSERT(offsetof(JSClass, construct) == offsetof(Class, construct));
JS_STATIC_ASSERT(offsetof(JSClass, hasInstance) == offsetof(Class, hasInstance));
JS_STATIC_ASSERT(offsetof(JSClass, trace) == offsetof(Class, trace));
JS_STATIC_ASSERT(sizeof(JSClass) == sizeof(Class));

static JS_ALWAYS_INLINE const JSClass *
Jsvalify(const Class *c)
{
    return (const JSClass *)c;
}

static JS_ALWAYS_INLINE const Class *
Valueify(const JSClass *c)
{
    return (const Class *)c;
}





enum ESClassValue {
    ESClass_Array, ESClass_Number, ESClass_String, ESClass_Boolean,
    ESClass_RegExp, ESClass_ArrayBuffer, ESClass_Date
};







inline bool
ObjectClassIs(JSObject &obj, ESClassValue classValue, JSContext *cx);


inline bool
IsObjectWithClass(const JS::Value &v, ESClassValue classValue, JSContext *cx);

inline bool
IsPoisonedSpecialId(js::SpecialId iden)
{
    if (iden.isObject())
        return JS::IsPoisonedPtr(iden.toObject());
    return false;
}

template <> struct GCMethods<SpecialId>
{
    static SpecialId initial() { return SpecialId(); }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(SpecialId id) { return IsPoisonedSpecialId(id); }
};

}  

#endif  

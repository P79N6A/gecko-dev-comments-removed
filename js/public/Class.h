







#ifndef js_Class_h
#define js_Class_h

#include "mozilla/DebugOnly.h"

#include "jstypes.h"

#include "js/CallArgs.h"
#include "js/Id.h"
#include "js/TypeDecls.h"








struct JSAtomState;
struct JSFreeOp;
struct JSFunctionSpec;

namespace js {

struct Class;
class FreeOp;
class Shape;



extern JS_FRIEND_DATA(const js::Class* const) FunctionClassPtr;

} 

namespace JS {

class AutoIdVector;


































class ObjectOpResult
{
  private:
    








    uintptr_t code_;

  public:
    enum SpecialCodes : uintptr_t {
        OkCode = 0,
        Uninitialized = uintptr_t(-1)
    };

    ObjectOpResult() : code_(Uninitialized) {}

    
    bool ok() const {
        MOZ_ASSERT(code_ != Uninitialized);
        return code_ == OkCode;
    }

    explicit operator bool() const { return ok(); }

    
    bool succeed() {
        code_ = OkCode;
        return true;
    }

    










    bool fail(uint32_t msg) {
        MOZ_ASSERT(msg != OkCode);
        code_ = msg;
        return true;
    }

    JS_PUBLIC_API(bool) failCantRedefineProp();
    JS_PUBLIC_API(bool) failReadOnly();
    JS_PUBLIC_API(bool) failGetterOnly();
    JS_PUBLIC_API(bool) failCantDelete();

    JS_PUBLIC_API(bool) failCantSetInterposed();
    JS_PUBLIC_API(bool) failCantDefineWindowElement();
    JS_PUBLIC_API(bool) failCantDeleteWindowElement();
    JS_PUBLIC_API(bool) failCantDeleteWindowNamedProperty();
    JS_PUBLIC_API(bool) failCantPreventExtensions();

    uint32_t failureCode() const {
        MOZ_ASSERT(!ok());
        return uint32_t(code_);
    }

    













    bool checkStrictErrorOrWarning(JSContext* cx, HandleObject obj, HandleId id, bool strict) {
        if (ok())
            return true;
        return reportStrictErrorOrWarning(cx, obj, id, strict);
    }

    





    bool checkStrictErrorOrWarning(JSContext* cx, HandleObject obj, bool strict) {
        return ok() || reportStrictErrorOrWarning(cx, obj, strict);
    }

    
    bool reportError(JSContext* cx, HandleObject obj, HandleId id) {
        return reportStrictErrorOrWarning(cx, obj, id, true);
    }

    



    bool reportError(JSContext* cx, HandleObject obj) {
        return reportStrictErrorOrWarning(cx, obj, true);
    }

    
    JS_PUBLIC_API(bool) reportStrictErrorOrWarning(JSContext* cx, HandleObject obj, HandleId id, bool strict);
    JS_PUBLIC_API(bool) reportStrictErrorOrWarning(JSContext* cx, HandleObject obj, bool strict);

    



    bool checkStrict(JSContext* cx, HandleObject obj, HandleId id) {
        return checkStrictErrorOrWarning(cx, obj, id, true);
    }

    



    bool checkStrict(JSContext* cx, HandleObject obj) {
        return checkStrictErrorOrWarning(cx, obj, true);
    }
};

}






typedef bool
(* JSGetterOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
               JS::MutableHandleValue vp);


typedef bool
(* JSAddPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v);






typedef bool
(* JSSetterOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
               JS::MutableHandleValue vp, JS::ObjectOpResult& result);














typedef bool
(* JSDeletePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                       JS::ObjectOpResult& result);












typedef bool
(* JSNewEnumerateOp)(JSContext* cx, JS::HandleObject obj, JS::AutoIdVector& properties);



typedef bool
(* JSEnumerateOp)(JSContext* cx, JS::HandleObject obj);









typedef bool
(* JSResolveOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                bool* resolvedp);










typedef bool
(* JSMayResolveOp)(const JSAtomState& names, jsid id, JSObject* maybeObj);



typedef bool
(* JSConvertOp)(JSContext* cx, JS::HandleObject obj, JSType type,
                JS::MutableHandleValue vp);




typedef void
(* JSFinalizeOp)(JSFreeOp* fop, JSObject* obj);


struct JSStringFinalizer {
    void (*finalize)(const JSStringFinalizer* fin, char16_t* chars);
};




typedef bool
(* JSHasInstanceOp)(JSContext* cx, JS::HandleObject obj, JS::MutableHandleValue vp,
                    bool* bp);













typedef void
(* JSTraceOp)(JSTracer* trc, JSObject* obj);

typedef JSObject*
(* JSWeakmapKeyDelegateOp)(JSObject* obj);

typedef void
(* JSObjectMovedOp)(JSObject* obj, const JSObject* old);



namespace js {

typedef bool
(* LookupPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::MutableHandleObject objp, JS::MutableHandle<Shape*> propp);
typedef bool
(* DefinePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::Handle<JSPropertyDescriptor> desc,
                     JS::ObjectOpResult& result);
typedef bool
(* HasPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp);
typedef bool
(* GetPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleObject receiver, JS::HandleId id,
                  JS::MutableHandleValue vp);
typedef bool
(* SetPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v,
                  JS::HandleValue receiver, JS::ObjectOpResult& result);
typedef bool
(* GetOwnPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::MutableHandle<JSPropertyDescriptor> desc);
typedef bool
(* DeletePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::ObjectOpResult& result);

typedef bool
(* WatchOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);

typedef bool
(* UnwatchOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id);

class JS_FRIEND_API(ElementAdder)
{
  public:
    enum GetBehavior {
        
        
        CheckHasElemPreserveHoles,

        
        GetElement
    };

  private:
    
    JS::RootedObject resObj_;
    JS::Value* vp_;

    uint32_t index_;
    mozilla::DebugOnly<uint32_t> length_;
    GetBehavior getBehavior_;

  public:
    ElementAdder(JSContext* cx, JSObject* obj, uint32_t length, GetBehavior behavior)
      : resObj_(cx, obj), vp_(nullptr), index_(0), length_(length), getBehavior_(behavior)
    {}
    ElementAdder(JSContext* cx, JS::Value* vp, uint32_t length, GetBehavior behavior)
      : resObj_(cx), vp_(vp), index_(0), length_(length), getBehavior_(behavior)
    {}

    GetBehavior getBehavior() const { return getBehavior_; }

    void append(JSContext* cx, JS::HandleValue v);
    void appendHole();
};

typedef bool
(* GetElementsOp)(JSContext* cx, JS::HandleObject obj, uint32_t begin, uint32_t end,
                  ElementAdder* adder);



typedef JSObject*
(* ObjectOp)(JSContext* cx, JS::HandleObject obj);


typedef JSObject*
(* InnerObjectOp)(JSObject* obj);

typedef void
(* FinalizeOp)(FreeOp* fop, JSObject* obj);

#define JS_CLASS_MEMBERS(FinalizeOpType)                                      \
    const char*         name;                                                \
    uint32_t            flags;                                                \
                                                                              \
    /* Function pointer members (may be null). */                             \
    JSAddPropertyOp     addProperty;                                          \
    JSDeletePropertyOp  delProperty;                                          \
    JSGetterOp          getProperty;                                          \
    JSSetterOp          setProperty;                                          \
    JSEnumerateOp       enumerate;                                            \
    JSResolveOp         resolve;                                              \
    JSMayResolveOp      mayResolve;                                           \
    JSConvertOp         convert;                                              \
    FinalizeOpType      finalize;                                             \
    JSNative            call;                                                 \
    JSHasInstanceOp     hasInstance;                                          \
    JSNative            construct;                                            \
    JSTraceOp           trace


typedef JSObject* (*ClassObjectCreationOp)(JSContext* cx, JSProtoKey key);


typedef bool (*FinishClassInitOp)(JSContext* cx, JS::HandleObject ctor,
                                  JS::HandleObject proto);

const size_t JSCLASS_CACHED_PROTO_WIDTH = 6;

struct ClassSpec
{
    ClassObjectCreationOp createConstructor;
    ClassObjectCreationOp createPrototype;
    const JSFunctionSpec* constructorFunctions;
    const JSPropertySpec* constructorProperties;
    const JSFunctionSpec* prototypeFunctions;
    const JSPropertySpec* prototypeProperties;
    FinishClassInitOp finishInit;
    uintptr_t flags;

    static const size_t ParentKeyWidth = JSCLASS_CACHED_PROTO_WIDTH;

    static const uintptr_t ParentKeyMask = (1 << ParentKeyWidth) - 1;
    static const uintptr_t DontDefineConstructor = 1 << ParentKeyWidth;

    bool defined() const { return !!createConstructor; }

    bool dependent() const {
        MOZ_ASSERT(defined());
        return (flags & ParentKeyMask);
    }

    JSProtoKey parentKey() const {
        static_assert(JSProto_Null == 0, "zeroed key must be null");
        return JSProtoKey(flags & ParentKeyMask);
    }

    bool shouldDefineConstructor() const {
        MOZ_ASSERT(defined());
        return !(flags & DontDefineConstructor);
    }
};

struct ClassExtension
{
    ObjectOp            outerObject;
    InnerObjectOp       innerObject;

    



    bool                isWrappedNative;

    










    JSWeakmapKeyDelegateOp weakmapKeyDelegateOp;

    










    JSObjectMovedOp objectMovedOp;
};

#define JS_NULL_CLASS_SPEC  {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr}
#define JS_NULL_CLASS_EXT   {nullptr,nullptr,false,nullptr,nullptr}

struct ObjectOps
{
    LookupPropertyOp    lookupProperty;
    DefinePropertyOp    defineProperty;
    HasPropertyOp       hasProperty;
    GetPropertyOp       getProperty;
    SetPropertyOp       setProperty;
    GetOwnPropertyOp    getOwnPropertyDescriptor;
    DeletePropertyOp    deleteProperty;
    WatchOp             watch;
    UnwatchOp           unwatch;
    GetElementsOp       getElements;
    JSNewEnumerateOp    enumerate;
    ObjectOp            thisObject;
};

#define JS_NULL_OBJECT_OPS                                                    \
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  \
     nullptr, nullptr, nullptr, nullptr}

} 



typedef void (*JSClassInternal)();

struct JSClass {
    JS_CLASS_MEMBERS(JSFinalizeOp);

    void*               reserved[25];
};

#define JSCLASS_HAS_PRIVATE             (1<<0)  // objects have private slot
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)  // private is (nsISupports*)
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

#define JSCLASS_FINALIZE_FROM_NURSERY   (1<<(JSCLASS_HIGH_FLAGS_SHIFT+5))


#define JSCLASS_USERBIT2                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+6))
#define JSCLASS_USERBIT3                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+7))

#define JSCLASS_BACKGROUND_FINALIZE     (1<<(JSCLASS_HIGH_FLAGS_SHIFT+8))

















#define JSCLASS_GLOBAL_APPLICATION_SLOTS 4
#define JSCLASS_GLOBAL_SLOT_COUNT      (JSCLASS_GLOBAL_APPLICATION_SLOTS + JSProto_LIMIT * 3 + 31)
#define JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(n)                                    \
    (JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(JSCLASS_GLOBAL_SLOT_COUNT + (n)))
#define JSCLASS_GLOBAL_FLAGS                                                  \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0)
#define JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(clasp)                              \
  (((clasp)->flags & JSCLASS_IS_GLOBAL)                                       \
   && JSCLASS_RESERVED_SLOTS(clasp) >= JSCLASS_GLOBAL_SLOT_COUNT)


#define JSCLASS_CACHED_PROTO_SHIFT      (JSCLASS_HIGH_FLAGS_SHIFT + 10)
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

    bool isJSFunction() const {
        return this == js::FunctionClassPtr;
    }

    bool nonProxyCallable() const {
        MOZ_ASSERT(!isProxy());
        return isJSFunction() || call;
    }

    bool isProxy() const {
        return flags & JSCLASS_IS_PROXY;
    }

    bool isDOMClass() const {
        return flags & JSCLASS_IS_DOMJSCLASS;
    }

    static size_t offsetOfFlags() { return offsetof(Class, flags); }
};

static_assert(offsetof(JSClass, name) == offsetof(Class, name),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, flags) == offsetof(Class, flags),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, addProperty) == offsetof(Class, addProperty),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, delProperty) == offsetof(Class, delProperty),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, getProperty) == offsetof(Class, getProperty),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, setProperty) == offsetof(Class, setProperty),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, enumerate) == offsetof(Class, enumerate),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, resolve) == offsetof(Class, resolve),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, mayResolve) == offsetof(Class, mayResolve),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, convert) == offsetof(Class, convert),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, finalize) == offsetof(Class, finalize),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, call) == offsetof(Class, call),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, construct) == offsetof(Class, construct),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, hasInstance) == offsetof(Class, hasInstance),
              "Class and JSClass must be consistent");
static_assert(offsetof(JSClass, trace) == offsetof(Class, trace),
              "Class and JSClass must be consistent");
static_assert(sizeof(JSClass) == sizeof(Class),
              "Class and JSClass must be consistent");

static MOZ_ALWAYS_INLINE const JSClass*
Jsvalify(const Class* c)
{
    return (const JSClass*)c;
}

static MOZ_ALWAYS_INLINE const Class*
Valueify(const JSClass* c)
{
    return (const Class*)c;
}





enum ESClassValue {
    ESClass_Object, ESClass_Array, ESClass_Number, ESClass_String,
    ESClass_Boolean, ESClass_RegExp, ESClass_ArrayBuffer, ESClass_SharedArrayBuffer,
    ESClass_Date, ESClass_Set, ESClass_Map,

    
    
    ESClass_IsArray
};







inline bool
ObjectClassIs(JSObject& obj, ESClassValue classValue, JSContext* cx);


inline bool
IsObjectWithClass(const JS::Value& v, ESClassValue classValue, JSContext* cx);


inline bool
Unbox(JSContext* cx, JS::HandleObject obj, JS::MutableHandleValue vp);

#ifdef DEBUG
JS_FRIEND_API(bool)
HasObjectMovedOp(JSObject* obj);
#endif

}  

#endif  

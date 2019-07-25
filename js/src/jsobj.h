







































#ifndef jsobj_h___
#define jsobj_h___









#include "jsapi.h"
#include "jsclass.h"
#include "jsinfer.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jsvector.h"
#include "jscell.h"

#include "vm/String.h"

namespace nanojit { class ValidateWriter; }

namespace js {

class AutoPropDescArrayRooter;
class ProxyHandler;
class RegExp;
class CallObject;
struct GCMarker;
struct NativeIterator;

namespace mjit { class Compiler; }

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline StrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
}

static inline PropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline StrictPropertyOp
CastAsJSStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline JSObject *
CastAsObject(StrictPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(StrictPropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}







#define JS_PSG(name,getter,flags)                                             \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     (JSPropertyOp)getter, NULL}
#define JS_PSGS(name,getter,setter,flags)                                     \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     (JSPropertyOp)getter, (JSStrictPropertyOp)setter}
#define JS_PS_END {0, 0, 0, 0, 0}







struct PropDesc {
    



    js::Value pd;

    js::Value value, get, set;

    
    uint8 attrs;

    
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;

    friend class js::AutoPropDescArrayRooter;

    PropDesc();

    









    bool initialize(JSContext* cx, const js::Value &v, bool checkAccessors=true);

    








    void initFromPropertyDescriptor(const PropertyDescriptor &desc);
    bool makeObject(JSContext *cx);

    
    bool isAccessorDescriptor() const {
        return hasGet || hasSet;
    }

    
    bool isDataDescriptor() const {
        return hasValue || hasWritable;
    }

    
    bool isGenericDescriptor() const {
        return !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }

    JSObject* getterObject() const {
        return get.isUndefined() ? NULL : &get.toObject();
    }
    JSObject* setterObject() const {
        return set.isUndefined() ? NULL : &set.toObject();
    }

    const js::Value &getterValue() const {
        return get;
    }
    const js::Value &setterValue() const {
        return set;
    }

    PropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    StrictPropertyOp setter() const {
        return js::CastAsStrictPropertyOp(setterObject());
    }

    




    inline bool checkGetter(JSContext *cx);
    inline bool checkSetter(JSContext *cx);
};

typedef Vector<PropDesc, 1> PropDescArray;

} 

enum {
    INVALID_SHAPE = 0x8fffffff,
    SHAPELESS = 0xffffffff
};






extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);

extern JS_FRIEND_API(JSBool)
js_LookupElement(JSContext *cx, JSObject *obj, uint32 index, JSObject **objp, JSProperty **propp);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_DefineElement(JSContext *cx, JSObject *obj, uint32 index, const js::Value *value,
                 JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, js::Value *vp);

extern JSBool
js_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32, js::Value *vp);

inline JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp)
{
    return js_GetProperty(cx, obj, obj, id, vp);
}

inline JSBool
js_GetElement(JSContext *cx, JSObject *obj, uint32 index, js::Value *vp)
{
    return js_GetElement(cx, obj, obj, index, vp);
}

namespace js {

extern JSBool
GetPropertyDefault(JSContext *cx, JSObject *obj, jsid id, const Value &def, Value *vp);

} 

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     js::Value *vp, JSBool strict);

extern JSBool
js_SetElementHelper(JSContext *cx, JSObject *obj, uint32 index, uintN defineHow,
                    js::Value *vp, JSBool strict);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_GetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteElement(JSContext *cx, JSObject *obj, uint32 index, js::Value *rval, JSBool strict);

extern JS_FRIEND_API(JSBool)
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             js::Value *statep, jsid *idp);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

namespace js {


extern JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);

extern JS_FRIEND_DATA(Class) AnyNameClass;
extern JS_FRIEND_DATA(Class) AttributeNameClass;
extern JS_FRIEND_DATA(Class) CallClass;
extern JS_FRIEND_DATA(Class) DeclEnvClass;
extern JS_FRIEND_DATA(Class) FunctionClass;
extern JS_FRIEND_DATA(Class) FunctionProxyClass;
extern JS_FRIEND_DATA(Class) NamespaceClass;
extern JS_FRIEND_DATA(Class) OuterWindowProxyClass;
extern JS_FRIEND_DATA(Class) ObjectProxyClass;
extern JS_FRIEND_DATA(Class) QNameClass;
extern JS_FRIEND_DATA(Class) ScriptClass;
extern JS_FRIEND_DATA(Class) XMLClass;

extern Class ArrayClass;
extern Class ArrayBufferClass;
extern Class BlockClass;
extern Class BooleanClass;
extern Class CallableObjectClass;
extern Class DateClass;
extern Class ErrorClass;
extern Class GeneratorClass;
extern Class IteratorClass;
extern Class JSONClass;
extern Class MathClass;
extern Class NumberClass;
extern Class NormalArgumentsObjectClass;
extern Class ObjectClass;
extern Class ProxyClass;
extern Class RegExpClass;
extern Class SlowArrayClass;
extern Class StopIterationClass;
extern Class StringClass;
extern Class StrictArgumentsObjectClass;
extern Class WeakMapClass;
extern Class WithClass;
extern Class XMLFilterClass;

class ArgumentsObject;
class GlobalObject;
class NormalArgumentsObject;
class NumberObject;
class StrictArgumentsObject;
class StringObject;

}  
























































struct JSObject : js::gc::Cell {
    





    friend class js::TraceRecorder;
    friend class nanojit::ValidateWriter;

    



    js::Shape           *lastProp;

  private:
    js::Class           *clasp;

    inline void setLastProperty(const js::Shape *shape);
    inline void removeLastProperty();

    
    friend class js::StringObject;

#ifdef DEBUG
    void checkShapeConsistency();
#endif

  public:
    inline const js::Shape *lastProperty() const;

    inline js::Shape **nativeSearch(JSContext *cx, jsid id, bool adding = false);
    inline const js::Shape *nativeLookup(JSContext *cx, jsid id);

    inline bool nativeContains(JSContext *cx, jsid id);
    inline bool nativeContains(JSContext *cx, const js::Shape &shape);

    enum {
        DELEGATE                  =       0x01,
        SYSTEM                    =       0x02,
        NOT_EXTENSIBLE            =       0x04,
        BRANDED                   =       0x08,
        GENERIC                   =       0x10,
        METHOD_BARRIER            =       0x20,
        INDEXED                   =       0x40,
        OWN_SHAPE                 =       0x80,
        METHOD_THRASH_COUNT_MASK  =      0x300,
        METHOD_THRASH_COUNT_SHIFT =          8,
        METHOD_THRASH_COUNT_MAX   = METHOD_THRASH_COUNT_MASK >> METHOD_THRASH_COUNT_SHIFT,
        BOUND_FUNCTION            =      0x400,
        HAS_EQUALITY              =      0x800,
        VAROBJ                    =     0x1000,
        WATCHED                   =     0x2000,
        PACKED_ARRAY              =     0x4000,
        ITERATED                  =     0x8000,
        SINGLETON_TYPE            =    0x10000,
        LAZY_TYPE                 =    0x20000,

        
        FIXED_SLOTS_SHIFT         =         27,
        FIXED_SLOTS_MASK          =       0x1f << FIXED_SLOTS_SHIFT,

        UNUSED_FLAG_BITS          = 0x07FC0000
    };

    



    enum {
        NSLOTS_BITS     = 29,
        NSLOTS_LIMIT    = JS_BIT(NSLOTS_BITS)
    };

    uint32      flags;                      
    uint32      objShape;                   

    union {
        
        js::types::TypeObject *newType;

        
        jsuword initializedLength;
    };

    JS_FRIEND_API(size_t) sizeOfSlotsArray(JSUsableSizeFun usf);

    JSObject    *parent;                    
    void        *privateData;               
    jsuword     capacity;                   

  private:
    js::Value   *slots;                     



    




    js::types::TypeObject *type_;

    
    void makeLazyType(JSContext *cx);

  public:

    inline bool isNative() const;
    inline bool isNewborn() const;

    void setClass(js::Class *c) { clasp = c; }
    js::Class *getClass() const { return clasp; }
    JSClass *getJSClass() const { return Jsvalify(clasp); }

    bool hasClass(const js::Class *c) const {
        return c == clasp;
    }

    const js::ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    inline void trace(JSTracer *trc);
    inline void scanSlots(js::GCMarker *gcmarker);

    uint32 shape() const {
        JS_ASSERT(objShape != INVALID_SHAPE);
        return objShape;
    }

    bool isDelegate() const     { return !!(flags & DELEGATE); }
    void setDelegate()          { flags |= DELEGATE; }
    void clearDelegate()        { flags &= ~DELEGATE; }

    bool isBoundFunction() const { return !!(flags & BOUND_FUNCTION); }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const       { return !!(flags & SYSTEM); }
    void setSystem()            { flags |= SYSTEM; }

    




    bool branded()              { return !!(flags & BRANDED); }

    




    bool brand(JSContext *cx);
    bool unbrand(JSContext *cx);

    bool generic()              { return !!(flags & GENERIC); }
    void setGeneric()           { flags |= GENERIC; }

    uintN getMethodThrashCount() const {
        return (flags & METHOD_THRASH_COUNT_MASK) >> METHOD_THRASH_COUNT_SHIFT;
    }

    void setMethodThrashCount(uintN count) {
        JS_ASSERT(count <= METHOD_THRASH_COUNT_MAX);
        flags = (flags & ~METHOD_THRASH_COUNT_MASK) | (count << METHOD_THRASH_COUNT_SHIFT);
    }

    bool hasSpecialEquality() const { return !!(flags & HAS_EQUALITY); }
    void assertSpecialEqualitySynced() const {
        JS_ASSERT(!!clasp->ext.equality == hasSpecialEquality());
    }

    
    inline void syncSpecialEquality();

    bool watched() const { return !!(flags & WATCHED); }

    void setWatched(JSContext *cx) {
        if (!watched()) {
            flags |= WATCHED;
            generateOwnShape(cx);
        }
    }

   
   inline bool isVarObj() const { return flags & VAROBJ; }
   inline void makeVarObj() { flags |= VAROBJ; }
  private:
    void generateOwnShape(JSContext *cx);

    inline void setOwnShape(uint32 s);
    inline void clearOwnShape();

  public:
    inline bool nativeEmpty() const;

    bool hasOwnShape() const    { return !!(flags & OWN_SHAPE); }

    inline void setMap(js::Shape *amap);

    inline void setSharedNonNativeMap();

    
    void initCall(JSContext *cx, const js::Bindings &bindings, JSObject *parent);
    void initClonedBlock(JSContext *cx, js::types::TypeObject *type, js::StackFrame *priv);
    void setBlockOwnShape(JSContext *cx);

    void deletingShapeChange(JSContext *cx, const js::Shape &shape);
    const js::Shape *methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool methodShapeChange(JSContext *cx, uint32 slot);
    void protoShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, const js::Shape &shape);
    bool globalObjectOwnShapeChange(JSContext *cx);

    void extensibleShapeChange(JSContext *cx) {
        
        generateOwnShape(cx);
    }

    







































    bool hasMethodBarrier()     { return !!(flags & METHOD_BARRIER); }
    void setMethodBarrier()     { flags |= METHOD_BARRIER; }

    





    bool brandedOrHasMethodBarrier() { return !!(flags & (BRANDED | METHOD_BARRIER)); }

    




    const js::Shape *methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    






    const js::Shape *methodWriteBarrier(JSContext *cx, const js::Shape &shape, const js::Value &v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, const js::Value &v);

    bool isIndexed() const          { return !!(flags & INDEXED); }
    void setIndexed()               { flags |= INDEXED; }

    




    inline bool inDictionaryMode() const;

    inline uint32 propertyCount() const;

    inline bool hasPropertyTable() const;

    uint32 numSlots() const { return uint32(capacity); }

    inline size_t structSize() const;
    inline size_t slotsAndStructSize() const;

    

    static inline size_t getFixedSlotOffset(size_t slot);
    static inline size_t offsetOfCapacity() { return offsetof(JSObject, capacity); }
    static inline size_t offsetOfSlots() { return offsetof(JSObject, slots); }

    



    inline const js::Value *getRawSlots();
    inline const js::Value *getRawSlot(size_t slot, const js::Value *slots);

    
    inline bool isFixedSlot(size_t slot);

    
    inline size_t dynamicSlotIndex(size_t slot);

    inline size_t numFixedSlots() const;

    
    inline bool hasSlotsArray() const;

    
    inline size_t numDynamicSlots(size_t capacity) const;

  private:
    inline js::Value* fixedSlots() const;

  protected:
    inline bool hasContiguousSlots(size_t start, size_t count) const;

  public:
    
    static const uint32 SLOT_CAPACITY_MIN = 8;

    bool allocSlots(JSContext *cx, size_t nslots);
    bool growSlots(JSContext *cx, size_t nslots);
    void shrinkSlots(JSContext *cx, size_t nslots);

    bool ensureSlots(JSContext *cx, size_t nslots) {
        if (numSlots() < nslots)
            return growSlots(cx, nslots);
        return true;
    }

    



    void clearSlotRange(size_t start, size_t length);

    



    void copySlotRange(size_t start, const js::Value *vector, size_t length);

    















    bool ensureInstanceReservedSlots(JSContext *cx, size_t nreserved);

    




    bool ensureClassReservedSlotsForEmptyObject(JSContext *cx);

    inline bool ensureClassReservedSlots(JSContext *cx);

    inline uint32 slotSpan() const;

    inline bool containsSlot(uint32 slot) const;

    void rollbackProperties(JSContext *cx, uint32 slotSpan);

    js::Value *getSlotAddress(uintN slot) {
        




        JS_ASSERT(slot <= capacity);
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    js::Value &getSlotRef(uintN slot) {
        JS_ASSERT(slot < capacity);
        return *getSlotAddress(slot);
    }

    inline js::Value &nativeGetSlotRef(uintN slot);

    const js::Value &getSlot(uintN slot) const {
        JS_ASSERT(slot < capacity);
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    inline const js::Value &nativeGetSlot(uintN slot) const;

    void setSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(slot < capacity);
        getSlotRef(slot) = value;
    }

    inline void nativeSetSlot(uintN slot, const js::Value &value);
    inline void nativeSetSlotWithType(JSContext *cx, const js::Shape *shape, const js::Value &value);

    inline js::Value getReservedSlot(uintN index) const;

    
    inline void setReservedSlot(uintN index, const js::Value &v);

    

    js::Value &getFixedSlotRef(uintN slot) {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const js::Value &getFixedSlot(uintN slot) const {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    void setFixedSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot] = value;
    }

    
    inline void updateShape(JSContext *cx);
    inline void updateFlags(const js::Shape *shape, bool isDefinitelyAtom = false);

    
    inline void extend(JSContext *cx, const js::Shape *shape, bool isDefinitelyAtom = false);

    



    bool hasSingletonType() const { return flags & SINGLETON_TYPE; }

    



    bool hasLazyType() const { return flags & LAZY_TYPE; }

    



    inline bool setSingletonType(JSContext *cx);

    
    inline void revertLazyType();

    inline js::types::TypeObject *getType(JSContext *cx);

    js::types::TypeObject *type() const {
        JS_ASSERT(!hasLazyType());
        return type_;
    }

    js::types::TypeObject *typeFromGC() const {
        
        return type_;
    }

    static inline size_t offsetOfType() { return offsetof(JSObject, type_); }

    inline void clearType();
    inline void setType(js::types::TypeObject *newType);

    inline js::types::TypeObject *getNewType(JSContext *cx, JSFunction *fun = NULL,
                                             bool markUnknown = false);
  private:
    void makeNewType(JSContext *cx, JSFunction *fun, bool markUnknown);
  public:

    
    bool splicePrototype(JSContext *cx, JSObject *proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    JSObject * getProto() const {
        return type_->proto;
    }

    JSObject *getParent() const {
        return parent;
    }

    void clearParent() {
        parent = NULL;
    }

    void setParent(JSObject *newParent) {
#ifdef DEBUG
        for (JSObject *obj = newParent; obj; obj = obj->getParent())
            JS_ASSERT(obj != this);
#endif
        setDelegateNullSafe(newParent);
        parent = newParent;
    }

    JS_FRIEND_API(js::GlobalObject *) getGlobal() const;

    bool isGlobal() const {
        return !!(getClass()->flags & JSCLASS_IS_GLOBAL);
    }

    inline js::GlobalObject *asGlobal();

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        return privateData;
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        privateData = data;
    }

    
    inline JSPrincipals *principals(JSContext *cx);

    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    bool sealOrFreeze(JSContext *cx, ImmutabilityType it);

    bool isSealedOrFrozen(JSContext *cx, ImmutabilityType it, bool *resultp);

  public:
    bool isExtensible() const { return !(flags & NOT_EXTENSIBLE); }
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);

    
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx, SEAL); }
    
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, FREEZE); }

    bool isSealed(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, SEAL, resultp); }
    bool isFrozen(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, FREEZE, resultp); }
        
    



  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = 0;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

    static size_t getPrimitiveThisOffset() {
        
        return getFixedSlotOffset(JSSLOT_PRIMITIVE_THIS);
    }

  public:
    inline js::NumberObject *asNumber();
    inline js::StringObject *asString();

    



    inline uint32 getArrayLength() const;
    inline void setArrayLength(JSContext *cx, uint32 length);

    inline uint32 getDenseArrayCapacity();
    inline uint32 getDenseArrayInitializedLength();
    inline void setDenseArrayLength(uint32 length);
    inline void setDenseArrayInitializedLength(uint32 length);
    inline void ensureDenseArrayInitializedLength(JSContext *cx, uintN index, uintN extra);
    inline void backfillDenseArrayHoles(JSContext *cx);
    inline const js::Value* getDenseArrayElements();
    inline const js::Value &getDenseArrayElement(uintN idx);
    inline void setDenseArrayElement(uintN idx, const js::Value &val);
    inline void setDenseArrayElementWithType(JSContext *cx, uintN idx, const js::Value &val);
    inline void copyDenseArrayElements(uintN dstStart, const js::Value *src, uintN count);
    inline void moveDenseArrayElements(uintN dstStart, uintN srcStart, uintN count);
    inline void shrinkDenseArrayElements(JSContext *cx, uintN cap);
    inline bool denseArrayHasInlineSlots() const;

    
    inline bool isPackedDenseArray();
    inline void markDenseArrayNotPacked(JSContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra);

    



    bool willBeSparseDenseArray(uintN requiredCapacity, uintN newElementsHint);

    JSBool makeDenseArraySlow(JSContext *cx);

    




    bool arrayGetOwnDataElement(JSContext *cx, size_t i, js::Value *vp);

  public:
    bool allocateArrayBufferSlots(JSContext *cx, uint32 size);
    inline uint32 arrayBufferByteLength();
    inline uint8 * arrayBufferDataOffset();

  public:
    inline js::ArgumentsObject *asArguments();
    inline js::NormalArgumentsObject *asNormalArguments();
    inline js::StrictArgumentsObject *asStrictArguments();

  public:
    inline js::CallObject &asCall();

  public:
    



    static const uint32 JSSLOT_DATE_UTC_TIME = 0;

    




    static const uint32 JSSLOT_DATE_COMPONENTS_START = 1;

    static const uint32 JSSLOT_DATE_LOCAL_TIME = 1;
    static const uint32 JSSLOT_DATE_LOCAL_YEAR = 2;
    static const uint32 JSSLOT_DATE_LOCAL_MONTH = 3;
    static const uint32 JSSLOT_DATE_LOCAL_DATE = 4;
    static const uint32 JSSLOT_DATE_LOCAL_DAY = 5;
    static const uint32 JSSLOT_DATE_LOCAL_HOURS = 6;
    static const uint32 JSSLOT_DATE_LOCAL_MINUTES = 7;
    static const uint32 JSSLOT_DATE_LOCAL_SECONDS = 8;

    static const uint32 DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



  private:
    friend struct JSFunction;
    friend class js::mjit::Compiler;

    



    static const uint32 JSSLOT_FLAT_CLOSURE_UPVARS = 0;

    




    static const uint32 JSSLOT_FUN_METHOD_ATOM = 0;
    static const uint32 JSSLOT_FUN_METHOD_OBJ  = 1;

    static const uint32 JSSLOT_BOUND_FUNCTION_THIS       = 0;
    static const uint32 JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

  public:
    static const uint32 FUN_CLASS_RESERVED_SLOTS = 2;

    static size_t getFlatClosureUpvarsOffset() {
        return getFixedSlotOffset(JSSLOT_FLAT_CLOSURE_UPVARS);
    }

    inline JSFunction *getFunctionPrivate() const;

    inline js::Value *getFlatClosureUpvars() const;
    inline js::Value getFlatClosureUpvar(uint32 i) const;
    inline const js::Value &getFlatClosureUpvar(uint32 i);
    inline void setFlatClosureUpvar(uint32 i, const js::Value &v);
    inline void setFlatClosureUpvars(js::Value *upvars);

    
    inline void finalizeUpvarsIfFlatClosure();

    inline bool hasMethodObj(const JSObject& obj) const;
    inline void setMethodObj(JSObject& obj);

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, uintN argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value &getBoundFunctionArgument(uintN which) const;
    inline size_t getBoundFunctionArgumentCount() const;

    



  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = 0;
    static const uint32 JSSLOT_REGEXP_SOURCE = 1;
    static const uint32 JSSLOT_REGEXP_GLOBAL = 2;
    static const uint32 JSSLOT_REGEXP_IGNORE_CASE = 3;
    static const uint32 JSSLOT_REGEXP_MULTILINE = 4;
    static const uint32 JSSLOT_REGEXP_STICKY = 5;

    




    const js::Shape *assignInitialRegExpShape(JSContext *cx);

  public:
    static const uint32 REGEXP_CLASS_RESERVED_SLOTS = 6;

    inline const js::Value &getRegExpLastIndex() const;
    inline void setRegExpLastIndex(const js::Value &v);
    inline void setRegExpLastIndex(jsdouble d);
    inline void zeroRegExpLastIndex();

    inline void setRegExpSource(JSString *source);
    inline void setRegExpGlobal(bool global);
    inline void setRegExpIgnoreCase(bool ignoreCase);
    inline void setRegExpMultiline(bool multiline);
    inline void setRegExpSticky(bool sticky);

    inline bool initRegExp(JSContext *cx, js::RegExp *re);

    



    inline js::NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *);

    



    inline JSScript *getScript() const;

    



    






  private:
    static const uint32 JSSLOT_NAME_PREFIX          = 0;   
    static const uint32 JSSLOT_NAME_URI             = 1;   

    static const uint32 JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32 JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32 NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32 QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline JSLinearString *getNamePrefix() const;
    inline jsval getNamePrefixVal() const;
    inline void setNamePrefix(JSLinearString *prefix);
    inline void clearNamePrefix();

    inline JSLinearString *getNameURI() const;
    inline jsval getNameURIVal() const;
    inline void setNameURI(JSLinearString *uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline JSAtom *getQNameLocalName() const;
    inline jsval getQNameLocalNameVal() const;
    inline void setQNameLocalName(JSAtom *name);

    



    inline js::ProxyHandler *getProxyHandler() const;
    inline const js::Value &getProxyPrivate() const;
    inline void setProxyPrivate(const js::Value &priv);
    inline const js::Value &getProxyExtra() const;
    inline void setProxyExtra(const js::Value &extra);
    inline js::Wrapper *getWrapperHandler() const;

    


    inline JSObject *getWithThis() const;
    inline void setWithThis(JSObject *thisp);

    


    inline bool isCallable();

    
    void earlyInit(jsuword capacity) {
        this->capacity = capacity;

        
        lastProp = NULL;
    }

    
    void init(JSContext *cx, js::Class *aclasp, js::types::TypeObject *type,
              JSObject *parent, void *priv, bool denseArray);

    inline void finish(JSContext *cx);
    JS_ALWAYS_INLINE void finalize(JSContext *cx);

    



    inline bool initSharingEmptyShape(JSContext *cx,
                                      js::Class *clasp,
                                      js::types::TypeObject *type,
                                      JSObject *parent,
                                      void *priv,
                                      js::gc::AllocKind kind);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    








    bool allocSlot(JSContext *cx, uint32 *slotp);
    bool freeSlot(JSContext *cx, uint32 slot);

  public:
    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, jsid id, uintN argc, js::Value *argv, js::Value *vp);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::Shape &child);

    






    const js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                         JSPropertyOp getter, JSStrictPropertyOp setter,
                                         uint32 slot, uintN attrs,
                                         uintN flags, intN shortid,
                                         js::Shape **spp);

    bool toDictionaryMode(JSContext *cx);

    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

    void updateFixedSlots(uintN fixed);

  public:
    
    const js::Shape *addProperty(JSContext *cx, jsid id,
                                 JSPropertyOp getter, JSStrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *addDataProperty(JSContext *cx, jsid id, uint32 slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    const js::Shape *putProperty(JSContext *cx, jsid id,
                                 JSPropertyOp getter, JSStrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    const js::Shape *changeProperty(JSContext *cx, const js::Shape *shape, uintN attrs, uintN mask,
                                    JSPropertyOp getter, JSStrictPropertyOp setter);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

    JSBool lookupProperty(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp) {
        js::LookupPropOp op = getOps()->lookupProperty;
        return (op ? op : js_LookupProperty)(cx, this, id, objp, propp);
    }

    inline JSBool lookupElement(JSContext *cx, uint32 index, JSObject **objp, JSProperty **propp);

    JSBool defineProperty(JSContext *cx, jsid id, const js::Value &value,
                          JSPropertyOp getter = JS_PropertyStub,
                          JSStrictPropertyOp setter = JS_StrictPropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        js::DefinePropOp op = getOps()->defineProperty;
        return (op ? op : js_DefineProperty)(cx, this, id, &value, getter, setter, attrs);
    }

    JSBool defineElement(JSContext *cx, uint32 index, const js::Value &value,
                         JSPropertyOp getter = JS_PropertyStub,
                         JSStrictPropertyOp setter = JS_StrictPropertyStub,
                         uintN attrs = JSPROP_ENUMERATE)
    {
        js::DefineElementOp op = getOps()->defineElement;
        return (op ? op : js_DefineElement)(cx, this, index, &value, getter, setter, attrs);
    }

    inline JSBool getGeneric(JSContext *cx, JSObject *receiver, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, JSObject *receiver, js::PropertyName *name,
                              js::Value *vp);
    inline JSBool getElement(JSContext *cx, JSObject *receiver, uint32 index, js::Value *vp);
    inline JSBool getSpecial(JSContext *cx, JSObject *receiver, js::SpecialId sid, js::Value *vp);

    inline JSBool getGeneric(JSContext *cx, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, js::PropertyName *name, js::Value *vp);
    inline JSBool getElement(JSContext *cx, uint32 index, js::Value *vp);
    inline JSBool getSpecial(JSContext *cx, js::SpecialId sid, js::Value *vp);

    JSBool setProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict) {
        if (getOps()->setProperty)
            return nonNativeSetProperty(cx, id, vp, strict);
        return js_SetPropertyHelper(cx, this, id, 0, vp, strict);
    }

    JSBool setElement(JSContext *cx, uint32 index, js::Value *vp, JSBool strict) {
        if (getOps()->setElement)
            return nonNativeSetElement(cx, index, vp, strict);
        return js_SetElementHelper(cx, this, index, 0, vp, strict);
    }

    JSBool nonNativeSetProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict);

    JSBool nonNativeSetElement(JSContext *cx, uint32 index, js::Value *vp, JSBool strict);

    JSBool getAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        js::AttributesOp op = getOps()->getAttributes;
        return (op ? op : js_GetAttributes)(cx, this, id, attrsp);
    }

    JSBool getElementAttributes(JSContext *cx, uint32 index, uintN *attrsp) {
        js::ElementAttributesOp op = getOps()->getElementAttributes;
        return (op ? op : js_GetElementAttributes)(cx, this, index, attrsp);
    }

    inline JSBool setAttributes(JSContext *cx, jsid id, uintN *attrsp);

    JSBool setElementAttributes(JSContext *cx, uint32 index, uintN *attrsp) {
        js::ElementAttributesOp op = getOps()->setElementAttributes;
        return (op ? op : js_SetElementAttributes)(cx, this, index, attrsp);
    }

    inline JSBool deleteProperty(JSContext *cx, jsid id, js::Value *rval, JSBool strict);

    inline JSBool deleteElement(JSContext *cx, uint32 index, js::Value *rval, JSBool strict);

    JSBool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp) {
        JSNewEnumerateOp op = getOps()->enumerate;
        return (op ? op : js_Enumerate)(cx, this, iterop, statep, idp);
    }

    bool defaultValue(JSContext *cx, JSType hint, js::Value *vp) {
        JSConvertOp op = getClass()->convert;
        bool ok = (op == JS_ConvertStub ? js::DefaultValue : op)(cx, this, hint, vp);
        JS_ASSERT_IF(ok, vp->isPrimitive());
        return ok;
    }

    JSType typeOf(JSContext *cx) {
        js::TypeOfOp op = getOps()->typeOf;
        return (op ? op : js_TypeOf)(cx, this);
    }

    
    JSObject *thisObject(JSContext *cx) {
        JSObjectOp op = getOps()->thisObject;
        return op ? op(cx, this) : this;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    inline JSCompartment *getCompartment() const;

    inline JSObject *getThrowTypeError() const;

    JS_FRIEND_API(JSObject *) clone(JSContext *cx, JSObject *proto, JSObject *parent);
    JS_FRIEND_API(bool) copyPropertiesFrom(JSContext *cx, JSObject *obj);
    bool swap(JSContext *cx, JSObject *other);

    const js::Shape *defineBlockVariable(JSContext *cx, jsid id, intN index);

    inline bool canHaveMethodBarrier() const;

    inline bool isArguments() const { return isNormalArguments() || isStrictArguments(); }
    inline bool isArrayBuffer() const { return clasp == &js::ArrayBufferClass; }
    inline bool isNormalArguments() const { return clasp == &js::NormalArgumentsObjectClass; }
    inline bool isStrictArguments() const { return clasp == &js::StrictArgumentsObjectClass; }
    inline bool isArray() const { return isSlowArray() || isDenseArray(); }
    inline bool isDenseArray() const { return clasp == &js::ArrayClass; }
    inline bool isSlowArray() const { return clasp == &js::SlowArrayClass; }
    inline bool isNumber() const { return clasp == &js::NumberClass; }
    inline bool isBoolean() const { return clasp == &js::BooleanClass; }
    inline bool isString() const { return clasp == &js::StringClass; }
    inline bool isPrimitive() const { return isNumber() || isString() || isBoolean(); }
    inline bool isDate() const { return clasp == &js::DateClass; }
    inline bool isFunction() const { return clasp == &js::FunctionClass; }
    inline bool isObject() const { return clasp == &js::ObjectClass; }
    inline bool isWith() const { return clasp == &js::WithClass; }
    inline bool isBlock() const { return clasp == &js::BlockClass; }
    inline bool isStaticBlock() const { return isBlock() && !getProto(); }
    inline bool isClonedBlock() const { return isBlock() && !!getProto(); }
    inline bool isCall() const { return clasp == &js::CallClass; }
    inline bool isDeclEnv() const { return clasp == &js::DeclEnvClass; }
    inline bool isRegExp() const { return clasp == &js::RegExpClass; }
    inline bool isScript() const { return clasp == &js::ScriptClass; }
    inline bool isGenerator() const { return clasp == &js::GeneratorClass; }
    inline bool isIterator() const { return clasp == &js::IteratorClass; }
    inline bool isStopIteration() const { return clasp == &js::StopIterationClass; }
    inline bool isError() const { return clasp == &js::ErrorClass; }
    inline bool isXML() const { return clasp == &js::XMLClass; }
    inline bool isNamespace() const { return clasp == &js::NamespaceClass; }
    inline bool isWeakMap() const { return clasp == &js::WeakMapClass; }
    inline bool isFunctionProxy() const { return clasp == &js::FunctionProxyClass; }
    inline bool isProxy() const { return isObjectProxy() || isFunctionProxy(); }

    inline bool isXMLId() const {
        return clasp == &js::QNameClass || clasp == &js::AttributeNameClass || clasp == &js::AnyNameClass;
    }
    inline bool isQName() const {
        return clasp == &js::QNameClass || clasp == &js::AttributeNameClass || clasp == &js::AnyNameClass;
    }
    inline bool isObjectProxy() const {
        return clasp == &js::ObjectProxyClass || clasp == &js::OuterWindowProxyClass;
    }

    JS_FRIEND_API(bool) isWrapper() const;
    bool isCrossCompartmentWrapper() const;
    JS_FRIEND_API(JSObject *) unwrap(uintN *flagsp = NULL);

    inline void initArrayClass();

    

    static size_t offsetOfClassPointer() { return offsetof(JSObject, clasp); }
};


JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0);






static JS_ALWAYS_INLINE bool
operator==(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs == &rhs;
}

static JS_ALWAYS_INLINE bool
operator!=(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs != &rhs;
}

inline js::Value*
JSObject::fixedSlots() const {
    return (js::Value*) (jsuword(this) + sizeof(JSObject));
}

inline size_t
JSObject::numFixedSlots() const
{
    return flags >> FIXED_SLOTS_SHIFT;
}

 inline size_t
JSObject::getFixedSlotOffset(size_t slot) {
    return sizeof(JSObject) + (slot * sizeof(js::Value));
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

#ifdef JS_THREADSAFE






#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif 

inline void
OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        obj = op(cx, obj);
}





extern JS_FRIEND_API(bool)
NULLABLE_OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj);

inline void
OBJ_TO_OUTER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.outerObject)
        obj = op(cx, obj);
}

class JSValueArray {
  public:
    jsval *array;
    size_t length;

    JSValueArray(jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};
















static const uint32 JSSLOT_BLOCK_DEPTH = 0;
static const uint32 JSSLOT_BLOCK_FIRST_FREE_SLOT = JSSLOT_BLOCK_DEPTH + 1;

static const uint32 JSSLOT_WITH_THIS = 1;

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    (obj)->propertyCount()
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    (obj)->getFixedSlot(JSSLOT_BLOCK_DEPTH).toInt32()
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    (obj)->setFixedSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Value(depth)))









extern JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);

inline JSObject *
js_UnwrapWithObject(JSContext *cx, JSObject *withobj);







extern JSObject *
js_NewBlockObject(JSContext *cx);

extern JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, js::StackFrame *fp);

extern JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind);

JSBool
js_XDRBlockObject(JSXDRState *xdr, JSObject **objp);

struct JSSharpObjectMap {
    jsrefcount  depth;
    uint32      sharpgen;
    JSHashTable *table;
};

#define SHARP_BIT       ((jsatomid) 1)
#define BUSY_BIT        ((jsatomid) 2)
#define SHARP_ID_SHIFT  2
#define IS_SHARP(he)    (uintptr_t((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = (void *) (uintptr_t((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (uintptr_t((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = (void *) (uintptr_t((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = (void *) (uintptr_t((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);





extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, js::LookupPropOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

#if JS_HAS_OBJ_PROTO_PROP
extern JSPropertySpec object_props[];
#else
#define object_props NULL
#endif

extern JSFunctionSpec object_methods[];
extern JSFunctionSpec object_static_methods[];

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) js_obj_defineGetter(JSContext *cx, uintN argc, js::Value *vp);
JS_FRIEND_API(JSBool) js_obj_defineSetter(JSContext *cx, uintN argc, js::Value *vp);
#endif

namespace js {

JSObject *
DefineConstructorAndPrototype(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, uintN nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
                              JSObject **ctorp = NULL);

bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp);

void
MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp);

}

extern JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             js::Class *clasp, JSNative constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
             JSObject **ctorp = NULL);




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

extern JSBool
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props);




extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);





extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key,
                   js::Value *vp, js::Class *clasp = NULL);

extern JSObject *
js_ConstructObject(JSContext *cx, js::Class *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, js::Value *argv);



extern JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto);


extern JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee, bool newType);


extern JSObject *
js_CreateThis(JSContext *cx, JSObject *callee);

extern jsid
js_CheckForStringIndex(jsid id);







extern void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id);

inline void
js_PurgeScopeChain(JSContext *cx, JSObject *obj, jsid id)
{
    if (obj->isDelegate())
        js_PurgeScopeChainHelper(cx, obj, id);
}





extern const js::Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern const js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const js::Shape *shape, uintN attrs, uintN mask,
                             JSPropertyOp getter, JSStrictPropertyOp setter);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);

namespace js {




const uintN DNP_CACHE_RESULT = 1;   
const uintN DNP_DONT_PURGE   = 2;   
const uintN DNP_SET_METHOD   = 4;   


const uintN DNP_UNQUALIFIED  = 8;   


const uintN DNP_SKIP_TYPE = 0x10;   




extern const Shape *
DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow = 0);




extern bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp, JSProperty **propp);










extern bool
DefineProperty(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc, bool throwError,
               bool *rval);





extern bool
ReadPropertyDescriptors(JSContext *cx, JSObject *props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs);





static const uintN RESOLVE_INFER = 0xffff;






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    JS_ASSERT(obj->getParent());

    bool cacheable = (obj->isCall() || obj->isBlock() || obj->isDeclEnv());

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

}




extern js::PropertyCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, bool cacheResult, bool global,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp);





extern JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, bool global,
                JSObject **objp, JSObject **pobjp, JSProperty **propp);

extern JS_REQUIRES_STACK JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id);

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);














const uintN JSGET_CACHE_RESULT      = 1; 
const uintN JSGET_METHOD_BARRIER    = 0; 
const uintN JSGET_NO_METHOD_BARRIER = 2; 







extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const js::Shape *shape, uintN getHow,
             js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const js::Shape *shape, bool added,
             bool strict, js::Value *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32 getHow, js::Value *vp);

namespace js {

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, PropertyDescriptor *desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp);

} 

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);







extern JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);





extern JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, js::Shape *shape,
                       uintN attrs);

namespace js {





extern bool
HasDataProperty(JSContext *cx, JSObject *obj, jsid methodid, js::Value *vp);

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            js::Value *vp, uintN *attrsp);

} 

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);





extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);





extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);





extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JSObject **objp);

namespace js {





extern JSObject *
ToObjectSlow(JSContext *cx, Value *vp);

JS_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, Value *vp)
{
    if (vp->isObject())
        return &vp->toObject();
    return ToObjectSlow(cx, vp);
}


static JS_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, Value *vp)
{
    if (vp->isPrimitive())
        return true;
    return vp->toObject().defaultValue(cx, JSTYPE_VOID, vp);
}


static JS_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, JSType preferredType, Value *vp)
{
    JS_ASSERT(preferredType != JSTYPE_VOID); 
    if (vp->isPrimitive())
        return true;
    return vp->toObject().defaultValue(cx, preferredType, vp);
}

} 





extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern bool
js_ClearNative(JSContext *cx, JSObject *obj);

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, js::Value *vp);

extern bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, const js::Value &v);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpValue(const js::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, js::StackFrame *start = NULL);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);

namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

extern JSBool
eval(JSContext *cx, uintN argc, Value *vp);






extern JS_REQUIRES_STACK bool
DirectEval(JSContext *cx, const CallArgs &call);





extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const js::Value &v);


extern bool
IsAnyBuiltinEval(JSFunction *fun);


extern JSPrincipals *
PrincipalsForCompiledCode(const CallArgs &call, JSContext *cx);

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);
}

#endif 

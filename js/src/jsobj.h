







































#ifndef jsobj_h___
#define jsobj_h___









#include "jsapi.h"
#include "jsatom.h"
#include "jsclass.h"
#include "jsfriendapi.h"
#include "jsinfer.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jscell.h"

#include "gc/Barrier.h"
#include "vm/String.h"

namespace js {

class AutoPropDescArrayRooter;
class ProxyHandler;
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

    
    uint8_t attrs;

    
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






extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);

namespace js {

inline bool
LookupProperty(JSContext *cx, JSObject *obj, PropertyName *name,
               JSObject **objp, JSProperty **propp)
{
    return js_LookupProperty(cx, obj, ATOM_TO_JSID(name), objp, propp);
}

}

extern JS_FRIEND_API(JSBool)
js_LookupElement(JSContext *cx, JSObject *obj, uint32_t index,
                 JSObject **objp, JSProperty **propp);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_DefineElement(JSContext *cx, JSObject *obj, uint32_t index, const js::Value *value,
                 JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, js::Value *vp);

extern JSBool
js_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t, js::Value *vp);

inline JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp)
{
    return js_GetProperty(cx, obj, obj, id, vp);
}

inline JSBool
js_GetElement(JSContext *cx, JSObject *obj, uint32_t index, js::Value *vp)
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

namespace js {

inline bool
SetPropertyHelper(JSContext *cx, JSObject *obj, PropertyName *name, uintN defineHow,
                  Value *vp, JSBool strict)
{
    return !!js_SetPropertyHelper(cx, obj, ATOM_TO_JSID(name), defineHow, vp, strict);
}

} 

extern JSBool
js_SetElementHelper(JSContext *cx, JSObject *obj, uint32_t index, uintN defineHow,
                    js::Value *vp, JSBool strict);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_GetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, js::PropertyName *name, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteElement(JSContext *cx, JSObject *obj, uint32_t index, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteSpecial(JSContext *cx, JSObject *obj, js::SpecialId sid, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteGeneric(JSContext *cx, JSObject *obj, jsid id, js::Value *rval, JSBool strict);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

namespace js {


extern JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);

extern Class ArrayClass;
extern Class ArrayBufferClass;
extern Class BlockClass;
extern Class BooleanClass;
extern Class CallableObjectClass;
extern Class DateClass;
extern Class ErrorClass;
extern Class ElementIteratorClass;
extern Class GeneratorClass;
extern Class IteratorClass;
extern Class JSONClass;
extern Class MathClass;
extern Class NumberClass;
extern Class NormalArgumentsObjectClass;
extern Class ObjectClass;
extern Class ProxyClass;
extern Class RegExpClass;
extern Class RegExpStaticsClass;
extern Class SlowArrayClass;
extern Class StopIterationClass;
extern Class StringClass;
extern Class StrictArgumentsObjectClass;
extern Class WeakMapClass;
extern Class WithClass;
extern Class XMLFilterClass;

class ArgumentsObject;
class BlockObject;
class BooleanObject;
class ClonedBlockObject;
class DeclEnvObject;
class ElementIteratorObject;
class GlobalObject;
class NestedScopeObject;
class NewObjectCache;
class NormalArgumentsObject;
class NumberObject;
class ScopeObject;
class StaticBlockObject;
class StrictArgumentsObject;
class StringObject;
class RegExpObject;
class WithObject;







class ObjectElements
{
    friend struct ::JSObject;

    
    uint32_t capacity;

    





    uint32_t initializedLength;

    
    uint32_t length;

    
    uint32_t unused;

    void staticAsserts() {
        JS_STATIC_ASSERT(sizeof(ObjectElements) == VALUES_PER_HEADER * sizeof(Value));
    }

  public:

    ObjectElements(uint32_t capacity, uint32_t length)
        : capacity(capacity), initializedLength(0), length(length)
    {}

    HeapValue * elements() { return (HeapValue *)(uintptr_t(this) + sizeof(ObjectElements)); }
    static ObjectElements * fromElements(HeapValue *elems) {
        return (ObjectElements *)(uintptr_t(elems) - sizeof(ObjectElements));
    }

    static int offsetOfCapacity() {
        return (int)offsetof(ObjectElements, capacity) - (int)sizeof(ObjectElements);
    }
    static int offsetOfInitializedLength() {
        return (int)offsetof(ObjectElements, initializedLength) - (int)sizeof(ObjectElements);
    }
    static int offsetOfLength() {
        return (int)offsetof(ObjectElements, length) - (int)sizeof(ObjectElements);
    }

    static const size_t VALUES_PER_HEADER = 2;
};


extern HeapValue *emptyObjectElements;

}  












































struct JSObject : js::gc::Cell
{
  private:
    friend struct js::Shape;
    friend struct js::GCMarker;
    friend class  js::NewObjectCache;

    



    js::HeapPtrShape shape_;

#ifdef DEBUG
    void checkShapeConsistency();
#endif

    




    js::HeapPtrTypeObject type_;

    
    void makeLazyType(JSContext *cx);

  public:
    inline js::Shape *lastProperty() const {
        JS_ASSERT(shape_);
        return shape_;
    }

    



    bool setLastProperty(JSContext *cx, const js::Shape *shape);

    
    inline void setLastPropertyInfallible(const js::Shape *shape);

    
    static inline JSObject *create(JSContext *cx,
                                   js::gc::AllocKind kind,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type,
                                   js::HeapValue *slots);

    
    static inline JSObject *createDenseArray(JSContext *cx,
                                             js::gc::AllocKind kind,
                                             js::HandleShape shape,
                                             js::HandleTypeObject type,
                                             uint32_t length);

    




    inline void removeLastProperty(JSContext *cx);
    inline bool canRemoveLastProperty();

    



    bool setSlotSpan(JSContext *cx, uint32_t span);

    static inline size_t offsetOfShape() { return offsetof(JSObject, shape_); }
    inline js::HeapPtrShape *addressOfShape() { return &shape_; }

    const js::Shape *nativeLookup(JSContext *cx, jsid id);

    inline bool nativeContains(JSContext *cx, jsid id);
    inline bool nativeContains(JSContext *cx, const js::Shape &shape);

    
    static const uint32_t NELEMENTS_LIMIT = JS_BIT(28);

  private:
    js::HeapValue   *slots;     
    js::HeapValue   *elements;  

  public:

    inline bool isNative() const;

    inline js::Class *getClass() const;
    inline JSClass *getJSClass() const;
    inline bool hasClass(const js::Class *c) const;
    inline const js::ObjectOps *getOps() const;

    








    inline bool isDelegate() const;
    inline bool setDelegate(JSContext *cx);

    inline bool isBoundFunction() const;

    




    inline bool isSystem() const;
    inline bool setSystem(JSContext *cx);

    inline bool hasSpecialEquality() const;

    inline bool watched() const;
    inline bool setWatched(JSContext *cx);

    
    inline bool isVarObj() const;
    inline bool setVarObj(JSContext *cx);

    





    inline bool hasUncacheableProto() const;
    inline bool setUncacheableProto(JSContext *cx);

    bool generateOwnShape(JSContext *cx, js::Shape *newShape = NULL) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

  private:
    js::Shape *replaceWithNewEquivalentShape(JSContext *cx, js::Shape *existingShape,
                                             js::Shape *newShape = NULL);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(JSContext *cx,  uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);

  public:
    inline bool nativeEmpty() const;

    js::Shape *methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool shadowingShapeChange(JSContext *cx, const js::Shape &shape);

    




    js::Shape *methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    
    inline bool canHaveMethodBarrier() const;

    
    inline bool isIndexed() const;

    




    inline bool inDictionaryMode() const;

    inline uint32_t propertyCount() const;

    inline bool hasPropertyTable() const;

    inline size_t sizeOfThis() const;
    inline size_t computedSizeOfThisSlotsElements() const;

    inline void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf,
                                    size_t *slotsSize, size_t *elementsSize,
                                    size_t *miscSize) const;

    inline size_t numFixedSlots() const;

    static const uint32_t MAX_FIXED_SLOTS = 16;

  private:
    inline js::HeapValue* fixedSlots() const;

    



    void getSlotRange(size_t start, size_t length,
                      js::HeapValue **fixedStart, js::HeapValue **fixedEnd,
                      js::HeapValue **slotsStart, js::HeapValue **slotsEnd);
  public:

    

    
    inline bool isFixedSlot(size_t slot);

    
    inline size_t dynamicSlotIndex(size_t slot);

    
    inline const js::HeapValue *getRawSlots();

    
    static inline size_t getFixedSlotOffset(size_t slot);
    static inline size_t getPrivateDataOffset(size_t nfixed);
    static inline size_t offsetOfSlots() { return offsetof(JSObject, slots); }

    
    static const uint32_t SLOT_CAPACITY_MIN = 8;

    




    bool growSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);
    void shrinkSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);

    bool hasDynamicSlots() const { return slots != NULL; }

    





    static inline size_t dynamicSlotsCount(size_t nfixed, size_t span);

    
    inline size_t numDynamicSlots() const;

  protected:
    inline bool hasContiguousSlots(size_t start, size_t count) const;

    inline void initializeSlotRange(size_t start, size_t count);
    inline void invalidateSlotRange(size_t start, size_t count);

    inline bool updateSlotsForSpan(JSContext *cx, size_t oldSpan, size_t newSpan);

  public:
    



    inline void prepareSlotRangeForOverwrite(size_t start, size_t end);
    inline void prepareElementRangeForOverwrite(size_t start, size_t end);

    



    void initSlotRange(size_t start, const js::Value *vector, size_t length);

    



    void copySlotRange(size_t start, const js::Value *vector, size_t length);

    inline uint32_t slotSpan() const;

    void rollbackProperties(JSContext *cx, uint32_t slotSpan);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    



    bool slotInRange(uintN slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
#endif

    js::HeapValue *getSlotAddressUnchecked(uintN slot) {
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    js::HeapValue *getSlotAddress(uintN slot) {
        




        JS_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    js::HeapValue &getSlotRef(uintN slot) {
        JS_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    inline js::HeapValue &nativeGetSlotRef(uintN slot);

    const js::Value &getSlot(uintN slot) const {
        JS_ASSERT(slotInRange(slot));
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    inline const js::Value &nativeGetSlot(uintN slot) const;
    inline JSFunction *nativeGetMethod(const js::Shape *shape) const;

    inline void setSlot(uintN slot, const js::Value &value);
    inline void initSlot(uintN slot, const js::Value &value);
    inline void initSlotUnchecked(uintN slot, const js::Value &value);

    inline void nativeSetSlot(uintN slot, const js::Value &value);
    inline void nativeSetSlotWithType(JSContext *cx, const js::Shape *shape, const js::Value &value);

    inline const js::Value &getReservedSlot(uintN index) const;
    inline js::HeapValue &getReservedSlotRef(uintN index);
    inline void setReservedSlot(uintN index, const js::Value &v);

    

    js::HeapValue &getFixedSlotRef(uintN slot) {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const js::Value &getFixedSlot(uintN slot) const {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    inline void setFixedSlot(uintN slot, const js::Value &value);
    inline void initFixedSlot(uintN slot, const js::Value &value);

    



    bool hasSingletonType() const { return !!type_->singleton; }

    



    bool hasLazyType() const { return type_->lazy(); }

    



    inline bool setSingletonType(JSContext *cx);

    inline js::types::TypeObject *getType(JSContext *cx);

    js::types::TypeObject *type() const {
        JS_ASSERT(!hasLazyType());
        return type_;
    }

    const js::HeapPtr<js::types::TypeObject> &typeFromGC() const {
        
        return type_;
    }

    static inline size_t offsetOfType() { return offsetof(JSObject, type_); }
    inline js::HeapPtrTypeObject *addressOfType() { return &type_; }

    inline void setType(js::types::TypeObject *newType);

    js::types::TypeObject *getNewType(JSContext *cx, JSFunction *fun = NULL);

#ifdef DEBUG
    bool hasNewType(js::types::TypeObject *newType);
#endif

    




    inline bool setIteratedSingleton(JSContext *cx);

    



    bool setNewTypeUnknown(JSContext *cx);

    
    bool splicePrototype(JSContext *cx, JSObject *proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    JSObject * getProto() const {
        return type_->proto;
    }

    

























    
    inline JSObject *getParent() const;
    bool setParent(JSContext *cx, JSObject *newParent);

    




    inline JSObject *enclosingScope();

    inline js::GlobalObject &global() const;

    

    inline bool hasPrivate() const;
    inline void *getPrivate() const;
    inline void setPrivate(void *data);

    
    inline void *getPrivate(size_t nfixed) const;

    
    inline JSPrincipals *principals(JSContext *cx);

    
    inline bool clearType(JSContext *cx);
    bool clearParent(JSContext *cx);

    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    bool sealOrFreeze(JSContext *cx, ImmutabilityType it);

    bool isSealedOrFrozen(JSContext *cx, ImmutabilityType it, bool *resultp);

    static inline uintN getSealedOrFrozenAttributes(uintN attrs, ImmutabilityType it);

    inline void *&privateRef(uint32_t nfixed) const;

  public:
    inline bool isExtensible() const;
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);

    
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx, SEAL); }
    
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, FREEZE); }

    bool isSealed(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, SEAL, resultp); }
    bool isFrozen(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, FREEZE, resultp); }

    

    js::ObjectElements *getElementsHeader() const {
        return js::ObjectElements::fromElements(elements);
    }

    inline bool ensureElements(JSContext *cx, uintN cap);
    bool growElements(JSContext *cx, uintN cap);
    void shrinkElements(JSContext *cx, uintN cap);

    inline js::HeapValue* fixedElements() const {
        JS_STATIC_ASSERT(2 * sizeof(js::Value) == sizeof(js::ObjectElements));
        return &fixedSlots()[2];
    }

    void setFixedElements() { this->elements = fixedElements(); }

    inline bool hasDynamicElements() const {
        






        return elements != js::emptyObjectElements && elements != fixedElements();
    }

    
    static inline size_t offsetOfElements() { return offsetof(JSObject, elements); }
    static inline size_t offsetOfFixedElements() {
        return sizeof(JSObject) + sizeof(js::ObjectElements);
    }

    inline js::ElementIteratorObject *asElementIterator();

    



    bool allocateSlowArrayElements(JSContext *cx);

    inline uint32_t getArrayLength() const;
    inline void setArrayLength(JSContext *cx, uint32_t length);

    inline uint32_t getDenseArrayCapacity();
    inline uint32_t getDenseArrayInitializedLength();
    inline void setDenseArrayLength(uint32_t length);
    inline void setDenseArrayInitializedLength(uint32_t length);
    inline void ensureDenseArrayInitializedLength(JSContext *cx, uintN index, uintN extra);
    inline js::HeapValueArray getDenseArrayElements();
    inline const js::Value &getDenseArrayElement(uintN idx);
    inline void setDenseArrayElement(uintN idx, const js::Value &val);
    inline void initDenseArrayElement(uintN idx, const js::Value &val);
    inline void setDenseArrayElementWithType(JSContext *cx, uintN idx, const js::Value &val);
    inline void initDenseArrayElementWithType(JSContext *cx, uintN idx, const js::Value &val);
    inline void copyDenseArrayElements(uintN dstStart, const js::Value *src, uintN count);
    inline void initDenseArrayElements(uintN dstStart, const js::Value *src, uintN count);
    inline void moveDenseArrayElements(uintN dstStart, uintN srcStart, uintN count);
    inline void moveDenseArrayElementsUnbarriered(uintN dstStart, uintN srcStart, uintN count);
    inline bool denseArrayHasInlineSlots() const;

    
    inline void markDenseArrayNotPacked(JSContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra);

    



    bool willBeSparseDenseArray(uintN requiredCapacity, uintN newElementsHint);

    JSBool makeDenseArraySlow(JSContext *cx);

    




    bool arrayGetOwnDataElement(JSContext *cx, size_t i, js::Value *vp);

  public:
    bool allocateArrayBufferSlots(JSContext *cx, uint32_t size, uint8_t *contents = NULL);
    inline uint32_t arrayBufferByteLength();
    inline uint8_t * arrayBufferDataOffset();

  public:
    



    static const uint32_t JSSLOT_DATE_UTC_TIME = 0;

    




    static const uint32_t JSSLOT_DATE_COMPONENTS_START = 1;

    static const uint32_t JSSLOT_DATE_LOCAL_TIME = 1;
    static const uint32_t JSSLOT_DATE_LOCAL_YEAR = 2;
    static const uint32_t JSSLOT_DATE_LOCAL_MONTH = 3;
    static const uint32_t JSSLOT_DATE_LOCAL_DATE = 4;
    static const uint32_t JSSLOT_DATE_LOCAL_DAY = 5;
    static const uint32_t JSSLOT_DATE_LOCAL_HOURS = 6;
    static const uint32_t JSSLOT_DATE_LOCAL_MINUTES = 7;
    static const uint32_t JSSLOT_DATE_LOCAL_SECONDS = 8;

    static const uint32_t DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



    friend struct JSFunction;

    inline JSFunction *toFunction();
    inline const JSFunction *toFunction() const;

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    inline js::NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *);

    



    






  private:
    static const uint32_t JSSLOT_NAME_PREFIX          = 0;   
    static const uint32_t JSSLOT_NAME_URI             = 1;   

    static const uint32_t JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32_t JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32_t NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32_t QNAME_CLASS_RESERVED_SLOTS     = 3;

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

    


    inline js::Wrapper *getWrapperHandler() const;

    


    inline bool isCallable();

    inline void finish(JSContext *cx);
    JS_ALWAYS_INLINE void finalize(JSContext *cx, bool background);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    






    bool allocSlot(JSContext *cx, uint32_t *slotp);
    void freeSlot(JSContext *cx, uint32_t slot);

  public:
    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, jsid id, uintN argc, js::Value *argv, js::Value *vp);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::StackShape &child);

  protected:
    






    js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                   JSPropertyOp getter, JSStrictPropertyOp setter,
                                   uint32_t slot, uintN attrs,
                                   uintN flags, intN shortid, js::Shape **spp,
                                   bool allowDictionary);

  private:
    bool toDictionaryMode(JSContext *cx);

    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

  public:
    
    js::Shape *addProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, uintN attrs,
                           uintN flags, intN shortid, bool allowDictionary = true);

    
    js::Shape *addDataProperty(JSContext *cx, jsid id, uint32_t slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    js::Shape *putProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, uintN attrs,
                           uintN flags, intN shortid);
    inline js::Shape *
    putProperty(JSContext *cx, js::PropertyName *name,
                JSPropertyOp getter, JSStrictPropertyOp setter,
                uint32_t slot, uintN attrs, uintN flags, intN shortid) {
        return putProperty(cx, js_CheckForStringIndex(ATOM_TO_JSID(name)), getter, setter, slot, attrs, flags, shortid);
    }

    
    js::Shape *changeProperty(JSContext *cx, js::Shape *shape, uintN attrs, uintN mask,
                              JSPropertyOp getter, JSStrictPropertyOp setter);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

    inline JSBool lookupGeneric(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp);
    inline JSBool lookupProperty(JSContext *cx, js::PropertyName *name, JSObject **objp, JSProperty **propp);
    inline JSBool lookupElement(JSContext *cx, uint32_t index,
                                JSObject **objp, JSProperty **propp);
    inline JSBool lookupSpecial(JSContext *cx, js::SpecialId sid,
                                JSObject **objp, JSProperty **propp);

    inline JSBool defineGeneric(JSContext *cx, jsid id, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);
    inline JSBool defineProperty(JSContext *cx, js::PropertyName *name, const js::Value &value,
                                 JSPropertyOp getter = JS_PropertyStub,
                                 JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                 uintN attrs = JSPROP_ENUMERATE);

    inline JSBool defineElement(JSContext *cx, uint32_t index, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);
    inline JSBool defineSpecial(JSContext *cx, js::SpecialId sid, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);

    inline JSBool getGeneric(JSContext *cx, JSObject *receiver, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, JSObject *receiver, js::PropertyName *name,
                              js::Value *vp);
    inline JSBool getElement(JSContext *cx, JSObject *receiver, uint32_t index, js::Value *vp);
    

    inline JSBool getElementIfPresent(JSContext *cx, JSObject *receiver, uint32_t index,
                                      js::Value *vp, bool *present);
    inline JSBool getSpecial(JSContext *cx, JSObject *receiver, js::SpecialId sid, js::Value *vp);

    inline JSBool getGeneric(JSContext *cx, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, js::PropertyName *name, js::Value *vp);
    inline JSBool getElement(JSContext *cx, uint32_t index, js::Value *vp);
    inline JSBool getSpecial(JSContext *cx, js::SpecialId sid, js::Value *vp);

    inline JSBool setGeneric(JSContext *cx, jsid id, js::Value *vp, JSBool strict);
    inline JSBool setProperty(JSContext *cx, js::PropertyName *name, js::Value *vp, JSBool strict);
    inline JSBool setElement(JSContext *cx, uint32_t index, js::Value *vp, JSBool strict);
    inline JSBool setSpecial(JSContext *cx, js::SpecialId sid, js::Value *vp, JSBool strict);

    JSBool nonNativeSetProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict);
    JSBool nonNativeSetElement(JSContext *cx, uint32_t index, js::Value *vp, JSBool strict);

    inline JSBool getGenericAttributes(JSContext *cx, jsid id, uintN *attrsp);
    inline JSBool getPropertyAttributes(JSContext *cx, js::PropertyName *name, uintN *attrsp);
    inline JSBool getElementAttributes(JSContext *cx, uint32_t index, uintN *attrsp);
    inline JSBool getSpecialAttributes(JSContext *cx, js::SpecialId sid, uintN *attrsp);

    inline JSBool setGenericAttributes(JSContext *cx, jsid id, uintN *attrsp);
    inline JSBool setPropertyAttributes(JSContext *cx, js::PropertyName *name, uintN *attrsp);
    inline JSBool setElementAttributes(JSContext *cx, uint32_t index, uintN *attrsp);
    inline JSBool setSpecialAttributes(JSContext *cx, js::SpecialId sid, uintN *attrsp);

    inline bool deleteProperty(JSContext *cx, js::PropertyName *name, js::Value *rval, bool strict);
    inline bool deleteElement(JSContext *cx, uint32_t index, js::Value *rval, bool strict);
    inline bool deleteSpecial(JSContext *cx, js::SpecialId sid, js::Value *rval, bool strict);
    bool deleteByValue(JSContext *cx, const js::Value &property, js::Value *rval, bool strict);

    inline bool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp);
    inline bool defaultValue(JSContext *cx, JSType hint, js::Value *vp);
    inline JSType typeOf(JSContext *cx);
    inline JSObject *thisObject(JSContext *cx);

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    bool swap(JSContext *cx, JSObject *other);

    inline void initArrayClass();

    static inline void writeBarrierPre(JSObject *obj);
    static inline void writeBarrierPost(JSObject *obj, void *addr);
    inline void privateWriteBarrierPre(void **oldval);
    inline void privateWriteBarrierPost(void **oldval);

    



























    
    inline bool isArguments() const;
    inline bool isArrayBuffer() const;
    inline bool isArray() const;
    inline bool isDate() const;
    inline bool isDenseArray() const;
    inline bool isElementIterator() const;
    inline bool isError() const;
    inline bool isFunction() const;
    inline bool isGenerator() const;
    inline bool isGlobal() const;
    inline bool isIterator() const;
    inline bool isNamespace() const;
    inline bool isObject() const;
    inline bool isQName() const;
    inline bool isPrimitive() const;
    inline bool isProxy() const;
    inline bool isRegExp() const;
    inline bool isRegExpStatics() const;
    inline bool isScope() const;
    inline bool isScript() const;
    inline bool isSlowArray() const;
    inline bool isStopIteration() const;
    inline bool isWeakMap() const;
    inline bool isXML() const;
    inline bool isXMLId() const;

    
    inline bool isBlock() const;
    inline bool isCall() const;
    inline bool isDeclEnv() const;
    inline bool isNestedScope() const;
    inline bool isWith() const;
    inline bool isClonedBlock() const;
    inline bool isStaticBlock() const;

    
    inline bool isBoolean() const;
    inline bool isNumber() const;
    inline bool isString() const;

    
    inline bool isNormalArguments() const;
    inline bool isStrictArguments() const;

    
    inline bool isWrapper() const;
    inline bool isFunctionProxy() const;
    inline bool isCrossCompartmentWrapper() const;

    inline js::ArgumentsObject &asArguments();
    inline const js::ArgumentsObject &asArguments() const;
    inline js::BlockObject &asBlock();
    inline js::BooleanObject &asBoolean();
    inline js::CallObject &asCall();
    inline js::ClonedBlockObject &asClonedBlock();
    inline js::DeclEnvObject &asDeclEnv();
    inline js::GlobalObject &asGlobal();
    inline js::NestedScopeObject &asNestedScope();
    inline js::NormalArgumentsObject &asNormalArguments();
    inline js::NumberObject &asNumber();
    inline js::RegExpObject &asRegExp();
    inline js::ScopeObject &asScope();
    inline js::StrictArgumentsObject &asStrictArguments();
    inline js::StaticBlockObject &asStaticBlock();
    inline js::StringObject &asString();
    inline js::WithObject &asWith();

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }

#ifdef DEBUG
    void dump();
#endif

  private:
    static void staticAsserts() {
        
        JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0);

        JS_STATIC_ASSERT(offsetof(JSObject, shape_) == offsetof(js::shadow::Object, shape));
        JS_STATIC_ASSERT(offsetof(JSObject, slots) == offsetof(js::shadow::Object, slots));
        JS_STATIC_ASSERT(offsetof(JSObject, type_) == offsetof(js::shadow::Object, type));
        JS_STATIC_ASSERT(sizeof(JSObject) == sizeof(js::shadow::Object));
    }
};






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

inline js::HeapValue*
JSObject::fixedSlots() const
{
    return (js::HeapValue *) (uintptr_t(this) + sizeof(JSObject));
}

inline size_t
JSObject::numFixedSlots() const
{
    return reinterpret_cast<const js::shadow::Object *>(this)->numFixedSlots();
}

 inline size_t
JSObject::getFixedSlotOffset(size_t slot) {
    return sizeof(JSObject) + (slot * sizeof(js::Value));
}

 inline size_t
JSObject::getPrivateDataOffset(size_t nfixed) {
    return getFixedSlotOffset(nfixed);
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

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


extern bool
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap, bool *alreadySeen, bool *isSharp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);





extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, js::LookupGenericOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupGenericOp lookup, JSObject *obj, jsid id,
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

namespace js {

bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp);

void
MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp);






class NewObjectCache
{
    struct Entry
    {
        
        Class *clasp;

        












        gc::Cell *key;

        
        gc::AllocKind kind;

        
        uint32_t nbytes;

        



        JSObject_Slots16 templateObject;
    };

    Entry entries[41];

    void staticAsserts() {
        JS_STATIC_ASSERT(gc::FINALIZE_OBJECT_LAST == gc::FINALIZE_OBJECT16_BACKGROUND);
    }

  public:

    typedef int EntryIndex;

    void reset() { PodZero(this); }

    



    inline bool lookupProto(Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupGlobal(Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupType(Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry);

    
    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry);

    
    inline void fillProto(EntryIndex entry, Class *clasp, JSObject *proto, gc::AllocKind kind, JSObject *obj);
    inline void fillGlobal(EntryIndex entry, Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj);
    inline void fillType(EntryIndex entry, Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, JSObject *obj);

    
    void invalidateEntriesForShape(JSContext *cx, Shape *shape, JSObject *proto);

  private:
    inline bool lookup(Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry);
    inline void fill(EntryIndex entry, Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj);
    static inline void copyCachedToObject(JSObject *dst, JSObject *src);
};

} 




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
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto);


extern JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee, bool newType);


extern JSObject *
js_CreateThis(JSContext *cx, JSObject *callee);

extern jsid
js_CheckForStringIndex(jsid id);





extern js::Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32_t slot,
                     uintN attrs, uintN flags, intN shortid);






extern js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             js::Shape *shape, uintN attrs, uintN mask,
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
DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow = 0);

inline const Shape *
DefineNativeProperty(JSContext *cx, JSObject *obj, PropertyName *name, const Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow = 0)
{
    return DefineNativeProperty(cx, obj, ATOM_TO_JSID(name), value, getter, setter, attrs, flags,
                                shortid, defineHow);
}




extern bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp, JSProperty **propp);

inline bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, PropertyName *name, uintN flags,
                        JSObject **objp, JSProperty **propp)
{
    return LookupPropertyWithFlags(cx, obj, ATOM_TO_JSID(name), flags, objp, propp);
}









extern bool
DefineProperty(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc, bool throwError,
               bool *rval);





extern bool
ReadPropertyDescriptors(JSContext *cx, JSObject *props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs);





static const uintN RESOLVE_INFER = 0xffff;




extern bool
FindPropertyHelper(JSContext *cx, PropertyName *name, bool cacheResult, JSObject *scopeChain,
                   JSObject **objp, JSObject **pobjp, JSProperty **propp);





extern bool
FindProperty(JSContext *cx, PropertyName *name, JSObject *scopeChain,
             JSObject **objp, JSObject **pobjp, JSProperty **propp);

extern JSObject *
FindIdentifierBase(JSContext *cx, JSObject *scopeChain, PropertyName *name);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);














const uintN JSGET_METHOD_BARRIER    = 0; 
const uintN JSGET_NO_METHOD_BARRIER = 1; 
const uintN JSGET_CACHE_RESULT      = 2; 







extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const js::Shape *shape, uintN getHow,
             js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const js::Shape *shape, bool added,
             bool strict, js::Value *vp);

namespace js {

bool
GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32_t getHow, Value *vp);

inline bool
GetPropertyHelper(JSContext *cx, JSObject *obj, PropertyName *name, uint32_t getHow, Value *vp)
{
    return GetPropertyHelper(cx, obj, ATOM_TO_JSID(name), getHow, vp);
}

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, PropertyDescriptor *desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp);

} 

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);

namespace js {

inline bool
GetMethod(JSContext *cx, JSObject *obj, PropertyName *name, uintN getHow, Value *vp)
{
    return js_GetMethod(cx, obj, ATOM_TO_JSID(name), getHow, vp);
}

} 





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





extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);

extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JSObject **objp);


extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

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


inline JSObject *
ValueToObject(JSContext *cx, const Value &v)
{
    if (v.isObject())
        return &v.toObject();
    return js_ValueToNonNullObject(cx, v);
}

} 

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern bool
js_ClearNative(JSContext *cx, JSObject *obj);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);








extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);

namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

extern JSBool
eval(JSContext *cx, uintN argc, Value *vp);






extern bool
DirectEval(JSContext *cx, const CallArgs &args);





extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const js::Value &v);


extern bool
IsAnyBuiltinEval(JSFunction *fun);


extern JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx);

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);










extern void
ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp);






























inline JSObject *
NonGenericMethodGuard(JSContext *cx, CallArgs args, Native native, Class *clasp, bool *ok);








extern bool
HandleNonGenericMethodClassMismatch(JSContext *cx, CallArgs args, Native native, Class *clasp);








template <typename T>
inline bool
BoxedPrimitiveMethodGuard(JSContext *cx, CallArgs args, Native native, T *v, bool *ok);

}  

#endif 

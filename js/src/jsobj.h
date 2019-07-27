





#ifndef jsobj_h
#define jsobj_h










#include "mozilla/MemoryReporting.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/GCAPI.h"
#include "js/HeapAPI.h"
#include "vm/ObjectImpl.h"
#include "vm/Shape.h"
#include "vm/Xdr.h"

namespace JS {
struct ClassInfo;
}

namespace js {

class AutoPropDescVector;
class GCMarker;
struct NativeIterator;
class Nursery;
struct StackShape;

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



typedef Vector<PropDesc, 1> PropDescArray;







namespace baseops {






template <AllowGC allowGC>
extern bool
LookupProperty(ExclusiveContext *cx,
               typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
               typename MaybeRooted<jsid, allowGC>::HandleType id,
               typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
               typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

extern bool
LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
              MutableHandleObject objp, MutableHandleShape propp);

extern bool
DefineGeneric(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
               JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern bool
DefineElement(ExclusiveContext *cx, HandleObject obj, uint32_t index, HandleValue value,
              JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern bool
GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, MutableHandleValue vp);

extern bool
GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

extern bool
GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index, MutableHandleValue vp);

inline bool
GetProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    return GetProperty(cx, obj, obj, id, vp);
}

inline bool
GetElement(JSContext *cx, HandleObject obj, uint32_t index, MutableHandleValue vp)
{
    return GetElement(cx, obj, obj, index, vp);
}








enum QualifiedBool {
    Unqualified = 0,
    Qualified = 1
};

template <ExecutionMode mode>
extern bool
SetPropertyHelper(typename ExecutionModeTraits<mode>::ContextType cx, HandleObject obj,
                  HandleObject receiver, HandleId id, QualifiedBool qualified,
                  MutableHandleValue vp, bool strict);

extern bool
SetElementHelper(JSContext *cx, HandleObject obj, HandleObject Receiver, uint32_t index,
                 MutableHandleValue vp, bool strict);

extern bool
GetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp);

extern bool
SetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp);

extern bool
DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded);

extern bool
Watch(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);

extern bool
Unwatch(JSContext *cx, JS::HandleObject obj, JS::HandleId id);

} 

extern const Class IntlClass;
extern const Class JSONClass;
extern const Class MathClass;

class GlobalObject;
class MapObject;
class NewObjectCache;
class NormalArgumentsObject;
class SetObject;
class StrictArgumentsObject;







inline void
DenseRangeWriteBarrierPost(JSRuntime *rt, JSObject *obj, uint32_t start, uint32_t count)
{
#ifdef JSGC_GENERATIONAL
    if (count > 0) {
        JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
        shadowRuntime->gcStoreBufferPtr()->putSlotFromAnyThread(obj, HeapSlot::Element, start, count);
    }
#endif
}

namespace gc {
class ForkJoinNursery;
}

}  











class JSObject : public js::ObjectImpl
{
  private:
    friend class js::Shape;
    friend class js::GCMarker;
    friend class js::NewObjectCache;
    friend class js::Nursery;
    friend class js::gc::ForkJoinNursery;

    
    static js::types::TypeObject *makeLazyType(JSContext *cx, js::HandleObject obj);

  public:
    static const js::Class class_;

    



    static bool setLastProperty(js::ThreadSafeContext *cx,
                                JS::HandleObject obj, js::HandleShape shape);

    
    inline void setLastPropertyInfallible(js::Shape *shape);

    



    static inline JSObject *create(js::ExclusiveContext *cx,
                                   js::gc::AllocKind kind,
                                   js::gc::InitialHeap heap,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type);

    
    static inline js::ArrayObject *createArray(js::ExclusiveContext *cx,
                                               js::gc::AllocKind kind,
                                               js::gc::InitialHeap heap,
                                               js::HandleShape shape,
                                               js::HandleTypeObject type,
                                               uint32_t length);

    
    static inline js::ArrayObject *createArray(js::ExclusiveContext *cx,
                                               js::gc::InitialHeap heap,
                                               js::HandleShape shape,
                                               js::HandleTypeObject type,
                                               js::HeapSlot *elements);

    
    static inline js::ArrayObject *createCopyOnWriteArray(js::ExclusiveContext *cx,
                                                          js::gc::InitialHeap heap,
                                                          js::HandleShape shape,
                                                          js::HandleObject sharedElementsOwner);

  private:
    
    static inline JSObject *
    createArrayInternal(js::ExclusiveContext *cx, js::gc::AllocKind kind, js::gc::InitialHeap heap,
                        js::HandleShape shape, js::HandleTypeObject type);

    static inline js::ArrayObject *finishCreateArray(JSObject *obj,
                                                     js::HandleShape shape);
  public:

    




    inline void removeLastProperty(js::ExclusiveContext *cx);
    inline bool canRemoveLastProperty();

    



    static bool setSlotSpan(js::ThreadSafeContext *cx, JS::HandleObject obj, uint32_t span);

    
    static const uint32_t NELEMENTS_LIMIT = JS_BIT(28);

  public:
    bool setDelegate(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::DELEGATE, GENERATE_SHAPE);
    }

    bool isBoundFunction() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::BOUND_FUNCTION);
    }

    inline bool hasSpecialEquality() const;

    bool watched() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::WATCHED);
    }
    bool setWatched(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::WATCHED, GENERATE_SHAPE);
    }

    
    inline bool isQualifiedVarObj();
    bool setQualifiedVarObj(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::QUALIFIED_VAROBJ);
    }

    inline bool isUnqualifiedVarObj();
    bool setUnqualifiedVarObj(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::UNQUALIFIED_VAROBJ);
    }

    





    bool hasUncacheableProto() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::UNCACHEABLE_PROTO);
    }
    bool setUncacheableProto(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::UNCACHEABLE_PROTO, GENERATE_SHAPE);
    }

    



    bool hadElementsAccess() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::HAD_ELEMENTS_ACCESS);
    }
    bool setHadElementsAccess(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::HAD_ELEMENTS_ACCESS);
    }

  public:
    bool nativeEmpty() const {
        return lastProperty()->isEmptyShape();
    }

    bool shadowingShapeChange(js::ExclusiveContext *cx, const js::Shape &shape);

    



    bool isIndexed() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::INDEXED);
    }

    uint32_t propertyCount() const {
        return lastProperty()->entryCount();
    }

    bool hasShapeTable() const {
        return lastProperty()->hasTable();
    }

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ClassInfo *info);

    bool hasIdempotentProtoChain() const;

    
    
    static const uint32_t MAX_FIXED_SLOTS = 16;

  public:

    

    
    bool isFixedSlot(size_t slot) {
        return slot < numFixedSlots();
    }

    
    size_t dynamicSlotIndex(size_t slot) {
        JS_ASSERT(slot >= numFixedSlots());
        return slot - numFixedSlots();
    }

    




    static bool growSlots(js::ThreadSafeContext *cx, js::HandleObject obj, uint32_t oldCount,
                          uint32_t newCount);
    static void shrinkSlots(js::ThreadSafeContext *cx, js::HandleObject obj, uint32_t oldCount,
                            uint32_t newCount);

    bool hasDynamicSlots() const { return !!slots; }

  protected:
    static inline bool updateSlotsForSpan(js::ThreadSafeContext *cx,
                                          js::HandleObject obj, size_t oldSpan, size_t newSpan);

  public:
    



    void prepareSlotRangeForOverwrite(size_t start, size_t end) {
        for (size_t i = start; i < end; i++)
            getSlotAddressUnchecked(i)->js::HeapSlot::~HeapSlot();
    }

    void prepareElementRangeForOverwrite(size_t start, size_t end) {
        JS_ASSERT(end <= getDenseInitializedLength());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        for (size_t i = start; i < end; i++)
            elements[i].js::HeapSlot::~HeapSlot();
    }

    static bool rollbackProperties(js::ExclusiveContext *cx, js::HandleObject obj,
                                   uint32_t slotSpan);

    void nativeSetSlot(uint32_t slot, const js::Value &value) {
        JS_ASSERT(isNative());
        JS_ASSERT(slot < slotSpan());
        return setSlot(slot, value);
    }

    inline bool nativeSetSlotIfHasType(js::Shape *shape, const js::Value &value,
                                       bool overwriting = true);
    inline void nativeSetSlotWithType(js::ExclusiveContext *cx, js::Shape *shape,
                                      const js::Value &value, bool overwriting = true);

    inline const js::Value &getReservedSlot(uint32_t index) const {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlot(index);
    }

    const js::HeapSlot &getReservedSlotRef(uint32_t index) const {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlotRef(index);
    }

    js::HeapSlot &getReservedSlotRef(uint32_t index) {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        return getSlotRef(index);
    }

    void initReservedSlot(uint32_t index, const js::Value &v) {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        initSlot(index, v);
    }

    void setReservedSlot(uint32_t index, const js::Value &v) {
        JS_ASSERT(index < JSSLOT_FREE(getClass()));
        setSlot(index, v);
    }

    



    static inline bool setSingletonType(js::ExclusiveContext *cx, js::HandleObject obj);

    
    inline js::types::TypeObject* getType(JSContext *cx);
    js::types::TypeObject* uninlinedGetType(JSContext *cx);

    const js::HeapPtrTypeObject &typeFromGC() const {
        
        return type_;
    }

    











    bool uninlinedIsProxy() const;
    JSObject *getProto() const {
        JS_ASSERT(!uninlinedIsProxy());
        return getTaggedProto().toObjectOrNull();
    }
    static inline bool getProto(JSContext *cx, js::HandleObject obj,
                                js::MutableHandleObject protop);
    
    static inline bool setProto(JSContext *cx, JS::HandleObject obj,
                                JS::HandleObject proto, bool *succeeded);

    
    inline void setType(js::types::TypeObject *newType);
    void uninlinedSetType(js::types::TypeObject *newType);

#ifdef DEBUG
    bool hasNewType(const js::Class *clasp, js::types::TypeObject *newType);
#endif

    




    bool isIteratedSingleton() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::ITERATED_SINGLETON);
    }
    bool setIteratedSingleton(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::ITERATED_SINGLETON);
    }

    



    bool isNewTypeUnknown() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::NEW_TYPE_UNKNOWN);
    }
    static bool setNewTypeUnknown(JSContext *cx, const js::Class *clasp, JS::HandleObject obj);

    
    bool splicePrototype(JSContext *cx, const js::Class *clasp, js::Handle<js::TaggedProto> proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    

























    
    JSObject *getParent() const {
        return lastProperty()->getObjectParent();
    }
    static bool setParent(JSContext *cx, js::HandleObject obj, js::HandleObject newParent);

    




    inline JSObject *enclosingScope();

    
    inline JSObject *getMetadata() const {
        return lastProperty()->getObjectMetadata();
    }
    static bool setMetadata(JSContext *cx, js::HandleObject obj, js::HandleObject newMetadata);

    inline js::GlobalObject &global() const;
    inline bool isOwnGlobal() const;

    
    static inline bool clearType(JSContext *cx, js::HandleObject obj);
    static bool clearParent(JSContext *cx, js::HandleObject obj);

    



  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    static bool sealOrFreeze(JSContext *cx, js::HandleObject obj, ImmutabilityType it);

    static bool isSealedOrFrozen(JSContext *cx, js::HandleObject obj, ImmutabilityType it, bool *resultp);

    static inline unsigned getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it);

  public:
    
    static inline bool seal(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, SEAL); }
    
    static inline bool freeze(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, FREEZE); }

    static inline bool isSealed(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, SEAL, resultp);
    }
    static inline bool isFrozen(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, FREEZE, resultp);
    }

    
    static const char *className(JSContext *cx, js::HandleObject obj);

    
    bool ensureElements(js::ThreadSafeContext *cx, uint32_t capacity) {
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        if (capacity > getDenseCapacity())
            return growElements(cx, capacity);
        return true;
    }

    static uint32_t goodAllocated(uint32_t n, uint32_t length);
    bool growElements(js::ThreadSafeContext *cx, uint32_t newcap);
    void shrinkElements(js::ThreadSafeContext *cx, uint32_t cap);
    void setDynamicElements(js::ObjectElements *header) {
        JS_ASSERT(!hasDynamicElements());
        elements = header->elements();
        JS_ASSERT(hasDynamicElements());
    }

    uint32_t getDenseCapacity() {
        JS_ASSERT(isNative());
        JS_ASSERT(getElementsHeader()->capacity >= getElementsHeader()->initializedLength);
        return getElementsHeader()->capacity;
    }

    static bool CopyElementsForWrite(js::ThreadSafeContext *cx, JSObject *obj);

    bool maybeCopyElementsForWrite(js::ThreadSafeContext *cx) {
        if (denseElementsAreCopyOnWrite())
            return CopyElementsForWrite(cx, this);
        return true;
    }

  private:
    inline void ensureDenseInitializedLengthNoPackedCheck(js::ThreadSafeContext *cx,
                                                          uint32_t index, uint32_t extra);

  public:
    void setDenseInitializedLength(uint32_t length) {
        JS_ASSERT(isNative());
        JS_ASSERT(length <= getDenseCapacity());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        prepareElementRangeForOverwrite(length, getElementsHeader()->initializedLength);
        getElementsHeader()->initializedLength = length;
    }

    inline void ensureDenseInitializedLength(js::ExclusiveContext *cx,
                                             uint32_t index, uint32_t extra);
    inline void ensureDenseInitializedLengthPreservePackedFlag(js::ThreadSafeContext *cx,
                                                               uint32_t index, uint32_t extra);
    void setDenseElement(uint32_t index, const js::Value &val) {
        JS_ASSERT(isNative() && index < getDenseInitializedLength());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        elements[index].set(this, js::HeapSlot::Element, index, val);
    }

    void initDenseElement(uint32_t index, const js::Value &val) {
        JS_ASSERT(isNative() && index < getDenseInitializedLength());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        elements[index].init(this, js::HeapSlot::Element, index, val);
    }

    void setDenseElementMaybeConvertDouble(uint32_t index, const js::Value &val) {
        if (val.isInt32() && shouldConvertDoubleElements())
            setDenseElement(index, js::DoubleValue(val.toInt32()));
        else
            setDenseElement(index, val);
    }

    inline bool setDenseElementIfHasType(uint32_t index, const js::Value &val);
    inline void setDenseElementWithType(js::ExclusiveContext *cx, uint32_t index,
                                        const js::Value &val);
    inline void initDenseElementWithType(js::ExclusiveContext *cx, uint32_t index,
                                         const js::Value &val);
    inline void setDenseElementHole(js::ExclusiveContext *cx, uint32_t index);
    static inline void removeDenseElementForSparseIndex(js::ExclusiveContext *cx,
                                                        js::HandleObject obj, uint32_t index);

    inline js::Value getDenseOrTypedArrayElement(uint32_t idx);

    void copyDenseElements(uint32_t dstStart, const js::Value *src, uint32_t count) {
        JS_ASSERT(dstStart + count <= getDenseCapacity());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        JSRuntime *rt = runtimeFromMainThread();
        if (JS::IsIncrementalBarrierNeeded(rt)) {
            JS::Zone *zone = this->zone();
            for (uint32_t i = 0; i < count; ++i)
                elements[dstStart + i].set(zone, this, js::HeapSlot::Element, dstStart + i, src[i]);
        } else {
            memcpy(&elements[dstStart], src, count * sizeof(js::HeapSlot));
            DenseRangeWriteBarrierPost(rt, this, dstStart, count);
        }
    }

    void initDenseElements(uint32_t dstStart, const js::Value *src, uint32_t count) {
        JS_ASSERT(dstStart + count <= getDenseCapacity());
        JS_ASSERT(!denseElementsAreCopyOnWrite());
        memcpy(&elements[dstStart], src, count * sizeof(js::HeapSlot));
        DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
    }

    void initDenseElementsUnbarriered(uint32_t dstStart, const js::Value *src, uint32_t count);

    void moveDenseElements(uint32_t dstStart, uint32_t srcStart, uint32_t count) {
        JS_ASSERT(dstStart + count <= getDenseCapacity());
        JS_ASSERT(srcStart + count <= getDenseInitializedLength());
        JS_ASSERT(!denseElementsAreCopyOnWrite());

        











        JS::Zone *zone = this->zone();
        JS::shadow::Zone *shadowZone = JS::shadow::Zone::asShadowZone(zone);
        if (shadowZone->needsIncrementalBarrier()) {
            if (dstStart < srcStart) {
                js::HeapSlot *dst = elements + dstStart;
                js::HeapSlot *src = elements + srcStart;
                for (uint32_t i = 0; i < count; i++, dst++, src++)
                    dst->set(zone, this, js::HeapSlot::Element, dst - elements, *src);
            } else {
                js::HeapSlot *dst = elements + dstStart + count - 1;
                js::HeapSlot *src = elements + srcStart + count - 1;
                for (uint32_t i = 0; i < count; i++, dst--, src--)
                    dst->set(zone, this, js::HeapSlot::Element, dst - elements, *src);
            }
        } else {
            memmove(elements + dstStart, elements + srcStart, count * sizeof(js::HeapSlot));
            DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
        }
    }

    void moveDenseElementsNoPreBarrier(uint32_t dstStart, uint32_t srcStart, uint32_t count) {
        JS_ASSERT(!shadowZone()->needsIncrementalBarrier());

        JS_ASSERT(dstStart + count <= getDenseCapacity());
        JS_ASSERT(srcStart + count <= getDenseCapacity());
        JS_ASSERT(!denseElementsAreCopyOnWrite());

        memmove(elements + dstStart, elements + srcStart, count * sizeof(js::Value));
        DenseRangeWriteBarrierPost(runtimeFromMainThread(), this, dstStart, count);
    }

    bool shouldConvertDoubleElements() {
        JS_ASSERT(getClass()->isNative());
        return getElementsHeader()->shouldConvertDoubleElements();
    }

    inline void setShouldConvertDoubleElements();
    inline void clearShouldConvertDoubleElements();

    bool denseElementsAreCopyOnWrite() {
        JS_ASSERT(isNative());
        return getElementsHeader()->isCopyOnWrite();
    }

    void fixupAfterMovingGC();

    
    inline bool writeToIndexWouldMarkNotPacked(uint32_t index);
    inline void markDenseElementsNotPacked(js::ExclusiveContext *cx);

    






    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };

  private:
    inline EnsureDenseResult ensureDenseElementsNoPackedCheck(js::ThreadSafeContext *cx,
                                                              uint32_t index, uint32_t extra);

  public:
    inline EnsureDenseResult ensureDenseElements(js::ExclusiveContext *cx,
                                                 uint32_t index, uint32_t extra);
    inline EnsureDenseResult ensureDenseElementsPreservePackedFlag(js::ThreadSafeContext *cx,
                                                                   uint32_t index, uint32_t extra);

    inline EnsureDenseResult extendDenseElements(js::ThreadSafeContext *cx,
                                                 uint32_t requiredCapacity, uint32_t extra);

    
    static bool sparsifyDenseElement(js::ExclusiveContext *cx,
                                     js::HandleObject obj, uint32_t index);

    
    static bool sparsifyDenseElements(js::ExclusiveContext *cx, js::HandleObject obj);

    
    static const uint32_t MIN_SPARSE_INDEX = 1000;

    



    static const unsigned SPARSE_DENSITY_RATIO = 8;

    



    bool willBeSparseElements(uint32_t requiredCapacity, uint32_t newElementsHint);

    



    static EnsureDenseResult maybeDensifySparseElements(js::ExclusiveContext *cx, js::HandleObject obj);

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    bool isCallable() {
        return getClass()->isCallable();
    }
    bool isConstructor() const;

    inline void finish(js::FreeOp *fop);
    MOZ_ALWAYS_INLINE void finalize(js::FreeOp *fop);

    static inline bool hasProperty(JSContext *cx, js::HandleObject obj,
                                   js::HandleId id, bool *foundp);

    






    static bool allocSlot(js::ThreadSafeContext *cx, JS::HandleObject obj, uint32_t *slotp);
    void freeSlot(uint32_t slot);

  public:
    static bool reportReadOnly(js::ThreadSafeContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(js::ThreadSafeContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(js::ThreadSafeContext *cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, js::HandleId id, unsigned argc, js::Value *argv,
                    js::MutableHandleValue vp);

  private:
    static js::Shape *getChildPropertyOnDictionary(js::ThreadSafeContext *cx, JS::HandleObject obj,
                                                   js::HandleShape parent, js::StackShape &child);
    static js::Shape *getChildProperty(js::ExclusiveContext *cx, JS::HandleObject obj,
                                       js::HandleShape parent, js::StackShape &child);
    template <js::ExecutionMode mode>
    static inline js::Shape *
    getOrLookupChildProperty(typename js::ExecutionModeTraits<mode>::ExclusiveContextType cx,
                             JS::HandleObject obj, js::HandleShape parent, js::StackShape &child)
    {
        if (mode == js::ParallelExecution)
            return lookupChildProperty(cx, obj, parent, child);
        return getChildProperty(cx->asExclusiveContext(), obj, parent, child);
    }

  public:
    



    static js::Shape *lookupChildProperty(js::ThreadSafeContext *cx, JS::HandleObject obj,
                                          js::HandleShape parent, js::StackShape &child);


  protected:
    






    template <js::ExecutionMode mode>
    static js::Shape *
    addPropertyInternal(typename js::ExecutionModeTraits<mode>::ExclusiveContextType cx,
                        JS::HandleObject obj, JS::HandleId id,
                        JSPropertyOp getter, JSStrictPropertyOp setter,
                        uint32_t slot, unsigned attrs, unsigned flags, js::Shape **spp,
                        bool allowDictionary);

  private:
    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

  public:
    
    static js::Shape *addProperty(js::ExclusiveContext *cx, JS::HandleObject, JS::HandleId id,
                                  JSPropertyOp getter, JSStrictPropertyOp setter,
                                  uint32_t slot, unsigned attrs, unsigned flags,
                                  bool allowDictionary = true);

    
    js::Shape *addDataProperty(js::ExclusiveContext *cx,
                               jsid id_, uint32_t slot, unsigned attrs);
    js::Shape *addDataProperty(js::ExclusiveContext *cx, js::HandlePropertyName name,
                               uint32_t slot, unsigned attrs);

    
    template <js::ExecutionMode mode>
    static js::Shape *
    putProperty(typename js::ExecutionModeTraits<mode>::ExclusiveContextType cx,
                JS::HandleObject obj, JS::HandleId id,
                JSPropertyOp getter, JSStrictPropertyOp setter,
                uint32_t slot, unsigned attrs,
                unsigned flags);
    template <js::ExecutionMode mode>
    static inline js::Shape *
    putProperty(typename js::ExecutionModeTraits<mode>::ExclusiveContextType cx,
                JS::HandleObject obj, js::PropertyName *name,
                JSPropertyOp getter, JSStrictPropertyOp setter,
                uint32_t slot, unsigned attrs,
                unsigned flags);

    
    template <js::ExecutionMode mode>
    static js::Shape *
    changeProperty(typename js::ExecutionModeTraits<mode>::ExclusiveContextType cx,
                   js::HandleObject obj, js::HandleShape shape, unsigned attrs, unsigned mask,
                   JSPropertyOp getter, JSStrictPropertyOp setter);

    static inline bool changePropertyAttributes(JSContext *cx, js::HandleObject obj,
                                                js::HandleShape shape, unsigned attrs);

    
    bool removeProperty(js::ExclusiveContext *cx, jsid id);

    
    static void clear(JSContext *cx, js::HandleObject obj);

    static bool lookupGeneric(JSContext *cx, js::HandleObject obj, js::HandleId id,
                              js::MutableHandleObject objp, js::MutableHandleShape propp);

    static bool lookupProperty(JSContext *cx, js::HandleObject obj, js::PropertyName *name,
                               js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return lookupGeneric(cx, obj, id, objp, propp);
    }

    static bool lookupElement(JSContext *cx, js::HandleObject obj, uint32_t index,
                              js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        js::LookupElementOp op = obj->getOps()->lookupElement;
        return (op ? op : js::baseops::LookupElement)(cx, obj, index, objp, propp);
    }

    static bool defineGeneric(js::ExclusiveContext *cx, js::HandleObject obj,
                              js::HandleId id, js::HandleValue value,
                              JSPropertyOp getter = JS_PropertyStub,
                              JSStrictPropertyOp setter = JS_StrictPropertyStub,
                              unsigned attrs = JSPROP_ENUMERATE);

    static bool defineProperty(js::ExclusiveContext *cx, js::HandleObject obj,
                               js::PropertyName *name, js::HandleValue value,
                               JSPropertyOp getter = JS_PropertyStub,
                               JSStrictPropertyOp setter = JS_StrictPropertyStub,
                               unsigned attrs = JSPROP_ENUMERATE);

    static bool defineElement(js::ExclusiveContext *cx, js::HandleObject obj,
                              uint32_t index, js::HandleValue value,
                              JSPropertyOp getter = JS_PropertyStub,
                              JSStrictPropertyOp setter = JS_StrictPropertyStub,
                              unsigned attrs = JSPROP_ENUMERATE);

    static bool getGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                           js::HandleId id, js::MutableHandleValue vp)
    {
        JS_ASSERT(!!obj->getOps()->getGeneric == !!obj->getOps()->getProperty);
        js::GenericIdOp op = obj->getOps()->getGeneric;
        if (op) {
            if (!op(cx, obj, receiver, id, vp))
                return false;
        } else {
            if (!js::baseops::GetProperty(cx, obj, receiver, id, vp))
                return false;
        }
        return true;
    }

    static bool getGenericNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                               jsid id, js::Value *vp)
    {
        js::GenericIdOp op = obj->getOps()->getGeneric;
        if (op)
            return false;
        return js::baseops::GetPropertyNoGC(cx, obj, receiver, id, vp);
    }

    static bool getProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                            js::PropertyName *name, js::MutableHandleValue vp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return getGeneric(cx, obj, receiver, id, vp);
    }

    static bool getPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                js::PropertyName *name, js::Value *vp)
    {
        return getGenericNoGC(cx, obj, receiver, js::NameToId(name), vp);
    }

    static inline bool getElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                  uint32_t index, js::MutableHandleValue vp);
    static inline bool getElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                      uint32_t index, js::Value *vp);

    static bool setGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                           js::HandleId id, js::MutableHandleValue vp, bool strict)
    {
        if (obj->getOps()->setGeneric)
            return nonNativeSetProperty(cx, obj, id, vp, strict);
        return js::baseops::SetPropertyHelper<js::SequentialExecution>(
            cx, obj, receiver, id, js::baseops::Qualified, vp, strict);
    }

    static bool setProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                            js::PropertyName *name,
                            js::MutableHandleValue vp, bool strict)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return setGeneric(cx, obj, receiver, id, vp, strict);
    }

    static bool setElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                           uint32_t index, js::MutableHandleValue vp, bool strict)
    {
        if (obj->getOps()->setElement)
            return nonNativeSetElement(cx, obj, index, vp, strict);
        return js::baseops::SetElementHelper(cx, obj, receiver, index, vp, strict);
    }

    static bool nonNativeSetProperty(JSContext *cx, js::HandleObject obj,
                                     js::HandleId id, js::MutableHandleValue vp, bool strict);
    static bool nonNativeSetElement(JSContext *cx, js::HandleObject obj,
                                    uint32_t index, js::MutableHandleValue vp, bool strict);

    static bool getGenericAttributes(JSContext *cx, js::HandleObject obj,
                                     js::HandleId id, unsigned *attrsp)
    {
        js::GenericAttributesOp op = obj->getOps()->getGenericAttributes;
        return (op ? op : js::baseops::GetAttributes)(cx, obj, id, attrsp);
    }

    static inline bool setGenericAttributes(JSContext *cx, js::HandleObject obj,
                                            js::HandleId id, unsigned *attrsp);

    static inline bool deleteGeneric(JSContext *cx, js::HandleObject obj, js::HandleId id,
                                     bool *succeeded);
    static inline bool deleteElement(JSContext *cx, js::HandleObject obj, uint32_t index,
                                     bool *succeeded);

    static inline bool watch(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                             JS::HandleObject callable);
    static inline bool unwatch(JSContext *cx, JS::HandleObject obj, JS::HandleId id);

    static bool enumerate(JSContext *cx, JS::HandleObject obj, JSIterateOp iterop,
                          JS::MutableHandleValue statep, JS::MutableHandleId idp)
    {
        JSNewEnumerateOp op = obj->getOps()->enumerate;
        return (op ? op : JS_EnumerateState)(cx, obj, iterop, statep, idp);
    }

    static bool defaultValue(JSContext *cx, js::HandleObject obj, JSType hint,
                             js::MutableHandleValue vp)
    {
        JSConvertOp op = obj->getClass()->convert;
        bool ok;
        if (op == JS_ConvertStub)
            ok = js::DefaultValue(cx, obj, hint, vp);
        else
            ok = op(cx, obj, hint, vp);
        JS_ASSERT_IF(ok, vp.isPrimitive());
        return ok;
    }

    static JSObject *thisObject(JSContext *cx, js::HandleObject obj)
    {
        if (js::ObjectOp op = obj->getOps()->thisObject)
            return op(cx, obj);
        return obj;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    static bool swap(JSContext *cx, JS::HandleObject a, JS::HandleObject b);

    inline void initArrayClass();

    






















    template <class T>
    inline bool is() const { return getClass() == &T::class_; }

    template <class T>
    T &as() {
        JS_ASSERT(is<T>());
        return *static_cast<T *>(this);
    }

    template <class T>
    const T &as() const {
        JS_ASSERT(is<T>());
        return *static_cast<const T *>(this);
    }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }

#ifdef DEBUG
    void dump();
#endif

  private:
    static void staticAsserts() {
        static_assert(sizeof(JSObject) == sizeof(js::shadow::Object),
                      "shadow interface must match actual interface");
        static_assert(sizeof(JSObject) == sizeof(js::ObjectImpl),
                      "JSObject itself must not have any fields");
        static_assert(sizeof(JSObject) % sizeof(js::Value) == 0,
                      "fixed slots after an object must be aligned");
        static_assert(js::shadow::Object::MAX_FIXED_SLOTS == MAX_FIXED_SLOTS,
                      "We shouldn't be confused about our actual maximum "
                      "number of fixed slots");
    }

    JSObject() MOZ_DELETE;
    JSObject(const JSObject &other) MOZ_DELETE;
    void operator=(const JSObject &other) MOZ_DELETE;
};

template <class U>
MOZ_ALWAYS_INLINE JS::Handle<U*>
js::RootedBase<JSObject*>::as() const
{
    const JS::Rooted<JSObject*> &self = *static_cast<const JS::Rooted<JSObject*>*>(this);
    JS_ASSERT(self->is<U>());
    return Handle<U*>::fromMarkedLocation(reinterpret_cast<U* const*>(self.address()));
}






static MOZ_ALWAYS_INLINE bool
operator==(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs == &rhs;
}

static MOZ_ALWAYS_INLINE bool
operator!=(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs != &rhs;
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

namespace js {

inline bool
IsCallable(const Value &v)
{
    return v.isObject() && v.toObject().isCallable();
}


inline bool
IsConstructor(const Value &v)
{
    return v.isObject() && v.toObject().isConstructor();
}

inline JSObject *
GetInnerObject(JSObject *obj)
{
    if (js::InnerObjectOp op = obj->getClass()->ext.innerObject) {
        JS::AutoSuppressGCAnalysis nogc;
        return op(obj);
    }
    return obj;
}

inline JSObject *
GetOuterObject(JSContext *cx, js::HandleObject obj)
{
    if (js::ObjectOp op = obj->getClass()->ext.outerObject)
        return op(cx, obj);
    return obj;
}

} 

class JSValueArray {
  public:
    const jsval *array;
    size_t length;

    JSValueArray(const jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};

namespace js {


bool
HasOwnProperty(JSContext *cx, HandleObject obj, HandleId id, bool *resultp);

template <AllowGC allowGC>
extern bool
HasOwnProperty(JSContext *cx, LookupGenericOp lookup,
               typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
               typename MaybeRooted<jsid, allowGC>::HandleType id,
               typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
               typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

typedef JSObject *(*ClassInitializerOp)(JSContext *cx, JS::HandleObject obj);


bool
GetBuiltinConstructor(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject objp);

bool
GetBuiltinPrototype(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject objp);

JSObject *
GetBuiltinPrototypePure(GlobalObject *global, JSProtoKey protoKey);

extern bool
SetClassAndProto(JSContext *cx, HandleObject obj,
                 const Class *clasp, Handle<TaggedProto> proto, bool *succeeded);







bool
FindClassObject(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp);

extern bool
FindClassPrototype(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp);

} 




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef JS_OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

extern bool
js_PopulateObject(JSContext *cx, js::HandleObject newborn, js::HandleObject props);


namespace js {

extern bool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::HandleValue descriptor, bool *bp);

extern bool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::Handle<js::PropertyDescriptor> descriptor, bool *bp);





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    MaybeSingletonObject,

    




    TenuredObject
};

inline gc::InitialHeap
GetInitialHeap(NewObjectKind newKind, const Class *clasp)
{
    if (clasp->finalize || newKind != GenericObject)
        return gc::TenuredHeap;
    return gc::DefaultHeap;
}



extern JSObject *
CreateThisForFunctionWithProto(JSContext *cx, js::HandleObject callee, JSObject *proto,
                               NewObjectKind newKind = GenericObject);


extern JSObject *
CreateThisForFunction(JSContext *cx, js::HandleObject callee, NewObjectKind newKind);


extern JSObject *
CreateThis(JSContext *cx, const js::Class *clasp, js::HandleObject callee);

extern JSObject *
CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent);

extern JSObject *
DeepCloneObjectLiteral(JSContext *cx, HandleObject obj, NewObjectKind newKind = GenericObject);




extern bool
DefineNativeProperty(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

extern bool
LookupNativeProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                     js::MutableHandleObject objp, js::MutableHandleShape propp);









extern bool
DefineProperty(JSContext *cx, js::HandleObject obj,
               js::HandleId id, const PropDesc &desc, bool throwError,
               bool *rval);

bool
DefineProperties(JSContext *cx, HandleObject obj, HandleObject props);





extern bool
ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescVector *descs);


extern bool
LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
           MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp);

extern bool
LookupNameNoGC(JSContext *cx, PropertyName *name, JSObject *scopeChain,
               JSObject **objp, JSObject **pobjp, Shape **propp);








extern bool
LookupNameWithGlobalDefault(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                            MutableHandleObject objp);








extern bool
LookupNameUnqualified(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                      MutableHandleObject objp);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);


namespace js {

bool
NativeGet(JSContext *cx, js::Handle<JSObject*> obj, js::Handle<JSObject*> pobj,
          js::Handle<js::Shape*> shape, js::MutableHandle<js::Value> vp);

template <js::ExecutionMode mode>
bool
NativeSet(typename js::ExecutionModeTraits<mode>::ContextType cx,
          js::Handle<JSObject*> obj, js::Handle<JSObject*> receiver,
          js::Handle<js::Shape*> shape, bool strict, js::MutableHandleValue vp);

bool
LookupPropertyPure(JSObject *obj, jsid id, JSObject **objp, Shape **propp);

bool
GetPropertyPure(ThreadSafeContext *cx, JSObject *obj, jsid id, Value *vp);

inline bool
GetPropertyPure(ThreadSafeContext *cx, JSObject *obj, PropertyName *name, Value *vp)
{
    return GetPropertyPure(cx, obj, NameToId(name), vp);
}

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                         MutableHandle<PropertyDescriptor> desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp);

bool
NewPropertyDescriptorObject(JSContext *cx, Handle<PropertyDescriptor> desc, MutableHandleValue vp);





extern bool
HasDataProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp);

inline bool
HasDataProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp)
{
    return HasDataProperty(cx, obj, NameToId(name), vp);
}

extern bool
IsDelegate(JSContext *cx, HandleObject obj, const Value &v, bool *result);



extern bool
IsDelegateOfObject(JSContext *cx, HandleObject protoObj, JSObject* obj, bool *result);

bool
GetObjectElementOperationPure(ThreadSafeContext *cx, JSObject *obj, const Value &prop, Value *vp);


extern JSObject *
PrimitiveToObject(JSContext *cx, const Value &v);

} 

namespace js {






extern JSObject *
ToObjectSlow(JSContext *cx, HandleValue vp, bool reportScanStack);


MOZ_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, false);
}


MOZ_ALWAYS_INLINE JSObject *
ToObjectFromStack(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, true);
}

template<XDRMode mode>
bool
XDRObjectLiteral(XDRState<mode> *xdr, MutableHandleObject obj);

extern JSObject *
CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj);

} 

extern void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern bool
js_ReportGetterOnlyAssignment(JSContext *cx, bool strict);


namespace js {

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);

extern bool
GetFirstArgumentAsObject(JSContext *cx, const CallArgs &args, const char *method,
                         MutableHandleObject objp);


extern bool
Throw(JSContext *cx, jsid id, unsigned errorNumber);

extern bool
Throw(JSContext *cx, JSObject *obj, unsigned errorNumber);

}  

#endif 

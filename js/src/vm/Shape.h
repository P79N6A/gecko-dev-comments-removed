





#ifndef vm_Shape_h
#define vm_Shape_h

#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TemplateLib.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsinfer.h"
#include "jspropertytree.h"
#include "jstypes.h"
#include "NamespaceImports.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "gc/Rooting.h"
#include "js/HashTable.h"
#include "js/MemoryMetrics.h"
#include "js/RootingAPI.h"
#include "vm/PropDesc.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif


































































#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

namespace js {

class Bindings;
class Debugger;
class Nursery;
class ObjectImpl;
class StaticBlockObject;

namespace gc {
class ForkJoinNursery;
}

typedef JSPropertyOp         PropertyOp;
typedef JSStrictPropertyOp   StrictPropertyOp;
typedef JSPropertyDescriptor PropertyDescriptor;


static const uint32_t SHAPE_INVALID_SLOT = JS_BIT(24) - 1;
static const uint32_t SHAPE_MAXIMUM_SLOT = JS_BIT(24) - 2;





struct ShapeTable {
    static const uint32_t HASH_BITS     = mozilla::tl::BitSize<HashNumber>::value;
    static const uint32_t MIN_ENTRIES   = 11;

    
    
    static const uint32_t MIN_SIZE_LOG2 = 2;
    static const uint32_t MIN_SIZE      = JS_BIT(MIN_SIZE_LOG2);

    int             hashShift;          

    uint32_t        entryCount;         
    uint32_t        removedCount;       
    uint32_t        freelist;           


    js::Shape       **entries;          

    explicit ShapeTable(uint32_t nentries)
      : hashShift(HASH_BITS - MIN_SIZE_LOG2),
        entryCount(nentries),
        removedCount(0),
        freelist(SHAPE_INVALID_SLOT)
    {
        
    }

    ~ShapeTable() {
        js_free(entries);
    }

    
    uint32_t capacity() const { return JS_BIT(HASH_BITS - hashShift); }

    



    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(this) + mallocSizeOf(entries);
    }

    
    bool needsToGrow() const {
        uint32_t size = capacity();
        return entryCount + removedCount >= size - (size >> 2);
    }

    




    bool grow(ThreadSafeContext *cx);

    




    bool            init(ThreadSafeContext *cx, Shape *lastProp);
    bool            change(int log2Delta, ThreadSafeContext *cx);
    Shape           **search(jsid id, bool adding);

#ifdef JSGC_COMPACTING
    
    void            fixupAfterMovingGC();
#endif
};




#define JSPROP_SHADOWABLE       JSPROP_INDEX

















































class Shape;
class UnownedBaseShape;
struct StackBaseShape;

namespace gc {
void MergeCompartments(JSCompartment *source, JSCompartment *target);
}

static inline void
GetterSetterWriteBarrierPost(JSRuntime *rt, JSObject **objp)
{
#ifdef JSGC_GENERATIONAL
    JS_ASSERT(objp);
    JS_ASSERT(*objp);
    gc::Cell **cellp = reinterpret_cast<gc::Cell **>(objp);
    gc::StoreBuffer *storeBuffer = (*cellp)->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(cellp);
#endif
}

static inline void
GetterSetterWriteBarrierPostRemove(JSRuntime *rt, JSObject **objp)
{
#ifdef JSGC_GENERATIONAL
    JS::shadow::Runtime *shadowRuntime = JS::shadow::Runtime::asShadowRuntime(rt);
    shadowRuntime->gcStoreBufferPtr()->removeRelocatableCellFromAnyThread(reinterpret_cast<gc::Cell **>(objp));
#endif
}

class BaseShape : public gc::TenuredCell
{
  public:
    friend class Shape;
    friend struct StackBaseShape;
    friend struct StackShape;
    friend void gc::MergeCompartments(JSCompartment *source, JSCompartment *target);

    enum Flag {
        
        OWNED_SHAPE        = 0x1,

        
        HAS_GETTER_OBJECT  = 0x2,
        HAS_SETTER_OBJECT  = 0x4,

        









        DELEGATE            =    0x8,
        NOT_EXTENSIBLE      =   0x10,
        INDEXED             =   0x20,
        BOUND_FUNCTION      =   0x40,
        HAD_ELEMENTS_ACCESS =   0x80,
        WATCHED             =  0x100,
        ITERATED_SINGLETON  =  0x200,
        NEW_TYPE_UNKNOWN    =  0x400,
        UNCACHEABLE_PROTO   =  0x800,

        
        
        
        
        
        
        
        
        QUALIFIED_VAROBJ    = 0x1000,
        UNQUALIFIED_VAROBJ  = 0x2000,

        OBJECT_FLAG_MASK    = 0x3ff8
    };

  private:
    const Class         *clasp_;        
    HeapPtrObject       parent;         
    HeapPtrObject       metadata;       

    JSCompartment       *compartment_;  
    uint32_t            flags;          
    uint32_t            slotSpan_;      


    union {
        PropertyOp      rawGetter;      
        JSObject        *getterObj;     

    };

    union {
        StrictPropertyOp rawSetter;     
        JSObject        *setterObj;     

    };

    
    HeapPtrUnownedBaseShape unowned_;

    
    ShapeTable       *table_;

    BaseShape(const BaseShape &base) MOZ_DELETE;

  public:
    void finalize(FreeOp *fop);

    BaseShape(JSCompartment *comp, const Class *clasp, JSObject *parent, JSObject *metadata,
              uint32_t objectFlags)
    {
        JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
        mozilla::PodZero(this);
        this->clasp_ = clasp;
        this->parent = parent;
        this->metadata = metadata;
        this->flags = objectFlags;
        this->compartment_ = comp;
    }

    BaseShape(JSCompartment *comp, const Class *clasp, JSObject *parent, JSObject *metadata,
              uint32_t objectFlags, uint8_t attrs,
              PropertyOp rawGetter, StrictPropertyOp rawSetter)
    {
        JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
        mozilla::PodZero(this);
        this->clasp_ = clasp;
        this->parent = parent;
        this->metadata = metadata;
        this->flags = objectFlags;
        this->rawGetter = rawGetter;
        this->rawSetter = rawSetter;
        if ((attrs & JSPROP_GETTER) && rawGetter) {
            this->flags |= HAS_GETTER_OBJECT;
            GetterSetterWriteBarrierPost(runtimeFromMainThread(), &this->getterObj);
        }
        if ((attrs & JSPROP_SETTER) && rawSetter) {
            this->flags |= HAS_SETTER_OBJECT;
            GetterSetterWriteBarrierPost(runtimeFromMainThread(), &this->setterObj);
        }
        this->compartment_ = comp;
    }

    explicit inline BaseShape(const StackBaseShape &base);

    
    ~BaseShape();

    BaseShape &operator=(const BaseShape &other) {
        clasp_ = other.clasp_;
        parent = other.parent;
        metadata = other.metadata;
        flags = other.flags;
        slotSpan_ = other.slotSpan_;
        if (flags & HAS_GETTER_OBJECT) {
            getterObj = other.getterObj;
            GetterSetterWriteBarrierPost(runtimeFromMainThread(), &getterObj);
        } else {
            if (rawGetter)
                GetterSetterWriteBarrierPostRemove(runtimeFromMainThread(), &getterObj);
            rawGetter = other.rawGetter;
        }
        if (flags & HAS_SETTER_OBJECT) {
            setterObj = other.setterObj;
            GetterSetterWriteBarrierPost(runtimeFromMainThread(), &setterObj);
        } else {
            if (rawSetter)
                GetterSetterWriteBarrierPostRemove(runtimeFromMainThread(), &setterObj);
            rawSetter = other.rawSetter;
        }
        compartment_ = other.compartment_;
        return *this;
    }

    const Class *clasp() const { return clasp_; }

    bool isOwned() const { return !!(flags & OWNED_SHAPE); }

    bool matchesGetterSetter(PropertyOp rawGetter, StrictPropertyOp rawSetter) const {
        return rawGetter == this->rawGetter && rawSetter == this->rawSetter;
    }

    inline void adoptUnowned(UnownedBaseShape *other);

    void setOwned(UnownedBaseShape *unowned) {
        flags |= OWNED_SHAPE;
        this->unowned_ = unowned;
    }

    JSObject *getObjectParent() const { return parent; }
    JSObject *getObjectMetadata() const { return metadata; }
    uint32_t getObjectFlags() const { return flags & OBJECT_FLAG_MASK; }

    bool hasGetterObject() const { return !!(flags & HAS_GETTER_OBJECT); }
    JSObject *getterObject() const { JS_ASSERT(hasGetterObject()); return getterObj; }

    bool hasSetterObject() const { return !!(flags & HAS_SETTER_OBJECT); }
    JSObject *setterObject() const { JS_ASSERT(hasSetterObject()); return setterObj; }

    bool hasTable() const { JS_ASSERT_IF(table_, isOwned()); return table_ != nullptr; }
    ShapeTable &table() const { JS_ASSERT(table_ && isOwned()); return *table_; }
    void setTable(ShapeTable *table) { JS_ASSERT(isOwned()); table_ = table; }

    uint32_t slotSpan() const { JS_ASSERT(isOwned()); return slotSpan_; }
    void setSlotSpan(uint32_t slotSpan) { JS_ASSERT(isOwned()); slotSpan_ = slotSpan; }

    JSCompartment *compartment() const { return compartment_; }

    



    static UnownedBaseShape* getUnowned(ExclusiveContext *cx, StackBaseShape &base);

    



    static UnownedBaseShape *lookupUnowned(ThreadSafeContext *cx, const StackBaseShape &base);

    
    inline UnownedBaseShape* unowned();

    
    inline UnownedBaseShape* baseUnowned();

    
    inline UnownedBaseShape* toUnowned();

    
    void assertConsistency();

    
    static inline size_t offsetOfParent() { return offsetof(BaseShape, parent); }
    static inline size_t offsetOfFlags() { return offsetof(BaseShape, flags); }

    static inline ThingRootKind rootKind() { return THING_ROOT_BASE_SHAPE; }

    void markChildren(JSTracer *trc) {
        if (hasGetterObject())
            gc::MarkObjectUnbarriered(trc, &getterObj, "getter");

        if (hasSetterObject())
            gc::MarkObjectUnbarriered(trc, &setterObj, "setter");

        if (isOwned())
            gc::MarkBaseShape(trc, &unowned_, "base");

        if (parent)
            gc::MarkObject(trc, &parent, "parent");

        if (metadata)
            gc::MarkObject(trc, &metadata, "metadata");
    }

#ifdef JSGC_COMPACTING
    void fixupAfterMovingGC();
#endif

  private:
    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(BaseShape, clasp_) == offsetof(js::shadow::BaseShape, clasp_));
    }
};

class UnownedBaseShape : public BaseShape {};

inline void
BaseShape::adoptUnowned(UnownedBaseShape *other)
{
    
    
    JS_ASSERT(isOwned());

    uint32_t span = slotSpan();
    ShapeTable *table = &this->table();

    *this = *other;
    setOwned(other);
    setTable(table);
    setSlotSpan(span);

    assertConsistency();
}

UnownedBaseShape *
BaseShape::unowned()
{
    return isOwned() ? baseUnowned() : toUnowned();
}

UnownedBaseShape *
BaseShape::toUnowned()
{
    JS_ASSERT(!isOwned() && !unowned_); return static_cast<UnownedBaseShape *>(this);
}

UnownedBaseShape*
BaseShape::baseUnowned()
{
    JS_ASSERT(isOwned() && unowned_); return unowned_;
}


struct StackBaseShape : public DefaultHasher<ReadBarrieredUnownedBaseShape>
{
    typedef const StackBaseShape *Lookup;

    uint32_t flags;
    const Class *clasp;
    JSObject *parent;
    JSObject *metadata;
    PropertyOp rawGetter;
    StrictPropertyOp rawSetter;
    JSCompartment *compartment;

    explicit StackBaseShape(BaseShape *base)
      : flags(base->flags & BaseShape::OBJECT_FLAG_MASK),
        clasp(base->clasp_),
        parent(base->parent),
        metadata(base->metadata),
        rawGetter(nullptr),
        rawSetter(nullptr),
        compartment(base->compartment())
    {}

    inline StackBaseShape(ThreadSafeContext *cx, const Class *clasp,
                          JSObject *parent, JSObject *metadata, uint32_t objectFlags);
    explicit inline StackBaseShape(Shape *shape);

    void updateGetterSetter(uint8_t attrs, PropertyOp rawGetter, StrictPropertyOp rawSetter) {
        flags &= ~(BaseShape::HAS_GETTER_OBJECT | BaseShape::HAS_SETTER_OBJECT);
        if ((attrs & JSPROP_GETTER) && rawGetter) {
            JS_ASSERT(!IsPoisonedPtr(rawGetter));
            flags |= BaseShape::HAS_GETTER_OBJECT;
        }
        if ((attrs & JSPROP_SETTER) && rawSetter) {
            JS_ASSERT(!IsPoisonedPtr(rawSetter));
            flags |= BaseShape::HAS_SETTER_OBJECT;
        }

        this->rawGetter = rawGetter;
        this->rawSetter = rawSetter;
    }

    static inline HashNumber hash(const StackBaseShape *lookup);
    static inline bool match(UnownedBaseShape *key, const StackBaseShape *lookup);

    
    void trace(JSTracer *trc);
};

inline
BaseShape::BaseShape(const StackBaseShape &base)
{
    mozilla::PodZero(this);
    this->clasp_ = base.clasp;
    this->parent = base.parent;
    this->metadata = base.metadata;
    this->flags = base.flags;
    this->rawGetter = base.rawGetter;
    this->rawSetter = base.rawSetter;
    if ((base.flags & HAS_GETTER_OBJECT) && base.rawGetter)
        GetterSetterWriteBarrierPost(runtimeFromMainThread(), &this->getterObj);
    if ((base.flags & HAS_SETTER_OBJECT) && base.rawSetter)
        GetterSetterWriteBarrierPost(runtimeFromMainThread(), &this->setterObj);
    this->compartment_ = base.compartment;
}

typedef HashSet<ReadBarrieredUnownedBaseShape,
                StackBaseShape,
                SystemAllocPolicy> BaseShapeSet;


class Shape : public gc::TenuredCell
{
    friend class ::JSObject;
    friend class ::JSFunction;
    friend class js::Bindings;
    friend class js::Nursery;
    friend class js::gc::ForkJoinNursery;
    friend class js::ObjectImpl;
    friend class js::PropertyTree;
    friend class js::StaticBlockObject;
    friend struct js::StackShape;
    friend struct js::StackBaseShape;

  protected:
    HeapPtrBaseShape    base_;
    PreBarrieredId      propid_;

    JS_ENUM_HEADER(SlotInfo, uint32_t)
    {
        
        
        FIXED_SLOTS_MAX        = 0x1f,
        FIXED_SLOTS_SHIFT      = 27,
        FIXED_SLOTS_MASK       = uint32_t(FIXED_SLOTS_MAX << FIXED_SLOTS_SHIFT),

        





        LINEAR_SEARCHES_MAX    = 0x7,
        LINEAR_SEARCHES_SHIFT  = 24,
        LINEAR_SEARCHES_MASK   = LINEAR_SEARCHES_MAX << LINEAR_SEARCHES_SHIFT,

        





        SLOT_MASK              = JS_BIT(24) - 1
    } JS_ENUM_FOOTER(SlotInfo);

    uint32_t            slotInfo;       
    uint8_t             attrs;          
    uint8_t             flags;          

    HeapPtrShape        parent;        
    
    union {
        KidsPointer kids;       

        HeapPtrShape *listp;    



    };

    static inline Shape *search(ExclusiveContext *cx, Shape *start, jsid id,
                                Shape ***pspp, bool adding = false);
    static inline Shape *searchThreadLocal(ThreadSafeContext *cx, Shape *start, jsid id,
                                           Shape ***pspp, bool adding = false);
    static inline Shape *searchNoHashify(Shape *start, jsid id);

    void removeFromDictionary(ObjectImpl *obj);
    void insertIntoDictionary(HeapPtrShape *dictp);

    void initDictionaryShape(const StackShape &child, uint32_t nfixed, HeapPtrShape *dictp) {
        new (this) Shape(child, nfixed);
        this->flags |= IN_DICTIONARY;

        this->listp = nullptr;
        insertIntoDictionary(dictp);
    }

    
    static Shape *replaceLastProperty(ExclusiveContext *cx, StackBaseShape &base,
                                      TaggedProto proto, HandleShape shape);

    




    static bool hashify(ThreadSafeContext *cx, Shape *shape);
    void handoffTableTo(Shape *newShape);

    void setParent(Shape *p) {
        JS_ASSERT_IF(p && !p->hasMissingSlot() && !inDictionary(),
                     p->maybeSlot() <= maybeSlot());
        JS_ASSERT_IF(p && !inDictionary(),
                     hasSlot() == (p->maybeSlot() != maybeSlot()));
        parent = p;
    }

    bool ensureOwnBaseShape(ThreadSafeContext *cx) {
        if (base()->isOwned())
            return true;
        return makeOwnBaseShape(cx);
    }

    bool makeOwnBaseShape(ThreadSafeContext *cx);

  public:
    bool hasTable() const { return base()->hasTable(); }
    ShapeTable &table() const { return base()->table(); }

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                JS::ClassInfo *info) const
    {
        if (hasTable()) {
            if (inDictionary())
                info->shapesMallocHeapDictTables += table().sizeOfIncludingThis(mallocSizeOf);
            else
                info->shapesMallocHeapTreeTables += table().sizeOfIncludingThis(mallocSizeOf);
        }

        if (!inDictionary() && kids.isHash())
            info->shapesMallocHeapTreeKids += kids.toHash()->sizeOfIncludingThis(mallocSizeOf);
    }

    bool isNative() const {
        JS_ASSERT(!(flags & NON_NATIVE) == getObjectClass()->isNative());
        return !(flags & NON_NATIVE);
    }

    const HeapPtrShape &previous() const { return parent; }
    JSCompartment *compartment() const { return base()->compartment(); }

    template <AllowGC allowGC>
    class Range {
      protected:
        friend class Shape;

        typename MaybeRooted<Shape*, allowGC>::RootType cursor;

      public:
        Range(ExclusiveContext *cx, Shape *shape) : cursor(cx, shape) {
            JS_STATIC_ASSERT(allowGC == CanGC);
        }

        explicit Range(Shape *shape) : cursor((ExclusiveContext *) nullptr, shape) {
            JS_STATIC_ASSERT(allowGC == NoGC);
        }

        bool empty() const {
            return !cursor || cursor->isEmptyShape();
        }

        Shape &front() const {
            JS_ASSERT(!empty());
            return *cursor;
        }

        void popFront() {
            JS_ASSERT(!empty());
            cursor = cursor->parent;
        }
    };

    const Class *getObjectClass() const {
        return base()->clasp_;
    }
    JSObject *getObjectParent() const { return base()->parent; }
    JSObject *getObjectMetadata() const { return base()->metadata; }

    static Shape *setObjectParent(ExclusiveContext *cx,
                                  JSObject *obj, TaggedProto proto, Shape *last);
    static Shape *setObjectMetadata(JSContext *cx,
                                    JSObject *metadata, TaggedProto proto, Shape *last);
    static Shape *setObjectFlag(ExclusiveContext *cx,
                                BaseShape::Flag flag, TaggedProto proto, Shape *last);

    uint32_t getObjectFlags() const { return base()->getObjectFlags(); }
    bool hasObjectFlag(BaseShape::Flag flag) const {
        JS_ASSERT(!(flag & ~BaseShape::OBJECT_FLAG_MASK));
        return !!(base()->flags & flag);
    }

  protected:
    




    enum {
        
        NON_NATIVE      = 0x01,

        
        IN_DICTIONARY   = 0x02,

        



        OVERWRITTEN     = 0x04,

        UNUSED_BITS     = 0x38
    };

    
    inline Shape(const StackShape &other, uint32_t nfixed);

    
    inline Shape(UnownedBaseShape *base, uint32_t nfixed);

    
    Shape(const Shape &other) MOZ_DELETE;

    





    bool hasMissingSlot() const { return maybeSlot() == SHAPE_INVALID_SLOT; }

  public:
    bool inDictionary() const {
        return (flags & IN_DICTIONARY) != 0;
    }

    PropertyOp getter() const { return base()->rawGetter; }
    bool hasDefaultGetter() const {return !base()->rawGetter; }
    PropertyOp getterOp() const { JS_ASSERT(!hasGetterValue()); return base()->rawGetter; }
    JSObject *getterObject() const { JS_ASSERT(hasGetterValue()); return base()->getterObj; }

    
    Value getterValue() const {
        JS_ASSERT(hasGetterValue());
        return base()->getterObj ? ObjectValue(*base()->getterObj) : UndefinedValue();
    }

    Value getterOrUndefined() const {
        return (hasGetterValue() && base()->getterObj)
               ? ObjectValue(*base()->getterObj)
               : UndefinedValue();
    }

    StrictPropertyOp setter() const { return base()->rawSetter; }
    bool hasDefaultSetter() const  { return !base()->rawSetter; }
    StrictPropertyOp setterOp() const { JS_ASSERT(!hasSetterValue()); return base()->rawSetter; }
    JSObject *setterObject() const { JS_ASSERT(hasSetterValue()); return base()->setterObj; }

    
    Value setterValue() const {
        JS_ASSERT(hasSetterValue());
        return base()->setterObj ? ObjectValue(*base()->setterObj) : UndefinedValue();
    }

    Value setterOrUndefined() const {
        return (hasSetterValue() && base()->setterObj)
               ? ObjectValue(*base()->setterObj)
               : UndefinedValue();
    }

    void setOverwritten() {
        flags |= OVERWRITTEN;
    }
    bool hadOverwrite() const {
        return flags & OVERWRITTEN;
    }

    void update(PropertyOp getter, StrictPropertyOp setter, uint8_t attrs);

    bool matches(const Shape *other) const {
        return propid_.get() == other->propid_.get() &&
               matchesParamsAfterId(other->base(), other->maybeSlot(), other->attrs, other->flags);
    }

    inline bool matches(const StackShape &other) const;

    bool matchesParamsAfterId(BaseShape *base, uint32_t aslot, unsigned aattrs, unsigned aflags) const
    {
        return base->unowned() == this->base()->unowned() &&
               maybeSlot() == aslot &&
               attrs == aattrs;
    }

    bool get(JSContext* cx, HandleObject receiver, JSObject *obj, JSObject *pobj, MutableHandleValue vp);
    bool set(JSContext* cx, HandleObject obj, HandleObject receiver, bool strict, MutableHandleValue vp);

    BaseShape *base() const { return base_.get(); }

    bool hasSlot() const {
        return (attrs & JSPROP_SHARED) == 0;
    }
    uint32_t slot() const { JS_ASSERT(hasSlot() && !hasMissingSlot()); return maybeSlot(); }
    uint32_t maybeSlot() const {
        return slotInfo & SLOT_MASK;
    }

    bool isEmptyShape() const {
        JS_ASSERT_IF(JSID_IS_EMPTY(propid_), hasMissingSlot());
        return JSID_IS_EMPTY(propid_);
    }

    uint32_t slotSpan(const Class *clasp) const {
        JS_ASSERT(!inDictionary());
        uint32_t free = JSSLOT_FREE(clasp);
        return hasMissingSlot() ? free : Max(free, maybeSlot() + 1);
    }

    uint32_t slotSpan() const {
        return slotSpan(getObjectClass());
    }

    void setSlot(uint32_t slot) {
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        slotInfo = slotInfo & ~Shape::SLOT_MASK;
        slotInfo = slotInfo | slot;
    }

    uint32_t numFixedSlots() const {
        return slotInfo >> FIXED_SLOTS_SHIFT;
    }

    void setNumFixedSlots(uint32_t nfixed) {
        JS_ASSERT(nfixed < FIXED_SLOTS_MAX);
        slotInfo = slotInfo & ~FIXED_SLOTS_MASK;
        slotInfo = slotInfo | (nfixed << FIXED_SLOTS_SHIFT);
    }

    uint32_t numLinearSearches() const {
        return (slotInfo & LINEAR_SEARCHES_MASK) >> LINEAR_SEARCHES_SHIFT;
    }

    void incrementNumLinearSearches() {
        uint32_t count = numLinearSearches();
        JS_ASSERT(count < LINEAR_SEARCHES_MAX);
        slotInfo = slotInfo & ~LINEAR_SEARCHES_MASK;
        slotInfo = slotInfo | ((count + 1) << LINEAR_SEARCHES_SHIFT);
    }

    const PreBarrieredId &propid() const {
        JS_ASSERT(!isEmptyShape());
        JS_ASSERT(!JSID_IS_VOID(propid_));
        return propid_;
    }
    PreBarrieredId &propidRef() { JS_ASSERT(!JSID_IS_VOID(propid_)); return propid_; }
    jsid propidRaw() const {
        
        return propid();
    }

    uint8_t attributes() const { return attrs; }
    bool configurable() const { return (attrs & JSPROP_PERMANENT) == 0; }
    bool enumerable() const { return (attrs & JSPROP_ENUMERATE) != 0; }
    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }
    bool hasGetterValue() const { return attrs & JSPROP_GETTER; }
    bool hasSetterValue() const { return attrs & JSPROP_SETTER; }

    bool isDataDescriptor() const {
        return (attrs & (JSPROP_SETTER | JSPROP_GETTER)) == 0;
    }
    bool isAccessorDescriptor() const {
        return (attrs & (JSPROP_SETTER | JSPROP_GETTER)) != 0;
    }

    PropDesc::Writability writability() const {
        return (attrs & JSPROP_READONLY) ? PropDesc::NonWritable : PropDesc::Writable;
    }
    PropDesc::Enumerability enumerability() const {
        return (attrs & JSPROP_ENUMERATE) ? PropDesc::Enumerable : PropDesc::NonEnumerable;
    }
    PropDesc::Configurability configurability() const {
        return (attrs & JSPROP_PERMANENT) ? PropDesc::NonConfigurable : PropDesc::Configurable;
    }

    





    bool shadowable() const {
        JS_ASSERT_IF(isDataDescriptor(), writable());
        return hasSlot() || (attrs & JSPROP_SHADOWABLE);
    }

    uint32_t entryCount() {
        if (hasTable())
            return table().entryCount;
        uint32_t count = 0;
        for (Shape::Range<NoGC> r(this); !r.empty(); r.popFront())
            ++count;
        return count;
    }

    bool isBigEnoughForAShapeTable() {
        JS_ASSERT(!hasTable());
        Shape *shape = this;
        uint32_t count = 0;
        for (Shape::Range<NoGC> r(shape); !r.empty(); r.popFront()) {
            ++count;
            if (count >= ShapeTable::MIN_ENTRIES)
                return true;
        }
        return false;
    }

#ifdef DEBUG
    void dump(JSContext *cx, FILE *fp) const;
    void dumpSubtree(JSContext *cx, int level, FILE *fp) const;
#endif

    void sweep();
    void finalize(FreeOp *fop);
    void removeChild(Shape *child);

    static inline ThingRootKind rootKind() { return THING_ROOT_SHAPE; }

    void markChildren(JSTracer *trc) {
        MarkBaseShape(trc, &base_, "base");
        gc::MarkId(trc, &propidRef(), "propid");
        if (parent)
            MarkShape(trc, &parent, "parent");
    }

    inline Shape *search(ExclusiveContext *cx, jsid id);
    inline Shape *searchLinear(jsid id);

#ifdef JSGC_COMPACTING
    void fixupAfterMovingGC();
#endif

    
    static inline size_t offsetOfBase() { return offsetof(Shape, base_); }

  private:
#ifdef JSGC_COMPACTING
    void fixupDictionaryShapeAfterMovingGC();
    void fixupShapeTreeAfterMovingGC();
#endif

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(Shape, base_) == offsetof(js::shadow::Shape, base));
        JS_STATIC_ASSERT(offsetof(Shape, slotInfo) == offsetof(js::shadow::Shape, slotInfo));
        JS_STATIC_ASSERT(FIXED_SLOTS_SHIFT == js::shadow::Shape::FIXED_SLOTS_SHIFT);
        static_assert(js::shadow::Object::MAX_FIXED_SLOTS <= FIXED_SLOTS_MAX,
                      "verify numFixedSlots() bitfield is big enough");
    }
};

inline
StackBaseShape::StackBaseShape(Shape *shape)
  : flags(shape->getObjectFlags()),
    clasp(shape->getObjectClass()),
    parent(shape->getObjectParent()),
    metadata(shape->getObjectMetadata()),
    compartment(shape->compartment())
{
    updateGetterSetter(shape->attrs, shape->getter(), shape->setter());
}

class AutoRooterGetterSetter
{
    class Inner : private JS::CustomAutoRooter
    {
      public:
        inline Inner(ThreadSafeContext *cx, uint8_t attrs,
                     PropertyOp *pgetter_, StrictPropertyOp *psetter_);

      private:
        virtual void trace(JSTracer *trc);

        uint8_t attrs;
        PropertyOp *pgetter;
        StrictPropertyOp *psetter;
    };

  public:
    inline AutoRooterGetterSetter(ThreadSafeContext *cx, uint8_t attrs,
                                  PropertyOp *pgetter, StrictPropertyOp *psetter
                                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  private:
    mozilla::Maybe<Inner> inner;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

struct EmptyShape : public js::Shape
{
    EmptyShape(UnownedBaseShape *base, uint32_t nfixed)
      : js::Shape(base, nfixed)
    {
        
        if (!getObjectClass()->isNative())
            flags |= NON_NATIVE;
    }

    



    static Shape *getInitialShape(ExclusiveContext *cx, const Class *clasp,
                                  TaggedProto proto, JSObject *metadata,
                                  JSObject *parent, size_t nfixed, uint32_t objectFlags = 0);
    static Shape *getInitialShape(ExclusiveContext *cx, const Class *clasp,
                                  TaggedProto proto, JSObject *metadata,
                                  JSObject *parent, gc::AllocKind kind, uint32_t objectFlags = 0);

    




    static void insertInitialShape(ExclusiveContext *cx, HandleShape shape, HandleObject proto);

    








    template<class ObjectSubclass>
    static inline bool
    ensureInitialCustomShape(ExclusiveContext *cx, Handle<ObjectSubclass*> obj);
};





struct InitialShapeEntry
{
    




    ReadBarrieredShape shape;

    



    TaggedProto proto;

    
    struct Lookup {
        const Class *clasp;
        TaggedProto hashProto;
        TaggedProto matchProto;
        JSObject *hashParent;
        JSObject *matchParent;
        JSObject *hashMetadata;
        JSObject *matchMetadata;
        uint32_t nfixed;
        uint32_t baseFlags;
        Lookup(const Class *clasp, TaggedProto proto, JSObject *parent, JSObject *metadata,
               uint32_t nfixed, uint32_t baseFlags)
          : clasp(clasp),
            hashProto(proto), matchProto(proto),
            hashParent(parent), matchParent(parent),
            hashMetadata(metadata), matchMetadata(metadata),
            nfixed(nfixed), baseFlags(baseFlags)
        {}

#ifdef JSGC_GENERATIONAL
        




        Lookup(const Class *clasp, TaggedProto proto,
               JSObject *hashParent, JSObject *matchParent,
               JSObject *hashMetadata, JSObject *matchMetadata,
               uint32_t nfixed, uint32_t baseFlags)
          : clasp(clasp),
            hashProto(proto), matchProto(proto),
            hashParent(hashParent), matchParent(matchParent),
            hashMetadata(hashMetadata), matchMetadata(matchMetadata),
            nfixed(nfixed), baseFlags(baseFlags)
        {}
#endif
    };

    inline InitialShapeEntry();
    inline InitialShapeEntry(const ReadBarrieredShape &shape, TaggedProto proto);

    inline Lookup getLookup() const;

    static inline HashNumber hash(const Lookup &lookup);
    static inline bool match(const InitialShapeEntry &key, const Lookup &lookup);
    static void rekey(InitialShapeEntry &k, const InitialShapeEntry& newKey) { k = newKey; }
};

typedef HashSet<InitialShapeEntry, InitialShapeEntry, SystemAllocPolicy> InitialShapeSet;

struct StackShape
{
    
    UnownedBaseShape *base;
    jsid             propid;
    uint32_t         slot_;
    uint8_t          attrs;
    uint8_t          flags;

    explicit StackShape(UnownedBaseShape *base, jsid propid, uint32_t slot,
                        unsigned attrs, unsigned flags)
      : base(base),
        propid(propid),
        slot_(slot),
        attrs(uint8_t(attrs)),
        flags(uint8_t(flags))
    {
        JS_ASSERT(base);
        JS_ASSERT(!JSID_IS_VOID(propid));
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        JS_ASSERT_IF(attrs & (JSPROP_GETTER | JSPROP_SETTER), attrs & JSPROP_SHARED);
    }

    explicit StackShape(Shape *shape)
      : base(shape->base()->unowned()),
        propid(shape->propidRef()),
        slot_(shape->maybeSlot()),
        attrs(shape->attrs),
        flags(shape->flags)
    {}

    bool hasSlot() const { return (attrs & JSPROP_SHARED) == 0; }
    bool hasMissingSlot() const { return maybeSlot() == SHAPE_INVALID_SLOT; }

    uint32_t slot() const { JS_ASSERT(hasSlot() && !hasMissingSlot()); return slot_; }
    uint32_t maybeSlot() const { return slot_; }

    uint32_t slotSpan() const {
        uint32_t free = JSSLOT_FREE(base->clasp_);
        return hasMissingSlot() ? free : (maybeSlot() + 1);
    }

    void setSlot(uint32_t slot) {
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        slot_ = slot;
    }

    HashNumber hash() const {
        HashNumber hash = uintptr_t(base);

        
        hash = mozilla::RotateLeft(hash, 4) ^ attrs;
        hash = mozilla::RotateLeft(hash, 4) ^ slot_;
        hash = mozilla::RotateLeft(hash, 4) ^ JSID_BITS(propid);
        return hash;
    }

    
    void trace(JSTracer *trc);
};

} 


#define SHAPE_COLLISION                 (uintptr_t(1))
#define SHAPE_REMOVED                   ((js::Shape *) SHAPE_COLLISION)



inline bool
SHAPE_IS_FREE(js::Shape *shape)
{
    return shape == nullptr;
}

inline bool
SHAPE_IS_REMOVED(js::Shape *shape)
{
    return shape == SHAPE_REMOVED;
}

inline bool
SHAPE_IS_LIVE(js::Shape *shape)
{
    return shape > SHAPE_REMOVED;
}

inline void
SHAPE_FLAG_COLLISION(js::Shape **spp, js::Shape *shape)
{
    *spp = reinterpret_cast<js::Shape*>(uintptr_t(shape) | SHAPE_COLLISION);
}

inline bool
SHAPE_HAD_COLLISION(js::Shape *shape)
{
    return uintptr_t(shape) & SHAPE_COLLISION;
}

inline js::Shape *
SHAPE_CLEAR_COLLISION(js::Shape *shape)
{
    return reinterpret_cast<js::Shape*>(uintptr_t(shape) & ~SHAPE_COLLISION);
}

inline js::Shape *
SHAPE_FETCH(js::Shape **spp)
{
    return SHAPE_CLEAR_COLLISION(*spp);
}

inline void
SHAPE_STORE_PRESERVING_COLLISION(js::Shape **spp, js::Shape *shape)
{
    *spp = reinterpret_cast<js::Shape*>(uintptr_t(shape) |
                                        uintptr_t(SHAPE_HAD_COLLISION(*spp)));
}

namespace js {

inline
Shape::Shape(const StackShape &other, uint32_t nfixed)
  : base_(other.base),
    propid_(other.propid),
    slotInfo(other.maybeSlot() | (nfixed << FIXED_SLOTS_SHIFT)),
    attrs(other.attrs),
    flags(other.flags),
    parent(nullptr)
{
    JS_ASSERT_IF(attrs & (JSPROP_GETTER | JSPROP_SETTER), attrs & JSPROP_SHARED);
    kids.setNull();
}

inline
Shape::Shape(UnownedBaseShape *base, uint32_t nfixed)
  : base_(base),
    propid_(JSID_EMPTY),
    slotInfo(SHAPE_INVALID_SLOT | (nfixed << FIXED_SLOTS_SHIFT)),
    attrs(JSPROP_SHARED),
    flags(0),
    parent(nullptr)
{
    JS_ASSERT(base);
    kids.setNull();
}

inline Shape *
Shape::searchLinear(jsid id)
{
    





    JS_ASSERT(!inDictionary());

    for (Shape *shape = this; shape; ) {
        if (shape->propidRef() == id)
            return shape;
        shape = shape->parent;
    }

    return nullptr;
}





inline Shape *
Shape::searchNoHashify(Shape *start, jsid id)
{
    



    if (start->hasTable()) {
        Shape **spp = start->table().search(id, false);
        return SHAPE_FETCH(spp);
    }

    return start->searchLinear(id);
}

inline bool
Shape::matches(const StackShape &other) const
{
    return propid_.get() == other.propid &&
           matchesParamsAfterId(other.base, other.slot_, other.attrs, other.flags);
}

template<> struct RootKind<Shape *> : SpecificRootKind<Shape *, THING_ROOT_SHAPE> {};
template<> struct RootKind<BaseShape *> : SpecificRootKind<BaseShape *, THING_ROOT_BASE_SHAPE> {};







static inline void
MarkNonNativePropertyFound(MutableHandleShape propp)
{
    propp.set(reinterpret_cast<Shape*>(1));
}

template <AllowGC allowGC>
static inline void
MarkDenseOrTypedArrayElementFound(typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    propp.set(reinterpret_cast<Shape*>(1));
}

static inline bool
IsImplicitDenseOrTypedArrayElement(Shape *prop)
{
    return prop == reinterpret_cast<Shape*>(1);
}

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif 

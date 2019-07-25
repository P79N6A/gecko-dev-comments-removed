






#ifndef jsscope_h___
#define jsscope_h___



#include <new>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "jsobj.h"
#include "jspropertytree.h"
#include "jstypes.h"

#include "js/HashTable.h"
#include "gc/Root.h"
#include "mozilla/Attributes.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif


































































namespace js {


static const uint32_t SHAPE_INVALID_SLOT = JS_BIT(24) - 1;
static const uint32_t SHAPE_MAXIMUM_SLOT = JS_BIT(24) - 2;





struct ShapeTable {
    static const uint32_t HASH_BITS     = tl::BitSize<HashNumber>::result;
    static const uint32_t MIN_ENTRIES   = 7;
    static const uint32_t MIN_SIZE_LOG2 = 4;
    static const uint32_t MIN_SIZE      = JS_BIT(MIN_SIZE_LOG2);

    int             hashShift;          

    uint32_t        entryCount;         
    uint32_t        removedCount;       
    uint32_t        freelist;           


    js::Shape       **entries;          

    ShapeTable(uint32_t nentries)
      : hashShift(HASH_BITS - MIN_SIZE_LOG2),
        entryCount(nentries),
        removedCount(0),
        freelist(SHAPE_INVALID_SLOT)
    {
        
    }

    ~ShapeTable() {
        js::UnwantedForeground::free_(entries);
    }

    
    uint32_t capacity() const { return JS_BIT(HASH_BITS - hashShift); }

    
    static size_t sizeOfEntries(size_t cap) { return cap * sizeof(Shape *); }

    



    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + mallocSizeOf(entries);
    }

    
    bool needsToGrow() const {
        uint32_t size = capacity();
        return entryCount + removedCount >= size - (size >> 2);
    }

    




    bool grow(JSContext *cx);

    




    bool            init(JSRuntime *rt, js::Shape *lastProp);
    bool            change(int log2Delta, JSContext *cx);
    js::Shape       **search(jsid id, bool adding);
};

} 

struct JSObject;

namespace js {

class PropertyTree;




#define JSPROP_SHADOWABLE       JSPROP_INDEX

















































class UnownedBaseShape;

class BaseShape : public js::gc::Cell
{
  public:
    friend struct Shape;
    friend struct StackBaseShape;
    friend struct StackShape;

    enum Flag {
        
        OWNED_SHAPE        = 0x1,

        
        HAS_GETTER_OBJECT  = 0x2,
        HAS_SETTER_OBJECT  = 0x4,

        





        DELEGATE           =    0x8,
        NOT_EXTENSIBLE     =   0x10,
        INDEXED            =   0x20,
        BOUND_FUNCTION     =   0x40,
        VAROBJ             =   0x80,
        WATCHED            =  0x100,
        ITERATED_SINGLETON =  0x200,
        NEW_TYPE_UNKNOWN   =  0x400,
        UNCACHEABLE_PROTO  =  0x800,

        OBJECT_FLAG_MASK   = 0x1ff8
    };

  private:
    Class               *clasp;         
    HeapPtrObject       parent;         
    uint32_t            flags;          
    uint32_t            slotSpan_;      


    union {
        js::PropertyOp  rawGetter;      
        JSObject        *getterObj;     

    };

    union {
        js::StrictPropertyOp rawSetter; 
        JSObject        *setterObj;     

    };

    
    HeapPtr<UnownedBaseShape> unowned_;

    
    ShapeTable       *table_;

    BaseShape(const BaseShape &base) MOZ_DELETE;

  public:
    void finalize(FreeOp *fop);

    inline BaseShape(Class *clasp, JSObject *parent, uint32_t objectFlags);
    inline BaseShape(Class *clasp, JSObject *parent, uint32_t objectFlags,
                     uint8_t attrs, PropertyOp rawGetter, StrictPropertyOp rawSetter);
    inline BaseShape(const StackBaseShape &base);

    
    ~BaseShape();

    inline BaseShape &operator=(const BaseShape &other);

    bool isOwned() const { return !!(flags & OWNED_SHAPE); }

    inline bool matchesGetterSetter(PropertyOp rawGetter,
                                    StrictPropertyOp rawSetter) const;

    inline void adoptUnowned(UnownedBaseShape *other);
    inline void setOwned(UnownedBaseShape *unowned);

    JSObject *getObjectParent() const { return parent; }
    uint32_t getObjectFlags() const { return flags & OBJECT_FLAG_MASK; }

    bool hasGetterObject() const { return !!(flags & HAS_GETTER_OBJECT); }
    JSObject *getterObject() const { JS_ASSERT(hasGetterObject()); return getterObj; }

    bool hasSetterObject() const { return !!(flags & HAS_SETTER_OBJECT); }
    JSObject *setterObject() const { JS_ASSERT(hasSetterObject()); return setterObj; }

    bool hasTable() const { JS_ASSERT_IF(table_, isOwned()); return table_ != NULL; }
    ShapeTable &table() const { JS_ASSERT(table_ && isOwned()); return *table_; }
    void setTable(ShapeTable *table) { JS_ASSERT(isOwned()); table_ = table; }

    uint32_t slotSpan() const { JS_ASSERT(isOwned()); return slotSpan_; }
    void setSlotSpan(uint32_t slotSpan) { JS_ASSERT(isOwned()); slotSpan_ = slotSpan; }

    
    static UnownedBaseShape *getUnowned(JSContext *cx, const StackBaseShape &base);

    
    inline UnownedBaseShape *unowned();

    
    inline UnownedBaseShape *baseUnowned();

    
    inline UnownedBaseShape *toUnowned();

    
    inline void assertConsistency();

    
    static inline size_t offsetOfClass() { return offsetof(BaseShape, clasp); }
    static inline size_t offsetOfParent() { return offsetof(BaseShape, parent); }
    static inline size_t offsetOfFlags() { return offsetof(BaseShape, flags); }

    static inline void writeBarrierPre(BaseShape *shape);
    static inline void writeBarrierPost(BaseShape *shape, void *addr);
    static inline void readBarrier(BaseShape *shape);

    static inline ThingRootKind rootKind() { return THING_ROOT_BASE_SHAPE; }

    inline void markChildren(JSTracer *trc);

  private:
    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(BaseShape, clasp) == offsetof(js::shadow::BaseShape, clasp));
    }
};

class UnownedBaseShape : public BaseShape {};

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

UnownedBaseShape *
BaseShape::baseUnowned()
{
    JS_ASSERT(isOwned() && unowned_); return unowned_;
}


struct StackBaseShape
{
    typedef const StackBaseShape *Lookup;

    uint32_t flags;
    Class *clasp;
    JSObject *parent;
    PropertyOp rawGetter;
    StrictPropertyOp rawSetter;

    StackBaseShape(BaseShape *base)
      : flags(base->flags & BaseShape::OBJECT_FLAG_MASK),
        clasp(base->clasp),
        parent(base->parent),
        rawGetter(NULL),
        rawSetter(NULL)
    {}

    StackBaseShape(Class *clasp, JSObject *parent, uint32_t objectFlags)
      : flags(objectFlags),
        clasp(clasp),
        parent(parent),
        rawGetter(NULL),
        rawSetter(NULL)
    {}

    inline StackBaseShape(Shape *shape);

    inline void updateGetterSetter(uint8_t attrs,
                                   PropertyOp rawGetter,
                                   StrictPropertyOp rawSetter);

    static inline HashNumber hash(const StackBaseShape *lookup);
    static inline bool match(UnownedBaseShape *key, const StackBaseShape *lookup);

    class AutoRooter : private AutoGCRooter
    {
      public:
        explicit AutoRooter(JSContext *cx, const StackBaseShape *base_
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
          : AutoGCRooter(cx, STACKBASESHAPE), base(base_), skip(cx, base_)
        {
            JS_GUARD_OBJECT_NOTIFIER_INIT;
        }

        friend void AutoGCRooter::trace(JSTracer *trc);

      private:
        const StackBaseShape *base;
        SkipRoot skip;
        JS_DECL_USE_GUARD_OBJECT_NOTIFIER
    };
};

typedef HashSet<ReadBarriered<UnownedBaseShape>,
                StackBaseShape,
                SystemAllocPolicy> BaseShapeSet;

struct Shape : public js::gc::Cell
{
    friend struct ::JSObject;
    friend struct ::JSFunction;
    friend class js::Bindings;
    friend class js::ObjectImpl;
    friend class js::PropertyTree;
    friend class js::StaticBlockObject;
    friend struct js::StackShape;
    friend struct js::StackBaseShape;

  protected:
    HeapPtrBaseShape    base_;
    HeapId              propid_;

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
    int16_t             shortid_;       

    HeapPtrShape        parent;        
    
    union {
        KidsPointer kids;       

        HeapPtrShape *listp;    



    };

    static inline Shape *search(JSContext *cx, Shape *start, jsid id,
                                Shape ***pspp, bool adding = false);

    static inline Shape *searchNoAllocation(Shape *start, jsid id);

    inline void removeFromDictionary(JSObject *obj);
    inline void insertIntoDictionary(HeapPtrShape *dictp);

    inline void initDictionaryShape(const StackShape &child, uint32_t nfixed,
                                    HeapPtrShape *dictp);

    Shape *getChildBinding(JSContext *cx, const StackShape &child);

    
    static Shape *replaceLastProperty(JSContext *cx, const StackBaseShape &base,
                                      JSObject *proto, Shape *shape);

    bool hashify(JSContext *cx);
    void handoffTableTo(Shape *newShape);

    inline void setParent(js::Shape *p);

    bool ensureOwnBaseShape(JSContext *cx) {
        if (base()->isOwned())
            return true;
        return makeOwnBaseShape(cx);
    }

    bool makeOwnBaseShape(JSContext *cx);

  public:
    bool hasTable() const { return base()->hasTable(); }
    js::ShapeTable &table() const { return base()->table(); }

    void sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf,
                             size_t *propTableSize, size_t *kidsSize) const {
        *propTableSize = hasTable() ? table().sizeOfIncludingThis(mallocSizeOf) : 0;
        *kidsSize = !inDictionary() && kids.isHash()
                  ? kids.toHash()->sizeOfIncludingThis(mallocSizeOf)
                  : 0;
    }

    bool isNative() const {
        JS_ASSERT(!(flags & NON_NATIVE) == getObjectClass()->isNative());
        return !(flags & NON_NATIVE);
    }

    const HeapPtrShape &previous() const {
        return parent;
    }

    class Range {
      protected:
        friend struct Shape;
        Shape *cursor;

      public:
        Range(Shape *shape) : cursor(shape) { }

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

        class AutoRooter : private AutoGCRooter
        {
          public:
            explicit AutoRooter(JSContext *cx, Range *r_
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
              : AutoGCRooter(cx, SHAPERANGE), r(r_), skip(cx, r_)
            {
                JS_GUARD_OBJECT_NOTIFIER_INIT;
            }

            friend void AutoGCRooter::trace(JSTracer *trc);
            void trace(JSTracer *trc);

          private:
            Range *r;
            SkipRoot skip;
            JS_DECL_USE_GUARD_OBJECT_NOTIFIER
        };
    };

    Range all() {
        return Range(this);
    }

    Class *getObjectClass() const { return base()->clasp; }
    JSObject *getObjectParent() const { return base()->parent; }

    static Shape *setObjectParent(JSContext *cx, JSObject *obj, JSObject *proto, Shape *last);
    static Shape *setObjectFlag(JSContext *cx, BaseShape::Flag flag, JSObject *proto, Shape *last);

    uint32_t getObjectFlags() const { return base()->getObjectFlags(); }
    bool hasObjectFlag(BaseShape::Flag flag) const {
        JS_ASSERT(!(flag & ~BaseShape::OBJECT_FLAG_MASK));
        return !!(base()->flags & flag);
    }

  protected:
    




    enum {
        
        NON_NATIVE      = 0x01,

        
        IN_DICTIONARY   = 0x02,

        UNUSED_BITS     = 0x3C
    };

    
    Shape(const StackShape &other, uint32_t nfixed);

    
    Shape(UnownedBaseShape *base, uint32_t nfixed);

    
    Shape(const Shape &other) MOZ_DELETE;

    





    bool hasMissingSlot() const { return maybeSlot() == SHAPE_INVALID_SLOT; }

  public:
    
    enum {
        HAS_SHORTID     = 0x40,
        PUBLIC_FLAGS    = HAS_SHORTID
    };

    bool inDictionary() const   { return (flags & IN_DICTIONARY) != 0; }
    unsigned getFlags() const  { return flags & PUBLIC_FLAGS; }
    bool hasShortID() const { return (flags & HAS_SHORTID) != 0; }

    PropertyOp getter() const { return base()->rawGetter; }
    bool hasDefaultGetter() const  { return !base()->rawGetter; }
    PropertyOp getterOp() const { JS_ASSERT(!hasGetterValue()); return base()->rawGetter; }
    JSObject *getterObject() const { JS_ASSERT(hasGetterValue()); return base()->getterObj; }

    
    Value getterValue() const {
        JS_ASSERT(hasGetterValue());
        return base()->getterObj ? js::ObjectValue(*base()->getterObj) : js::UndefinedValue();
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
        return base()->setterObj ? js::ObjectValue(*base()->setterObj) : js::UndefinedValue();
    }

    Value setterOrUndefined() const {
        return (hasSetterValue() && base()->setterObj)
               ? ObjectValue(*base()->setterObj)
               : UndefinedValue();
    }

    void update(js::PropertyOp getter, js::StrictPropertyOp setter, uint8_t attrs);

    inline bool matches(const Shape *other) const;
    inline bool matches(const StackShape &other) const;
    inline bool matchesParamsAfterId(BaseShape *base,
                                     uint32_t aslot, unsigned aattrs, unsigned aflags,
                                     int ashortid) const;

    bool get(JSContext* cx, HandleObject receiver, JSObject *obj, JSObject *pobj, MutableHandleValue vp);
    bool set(JSContext* cx, HandleObject obj, HandleObject receiver, bool strict, MutableHandleValue vp);

    BaseShape *base() const { return base_; }

    bool hasSlot() const { return (attrs & JSPROP_SHARED) == 0; }
    uint32_t slot() const { JS_ASSERT(hasSlot() && !hasMissingSlot()); return maybeSlot(); }
    uint32_t maybeSlot() const { return slotInfo & SLOT_MASK; }

    bool isEmptyShape() const {
        JS_ASSERT_IF(JSID_IS_EMPTY(propid_), hasMissingSlot());
        return JSID_IS_EMPTY(propid_);
    }

    uint32_t slotSpan() const {
        JS_ASSERT(!inDictionary());
        uint32_t free = JSSLOT_FREE(getObjectClass());
        return hasMissingSlot() ? free : Max(free, maybeSlot() + 1);
    }

    void setSlot(uint32_t slot) {
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        slotInfo = slotInfo & ~Shape::SLOT_MASK;
        slotInfo = slotInfo | slot;
    }

    uint32_t numFixedSlots() const {
        return (slotInfo >> FIXED_SLOTS_SHIFT);
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

    const HeapId &propid() const {
        JS_ASSERT(!isEmptyShape());
        JS_ASSERT(!JSID_IS_VOID(propid_));
        return propid_;
    }
    HeapId &propidRef() { JS_ASSERT(!JSID_IS_VOID(propid_)); return propid_; }

    int16_t shortid() const { JS_ASSERT(hasShortID()); return maybeShortid(); }
    int16_t maybeShortid() const { return shortid_; }

    



    inline bool getUserId(JSContext *cx, jsid *idp) const;

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

        js::Shape *shape = this;
        uint32_t count = 0;
        for (js::Shape::Range r = shape->all(); !r.empty(); r.popFront())
            ++count;
        return count;
    }

    bool isBigEnoughForAShapeTable() {
        JS_ASSERT(!hasTable());
        js::Shape *shape = this;
        uint32_t count = 0;
        for (js::Shape::Range r = shape->all(); !r.empty(); r.popFront()) {
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

    void finalize(FreeOp *fop);
    void removeChild(js::Shape *child);

    static inline void writeBarrierPre(Shape *shape);
    static inline void writeBarrierPost(Shape *shape, void *addr);

    




    static inline void readBarrier(Shape *shape);

    static inline ThingRootKind rootKind() { return THING_ROOT_SHAPE; }

    inline void markChildren(JSTracer *trc);

    inline Shape *search(JSContext *cx, jsid id) {
        Shape **_;
        return search(cx, this, id, &_);
    }

    
    static inline size_t offsetOfBase() { return offsetof(Shape, base_); }

  private:
    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(Shape, base_) == offsetof(js::shadow::Shape, base));
        JS_STATIC_ASSERT(offsetof(Shape, slotInfo) == offsetof(js::shadow::Shape, slotInfo));
        JS_STATIC_ASSERT(FIXED_SLOTS_SHIFT == js::shadow::Shape::FIXED_SLOTS_SHIFT);
    }
};

class AutoRooterGetterSetter
{
    class Inner : private AutoGCRooter
    {
      public:
        Inner(JSContext *cx, uint8_t attrs,
              PropertyOp *pgetter_, StrictPropertyOp *psetter_)
            : AutoGCRooter(cx, GETTERSETTER), attrs(attrs),
              pgetter(pgetter_), psetter(psetter_),
              getterRoot(cx, pgetter_), setterRoot(cx, psetter_)
        {
            JS_ASSERT_IF(attrs & JSPROP_GETTER, !IsPoisonedPtr(*pgetter));
            JS_ASSERT_IF(attrs & JSPROP_SETTER, !IsPoisonedPtr(*psetter));
        }

        friend void AutoGCRooter::trace(JSTracer *trc);

      private:
        uint8_t attrs;
        PropertyOp *pgetter;
        StrictPropertyOp *psetter;
        SkipRoot getterRoot, setterRoot;
    };

  public:
    explicit AutoRooterGetterSetter(JSContext *cx, uint8_t attrs,
                                    PropertyOp *pgetter, StrictPropertyOp *psetter
                                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
            inner.construct(cx, attrs, pgetter, psetter);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Maybe<Inner> inner;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

struct EmptyShape : public js::Shape
{
    EmptyShape(UnownedBaseShape *base, uint32_t nfixed);

    



    static Shape *getInitialShape(JSContext *cx, Class *clasp, JSObject *proto,
                                  JSObject *parent, gc::AllocKind kind, uint32_t objectFlags = 0);

    




    static void insertInitialShape(JSContext *cx, Shape *shape, JSObject *proto);
};





struct InitialShapeEntry
{
    




    ReadBarriered<Shape> shape;

    



    JSObject *proto;

    
    struct Lookup {
        Class *clasp;
        JSObject *proto;
        JSObject *parent;
        uint32_t nfixed;
        uint32_t baseFlags;
        Lookup(Class *clasp, JSObject *proto, JSObject *parent, uint32_t nfixed,
               uint32_t baseFlags)
            : clasp(clasp), proto(proto), parent(parent),
              nfixed(nfixed), baseFlags(baseFlags)
        {}
    };

    inline InitialShapeEntry();
    inline InitialShapeEntry(const ReadBarriered<Shape> &shape, JSObject *proto);

    inline Lookup getLookup();

    static inline HashNumber hash(const Lookup &lookup);
    static inline bool match(const InitialShapeEntry &key, const Lookup &lookup);
};

typedef HashSet<InitialShapeEntry, InitialShapeEntry, SystemAllocPolicy> InitialShapeSet;

struct StackShape
{
    UnownedBaseShape *base;
    jsid             propid;
    uint32_t         slot_;
    uint8_t          attrs;
    uint8_t          flags;
    int16_t          shortid;

    StackShape(UnownedBaseShape *base, jsid propid, uint32_t slot,
               uint32_t nfixed, unsigned attrs, unsigned flags, int shortid)
      : base(base),
        propid(propid),
        slot_(slot),
        attrs(uint8_t(attrs)),
        flags(uint8_t(flags)),
        shortid(int16_t(shortid))
    {
        JS_ASSERT(base);
        JS_ASSERT(!JSID_IS_VOID(propid));
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
    }

    StackShape(const Shape *shape)
      : base(shape->base()->unowned()),
        propid(const_cast<Shape *>(shape)->propidRef()),
        slot_(shape->slotInfo & Shape::SLOT_MASK),
        attrs(shape->attrs),
        flags(shape->flags),
        shortid(shape->shortid_)
    {}

    bool hasSlot() const { return (attrs & JSPROP_SHARED) == 0; }
    bool hasMissingSlot() const { return maybeSlot() == SHAPE_INVALID_SLOT; }

    uint32_t slot() const { JS_ASSERT(hasSlot() && !hasMissingSlot()); return slot_; }
    uint32_t maybeSlot() const { return slot_; }

    uint32_t slotSpan() const {
        uint32_t free = JSSLOT_FREE(base->clasp);
        return hasMissingSlot() ? free : (maybeSlot() + 1);
    }

    void setSlot(uint32_t slot) {
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        slot_ = slot;
    }

    inline HashNumber hash() const;

    class AutoRooter : private AutoGCRooter
    {
      public:
        explicit AutoRooter(JSContext *cx, const StackShape *shape_
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
          : AutoGCRooter(cx, STACKSHAPE), shape(shape_), skip(cx, shape_)
        {
            JS_GUARD_OBJECT_NOTIFIER_INIT;
        }

        friend void AutoGCRooter::trace(JSTracer *trc);

      private:
        const StackShape *shape;
        SkipRoot skip;
        JS_DECL_USE_GUARD_OBJECT_NOTIFIER
    };
 };

} 


#define SHAPE_COLLISION                 (uintptr_t(1))
#define SHAPE_REMOVED                   ((js::Shape *) SHAPE_COLLISION)


#define SHAPE_IS_FREE(shape)            ((shape) == NULL)
#define SHAPE_IS_REMOVED(shape)         ((shape) == SHAPE_REMOVED)
#define SHAPE_IS_LIVE(shape)            ((shape) > SHAPE_REMOVED)
#define SHAPE_FLAG_COLLISION(spp,shape) (*(spp) = (js::Shape *)               \
                                         (uintptr_t(shape) | SHAPE_COLLISION))
#define SHAPE_HAD_COLLISION(shape)      (uintptr_t(shape) & SHAPE_COLLISION)
#define SHAPE_FETCH(spp)                SHAPE_CLEAR_COLLISION(*(spp))

#define SHAPE_CLEAR_COLLISION(shape)                                          \
    ((js::Shape *) (uintptr_t(shape) & ~SHAPE_COLLISION))

#define SHAPE_STORE_PRESERVING_COLLISION(spp, shape)                          \
    (*(spp) = (js::Shape *) (uintptr_t(shape) | SHAPE_HAD_COLLISION(*(spp))))

namespace js {

inline Shape *
Shape::search(JSContext *cx, Shape *start, jsid id, Shape ***pspp, bool adding)
{
#ifdef DEBUG
    {
        SkipRoot skip0(cx, &start);
        SkipRoot skip1(cx, &id);
        MaybeCheckStackRoots(cx);
    }
#endif

    if (start->inDictionary()) {
        *pspp = start->table().search(id, adding);
        return SHAPE_FETCH(*pspp);
    }

    *pspp = NULL;

    if (start->hasTable()) {
        Shape **spp = start->table().search(id, adding);
        return SHAPE_FETCH(spp);
    }

    if (start->numLinearSearches() == LINEAR_SEARCHES_MAX) {
        if (start->isBigEnoughForAShapeTable()) {
            RootedShape startRoot(cx, start);
            RootedId idRoot(cx, id);
            if (startRoot->hashify(cx)) {
                Shape **spp = startRoot->table().search(idRoot, adding);
                return SHAPE_FETCH(spp);
            }
            start = startRoot;
            id = idRoot;
        }
        



        JS_ASSERT(!start->hasTable());
    } else {
        start->incrementNumLinearSearches();
    }

    for (Shape *shape = start; shape; shape = shape->parent) {
        if (shape->propidRef() == id)
            return shape;
    }

    return NULL;
}

 inline Shape *
Shape::searchNoAllocation(Shape *start, jsid id)
{
    if (start->hasTable()) {
        Shape **spp = start->table().search(id, false);
        return SHAPE_FETCH(spp);
    }

    for (Shape *shape = start; shape; shape = shape->parent) {
        if (shape->propidRef() == id)
            return shape;
    }

    return NULL;
}

void
MarkNonNativePropertyFound(HandleObject obj, MutableHandleShape propp);

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

namespace JS {
    template<> class AnchorPermitted<js::Shape *> { };
    template<> class AnchorPermitted<const js::Shape *> { };

    template<> struct RootKind<js::Shape *> { static ThingRootKind rootKind() { return THING_ROOT_SHAPE; }; };
    template<> struct RootKind<js::BaseShape *> { static ThingRootKind rootKind() { return THING_ROOT_BASE_SHAPE; }; };
}

#endif

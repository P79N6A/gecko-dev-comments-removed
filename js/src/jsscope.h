







































#ifndef jsscope_h___
#define jsscope_h___



#include <new>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "jstypes.h"

#include "jscntxt.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jspropertytree.h"

#include "js/HashTable.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif











































































































































namespace js {


static const uint32 SHAPE_INVALID_SLOT = JS_BIT(24) - 1;
static const uint32 SHAPE_MAXIMUM_SLOT = JS_BIT(24) - 2;





struct PropertyTable {
    static const uint32 MIN_ENTRIES         = 7;
    static const uint32 MIN_SIZE_LOG2       = 4;
    static const uint32 MIN_SIZE            = JS_BIT(MIN_SIZE_LOG2);

    int             hashShift;          

    uint32          entryCount;         
    uint32          removedCount;       
    uint32          freelist;           


    js::Shape       **entries;          

    PropertyTable(uint32 nentries)
      : hashShift(JS_DHASH_BITS - MIN_SIZE_LOG2),
        entryCount(nentries),
        removedCount(0),
        freelist(SHAPE_INVALID_SLOT)
    {
        
    }

    ~PropertyTable() {
        js::UnwantedForeground::free_(entries);
    }

    
    uint32 capacity() const { return JS_BIT(JS_DHASH_BITS - hashShift); }

    
    static size_t sizeOfEntries(size_t cap) { return cap * sizeof(Shape *); }

    



    size_t sizeOf(JSUsableSizeFun usf) const {
        size_t usable = usf((void*)this) + usf(entries);
        return usable ? usable : sizeOfEntries(capacity()) + sizeof(PropertyTable);
    }

    
    bool needsToGrow() const {
        uint32 size = capacity();
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
    friend struct BaseShapeEntry;

    enum Flag {
        
        OWNED_SHAPE        = 0x1,

        
        HAS_GETTER_OBJECT  = 0x2,
        HAS_SETTER_OBJECT  = 0x4,

        




        EXTENSIBLE_PARENTS = 0x8,

        






        DELEGATE           = 0x010,
        SYSTEM             = 0x020,
        NOT_EXTENSIBLE     = 0x040,
        INDEXED            = 0x080,
        BOUND_FUNCTION     = 0x100,
        VAROBJ             = 0x200,
        WATCHED            = 0x400,
        ITERATED_SINGLETON = 0x800,
        NEW_TYPE_UNKNOWN   = 0x1000,

        OBJECT_FLAG_MASK  = 0x1ff0
    };

  private:
    Class               *clasp;         
    JSObject            *parent;        
    uint32              flags;          
    uint32              slotSpan_;      


    union {
        js::PropertyOp  rawGetter;      
        JSObject        *getterObj;     

    };

    union {
        js::StrictPropertyOp rawSetter; 
        JSObject        *setterObj;     

    };

    
    UnownedBaseShape    *unowned_;

    
    PropertyTable       *table_;

  public:
    void finalize(JSContext *cx, bool background);

    BaseShape(Class *clasp, JSObject *parent, uint32 objectFlags) {
        JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
        PodZero(this);
        this->clasp = clasp;
        this->parent = parent;
        this->flags = objectFlags;
    }

    BaseShape(Class *clasp, JSObject *parent, uint32 objectFlags,
              uint8 attrs, js::PropertyOp rawGetter, js::StrictPropertyOp rawSetter) {
        JS_ASSERT(!(objectFlags & ~OBJECT_FLAG_MASK));
        PodZero(this);
        this->clasp = clasp;
        this->parent = parent;
        this->flags = objectFlags;
        this->rawGetter = rawGetter;
        this->rawSetter = rawSetter;
        if ((attrs & JSPROP_GETTER) && rawGetter)
            flags |= HAS_GETTER_OBJECT;
        if ((attrs & JSPROP_SETTER) && rawSetter)
            flags |= HAS_SETTER_OBJECT;
    }

    inline void adoptUnowned(UnownedBaseShape *other);

    bool isOwned() const { return !!(flags & OWNED_SHAPE); }
    void setOwned(UnownedBaseShape *unowned) { flags |= OWNED_SHAPE; this->unowned_ = unowned; }

    void setParent(JSObject *obj) { parent = obj; }

    void setObjectFlag(Flag flag) { JS_ASSERT(!(flag & ~OBJECT_FLAG_MASK)); flags |= flag; }

    bool hasGetterObject() const { return !!(flags & HAS_GETTER_OBJECT); }
    JSObject *getterObject() const { JS_ASSERT(hasGetterObject()); return getterObj; }

    bool hasSetterObject() const { return !!(flags & HAS_SETTER_OBJECT); }
    JSObject *setterObject() const { JS_ASSERT(hasSetterObject()); return setterObj; }

    bool hasTable() const { JS_ASSERT_IF(table_, isOwned()); return table_ != NULL; }
    PropertyTable &table() const { JS_ASSERT(table_ && isOwned()); return *table_; }
    void setTable(PropertyTable *table) { JS_ASSERT(isOwned()); table_ = table; }

    uint32 slotSpan() const { JS_ASSERT(isOwned()); return slotSpan_; }
    void setSlotSpan(uint32 slotSpan) { JS_ASSERT(isOwned()); slotSpan_ = slotSpan; }

    
    static UnownedBaseShape *lookup(JSContext *cx, const BaseShape &base);

    
    inline UnownedBaseShape *unowned();

    
    inline UnownedBaseShape *baseUnowned();

    
    inline UnownedBaseShape *toUnowned();

    
    static inline size_t offsetOfClass() { return offsetof(BaseShape, clasp); }
    static inline size_t offsetOfParent() { return offsetof(BaseShape, parent); }
    static inline size_t offsetOfFlags() { return offsetof(BaseShape, flags); }

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


struct BaseShapeEntry
{
    typedef const BaseShape *Lookup;

    static inline HashNumber hash(const BaseShape *base);
    static inline bool match(UnownedBaseShape *key, const BaseShape *lookup);
};
typedef HashSet<UnownedBaseShape *, BaseShapeEntry, SystemAllocPolicy> BaseShapeSet;

struct Shape : public js::gc::Cell
{
    friend struct ::JSObject;
    friend struct ::JSFunction;
    friend class js::PropertyTree;
    friend class js::Bindings;
    friend bool IsShapeAboutToBeFinalized(JSContext *cx, const js::Shape *shape);

  protected:
    BaseShape           *base_;
    jsid                propid_;

    enum {
        
        FIXED_SLOTS_MAX        = 0x1f,
        FIXED_SLOTS_SHIFT      = 27,
        FIXED_SLOTS_MASK       = FIXED_SLOTS_MAX << FIXED_SLOTS_SHIFT,

        





        LINEAR_SEARCHES_MAX    = 0x7,
        LINEAR_SEARCHES_SHIFT  = 24,
        LINEAR_SEARCHES_MASK   = LINEAR_SEARCHES_MAX << LINEAR_SEARCHES_SHIFT,

        





        SLOT_MASK              = JS_BIT(24) - 1
    };

    uint32              slotInfo;       
    uint8               attrs;          
    uint8               flags;          
    int16               shortid_;       

    js::Shape   *parent;        
    
    union {
        js::KidsPointer kids;   

        js::Shape **listp;      



    };

    static inline js::Shape **search(JSContext *cx, js::Shape **pstart, jsid id,
                                     bool adding = false);
    static js::Shape *newDictionaryList(JSContext *cx, js::Shape **listp);

    inline void removeFromDictionary(JSObject *obj);
    inline void insertIntoDictionary(js::Shape **dictp);

    inline void initDictionaryShape(const js::Shape &child, js::Shape **dictp);

    js::Shape *getChildBinding(JSContext *cx, const js::Shape &child, js::Shape **lastBinding);

    
    static bool replaceLastProperty(JSContext *cx, const BaseShape &base, Shape **lastp);

    bool hashify(JSContext *cx);
    void handoffTableTo(Shape *newShape);

    void setParent(js::Shape *p) {
        JS_ASSERT_IF(p && !p->hasMissingSlot() && !inDictionary(),
                     p->maybeSlot() <= maybeSlot());
        JS_ASSERT_IF(p && !inDictionary(),
                     hasSlot() == (p->maybeSlot() != maybeSlot()));
        parent = p;
    }

    bool ensureOwnBaseShape(JSContext *cx) {
        if (base()->isOwned())
            return true;
        return makeOwnBaseShape(cx);
    }

    bool makeOwnBaseShape(JSContext *cx);

  public:
    bool hasTable() const { return base()->hasTable(); }
    js::PropertyTable &table() const { return base()->table(); }

    size_t sizeOfPropertyTable(JSUsableSizeFun usf) const {
        return hasTable() ? table().sizeOf(usf) : 0;
    }

    size_t sizeOfKids(JSUsableSizeFun usf) const {
        
        JS_ASSERT(!inDictionary());
        return kids.isHash()
             ? kids.toHash()->sizeOf(usf, true)
             : 0;
    }

    bool isNative() const {
        JS_ASSERT(!(flags & NON_NATIVE) == getObjectClass()->isNative());
        return !(flags & NON_NATIVE);
    }

    const js::Shape *previous() const {
        return parent;
    }

    class Range {
      protected:
        friend struct Shape;

        const Shape *cursor;
        const Shape *end;

      public:
        Range(const Shape *shape) : cursor(shape) { }

        bool empty() const {
            return cursor->isEmptyShape();
        }

        const Shape &front() const {
            JS_ASSERT(!empty());
            return *cursor;
        }

        void popFront() {
            JS_ASSERT(!empty());
            cursor = cursor->parent;
        }
    };

    Range all() const {
        return Range(this);
    }

    Class *getObjectClass() const { return base()->clasp; }
    JSObject *getObjectParent() const { return base()->parent; }

    static bool setObjectParent(JSContext *cx, JSObject *obj, Shape **listp);
    static bool setObjectFlag(JSContext *cx, BaseShape::Flag flag, Shape **listp);

    uint32 getObjectFlags() const { return base()->flags & BaseShape::OBJECT_FLAG_MASK; }
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

    Shape(BaseShape *base, jsid id, uint32 slot, uint32 nfixed, uintN attrs, uintN flags, intN shortid);

    
    Shape(const Shape *other);

    
    Shape(BaseShape *base, uint32 nfixed);

    
    Shape(const Shape &other);

    





    bool hasMissingSlot() const { return maybeSlot() == SHAPE_INVALID_SLOT; }

  public:
    
    enum {
        HAS_SHORTID     = 0x40,
        METHOD          = 0x80,
        PUBLIC_FLAGS    = HAS_SHORTID | METHOD
    };

    bool inDictionary() const   { return (flags & IN_DICTIONARY) != 0; }
    uintN getFlags() const  { return flags & PUBLIC_FLAGS; }
    bool hasShortID() const { return (flags & HAS_SHORTID) != 0; }

    




















    bool isMethod() const {
        JS_ASSERT_IF(flags & METHOD, !base()->rawGetter);
        return (flags & METHOD) != 0;
    }

    PropertyOp getter() const { return base()->rawGetter; }
    bool hasDefaultGetterOrIsMethod() const { return !base()->rawGetter; }
    bool hasDefaultGetter() const  { return !base()->rawGetter && !isMethod(); }
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

    inline JSDHashNumber hash() const;
    inline bool matches(const js::Shape *p) const;
    inline bool matchesParamsAfterId(BaseShape *base,
                                     uint32 aslot, uintN aattrs, uintN aflags,
                                     intN ashortid) const;

    bool get(JSContext* cx, JSObject *receiver, JSObject *obj, JSObject *pobj, js::Value* vp) const;
    bool set(JSContext* cx, JSObject *obj, bool strict, js::Value* vp) const;

    BaseShape *base() const { return base_; }

    bool hasSlot() const { return (attrs & JSPROP_SHARED) == 0; }
    uint32 slot() const { JS_ASSERT(hasSlot() && !hasMissingSlot()); return maybeSlot(); }
    uint32 maybeSlot() const { return slotInfo & SLOT_MASK; }

    bool isEmptyShape() const {
        JS_ASSERT_IF(JSID_IS_EMPTY(propid_), hasMissingSlot());
        return JSID_IS_EMPTY(propid_);
    }

    uint32 slotSpan() const {
        JS_ASSERT(!inDictionary());
        uint32 free = JSSLOT_FREE(getObjectClass());
        return hasMissingSlot() ? free : Max(free, maybeSlot() + 1);
    }

    void setSlot(uint32 slot) {
        JS_ASSERT(slot <= SHAPE_INVALID_SLOT);
        slotInfo = slotInfo & ~SLOT_MASK;
        slotInfo = slotInfo | slot;
    }

    uint32 numFixedSlots() const {
        return (slotInfo >> FIXED_SLOTS_SHIFT);
    }

    void setNumFixedSlots(uint32 nfixed) {
        JS_ASSERT(nfixed < FIXED_SLOTS_MAX);
        slotInfo = slotInfo & ~FIXED_SLOTS_MASK;
        slotInfo = slotInfo | (nfixed << FIXED_SLOTS_SHIFT);
    }

    uint32 numLinearSearches() const {
        return (slotInfo & LINEAR_SEARCHES_MASK) >> LINEAR_SEARCHES_SHIFT;
    }

    void incrementNumLinearSearches() {
        uint32 count = numLinearSearches();
        JS_ASSERT(count < LINEAR_SEARCHES_MAX);
        slotInfo = slotInfo & ~LINEAR_SEARCHES_MASK;
        slotInfo = slotInfo | ((count + 1) << LINEAR_SEARCHES_SHIFT);
    }

    jsid propid() const { JS_ASSERT(!isEmptyShape()); return maybePropid(); }
    jsid maybePropid() const { JS_ASSERT(!JSID_IS_VOID(propid_)); return propid_; }

    int16 shortid() const { JS_ASSERT(hasShortID()); return maybeShortid(); }
    int16 maybeShortid() const { return shortid_; }

    



    jsid getUserId() const {
        return hasShortID() ? INT_TO_JSID(shortid()) : propid();
    }

    uint8 attributes() const { return attrs; }
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

    





    bool shadowable() const {
        JS_ASSERT_IF(isDataDescriptor(), writable());
        return hasSlot() || (attrs & JSPROP_SHADOWABLE);
    }

    










































    static bool setExtensibleParents(JSContext *cx, Shape **listp);
    bool extensibleParents() const { return !!(base()->flags & BaseShape::EXTENSIBLE_PARENTS); }

    uint32 entryCount() const {
        if (hasTable())
            return table().entryCount;

        const js::Shape *shape = this;
        uint32 count = 0;
        for (js::Shape::Range r = shape->all(); !r.empty(); r.popFront())
            ++count;
        return count;
    }

    bool isBigEnoughForAPropertyTable() const {
        JS_ASSERT(!hasTable());
        const js::Shape *shape = this;
        uint32 count = 0;
        for (js::Shape::Range r = shape->all(); !r.empty(); r.popFront()) {
            ++count;
            if (count >= PropertyTable::MIN_ENTRIES)
                return true;
        }
        return false;
    }

#ifdef DEBUG
    void dump(JSContext *cx, FILE *fp) const;
    void dumpSubtree(JSContext *cx, int level, FILE *fp) const;
#endif

    void finalize(JSContext *cx, bool background);
    void removeChild(js::Shape *child);

    
    static inline size_t offsetOfBase() { return offsetof(Shape, base_); }

  private:
    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(Shape, base_) == offsetof(js::shadow::Shape, base));
        JS_STATIC_ASSERT(offsetof(Shape, slotInfo) == offsetof(js::shadow::Shape, slotInfo));
        JS_STATIC_ASSERT(FIXED_SLOTS_SHIFT == js::shadow::Shape::FIXED_SLOTS_SHIFT);
    }
};

struct EmptyShape : public js::Shape
{
    EmptyShape(BaseShape *base, uint32 nfixed);

    



    static Shape *lookupInitialShape(JSContext *cx, Class *clasp, JSObject *proto,
                                     JSObject *parent, gc::AllocKind kind, uint32 objectFlags = 0);

    
    static void insertInitialShape(JSContext *cx, Shape *shape, JSObject *proto);
};





struct InitialShapeEntry
{
    




    js::Shape *shape;

    



    JSObject *proto;

    
    struct Lookup {
        Class *clasp;
        JSObject *proto;
        JSObject *parent;
        size_t nfixed;
        Lookup(Class *clasp, JSObject *proto, JSObject *parent, size_t nfixed)
            : clasp(clasp), proto(proto), parent(parent), nfixed(nfixed)
        {}
    };

    static inline HashNumber hash(const Lookup &lookup);
    static inline bool match(const InitialShapeEntry &key, const Lookup &lookup);
};
typedef HashSet<InitialShapeEntry, InitialShapeEntry, SystemAllocPolicy> InitialShapeSet;

} 


#define SHAPE_COLLISION                 (jsuword(1))
#define SHAPE_REMOVED                   ((js::Shape *) SHAPE_COLLISION)


#define SHAPE_IS_FREE(shape)            ((shape) == NULL)
#define SHAPE_IS_REMOVED(shape)         ((shape) == SHAPE_REMOVED)
#define SHAPE_IS_LIVE(shape)            ((shape) > SHAPE_REMOVED)
#define SHAPE_FLAG_COLLISION(spp,shape) (*(spp) = (js::Shape *)               \
                                         (jsuword(shape) | SHAPE_COLLISION))
#define SHAPE_HAD_COLLISION(shape)      (jsuword(shape) & SHAPE_COLLISION)
#define SHAPE_FETCH(spp)                SHAPE_CLEAR_COLLISION(*(spp))

#define SHAPE_CLEAR_COLLISION(shape)                                          \
    ((js::Shape *) (jsuword(shape) & ~SHAPE_COLLISION))

#define SHAPE_STORE_PRESERVING_COLLISION(spp, shape)                          \
    (*(spp) = (js::Shape *) (jsuword(shape) | SHAPE_HAD_COLLISION(*(spp))))

namespace js {












JS_ALWAYS_INLINE js::Shape **
Shape::search(JSContext *cx, js::Shape **pstart, jsid id, bool adding)
{
    Shape *start = *pstart;
    if (start->hasTable())
        return start->table().search(id, adding);

    if (start->numLinearSearches() == LINEAR_SEARCHES_MAX) {
        if (start->isBigEnoughForAPropertyTable() && start->hashify(cx))
            return start->table().search(id, adding);
        



        JS_ASSERT(!start->hasTable());
    } else {
        start->incrementNumLinearSearches();
    }

    







    js::Shape **spp;
    for (spp = pstart; js::Shape *shape = *spp; spp = &shape->parent) {
        if (shape->maybePropid() == id)
            return spp;
    }
    return spp;
}

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

inline js::Class *
JSObject::getClass() const
{
    return lastProperty()->getObjectClass();
}

inline JSClass *
JSObject::getJSClass() const
{
    return Jsvalify(getClass());
}

inline bool
JSObject::hasClass(const js::Class *c) const
{
    return getClass() == c;
}

inline const js::ObjectOps *
JSObject::getOps() const
{
    return &getClass()->ops;
}

namespace JS {
    template<> class AnchorPermitted<js::Shape *> { };
    template<> class AnchorPermitted<const js::Shape *> { };
}

#endif

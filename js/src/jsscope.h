







































#ifndef jsscope_h___
#define jsscope_h___



#include <new>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "jstypes.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jshashtable.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jspropertytree.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif
















































































































































#define SHAPE_INVALID_SLOT              0xffffffff

namespace js {







struct PropertyTable {
    static const uint32 MAX_LINEAR_SEARCHES = 7;
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

static inline PropertyOp
CastAsPropertyOp(js::Class *clasp)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, clasp);
}




#define JSPROP_SHADOWABLE       JSPROP_INDEX

struct Shape : public js::gc::Cell
{
    friend struct ::JSObject;
    friend struct ::JSFunction;
    friend class js::PropertyTree;
    friend class js::Bindings;
    friend bool IsShapeAboutToBeFinalized(JSContext *cx, const js::Shape *shape);

    mutable uint32      shapeid;        
    uint32              slotSpan;       

    







    union {
        mutable size_t numLinearSearches;
        mutable js::PropertyTable *table;
    };

    inline void freeTable(JSContext *cx);

    jsid                propid;

  protected:
    union {
        js::PropertyOp  rawGetter;      
        JSObject        *getterObj;     



        js::Class       *clasp;         
    };

    union {
        js::StrictPropertyOp  rawSetter;

        JSObject        *setterObj;     

    };

  public:
    uint32              slot;           
  private:
    uint8               attrs;          
    mutable uint8       flags;          
  public:
    int16               shortid;        

  protected:
    mutable js::Shape   *parent;        
    union {
        mutable js::KidsPointer kids;   

        mutable js::Shape **listp;      



    };

    static inline js::Shape **search(JSRuntime *rt, js::Shape **startp, jsid id,
                                     bool adding = false);
    static js::Shape *newDictionaryShape(JSContext *cx, const js::Shape &child, js::Shape **listp);
    static js::Shape *newDictionaryList(JSContext *cx, js::Shape **listp);

    inline void removeFromDictionary(JSObject *obj) const;
    inline void insertIntoDictionary(js::Shape **dictp);

    js::Shape *getChild(JSContext *cx, const js::Shape &child, js::Shape **listp);

    bool hashify(JSRuntime *rt);

    bool hasTable() const {
        
        return numLinearSearches > PropertyTable::MAX_LINEAR_SEARCHES;
    }

    js::PropertyTable *getTable() const {
        JS_ASSERT(hasTable());
        return table;
    }

    void setTable(js::PropertyTable *t) const {
        JS_ASSERT_IF(t && t->freelist != SHAPE_INVALID_SLOT, t->freelist < slotSpan);
        table = t;
    }

    








































    void setParent(js::Shape *p) {
        JS_STATIC_ASSERT(uint32(SHAPE_INVALID_SLOT) == ~uint32(0));
        if (p)
            slotSpan = JS_MAX(p->slotSpan, slot + 1);
        JS_ASSERT(slotSpan < JSObject::NSLOTS_LIMIT);
        parent = p;
    }

  public:
    static JS_FRIEND_DATA(Shape) sharedNonNative;

    bool isNative() const { return this != &sharedNonNative; }

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
            JS_ASSERT_IF(!cursor->parent, JSID_IS_EMPTY(cursor->propid));
            return !cursor->parent;
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

  protected:
    




    enum {
        SHARED_EMPTY    = 0x01,

        
        IN_DICTIONARY   = 0x02,

        
        FROZEN          = 0x04
    };

    Shape(jsid id, js::PropertyOp getter, js::StrictPropertyOp setter, uint32 slot, uintN attrs,
          uintN flags, intN shortid, uint32 shape = INVALID_SHAPE, uint32 slotSpan = 0);

    
    Shape(JSCompartment *comp, Class *aclasp);

    
    explicit Shape(uint32 shape);

    bool inDictionary() const   { return (flags & IN_DICTIONARY) != 0; }
    bool frozen() const         { return (flags & FROZEN) != 0; }
    void setFrozen()            { flags |= FROZEN; }

    bool isEmptyShape() const   { JS_ASSERT_IF(!parent, JSID_IS_EMPTY(propid)); return !parent; }

  public:
    
    enum {
        ALIAS           = 0x20,
        HAS_SHORTID     = 0x40,
        METHOD          = 0x80,
        PUBLIC_FLAGS    = ALIAS | HAS_SHORTID | METHOD
    };

    uintN getFlags() const  { return flags & PUBLIC_FLAGS; }
    bool isAlias() const    { return (flags & ALIAS) != 0; }
    bool hasShortID() const { return (flags & HAS_SHORTID) != 0; }
    bool isMethod() const   { return (flags & METHOD) != 0; }

    JSObject &methodObject() const { JS_ASSERT(isMethod()); return *getterObj; }

    js::PropertyOp getter() const { return rawGetter; }
    bool hasDefaultGetter() const  { return !rawGetter; }
    js::PropertyOp getterOp() const { JS_ASSERT(!hasGetterValue()); return rawGetter; }
    JSObject *getterObject() const { JS_ASSERT(hasGetterValue()); return getterObj; }

    
    js::Value getterValue() const {
        JS_ASSERT(hasGetterValue());
        return getterObj ? js::ObjectValue(*getterObj) : js::UndefinedValue();
    }

    js::Value getterOrUndefined() const {
        return hasGetterValue() && getterObj ? js::ObjectValue(*getterObj) : js::UndefinedValue();
    }

    js::StrictPropertyOp setter() const { return rawSetter; }
    bool hasDefaultSetter() const  { return !rawSetter; }
    js::StrictPropertyOp setterOp() const { JS_ASSERT(!hasSetterValue()); return rawSetter; }
    JSObject *setterObject() const { JS_ASSERT(hasSetterValue()); return setterObj; }

    
    js::Value setterValue() const {
        JS_ASSERT(hasSetterValue());
        return setterObj ? js::ObjectValue(*setterObj) : js::UndefinedValue();
    }

    js::Value setterOrUndefined() const {
        return hasSetterValue() && setterObj ? js::ObjectValue(*setterObj) : js::UndefinedValue();
    }

    inline JSDHashNumber hash() const;
    inline bool matches(const js::Shape *p) const;
    inline bool matchesParamsAfterId(js::PropertyOp agetter, js::StrictPropertyOp asetter,
                                     uint32 aslot, uintN aattrs, uintN aflags,
                                     intN ashortid) const;

    bool get(JSContext* cx, JSObject *receiver, JSObject *obj, JSObject *pobj, js::Value* vp) const;
    bool set(JSContext* cx, JSObject *obj, bool strict, js::Value* vp) const;

    bool hasSlot() const { return (attrs & JSPROP_SHARED) == 0; }

    uint8 attributes() const { return attrs; }
    bool configurable() const { return (attrs & JSPROP_PERMANENT) == 0; }
    bool enumerable() const { return (attrs & JSPROP_ENUMERATE) != 0; }
    bool writable() const {
        
        return (attrs & JSPROP_READONLY) == 0;
    }
    bool hasGetterValue() const { return attrs & JSPROP_GETTER; }
    bool hasSetterValue() const { return attrs & JSPROP_SETTER; }

    bool hasDefaultGetterOrIsMethod() const {
        return hasDefaultGetter() || isMethod();
    }

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

    uint32 entryCount() const {
        if (hasTable())
            return getTable()->entryCount;

        const js::Shape *shape = this;
        uint32 count = 0;
        for (js::Shape::Range r = shape->all(); !r.empty(); r.popFront())
            ++count;
        return count;
    }

#ifdef DEBUG
    void dump(JSContext *cx, FILE *fp) const;
    void dumpSubtree(JSContext *cx, int level, FILE *fp) const;
#endif

    void finalize(JSContext *cx);
    void removeChild(js::Shape *child);
    void removeChildSlowly(js::Shape *child);
};

struct EmptyShape : public js::Shape
{
    EmptyShape(JSCompartment *comp, js::Class *aclasp);

    js::Class *getClass() const { return clasp; };

    static EmptyShape *create(JSContext *cx, js::Class *clasp) {
        js::Shape *eprop = JS_PROPERTY_TREE(cx).newShape(cx);
        if (!eprop)
            return NULL;
        return new (eprop) EmptyShape(cx->compartment, clasp);
    }

    static EmptyShape *ensure(JSContext *cx, js::Class *clasp, EmptyShape **shapep) {
        EmptyShape *shape = *shapep;
        if (!shape) {
            if (!(shape = create(cx, clasp)))
                return NULL;
            return *shapep = shape;
        }
        return shape;
    }

    static inline EmptyShape *getEmptyArgumentsShape(JSContext *cx);

    static inline EmptyShape *getEmptyBlockShape(JSContext *cx);
    static inline EmptyShape *getEmptyCallShape(JSContext *cx);
    static inline EmptyShape *getEmptyDeclEnvShape(JSContext *cx);
    static inline EmptyShape *getEmptyEnumeratorShape(JSContext *cx);
    static inline EmptyShape *getEmptyWithShape(JSContext *cx);
};

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





#define SHAPE_USERID(shape)                                                   \
    ((shape)->hasShortID() ? INT_TO_JSID((shape)->shortid)                    \
                           : (shape)->propid)

extern uint32
js_GenerateShape(JSRuntime *rt);

extern uint32
js_GenerateShape(JSContext *cx);

#ifdef DEBUG
struct JSScopeStats {
    jsrefcount          searches;
    jsrefcount          hits;
    jsrefcount          misses;
    jsrefcount          hashes;
    jsrefcount          hashHits;
    jsrefcount          hashMisses;
    jsrefcount          steps;
    jsrefcount          stepHits;
    jsrefcount          stepMisses;
    jsrefcount          initSearches;
    jsrefcount          changeSearches;
    jsrefcount          tableAllocFails;
    jsrefcount          toDictFails;
    jsrefcount          wrapWatchFails;
    jsrefcount          adds;
    jsrefcount          addFails;
    jsrefcount          puts;
    jsrefcount          redundantPuts;
    jsrefcount          putFails;
    jsrefcount          changes;
    jsrefcount          changePuts;
    jsrefcount          changeFails;
    jsrefcount          compresses;
    jsrefcount          grows;
    jsrefcount          removes;
    jsrefcount          removeFrees;
    jsrefcount          uselessRemoves;
    jsrefcount          shrinks;
};

extern JS_FRIEND_DATA(JSScopeStats) js_scope_stats;

# define METER(x)       JS_ATOMIC_INCREMENT(&js_scope_stats.x)
#else
# define METER(x)
#endif

namespace js {

JS_ALWAYS_INLINE js::Shape **
Shape::search(JSRuntime *rt, js::Shape **startp, jsid id, bool adding)
{
    js::Shape *start = *startp;
    METER(searches);

    if (start->hasTable())
        return start->getTable()->search(id, adding);

    if (start->numLinearSearches == PropertyTable::MAX_LINEAR_SEARCHES) {
        if (start->hashify(rt))
            return start->getTable()->search(id, adding);
        
        JS_ASSERT(!start->hasTable());
    } else {
        JS_ASSERT(start->numLinearSearches < PropertyTable::MAX_LINEAR_SEARCHES);
        start->numLinearSearches++;
    }

    







    js::Shape **spp;
    for (spp = startp; js::Shape *shape = *spp; spp = &shape->parent) {
        if (shape->propid == id) {
            METER(hits);
            return spp;
        }
    }
    METER(misses);
    return spp;
}

#undef METER

} 

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif

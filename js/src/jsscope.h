







































#ifndef jsscope_h___
#define jsscope_h___



#include <new>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "jstypes.h"
#include "jscntxt.h"
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
    enum {
        HASH_THRESHOLD  = 6,
        MIN_SIZE_LOG2   = 4,
        MIN_SIZE        = JS_BIT(MIN_SIZE_LOG2)
    };

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
        js_free(entries);
    }

    
    uint32 capacity() const { return JS_BIT(JS_DHASH_BITS - hashShift); }

    




    bool            init(js::Shape *lastProp, JSContext *cx);
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

struct Shape : public JSObjectMap
{
    friend struct ::JSObject;
    friend struct ::JSFunction;
    friend class js::PropertyTree;
    friend bool HasUnreachableGCThings(TreeFragment *f);

  protected:
    mutable js::PropertyTable *table;

  public:
    inline void freeTable(JSContext *cx);

    static bool initRuntimeState(JSContext *cx);
    static void finishRuntimeState(JSContext *cx);

    enum {
        EMPTY_ARGUMENTS_SHAPE   = 1,
        EMPTY_BLOCK_SHAPE       = 2,
        EMPTY_CALL_SHAPE        = 3,
        EMPTY_DECL_ENV_SHAPE    = 4,
        EMPTY_ENUMERATOR_SHAPE  = 5,
        EMPTY_WITH_SHAPE        = 6,
        LAST_RESERVED_SHAPE     = 6
    };

    jsid                id;

  protected:
    union {
        js::PropertyOp  rawGetter;      
        JSObject        *getterObj;     



        js::Class       *clasp;         
    };

    union {
        js::PropertyOp  rawSetter;      

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

    static inline js::Shape **search(js::Shape **startp, jsid id, bool adding = false);
    static js::Shape *newDictionaryShape(JSContext *cx, const js::Shape &child, js::Shape **listp);
    static js::Shape *newDictionaryList(JSContext *cx, js::Shape **listp);

    inline void removeFromDictionary(JSObject *obj) const;
    inline void insertIntoDictionary(js::Shape **dictp);

    js::Shape *getChild(JSContext *cx, const js::Shape &child, js::Shape **listp);

    bool maybeHash(JSContext *cx);

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

    void insertFree(js::Shape **freep) {
#ifdef DEBUG
        memset(this, JS_FREE_PATTERN, sizeof *this);
#endif
        id = JSID_VOID;
        parent = *freep;
        if (parent)
            parent->listp = &parent;
        listp = freep;
        *freep = this;
    }

    void removeFree() {
        JS_ASSERT(JSID_IS_VOID(id));
        *listp = parent;
        if (parent)
            parent->listp = listp;
    }

  public:
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
            JS_ASSERT_IF(!cursor->parent, JSID_IS_EMPTY(cursor->id));
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
        
        MARK            = 0x01,

        SHARED_EMPTY    = 0x02,

        



        SHAPE_REGEN     = 0x04,

        
        IN_DICTIONARY   = 0x08,

        
        FROZEN          = 0x10
    };

    Shape(jsid id, js::PropertyOp getter, js::PropertyOp setter, uint32 slot, uintN attrs,
          uintN flags, intN shortid, uint32 shape = INVALID_SHAPE, uint32 slotSpan = 0);

    
    Shape(JSContext *cx, Class *aclasp);

    bool marked() const         { return (flags & MARK) != 0; }
    void mark() const           { flags |= MARK; }
    void clearMark()            { flags &= ~MARK; }

    bool hasRegenFlag() const   { return (flags & SHAPE_REGEN) != 0; }
    void setRegenFlag()         { flags |= SHAPE_REGEN; }
    void clearRegenFlag()       { flags &= ~SHAPE_REGEN; }

    bool inDictionary() const   { return (flags & IN_DICTIONARY) != 0; }
    bool frozen() const         { return (flags & FROZEN) != 0; }
    void setFrozen()            { flags |= FROZEN; }

    bool isEmptyShape() const   { JS_ASSERT_IF(!parent, JSID_IS_EMPTY(id)); return !parent; }

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

    js::PropertyOp setter() const { return rawSetter; }
    bool hasDefaultSetter() const  { return !rawSetter; }
    js::PropertyOp setterOp() const { JS_ASSERT(!hasSetterValue()); return rawSetter; }
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
    inline bool matchesParamsAfterId(js::PropertyOp agetter, js::PropertyOp asetter,
                                     uint32 aslot, uintN aattrs, uintN aflags,
                                     intN ashortid) const;

    bool get(JSContext* cx, JSObject *obj, JSObject *pobj, js::Value* vp) const;
    bool set(JSContext* cx, JSObject *obj, js::Value* vp) const;

    inline bool isSharedPermanent() const;

    void trace(JSTracer *trc) const;

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

    uint32 entryCount() const {
        if (table)
            return table->entryCount;

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
};

struct EmptyShape : public js::Shape
{
    EmptyShape(JSContext *cx, js::Class *aclasp);

    js::Class *getClass() const { return clasp; };

    static EmptyShape *create(JSContext *cx, js::Class *clasp) {
        js::Shape *eprop = JS_PROPERTY_TREE(cx).newShape(cx);
        if (!eprop)
            return NULL;
        return new (eprop) EmptyShape(cx, clasp);
    }
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

inline js::Shape **
JSObject::nativeSearch(jsid id, bool adding)
{
    return js::Shape::search(&lastProp, id, adding);
}

inline const js::Shape *
JSObject::nativeLookup(jsid id)
{
    return SHAPE_FETCH(nativeSearch(id));
}

inline bool
JSObject::nativeContains(jsid id)
{
    return nativeLookup(id) != NULL;
}

inline bool
JSObject::nativeContains(const js::Shape &shape)
{
    return nativeLookup(shape.id) == &shape;
}

inline const js::Shape *
JSObject::lastProperty() const
{
    JS_ASSERT(isNative());
    JS_ASSERT(!JSID_IS_VOID(lastProp->id));
    return lastProp;
}

inline bool
JSObject::nativeEmpty() const
{
    return lastProperty()->isEmptyShape();
}

inline bool
JSObject::inDictionaryMode() const
{
    return lastProperty()->inDictionary();
}

inline uint32
JSObject::propertyCount() const
{
    return lastProperty()->entryCount();
}

inline bool
JSObject::hasPropertyTable() const
{
    return !!lastProperty()->table;
}




inline void
JSObject::setLastProperty(const js::Shape *shape)
{
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT(!JSID_IS_VOID(shape->id));
    JS_ASSERT_IF(lastProp, !JSID_IS_VOID(lastProp->id));

    lastProp = const_cast<js::Shape *>(shape);
}

inline void
JSObject::removeLastProperty()
{
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT(!JSID_IS_VOID(lastProp->parent->id));

    lastProp = lastProp->parent;
}

namespace js {

inline void
Shape::removeFromDictionary(JSObject *obj) const
{
    JS_ASSERT(!frozen());
    JS_ASSERT(inDictionary());
    JS_ASSERT(obj->inDictionaryMode());
    JS_ASSERT(listp);
    JS_ASSERT(!JSID_IS_VOID(id));

    JS_ASSERT(obj->lastProp->inDictionary());
    JS_ASSERT(obj->lastProp->listp == &obj->lastProp);
    JS_ASSERT_IF(obj->lastProp != this, !JSID_IS_VOID(obj->lastProp->id));
    JS_ASSERT_IF(obj->lastProp->parent, !JSID_IS_VOID(obj->lastProp->parent->id));

    if (parent)
        parent->listp = listp;
    *listp = parent;
    listp = NULL;
}

inline void
Shape::insertIntoDictionary(js::Shape **dictp)
{
    



    JS_ASSERT(inDictionary());
    JS_ASSERT(!listp);
    JS_ASSERT(!JSID_IS_VOID(id));

    JS_ASSERT_IF(*dictp, !(*dictp)->frozen());
    JS_ASSERT_IF(*dictp, (*dictp)->inDictionary());
    JS_ASSERT_IF(*dictp, (*dictp)->listp == dictp);
    JS_ASSERT_IF(*dictp, !JSID_IS_VOID((*dictp)->id));

    setParent(*dictp);
    if (parent)
        parent->listp = &parent;
    listp = dictp;
    *dictp = this;
}

} 





#define SHAPE_USERID(shape)                                                   \
    ((shape)->hasShortID() ? INT_TO_JSID((shape)->shortid)                    \
                           : (shape)->id)

#ifndef JS_THREADSAFE
# define js_GenerateShape(cx, gcLocked)    js_GenerateShape (cx)
#endif

extern uint32
js_GenerateShape(JSContext *cx, bool gcLocked);

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
Shape::search(js::Shape **startp, jsid id, bool adding)
{
    METER(searches);
    if (!(*startp)->table) {
        






        js::Shape **spp;

        for (spp = startp; js::Shape *shape = *spp; spp = &shape->parent) {
            if (shape->id == id) {
                METER(hits);
                return spp;
            }
        }
        METER(misses);
        return spp;
    }
    return (*startp)->table->search(id, adding);
}

#undef METER

inline bool
Shape::isSharedPermanent() const
{
    return (~attrs & (JSPROP_SHARED | JSPROP_PERMANENT)) == 0;
}

}

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif

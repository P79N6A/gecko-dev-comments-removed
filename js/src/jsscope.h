







































#ifndef jsscope_h___
#define jsscope_h___



#ifdef DEBUG
#include <stdio.h>
#endif

#include "jstypes.h"
#include "jscntxt.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jspropertycache.h"
#include "jspropertytree.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#endif





















































































































































struct JSEmptyScope;

#define SPROP_INVALID_SLOT              0xffffffff

struct JSScope : public JSObjectMap
{
#ifdef JS_THREADSAFE
    JSTitle         title;              
#endif
    JSObject        *object;            
    uint32          freeslot;           
  protected:
    uint8           flags;              
  public:
    int8            hashShift;          

    uint16          spare;              
    uint32          entryCount;         
    uint32          removedCount;       
    JSScopeProperty **table;            
    JSEmptyScope    *emptyScope;        

    



    inline JSScopeProperty *lastProperty() const;

  private:
    JSScopeProperty *getChildProperty(JSContext *cx, JSScopeProperty *parent,
                                      JSScopeProperty &child);

    JSScopeProperty *newDictionaryProperty(JSContext *cx, const JSScopeProperty &child,
                                           JSScopeProperty **childp);

    bool toDictionaryMode(JSContext *cx, JSScopeProperty *&aprop);

    










    JSScopeProperty *lastProp;

    
    inline void setLastProperty(JSScopeProperty *sprop);
    inline void removeLastProperty();
    inline void removeDictionaryProperty(JSScopeProperty *sprop);
    inline void insertDictionaryProperty(JSScopeProperty *sprop, JSScopeProperty **childp);

    
    inline void updateShape(JSContext *cx);
    inline void updateFlags(const JSScopeProperty *sprop, bool isDefinitelyAtom = false);

  protected:
    void initMinimal(JSContext *cx, uint32 newShape);

  private:
    bool createTable(JSContext *cx, bool report);
    bool changeTable(JSContext *cx, int change);
    void reportReadOnlyScope(JSContext *cx);

    void setOwnShape()          { flags |= OWN_SHAPE; }
    void clearOwnShape()        { flags &= ~OWN_SHAPE; }
    void generateOwnShape(JSContext *cx);

    JSScopeProperty **searchTable(jsid id, bool adding);
    inline JSScopeProperty **search(jsid id, bool adding);
    inline JSEmptyScope *createEmptyScope(JSContext *cx, js::Class *clasp);

    JSScopeProperty *addPropertyHelper(JSContext *cx, jsid id,
                                       js::PropertyOp getter, js::PropertyOp setter,
                                       uint32 slot, uintN attrs,
                                       uintN flags, intN shortid,
                                       JSScopeProperty **spp);

  public:
    JSScope(const JSObjectOps *ops, JSObject *obj)
      : JSObjectMap(ops, 0), object(obj) {}

    
    static JSScope *create(JSContext *cx, const JSObjectOps *ops,
                           js::Class *clasp, JSObject *obj, uint32 shape);

    void destroy(JSContext *cx);

    






    inline JSEmptyScope *getEmptyScope(JSContext *cx, js::Class *clasp);

    inline bool ensureEmptyScope(JSContext *cx, js::Class *clasp);

    inline bool canProvideEmptyScope(JSObjectOps *ops, js::Class *clasp);

    JSScopeProperty *lookup(jsid id);

    inline bool hasProperty(jsid id) { return lookup(id) != NULL; }
    inline bool hasProperty(JSScopeProperty *sprop);

    
    JSScopeProperty *addProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::PropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    JSScopeProperty *addDataProperty(JSContext *cx, jsid id, uint32 slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    
    JSScopeProperty *putProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::PropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    
    JSScopeProperty *changeProperty(JSContext *cx, JSScopeProperty *sprop,
                                    uintN attrs, uintN mask,
                                    js::PropertyOp getter, js::PropertyOp setter);

    
    bool removeProperty(JSContext *cx, jsid id);

    
    void clear(JSContext *cx);

    
    void extend(JSContext *cx, JSScopeProperty *sprop, bool isDefinitelyAtom = false);

    




    bool methodReadBarrier(JSContext *cx, JSScopeProperty *sprop, js::Value *vp);

    






    bool methodWriteBarrier(JSContext *cx, JSScopeProperty *sprop, const js::Value &v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, const js::Value &v);

    void trace(JSTracer *trc);

    void deletingShapeChange(JSContext *cx, JSScopeProperty *sprop);
    bool methodShapeChange(JSContext *cx, JSScopeProperty *sprop);
    bool methodShapeChange(JSContext *cx, uint32 slot);
    void protoShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, JSScopeProperty *sprop);
    bool globalObjectOwnShapeChange(JSContext *cx);


#define SCOPE_CAPACITY(scope)   JS_BIT(JS_DHASH_BITS-(scope)->hashShift)

    enum {
        DICTIONARY_MODE         = 0x0001,
        SEALED                  = 0x0002,
        BRANDED                 = 0x0004,
        INDEXED_PROPERTIES      = 0x0008,
        OWN_SHAPE               = 0x0010,
        METHOD_BARRIER          = 0x0020,

        



        SHAPE_REGEN             = 0x0040,

        
        GENERIC                 = 0x0080
    };

    bool inDictionaryMode()     { return flags & DICTIONARY_MODE; }
    void setDictionaryMode()    { flags |= DICTIONARY_MODE; }
    void clearDictionaryMode()  { flags &= ~DICTIONARY_MODE; }

    




    bool sealed()               { return flags & SEALED; }

    void seal(JSContext *cx) {
        JS_ASSERT(!isSharedEmpty());
        JS_ASSERT(!sealed());
        generateOwnShape(cx);
        flags |= SEALED;
    }

    




    bool branded()              { return flags & BRANDED; }

    bool brand(JSContext *cx, uint32 slot, const js::Value &) {
        JS_ASSERT(!generic());
        JS_ASSERT(!branded());
        generateOwnShape(cx);
        if (js_IsPropertyCacheDisabled(cx))  
            return false;
        flags |= BRANDED;
        return true;
    }

    bool generic()              { return flags & GENERIC; }

    





    void unbrand(JSContext *cx) {
        if (!branded())
            flags |= GENERIC;
    }

    bool hadIndexedProperties() { return flags & INDEXED_PROPERTIES; }
    void setIndexedProperties() { flags |= INDEXED_PROPERTIES; }

    bool hasOwnShape()          { return flags & OWN_SHAPE; }

    bool hasRegenFlag(uint8 regenFlag) { return (flags & SHAPE_REGEN) == regenFlag; }

    







































    bool hasMethodBarrier()     { return flags & METHOD_BARRIER; }
    void setMethodBarrier()     { flags |= METHOD_BARRIER; }

    





    bool
    brandedOrHasMethodBarrier() { return flags & (BRANDED | METHOD_BARRIER); }

    bool isSharedEmpty() const  { return !object; }

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
};

struct JSEmptyScope : public JSScope
{
    js::Class * const clasp;
    jsrefcount        nrefs;              

    JSEmptyScope(JSContext *cx, const JSObjectOps *ops, js::Class *clasp);

    JSEmptyScope *hold() {
        
        JS_ASSERT(nrefs >= 1);
        JS_ATOMIC_INCREMENT(&nrefs);
        return this;
    }

    void drop(JSContext *cx) {
        JS_ASSERT(nrefs >= 1);
        JS_ATOMIC_DECREMENT(&nrefs);
        if (nrefs == 0)
            destroy(cx);
    }

    



    void dropFromGC(JSContext *cx) {
#ifdef JS_THREADSAFE
        JS_ASSERT(CX_THREAD_IS_RUNNING_GC(cx));
#endif
        JS_ASSERT(nrefs >= 1);
        --nrefs;
        if (nrefs == 0)
            destroy(cx);
    }
};

inline bool
JS_IS_SCOPE_LOCKED(JSContext *cx, JSScope *scope)
{
    return JS_IS_TITLE_LOCKED(cx, &scope->title);
}

inline JSScope *
JSObject::scope() const
{
    JS_ASSERT(isNative());
    return (JSScope *) map;
}

inline uint32
JSObject::shape() const
{
    JS_ASSERT(map->shape != JSObjectMap::SHAPELESS);
    return map->shape;
}

inline const js::Value &
JSObject::lockedGetSlot(uintN slot) const
{
    OBJ_CHECK_SLOT(this, slot);
    return this->getSlot(slot);
}

inline void
JSObject::lockedSetSlot(uintN slot, const js::Value &value)
{
    OBJ_CHECK_SLOT(this, slot);
    this->setSlot(slot, value);
}

namespace js {

class PropertyTree;

} 

struct JSScopeProperty {
    friend struct JSScope;
    friend class js::PropertyTree;
    friend JSDHashOperator js::RemoveNodeIfDead(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                                uint32 number, void *arg);
    friend void js::SweepScopeProperties(JSContext *cx);

    jsid            id;

  private:
    union {
        js::PropertyOp  rawGetter;      
        JSObject        *getterObj;     



        JSScopeProperty *next;          
    };

    union {
        js::PropertyOp  rawSetter;      

        JSObject        *setterObj;     

        JSScopeProperty **prevp;        

    };

    void insertFree(JSScopeProperty *&list) {
        id = JSID_VOID;
        next = list;
        prevp = &list;
        if (list)
            list->prevp = &next;
        list = this;
    }

    void removeFree() {
        JS_ASSERT(JSID_IS_VOID(id));
        *prevp = next;
        if (next)
            next->prevp = prevp;
    }

  public:
    uint32          slot;               
  private:
    uint8           attrs;              
    uint8           flags;              
  public:
    int16           shortid;            
    JSScopeProperty *parent;            
    union {
        JSScopeProperty *kids;          

        JSScopeProperty **childp;       



    };
    uint32          shape;              

  private:
    




    enum {
        
        MARK            = 0x01,

        




        SHAPE_REGEN     = 0x08,

        
        IN_DICTIONARY   = 0x20
    };

    JSScopeProperty(jsid id, js::PropertyOp getter, js::PropertyOp setter, uint32 slot,
                    uintN attrs, uintN flags, intN shortid);

    bool marked() const { return (flags & MARK) != 0; }
    void mark() { flags |= MARK; }
    void clearMark() { flags &= ~MARK; }

    bool hasRegenFlag() const { return (flags & SHAPE_REGEN) != 0; }
    void setRegenFlag() { flags |= SHAPE_REGEN; }
    void clearRegenFlag() { flags &= ~SHAPE_REGEN; }

    bool inDictionary() const { return (flags & IN_DICTIONARY) != 0; }

  public:
    
    enum {
        ALIAS           = 0x02,
        HAS_SHORTID     = 0x04,
        METHOD          = 0x10,
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
    inline bool matches(const JSScopeProperty *p) const;
    inline bool matchesParamsAfterId(js::PropertyOp agetter, js::PropertyOp asetter,
                                     uint32 aslot, uintN aattrs, uintN aflags,
                                     intN ashortid) const;

    bool get(JSContext* cx, JSObject *obj, JSObject *pobj, js::Value* vp);
    bool set(JSContext* cx, JSObject *obj, js::Value* vp);

    inline bool isSharedPermanent() const;

    void trace(JSTracer *trc);

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

#ifdef DEBUG
    void dump(JSContext *cx, FILE *fp);
    void dumpSubtree(JSContext *cx, int level, FILE *fp);
#endif
};


#define SPROP_COLLISION                 ((jsuword)1)
#define SPROP_REMOVED                   ((JSScopeProperty *) SPROP_COLLISION)


#define SPROP_IS_FREE(sprop)            ((sprop) == NULL)
#define SPROP_IS_REMOVED(sprop)         ((sprop) == SPROP_REMOVED)
#define SPROP_IS_LIVE(sprop)            ((sprop) > SPROP_REMOVED)
#define SPROP_FLAG_COLLISION(spp,sprop) (*(spp) = (JSScopeProperty *)         \
                                         ((jsuword)(sprop) | SPROP_COLLISION))
#define SPROP_HAD_COLLISION(sprop)      ((jsuword)(sprop) & SPROP_COLLISION)
#define SPROP_FETCH(spp)                SPROP_CLEAR_COLLISION(*(spp))

#define SPROP_CLEAR_COLLISION(sprop)                                          \
    ((JSScopeProperty *) ((jsuword)(sprop) & ~SPROP_COLLISION))

#define SPROP_STORE_PRESERVING_COLLISION(spp, sprop)                          \
    (*(spp) = (JSScopeProperty *) ((jsuword)(sprop)                           \
                                   | SPROP_HAD_COLLISION(*(spp))))

inline JSScopeProperty *
JSScope::lookup(jsid id)
{
    return SPROP_FETCH(search(id, false));
}

inline bool
JSScope::hasProperty(JSScopeProperty *sprop)
{
    return lookup(sprop->id) == sprop;
}

inline JSScopeProperty *
JSScope::lastProperty() const
{
    JS_ASSERT_IF(lastProp, !JSID_IS_VOID(lastProp->id));
    return lastProp;
}





inline void
JSScope::setLastProperty(JSScopeProperty *sprop)
{
    JS_ASSERT(!JSID_IS_VOID(sprop->id));
    JS_ASSERT_IF(lastProp, !JSID_IS_VOID(lastProp->id));

    lastProp = sprop;
}

inline void
JSScope::removeLastProperty()
{
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT_IF(lastProp->parent, !JSID_IS_VOID(lastProp->parent->id));

    lastProp = lastProp->parent;
    --entryCount;
}

inline void
JSScope::removeDictionaryProperty(JSScopeProperty *sprop)
{
    JS_ASSERT(inDictionaryMode());
    JS_ASSERT(sprop->inDictionary());
    JS_ASSERT(sprop->childp);
    JS_ASSERT(!JSID_IS_VOID(sprop->id));

    JS_ASSERT(lastProp->inDictionary());
    JS_ASSERT(lastProp->childp == &lastProp);
    JS_ASSERT_IF(lastProp != sprop, !JSID_IS_VOID(lastProp->id));
    JS_ASSERT_IF(lastProp->parent, !JSID_IS_VOID(lastProp->parent->id));

    if (sprop->parent)
        sprop->parent->childp = sprop->childp;
    *sprop->childp = sprop->parent;
    --entryCount;
    sprop->childp = NULL;
}

inline void
JSScope::insertDictionaryProperty(JSScopeProperty *sprop, JSScopeProperty **childp)
{
    



    JS_ASSERT(sprop->inDictionary());
    JS_ASSERT(!sprop->childp);
    JS_ASSERT(!JSID_IS_VOID(sprop->id));

    JS_ASSERT_IF(*childp, (*childp)->inDictionary());
    JS_ASSERT_IF(lastProp, lastProp->inDictionary());
    JS_ASSERT_IF(lastProp, lastProp->childp == &lastProp);
    JS_ASSERT_IF(lastProp, !JSID_IS_VOID(lastProp->id));

    sprop->parent = *childp;
    *childp = sprop;
    if (sprop->parent)
        sprop->parent->childp = &sprop->parent;
    sprop->childp = childp;
    ++entryCount;
}





#define SPROP_USERID(sprop)                                                   \
    ((sprop)->hasShortID() ? INT_TO_JSID((sprop)->shortid)                    \
                           : (sprop)->id)

#define SLOT_IN_SCOPE(slot,scope)         ((slot) < (scope)->freeslot)
#define SPROP_HAS_VALID_SLOT(sprop,scope) SLOT_IN_SCOPE((sprop)->slot, scope)

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
    jsrefcount          steps;
    jsrefcount          stepHits;
    jsrefcount          stepMisses;
    jsrefcount          tableAllocFails;
    jsrefcount          toDictFails;
    jsrefcount          wrapWatchFails;
    jsrefcount          adds;
    jsrefcount          addFails;
    jsrefcount          puts;
    jsrefcount          redundantPuts;
    jsrefcount          putFails;
    jsrefcount          changes;
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

inline JSScopeProperty **
JSScope::search(jsid id, bool adding)
{
    JSScopeProperty *sprop, **spp;

    METER(searches);
    if (!table) {
        
        for (spp = &lastProp; (sprop = *spp); spp = &sprop->parent) {
            if (sprop->id == id) {
                METER(hits);
                return spp;
            }
        }
        METER(misses);
        return spp;
    }
    return searchTable(id, adding);
}

#undef METER

inline bool
JSScope::canProvideEmptyScope(JSObjectOps *ops, js::Class *clasp)
{
    



    if (!object)
        return false;
    return this->ops == ops && (!emptyScope || emptyScope->clasp == clasp);
}

inline bool
JSScopeProperty::isSharedPermanent() const
{
    return (~attrs & (JSPROP_SHARED | JSPROP_PERMANENT)) == 0;
}

extern JSScope *
js_GetMutableScope(JSContext *cx, JSObject *obj);

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#endif

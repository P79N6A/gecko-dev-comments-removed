







































#ifndef jsscope_h___
#define jsscope_h___



#include "jstypes.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

JS_BEGIN_EXTERN_C

















































































































































struct JSScope : public JSObjectMap
{
#ifdef JS_THREADSAFE
    JSTitle         title;              
#endif
    JSObject        *object;            
    jsrefcount      nrefs;              
    uint32          freeslot;           
    JSScope         *emptyScope;        
    uint8           flags;              
    int8            hashShift;          

    uint16          spare;              
    uint32          entryCount;         
    uint32          removedCount;       
    JSScopeProperty **table;            
    JSScopeProperty *lastProp;          

  private:
    void initMinimal(JSContext *cx, uint32 newShape);
    bool createTable(JSContext *cx, bool report);
    bool changeTable(JSContext *cx, int change);
    void reportReadOnlyScope(JSContext *cx);
    void generateOwnShape(JSContext *cx);
    JSScopeProperty **searchTable(jsid id, bool adding);
    inline JSScopeProperty **search(jsid id, bool adding);
    JSScope *createEmptyScope(JSContext *cx, JSClass *clasp);

  public:
    explicit JSScope(const JSObjectOps *ops, JSObject *obj = NULL)
      : JSObjectMap(ops, 0), object(obj) {}

    
    static JSScope *create(JSContext *cx, const JSObjectOps *ops, JSClass *clasp,
                           JSObject *obj, uint32 shape);

    static void destroy(JSContext *cx, JSScope *scope);

    






    JSScope *getEmptyScope(JSContext *cx, JSClass *clasp) {
        if (emptyScope) {
            emptyScope->hold();
            return emptyScope;
        }
        return createEmptyScope(cx, clasp);
    }

    bool getEmptyScopeShape(JSContext *cx, JSClass *clasp, uint32 *shapep) {
        if (emptyScope) {
            *shapep = emptyScope->shape;
            return true;
        }
        JSScope *e = getEmptyScope(cx, clasp);
        if (!e)
            return false;
        *shapep = e->shape;
        e->drop(cx, NULL);
        return true;
    }

    inline void hold();
    inline bool drop(JSContext *cx, JSObject *obj);

    JSScopeProperty *lookup(jsid id);
    bool has(JSScopeProperty *sprop);

    JSScopeProperty *add(JSContext *cx, jsid id,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uint32 slot, uintN attrs,
                         uintN flags, intN shortid);

    JSScopeProperty *change(JSContext *cx, JSScopeProperty *sprop,
                            uintN attrs, uintN mask,
                            JSPropertyOp getter, JSPropertyOp setter);

    bool remove(JSContext *cx, jsid id);
    void clear(JSContext *cx);

    void extend(JSContext *cx, JSScopeProperty *sprop);

    



    bool methodReadBarrier(JSContext *cx, JSScopeProperty *sprop, jsval *vp);

    






    bool methodWriteBarrier(JSContext *cx, JSScopeProperty *sprop, jsval v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, jsval v);

    void trace(JSTracer *trc);

    void brandingShapeChange(JSContext *cx, uint32 slot, jsval v);
    void deletingShapeChange(JSContext *cx, JSScopeProperty *sprop);
    bool methodShapeChange(JSContext *cx, JSScopeProperty *sprop, jsval toval);
    bool methodShapeChange(JSContext *cx, uint32 slot, jsval toval);
    void protoShapeChange(JSContext *cx);
    void replacingShapeChange(JSContext *cx, JSScopeProperty *sprop, JSScopeProperty *newsprop);
    void sealingShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, JSScopeProperty *sprop);


#define SCOPE_CAPACITY(scope)           JS_BIT(JS_DHASH_BITS-(scope)->hashShift)

    enum {
        MIDDLE_DELETE           = 0x0001,
        SEALED                  = 0x0002,
        BRANDED                 = 0x0004,
        INDEXED_PROPERTIES      = 0x0008,
        OWN_SHAPE               = 0x0010,
        METHOD_BARRIER          = 0x0020,

        



        SHAPE_REGEN             = 0x0040
    };

    bool hadMiddleDelete()      { return flags & MIDDLE_DELETE; }
    void setMiddleDelete()      { flags |= MIDDLE_DELETE; }
    void clearMiddleDelete()    { flags &= ~MIDDLE_DELETE; }

    




    bool sealed()               { return flags & SEALED; }
    void setSealed()            { flags |= SEALED; }

    




    bool branded()              { return flags & BRANDED; }
    void setBranded()           { flags |= BRANDED; }

    bool hadIndexedProperties() { return flags & INDEXED_PROPERTIES; }
    void setIndexedProperties() { flags |= INDEXED_PROPERTIES; }

    bool hasOwnShape()          { return flags & OWN_SHAPE; }
    void setOwnShape()          { flags |= OWN_SHAPE; }

    bool hasRegenFlag(uint8 regenFlag) { return (flags & SHAPE_REGEN) == regenFlag; }

    
































    bool hasMethodBarrier()     { return flags & METHOD_BARRIER; }
    void setMethodBarrier()     { flags |= METHOD_BARRIER | BRANDED; }

    bool owned()                { return object != NULL; }
};

inline bool
JS_IS_SCOPE_LOCKED(JSContext *cx, JSScope *scope)
{
    return JS_IS_TITLE_LOCKED(cx, &scope->title);
}

inline JSScope *
OBJ_SCOPE(JSObject *obj)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    return (JSScope *) obj->map;
}

inline uint32
OBJ_SHAPE(JSObject *obj)
{
    JS_ASSERT(obj->map->shape != JSObjectMap::SHAPELESS);
    return obj->map->shape;
}





#define SCOPE_LAST_PROP(scope)                                                \
    (JS_ASSERT_IF((scope)->lastProp, !JSVAL_IS_NULL((scope)->lastProp->id)),  \
     (scope)->lastProp)
#define SCOPE_REMOVE_LAST_PROP(scope)                                         \
    (JS_ASSERT_IF((scope)->lastProp->parent,                                  \
                  !JSVAL_IS_NULL((scope)->lastProp->parent->id)),             \
     (scope)->lastProp = (scope)->lastProp->parent)





inline JSObject *
js_CastAsObject(JSPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline jsval
js_CastAsObjectJSVal(JSPropertyOp op)
{
    return OBJECT_TO_JSVAL(JS_FUNC_TO_DATA_PTR(JSObject *, op));
}

inline JSPropertyOp
js_CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSPropertyOp, object);
}

struct JSScopeProperty {
    jsid            id;                 
    JSPropertyOp    getter;             
    JSPropertyOp    setter;             

    uint32          slot;               
    uint8           attrs;              
    uint8           flags;              
    int16           shortid;            
    JSScopeProperty *parent;            
    JSScopeProperty *kids;              

    uint32          shape;              


#define SPROP_MARK                      0x01
#define SPROP_IS_ALIAS                  0x02
#define SPROP_HAS_SHORTID               0x04
#define SPROP_FLAG_SHAPE_REGEN          0x08
#define SPROP_IS_METHOD                 0x10

    bool isMethod() const {
        return flags & SPROP_IS_METHOD;
    }
    JSObject *methodObject() const {
        JS_ASSERT(isMethod());
        return js_CastAsObject(getter);
    }
    jsval methodValue() const {
        JS_ASSERT(isMethod());
        return js_CastAsObjectJSVal(getter);
    }

    bool hasGetterObject() const {
        return attrs & JSPROP_GETTER;
    }
    JSObject *getterObject() const {
        JS_ASSERT(hasGetterObject());
        return js_CastAsObject(getter);
    }
    jsval getterValue() const {
        JS_ASSERT(hasGetterObject());
        return js_CastAsObjectJSVal(getter);
    }

    bool hasSetterObject() const {
        return attrs & JSPROP_SETTER;
    }
    JSObject *setterObject() const {
        JS_ASSERT(hasSetterObject());
        return js_CastAsObject(setter);
    }
    jsval setterValue() const {
        JS_ASSERT(hasSetterObject());
        return js_CastAsObjectJSVal(setter);
    }

    bool get(JSContext* cx, JSObject* obj, JSObject *pobj, jsval* vp);
    bool set(JSContext* cx, JSObject* obj, jsval* vp);

    void trace(JSTracer *trc);
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
JSScope::has(JSScopeProperty *sprop)
{
    return lookup(sprop->id) == sprop;
}





#define SPROP_USERID(sprop)                                                   \
    (((sprop)->flags & SPROP_HAS_SHORTID) ? INT_TO_JSVAL((sprop)->shortid)    \
                                          : ID_TO_VALUE((sprop)->id))

#define SPROP_INVALID_SLOT              0xffffffff

#define SLOT_IN_SCOPE(slot,scope)         ((slot) < (scope)->freeslot)
#define SPROP_HAS_VALID_SLOT(sprop,scope) SLOT_IN_SCOPE((sprop)->slot, scope)

#define SPROP_HAS_STUB_GETTER(sprop)    (!(sprop)->getter)
#define SPROP_HAS_STUB_SETTER(sprop)    (!(sprop)->setter)

#define SPROP_HAS_STUB_GETTER_OR_IS_METHOD(sprop)                             \
    (SPROP_HAS_STUB_GETTER(sprop) || (sprop)->isMethod())

#ifndef JS_THREADSAFE
# define js_GenerateShape(cx, gcLocked)    js_GenerateShape (cx)
#endif

extern uint32
js_GenerateShape(JSContext *cx, bool gcLocked);

#ifdef JS_DUMP_PROPTREE_STATS
struct JSScopeStats {
    jsrefcount          searches;
    jsrefcount          hits;
    jsrefcount          misses;
    jsrefcount          hashes;
    jsrefcount          steps;
    jsrefcount          stepHits;
    jsrefcount          stepMisses;
    jsrefcount          adds;
    jsrefcount          redundantAdds;
    jsrefcount          addFailures;
    jsrefcount          changeFailures;
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
        
        JS_ASSERT(!hadMiddleDelete());
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

inline void
JSScope::hold()
{
    JS_ASSERT(nrefs >= 0);
    JS_ATOMIC_INCREMENT(&nrefs);
}

inline bool
JSScope::drop(JSContext *cx, JSObject *obj)
{
#ifdef JS_THREADSAFE
    
    JS_ASSERT(!obj || CX_THREAD_IS_RUNNING_GC(cx));
#endif
    JS_ASSERT(nrefs > 0);
    --nrefs;

    if (nrefs == 0) {
        destroy(cx, this);
        return false;
    }
    if (object == obj)
        object = NULL;
    return true;
}

inline bool
JSScopeProperty::get(JSContext* cx, JSObject* obj, JSObject *pobj, jsval* vp)
{
    JS_ASSERT(!SPROP_HAS_STUB_GETTER(this));
    JS_ASSERT(!JSVAL_IS_NULL(this->id));

    if (attrs & JSPROP_GETTER) {
        JS_ASSERT(!isMethod());
        jsval fval = getterValue();
        return js_InternalGetOrSet(cx, obj, id, fval, JSACC_READ, 0, 0, vp);
    }

    if (isMethod()) {
        *vp = methodValue();

        JSScope *scope = OBJ_SCOPE(pobj);
        JS_ASSERT(scope->object == pobj);
        return scope->methodReadBarrier(cx, this, vp);
    }

    





    if (STOBJ_GET_CLASS(obj) == &js_WithClass)
        obj = obj->map->ops->thisObject(cx, obj);
    return getter(cx, obj, SPROP_USERID(this), vp);
}

inline bool
JSScopeProperty::set(JSContext* cx, JSObject* obj, jsval* vp)
{
    JS_ASSERT_IF(SPROP_HAS_STUB_SETTER(this), attrs & JSPROP_GETTER);

    if (attrs & JSPROP_SETTER) {
        jsval fval = setterValue();
        return js_InternalGetOrSet(cx, obj, id, fval, JSACC_WRITE, 1, vp, vp);
    }

    if (attrs & JSPROP_GETTER) {
        js_ReportGetterOnlyAssignment(cx);
        return false;
    }

    
    if (STOBJ_GET_CLASS(obj) == &js_WithClass)
        obj = obj->map->ops->thisObject(cx, obj);
    return setter(cx, obj, SPROP_USERID(this), vp);
}


#define SPROP_IS_SHARED_PERMANENT(sprop)                                      \
    ((~(sprop)->attrs & (JSPROP_SHARED | JSPROP_PERMANENT)) == 0)

extern JSScope *
js_GetMutableScope(JSContext *cx, JSObject *obj);

extern void
js_TraceId(JSTracer *trc, jsid id);

extern void
js_SweepScopeProperties(JSContext *cx);

extern bool
js_InitPropertyTree(JSRuntime *rt);

extern void
js_FinishPropertyTree(JSRuntime *rt);

JS_END_EXTERN_C

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif 









































#ifndef jsscope_h___
#define jsscope_h___



#include "jstypes.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

















































































































































struct JSScope {
    JSObjectMap     map;                
#ifdef JS_THREADSAFE
    JSTitle         title;              
#endif
    JSObject        *object;            
    jsrefcount      nrefs;              
    uint32          freeslot;           
    uint32          shape;              
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
    
    static JSScope *create(JSContext *cx, JSObjectOps *ops, JSClass *clasp, JSObject *obj,
                           uint32 shape);

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

    void trace(JSTracer *trc);

    void brandingShapeChange(JSContext *cx, uint32 slot, jsval v);
    void deletingShapeChange(JSContext *cx, JSScopeProperty *sprop);
    void methodShapeChange(JSContext *cx, uint32 slot, jsval toval);
    void protoShapeChange(JSContext *cx);
    void replacingShapeChange(JSContext *cx,
                              JSScopeProperty *sprop,
                              JSScopeProperty *newsprop);
    void sealingShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, JSScopeProperty *sprop);


#define SCOPE_CAPACITY(scope)           JS_BIT(JS_DHASH_BITS-(scope)->hashShift)

    enum {
        MIDDLE_DELETE           = 0x0001,
        SEALED                  = 0x0002,
        BRANDED                 = 0x0004,
        INDEXED_PROPERTIES      = 0x0008,
        OWN_SHAPE               = 0x0010,

        



        SHAPE_REGEN             = 0x0020
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

    bool owned()                { return object != NULL; }
};

#define JS_IS_SCOPE_LOCKED(cx, scope)   JS_IS_TITLE_LOCKED(cx, &(scope)->title)

#define OBJ_SCOPE(obj)                  (JS_ASSERT(OBJ_IS_NATIVE(obj)),       \
                                         (JSScope *) (obj)->map)
#define OBJ_SHAPE(obj)                  (OBJ_SCOPE(obj)->shape)





#define SCOPE_LAST_PROP(scope)          ((scope)->lastProp)
#define SCOPE_REMOVE_LAST_PROP(scope)   ((scope)->lastProp =                  \
                                         (scope)->lastProp->parent)




static inline JSObject *
js_CastAsObject(JSPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

static inline jsval
js_CastAsObjectJSVal(JSPropertyOp op)
{
    return OBJECT_TO_JSVAL(JS_FUNC_TO_DATA_PTR(JSObject *, op));
}

static inline JSPropertyOp
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

JS_ALWAYS_INLINE JSScopeProperty *
JSScope::lookup(jsid id)
{
    return SPROP_FETCH(search(id, false));
}

JS_ALWAYS_INLINE bool
JSScope::has(JSScopeProperty *sprop)
{
    return lookup(sprop->id) == sprop;
}


#define SPROP_MARK                      0x01
#define SPROP_IS_ALIAS                  0x02
#define SPROP_HAS_SHORTID               0x04
#define SPROP_FLAG_SHAPE_REGEN          0x08





#define SPROP_USERID(sprop)                                                   \
    (((sprop)->flags & SPROP_HAS_SHORTID) ? INT_TO_JSVAL((sprop)->shortid)    \
                                          : ID_TO_VALUE((sprop)->id))

#define SPROP_INVALID_SLOT              0xffffffff

#define SLOT_IN_SCOPE(slot,scope)         ((slot) < (scope)->freeslot)
#define SPROP_HAS_VALID_SLOT(sprop,scope) SLOT_IN_SCOPE((sprop)->slot, scope)

#define SPROP_HAS_STUB_GETTER(sprop)    (!(sprop)->getter)
#define SPROP_HAS_STUB_SETTER(sprop)    (!(sprop)->setter)

#ifndef JS_THREADSAFE
# define js_GenerateShape(cx, gcLocked)    js_GenerateShape (cx)
#endif

extern uint32
js_GenerateShape(JSContext *cx, bool gcLocked);

#ifdef JS_DUMP_PROPTREE_STATS
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

inline void
JSScope::extend(JSContext *cx, JSScopeProperty *sprop)
{
    js_LeaveTraceIfGlobalObject(cx, object);
    shape = (!lastProp || shape == lastProp->shape)
            ? sprop->shape
            : js_GenerateShape(cx, JS_FALSE);
    ++entryCount;
    lastProp = sprop;
}

inline void
JSScope::trace(JSTracer *trc)
{
    JSContext *cx = trc->context;
    JSScopeProperty *sprop = lastProp;
    uint8 regenFlag = cx->runtime->gcRegenShapesScopeFlag;
    if (IS_GC_MARKING_TRACER(trc) && cx->runtime->gcRegenShapes && hasRegenFlag(regenFlag)) {
        



        uint32 newShape;

        if (sprop) {
            if (!(sprop->flags & SPROP_FLAG_SHAPE_REGEN)) {
                sprop->shape = js_RegenerateShapeForGC(cx);
                sprop->flags |= SPROP_FLAG_SHAPE_REGEN;
            }
            newShape = sprop->shape;
        }
        if (!sprop || hasOwnShape()) {
            newShape = js_RegenerateShapeForGC(cx);
            JS_ASSERT_IF(sprop, newShape != sprop->shape);
        }
        shape = newShape;
        flags ^= JSScope::SHAPE_REGEN;

        
        for (JSScope *empty = emptyScope;
             empty && empty->hasRegenFlag(regenFlag);
             empty = empty->emptyScope) {
            empty->shape = js_RegenerateShapeForGC(cx);
            empty->flags ^= JSScope::SHAPE_REGEN;
        }
    }
    if (sprop) {
        JS_ASSERT(has(sprop));

        
        do {
            if (hadMiddleDelete() && !has(sprop))
                continue;
            sprop->trace(trc);
        } while ((sprop = sprop->parent) != NULL);
    }
}


static JS_INLINE bool
js_GetSprop(JSContext* cx, JSScopeProperty* sprop, JSObject* obj, jsval* vp)
{
    JS_ASSERT(!SPROP_HAS_STUB_GETTER(sprop));

    if (sprop->attrs & JSPROP_GETTER) {
        jsval fval = js_CastAsObjectJSVal(sprop->getter);
        return js_InternalGetOrSet(cx, obj, sprop->id, fval, JSACC_READ,
                                   0, 0, vp);
    }

    





    if (STOBJ_GET_CLASS(obj) == &js_WithClass)
        obj = obj->map->ops->thisObject(cx, obj);
    return sprop->getter(cx, obj, SPROP_USERID(sprop), vp);
}

static JS_INLINE bool
js_SetSprop(JSContext* cx, JSScopeProperty* sprop, JSObject* obj, jsval* vp)
{
    JS_ASSERT(!(SPROP_HAS_STUB_SETTER(sprop) &&
                !(sprop->attrs & JSPROP_GETTER)));

    if (sprop->attrs & JSPROP_SETTER) {
        jsval fval = js_CastAsObjectJSVal(sprop->setter);
        return js_InternalGetOrSet(cx, obj, (sprop)->id, fval, JSACC_WRITE,
                                   1, vp, vp);
    }

    if (sprop->attrs & JSPROP_GETTER) {
        js_ReportGetterOnlyAssignment(cx);
        return JS_FALSE;
    }

    
    if (STOBJ_GET_CLASS(obj) == &js_WithClass)
        obj = obj->map->ops->thisObject(cx, obj);
    return sprop->setter(cx, obj, SPROP_USERID(sprop), vp);
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

#endif 

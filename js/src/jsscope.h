







































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
    uint8           flags;              
    int8            hashShift;          
    uint16          spare;              
    uint32          entryCount;         
    uint32          removedCount;       
    JSScopeProperty **table;            
    JSScopeProperty *lastProp;          
};

#define JS_IS_SCOPE_LOCKED(cx, scope)   JS_IS_TITLE_LOCKED(cx, &(scope)->title)

#define OBJ_SCOPE(obj)                  (JS_ASSERT(OBJ_IS_NATIVE(obj)),       \
                                         (JSScope *) (obj)->map)
#define OBJ_SHAPE(obj)                  (OBJ_SCOPE(obj)->shape)


#define SCOPE_CAPACITY(scope)           JS_BIT(JS_DHASH_BITS-(scope)->hashShift)


#define SCOPE_MIDDLE_DELETE             0x0001
#define SCOPE_SEALED                    0x0002
#define SCOPE_BRANDED                   0x0004
#define SCOPE_INDEXED_PROPERTIES        0x0008

#define SCOPE_HAD_MIDDLE_DELETE(scope)  ((scope)->flags & SCOPE_MIDDLE_DELETE)
#define SCOPE_SET_MIDDLE_DELETE(scope)  ((scope)->flags |= SCOPE_MIDDLE_DELETE)
#define SCOPE_CLR_MIDDLE_DELETE(scope)  ((scope)->flags &= ~SCOPE_MIDDLE_DELETE)
#define SCOPE_HAS_INDEXED_PROPERTIES(scope)  ((scope)->flags & SCOPE_INDEXED_PROPERTIES)
#define SCOPE_SET_INDEXED_PROPERTIES(scope)  ((scope)->flags |= SCOPE_INDEXED_PROPERTIES)

#define SCOPE_IS_SEALED(scope)          ((scope)->flags & SCOPE_SEALED)
#define SCOPE_SET_SEALED(scope)         ((scope)->flags |= SCOPE_SEALED)
#if 0




#undef  SCOPE_CLR_SEALED(scope)         ((scope)->flags &= ~SCOPE_SEALED)
#endif






#define SCOPE_IS_BRANDED(scope)         ((scope)->flags & SCOPE_BRANDED)
#define SCOPE_SET_BRANDED(scope)        ((scope)->flags |= SCOPE_BRANDED)
#define SCOPE_CLR_BRANDED(scope)        ((scope)->flags &= ~SCOPE_BRANDED)





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

static inline void
js_MakeScopeShapeUnique(JSContext *cx, JSScope *scope)
{
    js_LeaveTraceIfGlobalObject(cx, scope->object);
    scope->shape = js_GenerateShape(cx, JS_FALSE);
}

static inline void
js_ExtendScopeShape(JSContext *cx, JSScope *scope, JSScopeProperty *sprop)
{
    js_LeaveTraceIfGlobalObject(cx, scope->object);
    if (!scope->lastProp ||
        scope->shape == scope->lastProp->shape) {
        scope->shape = sprop->shape;
    } else {
        scope->shape = js_GenerateShape(cx, JS_FALSE);
    }
}

static JS_INLINE JSBool
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

static JS_INLINE JSBool
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

extern JSScope *
js_NewScope(JSContext *cx, JSObjectOps *ops, JSClass *clasp, JSObject *obj);

extern void
js_DestroyScope(JSContext *cx, JSScope *scope);

extern void
js_HoldScope(JSScope *scope);




extern JSBool
js_DropScope(JSContext *cx, JSScope *scope, JSObject *obj);

extern JS_FRIEND_API(JSScopeProperty **)
js_SearchScope(JSScope *scope, jsid id, JSBool adding);

#define SCOPE_GET_PROPERTY(scope, id)                                         \
    SPROP_FETCH(js_SearchScope(scope, id, JS_FALSE))

#define SCOPE_HAS_PROPERTY(scope, sprop)                                      \
    (SCOPE_GET_PROPERTY(scope, (sprop)->id) == (sprop))

extern JSScopeProperty *
js_AddScopeProperty(JSContext *cx, JSScope *scope, jsid id,
                    JSPropertyOp getter, JSPropertyOp setter, uint32 slot,
                    uintN attrs, uintN flags, intN shortid);

extern JSScopeProperty *
js_ChangeScopePropertyAttrs(JSContext *cx, JSScope *scope,
                            JSScopeProperty *sprop, uintN attrs, uintN mask,
                            JSPropertyOp getter, JSPropertyOp setter);

extern JSBool
js_RemoveScopeProperty(JSContext *cx, JSScope *scope, jsid id);

extern void
js_ClearScope(JSContext *cx, JSScope *scope);






#define TRACE_ID(trc, id)                js_TraceId(trc, id)
#define TRACE_SCOPE_PROPERTY(trc, sprop) js_TraceScopeProperty(trc, sprop)

extern void
js_TraceId(JSTracer *trc, jsid id);

extern void
js_TraceScopeProperty(JSTracer *trc, JSScopeProperty *sprop);

extern void
js_SweepScopeProperties(JSContext *cx);

extern JSBool
js_InitPropertyTree(JSRuntime *rt);

extern void
js_FinishPropertyTree(JSRuntime *rt);

JS_END_EXTERN_C

#endif 

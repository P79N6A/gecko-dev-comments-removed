







































#ifndef jsscope_h___
#define jsscope_h___



#include "jstypes.h"
#include "jsobj.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#ifdef JS_THREADSAFE
# include "jslock.h"
#endif

JS_BEGIN_EXTERN_C

















































































































































struct JSScope {
    JSObjectMap     map;                
    JSObject        *object;            
    uint8           flags;              
    int8            hashShift;          
    uint16          spare;              
    uint32          entryCount;         
    uint32          removedCount;       
    JSScopeProperty **table;            
    JSScopeProperty *lastProp;          
#ifdef JS_THREADSAFE
    JSContext       *ownercx;           
    JSThinLock      lock;               
    union {                             
        jsrefcount  count;              
        JSScope     *link;              
    } u;
#ifdef DEBUG
    const char      *file[4];           
    unsigned int    line[4];            
#endif
#endif
};

#define OBJ_SCOPE(obj)                  ((JSScope *)(obj)->map)


#define SCOPE_CAPACITY(scope)           JS_BIT(JS_DHASH_BITS-(scope)->hashShift)


#define SCOPE_MIDDLE_DELETE             0x0001
#define SCOPE_SEALED                    0x0002

#define SCOPE_HAD_MIDDLE_DELETE(scope)  ((scope)->flags & SCOPE_MIDDLE_DELETE)
#define SCOPE_SET_MIDDLE_DELETE(scope)  ((scope)->flags |= SCOPE_MIDDLE_DELETE)
#define SCOPE_CLR_MIDDLE_DELETE(scope)  ((scope)->flags &= ~SCOPE_MIDDLE_DELETE)

#define SCOPE_IS_SEALED(scope)          ((scope)->flags & SCOPE_SEALED)
#define SCOPE_SET_SEALED(scope)         ((scope)->flags |= SCOPE_SEALED)
#if 0




#define SCOPE_CLR_SEALED(scope)         ((scope)->flags &= ~SCOPE_SEALED)
#endif





#define SCOPE_LAST_PROP(scope)          ((scope)->lastProp)
#define SCOPE_REMOVE_LAST_PROP(scope)   ((scope)->lastProp =                  \
                                         (scope)->lastProp->parent)

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
#define SPROP_IS_DUPLICATE              0x02
#define SPROP_IS_ALIAS                  0x04
#define SPROP_HAS_SHORTID               0x08
#define SPROP_IS_HIDDEN                 0x10    /* a normally-hidden property,
                                                   e.g., function arg or var */





#define SPROP_USERID(sprop)                                                   \
    (((sprop)->flags & SPROP_HAS_SHORTID) ? INT_TO_JSVAL((sprop)->shortid)    \
                                          : ID_TO_VALUE((sprop)->id))

#define SPROP_INVALID_SLOT              0xffffffff

#define SLOT_IN_SCOPE(slot,scope)         ((slot) < (scope)->map.freeslot)
#define SPROP_HAS_VALID_SLOT(sprop,scope) SLOT_IN_SCOPE((sprop)->slot, scope)

#define SPROP_HAS_STUB_GETTER(sprop)    (!(sprop)->getter)
#define SPROP_HAS_STUB_SETTER(sprop)    (!(sprop)->setter)




#define SPROP_GET(cx,sprop,obj,obj2,vp)                                       \
    (((sprop)->attrs & JSPROP_GETTER)                                         \
     ? js_InternalGetOrSet(cx, obj, (sprop)->id,                              \
                           OBJECT_TO_JSVAL((sprop)->getter), JSACC_READ,      \
                           0, 0, vp)                                          \
     : (sprop)->getter(cx, OBJ_THIS_OBJECT(cx,obj), SPROP_USERID(sprop), vp))





#define SPROP_SET(cx,sprop,obj,obj2,vp)                                       \
    (((sprop)->attrs & JSPROP_SETTER)                                         \
     ? js_InternalGetOrSet(cx, obj, (sprop)->id,                              \
                           OBJECT_TO_JSVAL((sprop)->setter), JSACC_WRITE,     \
                           1, vp, vp)                                         \
     : ((sprop)->attrs & JSPROP_GETTER)                                       \
     ? (JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,                    \
                             JSMSG_GETTER_ONLY, NULL), JS_FALSE)              \
     : (sprop)->setter(cx, OBJ_THIS_OBJECT(cx,obj), SPROP_USERID(sprop), vp))


#define SPROP_IS_SHARED_PERMANENT(sprop)                                      \
    ((~(sprop)->attrs & (JSPROP_SHARED | JSPROP_PERMANENT)) == 0)

extern JSScope *
js_GetMutableScope(JSContext *cx, JSObject *obj);

extern JSScope *
js_NewScope(JSContext *cx, jsrefcount nrefs, JSObjectOps *ops, JSClass *clasp,
            JSObject *obj);

extern void
js_DestroyScope(JSContext *cx, JSScope *scope);

#define ID_TO_VALUE(id) (JSID_IS_ATOM(id) ? ATOM_JSID_TO_JSVAL(id) :          \
                         JSID_IS_OBJECT(id) ? OBJECT_JSID_TO_JSVAL(id) :      \
                         (jsval)(id))
#define HASH_ID(id)     (JSID_IS_ATOM(id) ? JSID_TO_ATOM(id)->number :        \
                         JSID_IS_OBJECT(id) ? (jsatomid) JSID_CLRTAG(id) :    \
                         (jsatomid) JSID_TO_INT(id))

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









































#ifndef jsobj_h___
#define jsobj_h___








#include "jshash.h" 
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

struct JSObjectMap {
    jsrefcount  nrefs;          
    JSObjectOps *ops;           
    uint32      freeslot;       
};


#define OBJ_LOOKUP_PROPERTY(cx,obj,id,objp,propp)                             \
    (obj)->map->ops->lookupProperty(cx,obj,id,objp,propp)
#define OBJ_DEFINE_PROPERTY(cx,obj,id,value,getter,setter,attrs,propp)        \
    (obj)->map->ops->defineProperty(cx,obj,id,value,getter,setter,attrs,propp)
#define OBJ_GET_PROPERTY(cx,obj,id,vp)                                        \
    (obj)->map->ops->getProperty(cx,obj,id,vp)
#define OBJ_SET_PROPERTY(cx,obj,id,vp)                                        \
    (obj)->map->ops->setProperty(cx,obj,id,vp)
#define OBJ_GET_ATTRIBUTES(cx,obj,id,prop,attrsp)                             \
    (obj)->map->ops->getAttributes(cx,obj,id,prop,attrsp)
#define OBJ_SET_ATTRIBUTES(cx,obj,id,prop,attrsp)                             \
    (obj)->map->ops->setAttributes(cx,obj,id,prop,attrsp)
#define OBJ_DELETE_PROPERTY(cx,obj,id,rval)                                   \
    (obj)->map->ops->deleteProperty(cx,obj,id,rval)
#define OBJ_DEFAULT_VALUE(cx,obj,hint,vp)                                     \
    (obj)->map->ops->defaultValue(cx,obj,hint,vp)
#define OBJ_ENUMERATE(cx,obj,enum_op,statep,idp)                              \
    (obj)->map->ops->enumerate(cx,obj,enum_op,statep,idp)
#define OBJ_CHECK_ACCESS(cx,obj,id,mode,vp,attrsp)                            \
    (obj)->map->ops->checkAccess(cx,obj,id,mode,vp,attrsp)


#define OBJ_THIS_OBJECT(cx,obj)                                               \
    ((obj)->map->ops->thisObject                                              \
     ? (obj)->map->ops->thisObject(cx,obj)                                    \
     : (obj))
#define OBJ_DROP_PROPERTY(cx,obj,prop)                                        \
    ((obj)->map->ops->dropProperty                                            \
     ? (obj)->map->ops->dropProperty(cx,obj,prop)                             \
     : (void)0)
#define OBJ_GET_REQUIRED_SLOT(cx,obj,slot)                                    \
    ((obj)->map->ops->getRequiredSlot                                         \
     ? (obj)->map->ops->getRequiredSlot(cx, obj, slot)                        \
     : JSVAL_VOID)
#define OBJ_SET_REQUIRED_SLOT(cx,obj,slot,v)                                  \
    ((obj)->map->ops->setRequiredSlot                                         \
     ? (obj)->map->ops->setRequiredSlot(cx, obj, slot, v)                     \
     : JS_TRUE)

#define OBJ_TO_INNER_OBJECT(cx,obj)                                           \
    JS_BEGIN_MACRO                                                            \
        JSClass *clasp_ = OBJ_GET_CLASS(cx, obj);                             \
        if (clasp_->flags & JSCLASS_IS_EXTENDED) {                            \
            JSExtendedClass *xclasp_ = (JSExtendedClass*)clasp_;              \
            if (xclasp_->innerObject)                                         \
                obj = xclasp_->innerObject(cx, obj);                          \
        }                                                                     \
    JS_END_MACRO

#define JS_INITIAL_NSLOTS   6





struct JSObject {
    JSObjectMap *map;
    jsval       fslots[JS_INITIAL_NSLOTS];
    jsval       *dslots;        
};

#define JSSLOT_PROTO        0
#define JSSLOT_PARENT       1
#define JSSLOT_CLASS        2
#define JSSLOT_PRIVATE      3
#define JSSLOT_START(clasp) (((clasp)->flags & JSCLASS_HAS_PRIVATE)           \
                             ? JSSLOT_PRIVATE + 1                             \
                             : JSSLOT_CLASS + 1)

#define JSSLOT_FREE(clasp)  (JSSLOT_START(clasp)                              \
                             + JSCLASS_RESERVED_SLOTS(clasp))







#define STOBJ_NSLOTS(obj)                                                     \
    ((obj)->dslots ? (uint32)(obj)->dslots[-1] : (uint32)JS_INITIAL_NSLOTS)

#define STOBJ_GET_SLOT(obj,slot)                                              \
    ((slot) < JS_INITIAL_NSLOTS                                               \
     ? (obj)->fslots[(slot)]                                                  \
     : (JS_ASSERT((slot) < (uint32)(obj)->dslots[-1]),                        \
        (obj)->dslots[(slot) - JS_INITIAL_NSLOTS]))

#define STOBJ_SET_SLOT(obj,slot,value)                                        \
    ((slot) < JS_INITIAL_NSLOTS                                               \
     ? (obj)->fslots[(slot)] = (value)                                        \
     : (JS_ASSERT((slot) < (uint32)(obj)->dslots[-1]),                        \
        (obj)->dslots[(slot) - JS_INITIAL_NSLOTS] = (value)))

#define STOBJ_GET_PROTO(obj) \
    JSVAL_TO_OBJECT((obj)->fslots[JSSLOT_PROTO])
#define STOBJ_SET_PROTO(obj,proto) \
    ((obj)->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto))

#define STOBJ_GET_PARENT(obj) \
    JSVAL_TO_OBJECT((obj)->fslots[JSSLOT_PARENT])
#define STOBJ_SET_PARENT(obj,parent) \
    ((obj)->fslots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(parent))

#define STOBJ_GET_CLASS(obj) \
    ((JSClass *)JSVAL_TO_PRIVATE((obj)->fslots[JSSLOT_CLASS]))

#define OBJ_CHECK_SLOT(obj,slot)                                              \
    JS_ASSERT(slot < (obj)->map->freeslot)





#define LOCKED_OBJ_NSLOTS(obj)                                                \
   JS_MIN((obj)->map->freeslot, STOBJ_NSLOTS(obj))

#define LOCKED_OBJ_GET_SLOT(obj,slot) \
    (OBJ_CHECK_SLOT(obj, slot), STOBJ_GET_SLOT(obj, slot))
#define LOCKED_OBJ_SET_SLOT(obj,slot,value) \
    (OBJ_CHECK_SLOT(obj, slot), STOBJ_SET_SLOT(obj, slot, value))

#define LOCKED_OBJ_GET_PROTO(obj) \
    (OBJ_CHECK_SLOT(obj, JSSLOT_PROTO), STOBJ_GET_PROTO(obj))
#define LOCKED_OBJ_SET_PROTO(obj,proto) \
    (OBJ_CHECK_SLOT(obj, JSSLOT_PROTO), STOBJ_SET_PROTO(obj, proto))

#define LOCKED_OBJ_GET_PARENT(obj) \
    (OBJ_CHECK_SLOT(obj, JSSLOT_PARENT), STOBJ_GET_PARENT(obj))
#define LOCKED_OBJ_SET_PARENT(obj,parent) \
    (OBJ_CHECK_SLOT(obj, JSSLOT_PARENT), STOBJ_SET_PARENT(obj, parent))

#define LOCKED_OBJ_GET_CLASS(obj) \
    (OBJ_CHECK_SLOT(obj, JSSLOT_CLASS), STOBJ_GET_CLASS(obj))

#ifdef JS_THREADSAFE


#define OBJ_GET_SLOT(cx,obj,slot)                                             \
    (OBJ_CHECK_SLOT(obj, slot),                                               \
     (OBJ_IS_NATIVE(obj) && OBJ_SCOPE(obj)->ownercx == cx)                    \
     ? LOCKED_OBJ_GET_SLOT(obj, slot)                                         \
     : js_GetSlotThreadSafe(cx, obj, slot))

#define OBJ_SET_SLOT(cx,obj,slot,value)                                       \
    (OBJ_CHECK_SLOT(obj, slot),                                               \
     (OBJ_IS_NATIVE(obj) && OBJ_SCOPE(obj)->ownercx == cx)                    \
     ? (void) LOCKED_OBJ_SET_SLOT(obj, slot, value)                           \
     : js_SetSlotThreadSafe(cx, obj, slot, value))













#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#define GC_AWARE_GET_SLOT(cx, obj, slot)                                      \
    ((OBJ_IS_NATIVE(obj) && CX_THREAD_IS_RUNNING_GC(cx))                      \
     ? STOBJ_GET_SLOT(obj, slot)                                              \
     : OBJ_GET_SLOT(cx, obj, slot))

#else   

#define OBJ_GET_SLOT(cx,obj,slot)       LOCKED_OBJ_GET_SLOT(obj,slot)
#define OBJ_SET_SLOT(cx,obj,slot,value) LOCKED_OBJ_SET_SLOT(obj,slot,value)
#define GC_AWARE_GET_SLOT(cx,obj,slot)  STOBJ_GET_SLOT(obj,slot)

#endif 

#define GC_AWARE_GET_PROTO(cx, obj)                                           \
    JSVAL_TO_OBJECT(GC_AWARE_GET_SLOT(cx, obj, JSSLOT_PROTO))

#define GC_AWARE_GET_PARENT(cx, obj)                                          \
    JSVAL_TO_OBJECT(GC_AWARE_GET_SLOT(cx, obj, JSSLOT_PARENT))


#define OBJ_GET_PROTO(cx,obj) \
    JSVAL_TO_OBJECT(OBJ_GET_SLOT(cx, obj, JSSLOT_PROTO))
#define OBJ_SET_PROTO(cx,obj,proto) \
    OBJ_SET_SLOT(cx, obj, JSSLOT_PROTO, OBJECT_TO_JSVAL(proto))

#define OBJ_GET_PARENT(cx,obj) \
    JSVAL_TO_OBJECT(OBJ_GET_SLOT(cx, obj, JSSLOT_PARENT))
#define OBJ_SET_PARENT(cx,obj,parent) \
    OBJ_SET_SLOT(cx, obj, JSSLOT_PARENT, OBJECT_TO_JSVAL(parent))





#define GC_AWARE_GET_CLASS(cx, obj)     STOBJ_GET_CLASS(obj)
#define OBJ_GET_CLASS(cx,obj)           STOBJ_GET_CLASS(obj)


#define MAP_IS_NATIVE(map)                                                    \
    ((map)->ops == &js_ObjectOps ||                                           \
     ((map)->ops && (map)->ops->newObjectMap == js_ObjectOps.newObjectMap))

#define OBJ_IS_NATIVE(obj)  MAP_IS_NATIVE((obj)->map)

extern JS_FRIEND_DATA(JSObjectOps) js_ObjectOps;
extern JS_FRIEND_DATA(JSObjectOps) js_WithObjectOps;
extern JSClass  js_ObjectClass;
extern JSClass  js_WithClass;
extern JSClass  js_BlockClass;

















#define JSSLOT_BLOCK_DEPTH      (JSSLOT_PRIVATE + 1)

#define OBJ_BLOCK_COUNT(cx,obj) \
    ((obj)->map->freeslot - (JSSLOT_BLOCK_DEPTH + 1))
#define OBJ_BLOCK_DEPTH(cx,obj) \
    JSVAL_TO_INT(OBJ_GET_SLOT(cx, obj, JSSLOT_BLOCK_DEPTH))
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth) \
    OBJ_SET_SLOT(cx, obj, JSSLOT_BLOCK_DEPTH, INT_TO_JSVAL(depth))









extern JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);







extern JSObject *
js_NewBlockObject(JSContext *cx);

extern JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSObject *parent,
                    JSStackFrame *fp);

extern JSBool
js_PutBlockObject(JSContext *cx, JSObject *obj);

struct JSSharpObjectMap {
    jsrefcount  depth;
    jsatomid    sharpgen;
    JSHashTable *table;
};

#define SHARP_BIT       ((jsatomid) 1)
#define BUSY_BIT        ((jsatomid) 2)
#define SHARP_ID_SHIFT  2
#define IS_SHARP(he)    (JS_PTR_TO_UINT32((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = JS_UINT32_TO_PTR(JS_PTR_TO_UINT32((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (JS_PTR_TO_UINT32((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = JS_UINT32_TO_PTR(JS_PTR_TO_UINT32((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = JS_UINT32_TO_PTR(JS_PTR_TO_UINT32((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);





extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_obj_toSource(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);

extern JSBool
js_obj_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSObject *obj, JSLookupPropOp lookup,
                        uintN argc, jsval *argv, jsval *rval);

extern JSObject*
js_InitBlockClass(JSContext *cx, JSObject* obj);

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);


extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];

extern void
js_InitObjectMap(JSObjectMap *map, jsrefcount nrefs, JSObjectOps *ops,
                 JSClass *clasp);

extern JSObjectMap *
js_NewObjectMap(JSContext *cx, jsrefcount nrefs, JSObjectOps *ops,
                JSClass *clasp, JSObject *obj);

extern void
js_DestroyObjectMap(JSContext *cx, JSObjectMap *map);

extern JSObjectMap *
js_HoldObjectMap(JSContext *cx, JSObjectMap *map);

extern JSObjectMap *
js_DropObjectMap(JSContext *cx, JSObjectMap *map, JSObject *obj);

extern JSBool
js_GetClassId(JSContext *cx, JSClass *clasp, jsid *idp);

extern JSObject *
js_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);




extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject *cobj);

extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, jsid id, jsval *vp);

extern JSObject *
js_ConstructObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, jsval *argv);

extern void
js_FinalizeObject(JSContext *cx, JSObject *obj);

extern JSBool
js_AllocSlot(JSContext *cx, JSObject *obj, uint32 *slotp);

extern void
js_FreeSlot(JSContext *cx, JSObject *obj, uint32 slot);







extern JSScopeProperty *
js_AddHiddenProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);

extern JSBool
js_LookupHiddenProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                        JSProperty **propp);





extern JSScopeProperty *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern JSScopeProperty *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             JSScopeProperty *sprop, uintN attrs, uintN mask,
                             JSPropertyOp getter, JSPropertyOp setter);








extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                  JSPropertyOp getter, JSPropertyOp setter, uintN attrs,
                  JSProperty **propp);

extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                        JSPropertyOp getter, JSPropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp);








extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);






extern JSBool
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp);

#define JSRESOLVE_HIDDEN        0x8000

extern JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, JSObject **objp, JSObject **pobjp,
                JSProperty **propp);

extern JSObject *
js_FindIdentifierBase(JSContext *cx, jsid id);

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);







extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj,
             JSScopeProperty *sprop, jsval *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, JSScopeProperty *sprop, jsval *vp);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *rval);

extern JSBool
js_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp);

extern JSIdArray *
js_NewIdArray(JSContext *cx, jsint length);




extern JSIdArray *
js_SetIdArrayLength(JSContext *cx, JSIdArray *ida, jsint length);

extern JSBool
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             jsval *statep, jsid *idp);

extern void
js_TraceNativeIteratorStates(JSTracer *trc);

extern JSBool
js_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               jsval *vp, uintN *attrsp);

extern JSBool
js_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

extern JSBool
js_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
             jsval *rval);

extern JSBool
js_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JSBool
js_SetProtoOrParent(JSContext *cx, JSObject *obj, uint32 slot, JSObject *pobj);

extern JSBool
js_IsDelegate(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JSBool
js_GetClassPrototype(JSContext *cx, JSObject *scope, jsid id,
                     JSObject **protop);

extern JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto,
                     uintN attrs);





extern JSBool
js_PrimitiveToObject(JSContext *cx, jsval *vp);

extern JSBool
js_ValueToObject(JSContext *cx, jsval v, JSObject **objp);

extern JSObject *
js_ValueToNonNullObject(JSContext *cx, jsval v);

extern JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, jsval *rval);

extern JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, jsval *argv, jsval *rval);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_TraceObject(JSTracer *trc, JSObject *obj);

extern void
js_Clear(JSContext *cx, JSObject *obj);

extern jsval
js_GetRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot);

extern JSBool
js_SetRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval v);

extern JSObject *
js_CheckScopeChainValidity(JSContext *cx, JSObject *scopeobj, const char *caller);

extern JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller);
JS_END_EXTERN_C

#endif 









































#ifndef jsobj_h___
#define jsobj_h___








#include "jshash.h" 
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C


struct JSObjectOps {
    




    const JSObjectMap   *objectMap;

    
    JSLookupPropOp      lookupProperty;
    JSDefinePropOp      defineProperty;
    JSPropertyIdOp      getProperty;
    JSPropertyIdOp      setProperty;
    JSAttributesOp      getAttributes;
    JSAttributesOp      setAttributes;
    JSPropertyIdOp      deleteProperty;
    JSConvertOp         defaultValue;
    JSNewEnumerateOp    enumerate;
    JSCheckAccessIdOp   checkAccess;

    
    JSObjectOp          thisObject;
    JSPropertyRefOp     dropProperty;
    JSNative            call;
    JSNative            construct;
    JSHasInstanceOp     hasInstance;
    JSTraceOp           trace;
    JSFinalizeOp        clear;
};

struct JSObjectMap {
    const JSObjectOps * const   ops;    
    uint32                      shape;  

    explicit JSObjectMap(const JSObjectOps *ops, uint32 shape) : ops(ops), shape(shape) {}

    enum { SHAPELESS = 0xffffffff };
};

const uint32 JS_INITIAL_NSLOTS = 5;

const uint32 JSSLOT_PROTO   = 0;
const uint32 JSSLOT_PARENT  = 1;








const uint32 JSSLOT_PRIVATE = 2;

const uint32 JSSLOT_PRIMITIVE_THIS = JSSLOT_PRIVATE;

const uintptr_t JSSLOT_CLASS_MASK_BITS = 3;

































struct JSObject {
    JSObjectMap *map;                       
    jsuword     classword;                  
    jsval       fslots[JS_INITIAL_NSLOTS];  
    jsval       *dslots;                    

    JSClass *getClass() const {
        return (JSClass *) (classword & ~JSSLOT_CLASS_MASK_BITS);
    }

    bool isDelegate() const {
        return (classword & jsuword(1)) != jsuword(0);
    }

    void setDelegate() {
        classword |= jsuword(1);
    }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const {
        return (classword & jsuword(2)) != jsuword(0);
    }

    void setSystem() {
        classword |= jsuword(2);
    }

    JSObject *getProto() const {
        return JSVAL_TO_OBJECT(fslots[JSSLOT_PROTO]);
    }

    void clearProto() {
        fslots[JSSLOT_PROTO] = JSVAL_NULL;
    }

    void setProto(JSObject *newProto) {
        setDelegateNullSafe(newProto);
        fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(newProto);
    }

    JSObject *getParent() const {
        return JSVAL_TO_OBJECT(fslots[JSSLOT_PARENT]);
    }

    void clearParent() {
        fslots[JSSLOT_PARENT] = JSVAL_NULL;
    }

    void setParent(JSObject *newParent) {
        setDelegateNullSafe(newParent);
        fslots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(newParent);
    }

    void traceProtoAndParent(JSTracer *trc) const {
        JSObject *proto = getProto();
        if (proto)
            JS_CALL_OBJECT_TRACER(trc, proto, "__proto__");

        JSObject *parent = getParent();
        if (parent)
            JS_CALL_OBJECT_TRACER(trc, parent, "__parent__");
    }

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        jsval v = fslots[JSSLOT_PRIVATE];
        JS_ASSERT((v & jsval(1)) == jsval(0));
        return reinterpret_cast<void *>(v);
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        jsval v = reinterpret_cast<jsval>(data);
        JS_ASSERT((v & jsval(1)) == jsval(0));
        fslots[JSSLOT_PRIVATE] = v;
    }

    static jsval defaultPrivate(JSClass *clasp) {
        return (clasp->flags & JSCLASS_HAS_PRIVATE)
               ? JSVAL_NULL
               : JSVAL_VOID;
    }

    
    void init(JSClass *clasp, JSObject *proto, JSObject *parent,
              jsval privateSlotValue) {
        JS_ASSERT(((jsuword) clasp & 3) == 0);
        JS_STATIC_ASSERT(JSSLOT_PRIVATE + 3 == JS_INITIAL_NSLOTS);
        JS_ASSERT_IF(clasp->flags & JSCLASS_HAS_PRIVATE,
                     (privateSlotValue & jsval(1)) == jsval(0));

        classword = jsuword(clasp);
        JS_ASSERT(!isDelegate());
        JS_ASSERT(!isSystem());

        setProto(proto);
        setParent(parent);
        fslots[JSSLOT_PRIVATE] = privateSlotValue;
        fslots[JSSLOT_PRIVATE + 1] = JSVAL_VOID;
        fslots[JSSLOT_PRIVATE + 2] = JSVAL_VOID;
        dslots = NULL;
    }

    JSBool lookupProperty(JSContext *cx, jsid id,
                          JSObject **objp, JSProperty **propp) {
        return map->ops->lookupProperty(cx, this, id, objp, propp);
    }

    JSBool defineProperty(JSContext *cx, jsid id, jsval value,
                          JSPropertyOp getter = JS_PropertyStub,
                          JSPropertyOp setter = JS_PropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        return map->ops->defineProperty(cx, this, id, value, getter, setter, attrs);
    }

    JSBool getProperty(JSContext *cx, jsid id, jsval *vp) {
        return map->ops->getProperty(cx, this, id, vp);
    }

    JSBool setProperty(JSContext *cx, jsid id, jsval *vp) {
        return map->ops->setProperty(cx, this, id, vp);
    }

    JSBool getAttributes(JSContext *cx, jsid id, JSProperty *prop,
                         uintN *attrsp) {
        return map->ops->getAttributes(cx, this, id, prop, attrsp);
    }

    JSBool setAttributes(JSContext *cx, jsid id, JSProperty *prop,
                         uintN *attrsp) {
        return map->ops->setAttributes(cx, this, id, prop, attrsp);
    }

    JSBool deleteProperty(JSContext *cx, jsid id, jsval *rval) {
        return map->ops->deleteProperty(cx, this, id, rval);
    }

    JSBool defaultValue(JSContext *cx, JSType hint, jsval *vp) {
        return map->ops->defaultValue(cx, this, hint, vp);
    }

    JSBool enumerate(JSContext *cx, JSIterateOp op, jsval *statep,
                     jsid *idp) {
        return map->ops->enumerate(cx, this, op, statep, idp);
    }

    JSBool checkAccess(JSContext *cx, jsid id, JSAccessMode mode, jsval *vp,
                       uintN *attrsp) {
        return map->ops->checkAccess(cx, this, id, mode, vp, attrsp);
    }

    
    JSObject *thisObject(JSContext *cx) {
        return map->ops->thisObject ? map->ops->thisObject(cx, this) : this;
    }

    void dropProperty(JSContext *cx, JSProperty *prop) {
        if (map->ops->dropProperty)
            map->ops->dropProperty(cx, this, prop);
    }
};


#define STOBJ_GET_PROTO(obj)            ((obj)->getProto())
#define STOBJ_SET_PROTO(obj,proto)      ((obj)->setProto(proto))
#define STOBJ_CLEAR_PROTO(obj)          ((obj)->clearProto())

#define STOBJ_GET_PARENT(obj)           ((obj)->getParent())
#define STOBJ_SET_PARENT(obj,parent)    ((obj)->setParent(parent))
#define STOBJ_CLEAR_PARENT(obj)         ((obj)->clearParent())

#define OBJ_GET_PROTO(cx,obj)           STOBJ_GET_PROTO(obj)
#define OBJ_SET_PROTO(cx,obj,proto)     STOBJ_SET_PROTO(obj, proto)
#define OBJ_CLEAR_PROTO(cx,obj)         STOBJ_CLEAR_PROTO(obj)

#define OBJ_GET_PARENT(cx,obj)          STOBJ_GET_PARENT(obj)
#define OBJ_SET_PARENT(cx,obj,parent)   STOBJ_SET_PARENT(obj, parent)
#define OBJ_CLEAR_PARENT(cx,obj)        STOBJ_CLEAR_PARENT(obj)

#define JSSLOT_START(clasp) (((clasp)->flags & JSCLASS_HAS_PRIVATE)           \
                             ? JSSLOT_PRIVATE + 1                             \
                             : JSSLOT_PRIVATE)

#define JSSLOT_FREE(clasp)  (JSSLOT_START(clasp)                              \
                             + JSCLASS_RESERVED_SLOTS(clasp))






#define MAX_DSLOTS_LENGTH   (JS_MAX(~uint32(0), ~size_t(0)) / sizeof(jsval) - 1)
#define MAX_DSLOTS_LENGTH32 (~uint32(0) / sizeof(jsval) - 1)







#define STOBJ_NSLOTS(obj)                                                     \
    ((obj)->dslots ? (uint32)(obj)->dslots[-1] : (uint32)JS_INITIAL_NSLOTS)

inline jsval&
STOBJ_GET_SLOT(JSObject *obj, uintN slot)
{
    return (slot < JS_INITIAL_NSLOTS)
           ? obj->fslots[slot]
           : (JS_ASSERT(slot < (uint32)obj->dslots[-1]),
              obj->dslots[slot - JS_INITIAL_NSLOTS]);
}

inline void
STOBJ_SET_SLOT(JSObject *obj, uintN slot, jsval value)
{
    if (slot < JS_INITIAL_NSLOTS) {
        obj->fslots[slot] = value;
    } else {
        JS_ASSERT(slot < (uint32)obj->dslots[-1]);
        obj->dslots[slot - JS_INITIAL_NSLOTS] = value;
    }
}

inline JSClass*
STOBJ_GET_CLASS(const JSObject* obj)
{
    return obj->getClass();
}

#define OBJ_CHECK_SLOT(obj,slot)                                              \
    (JS_ASSERT(OBJ_IS_NATIVE(obj)), JS_ASSERT(slot < OBJ_SCOPE(obj)->freeslot))

#define LOCKED_OBJ_GET_SLOT(obj,slot)                                         \
    (OBJ_CHECK_SLOT(obj, slot), STOBJ_GET_SLOT(obj, slot))
#define LOCKED_OBJ_SET_SLOT(obj,slot,value)                                   \
    (OBJ_CHECK_SLOT(obj, slot), STOBJ_SET_SLOT(obj, slot, value))

#ifdef JS_THREADSAFE


#define OBJ_GET_SLOT(cx,obj,slot)                                             \
    (OBJ_CHECK_SLOT(obj, slot),                                               \
     (OBJ_SCOPE(obj)->title.ownercx == cx)                                    \
     ? LOCKED_OBJ_GET_SLOT(obj, slot)                                         \
     : js_GetSlotThreadSafe(cx, obj, slot))

#define OBJ_SET_SLOT(cx,obj,slot,value)                                       \
    JS_BEGIN_MACRO                                                            \
        OBJ_CHECK_SLOT(obj, slot);                                            \
        if (OBJ_SCOPE(obj)->title.ownercx == cx)                              \
            LOCKED_OBJ_SET_SLOT(obj, slot, value);                            \
        else                                                                  \
            js_SetSlotThreadSafe(cx, obj, slot, value);                       \
    JS_END_MACRO













#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#else   

#define OBJ_GET_SLOT(cx,obj,slot)       LOCKED_OBJ_GET_SLOT(obj,slot)
#define OBJ_SET_SLOT(cx,obj,slot,value) LOCKED_OBJ_SET_SLOT(obj,slot,value)

#endif 





#define OBJ_GET_CLASS(cx,obj)           STOBJ_GET_CLASS(obj)





#define OPS_IS_NATIVE(ops)                                                    \
    JS_LIKELY((ops) == &js_ObjectOps || !(ops)->objectMap)

#define OBJ_IS_NATIVE(obj)  OPS_IS_NATIVE((obj)->map->ops)

#ifdef __cplusplus
inline void
OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj)
{
    JSClass *clasp = OBJ_GET_CLASS(cx, obj);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->innerObject)
            obj = xclasp->innerObject(cx, obj);
    }
}





inline void
OBJ_TO_OUTER_OBJECT(JSContext *cx, JSObject *&obj)
{
    JSClass *clasp = OBJ_GET_CLASS(cx, obj);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->outerObject)
            obj = xclasp->outerObject(cx, obj);
    }
}
#endif

extern JS_FRIEND_DATA(JSObjectOps) js_ObjectOps;
extern JS_FRIEND_DATA(JSObjectOps) js_WithObjectOps;
extern JSClass  js_ObjectClass;
extern JSClass  js_WithClass;
extern JSClass  js_BlockClass;
















#define JSSLOT_BLOCK_DEPTH      (JSSLOT_PRIVATE + 1)

static inline bool
OBJ_IS_CLONED_BLOCK(JSObject *obj)
{
    return obj->getProto() != NULL;
}

extern JSBool
js_DefineBlockVariable(JSContext *cx, JSObject *obj, jsid id, intN index);

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    (OBJ_SCOPE(obj)->entryCount)
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    JSVAL_TO_INT(STOBJ_GET_SLOT(obj, JSSLOT_BLOCK_DEPTH))
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    STOBJ_SET_SLOT(obj, JSSLOT_BLOCK_DEPTH, INT_TO_JSVAL(depth))









extern JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);







extern JSObject *
js_NewBlockObject(JSContext *cx);

extern JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp);

extern JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind);

JSBool
js_XDRBlockObject(JSXDRState *xdr, JSObject **objp);

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
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        jsval *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  JSBool *foundp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSObject *
js_InitEval(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             JSClass *clasp, JSNative constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];

extern JSBool
js_GetClassId(JSContext *cx, JSClass *clasp, jsid *idp);

extern JSObject *
js_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto,
             JSObject *parent, size_t objectSize = 0);




extern JSObject *
js_NewObjectWithGivenProto(JSContext *cx, JSClass *clasp, JSObject *proto,
                           JSObject *parent, size_t objectSize = 0);










extern JSObject*
js_NewNativeObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   jsval privateSlotValue);




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

extern JSBool
js_AllocSlot(JSContext *cx, JSObject *obj, uint32 *slotp);

extern void
js_FreeSlot(JSContext *cx, JSObject *obj, uint32 slot);

extern bool
js_GrowSlots(JSContext *cx, JSObject *obj, size_t nslots);

extern void
js_ShrinkSlots(JSContext *cx, JSObject *obj, size_t nslots);

static inline void
js_FreeSlots(JSContext *cx, JSObject *obj)
{
    if (obj->dslots)
        js_ShrinkSlots(cx, obj, 0);
}









bool
js_EnsureReservedSlots(JSContext *cx, JSObject *obj, size_t nreserved);

extern jsid
js_CheckForStringIndex(jsid id);







extern void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id);

#ifdef __cplusplus 
static JS_INLINE void
js_PurgeScopeChain(JSContext *cx, JSObject *obj, jsid id)
{
    if (obj->isDelegate())
        js_PurgeScopeChainHelper(cx, obj, id);
}
#endif





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
                  JSPropertyOp getter, JSPropertyOp setter, uintN attrs);




const uintN JSDNP_CACHE_RESULT = 1; 
const uintN JSDNP_DONT_PURGE   = 2; 
const uintN JSDNP_SET_METHOD   = 4; 










extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                        JSPropertyOp getter, JSPropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow = 0);








extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);






extern int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp);







static inline bool
js_IsCacheableNonGlobalScope(JSObject *obj)
{
    extern JS_FRIEND_DATA(JSClass) js_CallClass;
    extern JS_FRIEND_DATA(JSClass) js_DeclEnvClass;
    JS_ASSERT(STOBJ_GET_PARENT(obj));

    JSClass *clasp = STOBJ_GET_CLASS(obj);
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, obj->map->ops->lookupProperty == js_LookupProperty);
    return cacheable;
}




extern JSPropCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp);





extern JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, JSObject **objp, JSObject **pobjp,
                JSProperty **propp);

extern JS_REQUIRES_STACK JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id);

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);














const uintN JSGET_CACHE_RESULT      = 1; 
const uintN JSGET_METHOD_BARRIER    = 0; 
const uintN JSGET_NO_METHOD_BARRIER = 2; 







extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj,
             JSScopeProperty *sprop, uintN getHow, jsval *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, JSScopeProperty *sprop, bool added,
             jsval *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN getHow,
                     jsval *vp);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, jsval *vp);





extern JS_FRIEND_API(JSBool)
js_CheckUndeclaredVarAssignment(JSContext *cx);

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     jsval *vp);

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

extern JSBool
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             jsval *statep, jsid *idp);

extern void
js_MarkEnumeratorState(JSTracer *trc, JSObject *obj, jsval state);

extern void
js_PurgeCachedNativeEnumerators(JSContext *cx, JSThreadData *data);

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
js_SetProtoOrParent(JSContext *cx, JSObject *obj, uint32 slot, JSObject *pobj,
                    JSBool checkForCycles);

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
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern void
js_Clear(JSContext *cx, JSObject *obj);

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, jsval *vp);

bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, jsval v);




extern JSBool
js_ReallocSlots(JSContext *cx, JSObject *obj, uint32 nslots,
                JSBool exactAllocation);

extern JSObject *
js_CheckScopeChainValidity(JSContext *cx, JSObject *scopeobj, const char *caller);

extern JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller);


extern JSObject *
js_GetWrappedObject(JSContext *cx, JSObject *obj);


extern const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop);


extern JSBool
js_IsCallable(JSObject *obj, JSContext *cx);

void
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
















static inline bool
js_ObjectIsSimilarToProto(JSContext *cx, JSObject *obj, const JSObjectOps *ops,
                          JSClass *clasp, JSObject *proto)
{
    JS_ASSERT(proto == OBJ_GET_PROTO(cx, obj));

    JSClass *protoclasp;
    return (proto->map->ops == ops &&
            ((protoclasp = OBJ_GET_CLASS(cx, proto)) == clasp ||
             (!((protoclasp->flags ^ clasp->flags) &
                (JSCLASS_HAS_PRIVATE |
                 (JSCLASS_RESERVED_SLOTS_MASK << JSCLASS_RESERVED_SLOTS_SHIFT))) &&
              protoclasp->reserveSlots == clasp->reserveSlots)));
}

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpValue(jsval val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpStackFrame(JSStackFrame *fp);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JS_END_EXTERN_C

#endif 

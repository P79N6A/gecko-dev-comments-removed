







































#ifndef jsobj_h___
#define jsobj_h___








#include "jsapi.h"
#include "jshash.h" 
#include "jspubtd.h"
#include "jsprvtd.h"

namespace js {

class AutoDescriptorArray;

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

extern JSBool
PropertyStub(JSContext *cx, JSObject *obj, jsid id, Value *vp);

extern JSBool
EnumerateStub(JSContext *cx, JSObject *obj);

extern JSBool
ResolveStub(JSContext *cx, JSObject *obj, jsid id);

extern JSBool
ConvertStub(JSContext *cx, JSObject *obj, JSType type, Value *vp);

extern void
FinalizeStub(JSContext *cx, JSObject *obj);

} 





struct PropertyDescriptor {
  friend class js::AutoDescriptorArray;

  private:
    PropertyDescriptor();

  public:
    
    bool initialize(JSContext* cx, jsid id, const js::Value &v);

    
    bool isAccessorDescriptor() const {
        return hasGet || hasSet;
    }

    
    bool isDataDescriptor() const {
        return hasValue || hasWritable;
    }

    
    bool isGenericDescriptor() const {
        return !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }

    JSObject* getterObject() const {
        return get.isUndefined() ? NULL : &get.asObject();
    }
    JSObject* setterObject() const {
        return set.isUndefined() ? NULL : &set.asObject();
    }

    const js::Value &getterValue() const {
        return get;
    }
    const js::Value &setterValue() const {
        return set;
    }

    js::PropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    js::PropertyOp setter() const {
        return js::CastAsPropertyOp(setterObject());
    }

    static void traceDescriptorArray(JSTracer* trc, JSObject* obj);
    static void finalizeDescriptorArray(JSContext* cx, JSObject* obj);

    jsid id;
    js::Value value, get, set;

    
    uint8 attrs;

    
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;
};


struct JSObjectOps {
    




    const JSObjectMap   *objectMap;

    
    JSLookupPropOp      lookupProperty;
    js::DefinePropOp    defineProperty;
    js::PropertyIdOp    getProperty;
    js::PropertyIdOp    setProperty;
    JSAttributesOp      getAttributes;
    JSAttributesOp      setAttributes;
    js::PropertyIdOp    deleteProperty;
    js::ConvertOp       defaultValue;
    js::NewEnumerateOp  enumerate;
    js::CheckAccessIdOp checkAccess;
    JSTypeOfOp          typeOf;
    JSTraceOp           trace;

    
    JSObjectOp          thisObject;
    JSPropertyRefOp     dropProperty;
    js::Native          call;
    js::Native          construct;
    js::HasInstanceOp   hasInstance;
    JSFinalizeOp        clear;

    bool inline isNative() const;
};

extern JS_FRIEND_DATA(JSObjectOps) js_ObjectOps;
extern JS_FRIEND_DATA(JSObjectOps) js_WithObjectOps;





inline bool
JSObjectOps::isNative() const
{
    return JS_LIKELY(this == &js_ObjectOps) || !objectMap;
}

struct JSObjectMap {
    const JSObjectOps * const   ops;    
    uint32                      shape;  

    explicit JSObjectMap(const JSObjectOps *ops, uint32 shape) : ops(ops), shape(shape) {}

    enum { SHAPELESS = 0xffffffff };

private:
    
    JSObjectMap(JSObjectMap &);
    void operator=(JSObjectMap &);
};

struct NativeIterator;

const uint32 JS_INITIAL_NSLOTS = 5;

const uint32 JSSLOT_PROTO   = 0;
const uint32 JSSLOT_PARENT  = 1;








const uint32 JSSLOT_PRIVATE = 2;




























struct JSObject {
    



    friend class js::TraceRecorder;

    JSObjectMap *map;                       
  private:
    js::Class   *clasp;                     
    jsuword     flags;                      
  public:
    js::Value   *dslots;                    
    js::Value   fslots[JS_INITIAL_NSLOTS];  

    bool isNative() const { return map->ops->isNative(); }

    js::Class *getClass() const {
        return clasp;
    }

    bool hasClass(const js::Class *c) const {
        return c == clasp;
    }

    inline JSScope *scope() const;
    inline uint32 shape() const;

    bool isDelegate() const {
        return (flags & jsuword(1)) != jsuword(0);
    }

    void setDelegate() {
        flags |= jsuword(1);
    }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const {
        return (flags & jsuword(2)) != jsuword(0);
    }

    void setSystem() {
        flags |= jsuword(2);
    }

    uint32 numSlots(void) const {
        return dslots ? dslots[-1].asPrivateUint32() : (uint32)JS_INITIAL_NSLOTS;
    }

  private:
    static size_t slotsToDynamicWords(size_t nslots) {
        JS_ASSERT(nslots > JS_INITIAL_NSLOTS);
        return nslots + 1 - JS_INITIAL_NSLOTS;
    }

    static size_t dynamicWordsToSlots(size_t nwords) {
        JS_ASSERT(nwords > 1);
        return nwords - 1 + JS_INITIAL_NSLOTS;
    }

  public:
    bool allocSlots(JSContext *cx, size_t nslots);
    bool growSlots(JSContext *cx, size_t nslots);
    void shrinkSlots(JSContext *cx, size_t nslots);

    js::Value& getSlotRef(uintN slot) {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < dslots[-1].asPrivateUint32()),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    const js::Value &getSlot(uintN slot) const {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < dslots[-1].asPrivateUint32()),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    void setSlot(uintN slot, const js::Value &value) {
        if (slot < JS_INITIAL_NSLOTS) {
            fslots[slot] = value;
        } else {
            JS_ASSERT(slot < dslots[-1].asPrivateUint32());
            dslots[slot - JS_INITIAL_NSLOTS] = value;
        }
    }

    inline const js::Value &lockedGetSlot(uintN slot) const;
    inline void lockedSetSlot(uintN slot, const js::Value &value);

    





    inline const js::Value &getSlotMT(JSContext *cx, uintN slot);
    inline void setSlotMT(JSContext *cx, uintN slot, const js::Value &value);

    JSObject *getProto() const {
        return fslots[JSSLOT_PROTO].asObjectOrNull();
    }

    const js::Value &getProtoValue() const {
        return fslots[JSSLOT_PROTO];
    }

    void clearProto() {
        fslots[JSSLOT_PROTO].setNull();
    }

    void setProto(const js::Value &newProto) {
        setDelegateNullSafe(newProto.asObjectOrNull());
        fslots[JSSLOT_PROTO] = newProto;
    }

    JSObject *getParent() const {
        return fslots[JSSLOT_PARENT].asObjectOrNull();
    }

    const js::Value &getParentValue() const {
        return fslots[JSSLOT_PARENT];
    }

    void clearParent() {
        fslots[JSSLOT_PARENT].setNull();
    }

    void setParent(const js::Value &newParent) {
        setDelegateNullSafe(newParent.asObjectOrNull());
        fslots[JSSLOT_PARENT] = newParent;
    }

    void traceProtoAndParent(JSTracer *trc) const {
        JSObject *proto = getProto();
        if (proto)
            JS_CALL_OBJECT_TRACER(trc, proto, "__proto__");

        JSObject *parent = getParent();
        if (parent)
            JS_CALL_OBJECT_TRACER(trc, parent, "parent");
    }

    JSObject *getGlobal();

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        void *priv = fslots[JSSLOT_PRIVATE].asPrivateVoidPtr();
        return priv;
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        JS_ASSERT((size_t(data) & 1) == 0);
        fslots[JSSLOT_PRIVATE].setPrivateVoidPtr(data);
    }

    static js::Value defaultPrivate(js::Class *clasp) {
        if (clasp->flags & JSCLASS_HAS_PRIVATE)
            return js::PrivateVoidPtrTag(NULL);
        return js::UndefinedTag();
    }

    



  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = JSSLOT_PRIVATE;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

    



  private:
    
    static const uint32 JSSLOT_ARRAY_LENGTH = JSSLOT_PRIVATE;

    
    static const uint32 JSSLOT_DENSE_ARRAY_COUNT     = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_DENSE_ARRAY_MINLENCAP = JSSLOT_PRIVATE + 2;

    
    
    
    inline void staticAssertArrayLengthIsInPrivateSlot();

    inline uint32 uncheckedGetArrayLength() const;
    inline uint32 uncheckedGetDenseArrayCapacity() const;

  public:
    inline uint32 getArrayLength() const;
    inline void setDenseArrayLength(uint32 length);
    inline void setSlowArrayLength(uint32 length);

    inline uint32 getDenseArrayCount() const;
    inline void setDenseArrayCount(uint32 count);
    inline void incDenseArrayCountBy(uint32 posDelta);
    inline void decDenseArrayCountBy(uint32 negDelta);

    inline uint32 getDenseArrayCapacity() const;
    inline void setDenseArrayCapacity(uint32 capacity); 

    inline bool isDenseArrayMinLenCapOk(bool strictAboutLength = true) const;

    inline const js::Value &getDenseArrayElement(uint32 i) const;
    inline js::Value *addressOfDenseArrayElement(uint32 i);
    inline void setDenseArrayElement(uint32 i, const js::Value &v);

    inline js::Value *getDenseArrayElements() const;   
    bool resizeDenseArrayElements(JSContext *cx, uint32 oldcap, uint32 newcap,
                               bool initializeAllSlots = true);
    bool ensureDenseArrayElements(JSContext *cx, uint32 newcap,
                               bool initializeAllSlots = true);
    inline void freeDenseArrayElements(JSContext *cx);

    inline void voidDenseOnlyArraySlots();  

    



    












  private:
    static const uint32 JSSLOT_ARGS_LENGTH = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_ARGS_CALLEE = JSSLOT_PRIVATE + 2;

  public:
    
    static const uint32 ARGS_FIXED_RESERVED_SLOTS = 2;

    inline uint32 getArgsLength() const;
    inline void setArgsLength(uint32 argc);
    inline void setArgsLengthOverridden();
    inline bool isArgsLengthOverridden() const;

    inline const js::Value &getArgsCallee() const;
    inline void setArgsCallee(const js::Value &callee);

    inline const js::Value &getArgsElement(uint32 i) const;
    inline js::Value *addressOfArgsElement(uint32 i) const;
    inline void setArgsElement(uint32 i, const js::Value &v);

    



  private:
    
    static const uint32 JSSLOT_DATE_UTC_TIME   = JSSLOT_PRIVATE;
    static const uint32 JSSLOT_DATE_LOCAL_TIME = JSSLOT_PRIVATE + 1;

  public:
    static const uint32 DATE_FIXED_RESERVED_SLOTS = 2;

    inline const js::Value &getDateLocalTime() const;
    inline void setDateLocalTime(const js::Value &pthis);

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    



  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = JSSLOT_PRIVATE + 1;

  public:
    static const uint32 REGEXP_FIXED_RESERVED_SLOTS = 1;

    inline const js::Value &getRegExpLastIndex() const;
    inline void setRegExpLastIndex(const js::Value &v);
    inline void zeroRegExpLastIndex();

    



    inline NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(NativeIterator *);

    



    bool isCallable();

    
    void init(js::Class *aclasp, const js::Value &proto, const js::Value &parent,
              const js::Value &privateSlotValue) {
        JS_STATIC_ASSERT(JSSLOT_PRIVATE + 3 == JS_INITIAL_NSLOTS);

        clasp = aclasp;
        flags = 0;
        JS_ASSERT(!isDelegate());
        JS_ASSERT(!isSystem());

        setProto(proto);
        setParent(parent);
        fslots[JSSLOT_PRIVATE] = privateSlotValue;
        fslots[JSSLOT_PRIVATE + 1].setUndefined();
        fslots[JSSLOT_PRIVATE + 2].setUndefined();
        dslots = NULL;
    }

    



    inline void initSharingEmptyScope(js::Class *clasp,
                                      const js::Value &proto,
                                      const js::Value &parent,
                                      const js::Value &privateSlotValue);

    inline bool hasSlotsArray() const { return !!dslots; }

    
    inline void freeSlotsArray(JSContext *cx);

    JSBool lookupProperty(JSContext *cx, jsid id,
                          JSObject **objp, JSProperty **propp) {
        return map->ops->lookupProperty(cx, this, id, objp, propp);
    }

    JSBool defineProperty(JSContext *cx, jsid id, const js::Value &value,
                          js::PropertyOp getter = js::PropertyStub,
                          js::PropertyOp setter = js::PropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        return map->ops->defineProperty(cx, this, id, &value, getter, setter, attrs);
    }

    JSBool getProperty(JSContext *cx, jsid id, js::Value *vp) {
        return map->ops->getProperty(cx, this, id, vp);
    }

    JSBool setProperty(JSContext *cx, jsid id, js::Value *vp) {
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

    JSBool deleteProperty(JSContext *cx, jsid id, js::Value *rval) {
        return map->ops->deleteProperty(cx, this, id, rval);
    }

    JSBool defaultValue(JSContext *cx, JSType hint, js::Value *vp) {
        return map->ops->defaultValue(cx, this, hint, vp);
    }

    JSBool enumerate(JSContext *cx, JSIterateOp op, js::Value *statep,
                     jsid *idp) {
        return map->ops->enumerate(cx, this, op, statep, idp);
    }

    JSBool checkAccess(JSContext *cx, jsid id, JSAccessMode mode, js::Value *vp,
                       uintN *attrsp) {
        return map->ops->checkAccess(cx, this, id, mode, vp, attrsp);
    }

    JSType typeOf(JSContext *cx) {
        return map->ops->typeOf(cx, this);
    }

    inline JSObject *thisObject(JSContext *cx);
    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    void dropProperty(JSContext *cx, JSProperty *prop) {
        if (map->ops->dropProperty)
            map->ops->dropProperty(cx, this, prop);
    }

    inline bool isArguments() const;
    inline bool isArray() const;
    inline bool isDenseArray() const;
    inline bool isSlowArray() const;
    inline bool isNumber() const;
    inline bool isBoolean() const;
    inline bool isString() const;
    inline bool isPrimitive() const;
    inline bool isDate() const;
    inline bool isFunction() const;
    inline bool isRegExp() const;
    inline bool isXML() const;

    inline bool unbrand(JSContext *cx);

    inline void initArrayClass();
    inline void changeClassToSlowArray();
    inline void changeClassToFastArray();
};

#define JSSLOT_START(clasp) (((clasp)->flags & JSCLASS_HAS_PRIVATE)           \
                             ? JSSLOT_PRIVATE + 1                             \
                             : JSSLOT_PRIVATE)

#define JSSLOT_FREE(clasp)  (JSSLOT_START(clasp)                              \
                             + JSCLASS_RESERVED_SLOTS(clasp))






#define MAX_DSLOTS_LENGTH   (JS_MAX(~uint32(0), ~size_t(0)) / sizeof(js::Value) - 1)
#define MAX_DSLOTS_LENGTH32 (~uint32(0) / sizeof(js::Value) - 1)

#define OBJ_CHECK_SLOT(obj,slot)                                              \
    (JS_ASSERT((obj)->isNative()), JS_ASSERT(slot < (obj)->scope()->freeslot))

#ifdef JS_THREADSAFE






#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif 


inline void
Innerize(JSContext *cx, JSObject **ppobj)
{
    JSObject *pobj = *ppobj;
    js::Class *clasp = pobj->getClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->innerObject)
            *ppobj = xclasp->innerObject(cx, pobj);
    }
}

inline void
Outerize(JSContext *cx, JSObject **ppobj)
{
    JSObject *pobj = *ppobj;
    js::Class *clasp = pobj->getClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->outerObject)
            *ppobj = xclasp->outerObject(cx, pobj);
    }
}

extern js::Class js_ObjectClass;
extern js::Class js_WithClass;
extern js::Class js_BlockClass;
















#define JSSLOT_BLOCK_DEPTH      (JSSLOT_PRIVATE + 1)

static inline bool
OBJ_IS_CLONED_BLOCK(JSObject *obj)
{
    return obj->getProto() != NULL;
}

extern JSBool
js_DefineBlockVariable(JSContext *cx, JSObject *obj, jsid id, intN index);

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    ((OBJ_IS_CLONED_BLOCK(obj) ? obj->getProto() : obj)->scope()->entryCount)
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    obj->getSlot(JSSLOT_BLOCK_DEPTH).asInt32()
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    obj->setSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Tag(depth)))









extern JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);

inline JSObject *
js_UnwrapWithObject(JSContext *cx, JSObject *withobj)
{
    JS_ASSERT(withobj->getClass() == &js_WithClass);
    return withobj->getProto();
}







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
#define IS_SHARP(he)    (uintptr_t((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = (void *) (uintptr_t((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (uintptr_t((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = (void *) (uintptr_t((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = (void *) (uintptr_t((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);





extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSObject *
js_InitEval(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             js::Class *clasp, js::Native constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif













extern JSObject*
js_NewObjectWithClassProto(JSContext *cx, js::Class *clasp, JSObject *proto,
                           const js::Value &privateSlotValue);




extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject *cobj);





extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key,
                   js::Value *vp, js::Class *clasp = NULL);

extern JSObject *
js_ConstructObject(JSContext *cx, js::Class *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, js::Value *argv);

extern JSBool
js_AllocSlot(JSContext *cx, JSObject *obj, uint32 *slotp);

extern void
js_FreeSlot(JSContext *cx, JSObject *obj, uint32 slot);









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
                     js::PropertyOp getter, js::PropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern JSScopeProperty *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             JSScopeProperty *sprop, uintN attrs, uintN mask,
                             js::PropertyOp getter, js::PropertyOp setter);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  js::PropertyOp getter, js::PropertyOp setter, uintN attrs);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);




const uintN JSDNP_CACHE_RESULT = 1; 
const uintN JSDNP_DONT_PURGE   = 2; 
const uintN JSDNP_SET_METHOD   = 4; 


const uintN JSDNP_UNQUALIFIED  = 8; 










extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value &value,
                        js::PropertyOp getter, js::PropertyOp setter, uintN attrs,
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
    extern JS_FRIEND_DATA(js::Class) js_CallClass;
    extern JS_FRIEND_DATA(js::Class) js_DeclEnvClass;
    JS_ASSERT(obj->getParent());

    js::Class *clasp = obj->getClass();
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, obj->map->ops->lookupProperty == js_LookupProperty);
    return cacheable;
}




extern js::PropertyCacheEntry *
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
             JSScopeProperty *sprop, uintN getHow, js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, JSScopeProperty *sprop, bool added,
             js::Value *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN getHow,
                     js::Value *vp);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);







extern JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     js::Value *vp);

extern JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *rval);

extern JSBool
js_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, js::Value *vp);

extern JSBool
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             js::Value *statep, jsid *idp);

extern JSBool
js_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               js::Value *vp, uintN *attrsp);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

extern JSBool
js_Call(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv,
        js::Value *rval);

extern JSBool
js_Construct(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv,
             js::Value *rval);

extern JSBool
js_HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

extern JSBool
js_SetProtoOrParent(JSContext *cx, JSObject *obj, uint32 slot, JSObject *pobj,
                    JSBool checkForCycles);

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);





extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);

extern JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto,
                     uintN attrs);





extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);





extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, js::Value *vp);





extern JSBool
js_ValueToNonNullObject(JSContext *cx, const js::Value &v, js::Value *vp);

extern JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, js::Value *rval);

extern JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, js::Value *argv, js::Value *rval);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_TraceObject(JSTracer *trc, JSObject *obj);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern void
js_Clear(JSContext *cx, JSObject *obj);

#ifdef JS_THREADSAFE
#define NATIVE_DROP_PROPERTY js_DropProperty

extern void
js_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop);
#else
#define NATIVE_DROP_PROPERTY NULL
#endif

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, js::Value *vp);

bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, const js::Value &v);




extern JSBool
js_ReallocSlots(JSContext *cx, JSObject *obj, uint32 nslots,
                JSBool exactAllocation);

extern JSObject *
js_CheckScopeChainValidity(JSContext *cx, JSObject *scopeobj, const char *caller);

extern JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller);


extern JSBool
js_CheckContentSecurityPolicy(JSContext *cx);


extern JSObject *
js_GetWrappedObject(JSContext *cx, JSObject *obj);


extern const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop);

static inline bool
js_IsCallable(const js::Value &v) {
    return v.isObject() && v.asObject().isCallable();
}

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

#ifdef DEBUG
namespace js {
JS_FRIEND_API(void) DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) DumpString(JSString *str);
JS_FRIEND_API(void) DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) DumpValue(const js::Value &val);
JS_FRIEND_API(void) DumpId(jsid id);
JS_FRIEND_API(void) DumpObject(JSObject *obj);
JS_FRIEND_API(void) DumpStackFrameChain(JSContext *cx, JSStackFrame *start = NULL);
}
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv, js::Value *rval);

#endif 

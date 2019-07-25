







































#ifndef jsobj_h___
#define jsobj_h___








#include "jsapi.h"
#include "jshash.h" 
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jsvector.h"

namespace js { class AutoDescriptorArray; }





struct PropertyDescriptor {
    friend class js::AutoDescriptorArray;

    PropertyDescriptor();

  public:
    
    bool initialize(JSContext* cx, jsid id, jsval v);

    
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
        return (get != JSVAL_VOID) ? JSVAL_TO_OBJECT(get) : NULL;
    }
    JSObject* setterObject() const {
        return (set != JSVAL_VOID) ? JSVAL_TO_OBJECT(set) : NULL;
    }

    jsval getterValue() const {
        return get;
    }
    jsval setterValue() const {
        return set;
    }

    JSPropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    JSPropertyOp setter() const {
        return js::CastAsPropertyOp(setterObject());
    }

    static void traceDescriptorArray(JSTracer* trc, JSObject* obj);
    static void finalizeDescriptorArray(JSContext* cx, JSObject* obj);

    jsval pd;
    jsid id;
    jsval value, get, set;

    
    uint8 attrs;

    
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;
};

namespace js {
    typedef Vector<PropertyDescriptor, 1> PropertyDescriptorArray;
}

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
    JSTypeOfOp          typeOf;
    JSTraceOp           trace;

    
    JSObjectOp          thisObject;
    JSNative            call;
    JSNative            construct;
    JSHasInstanceOp     hasInstance;
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

const uintptr_t JSSLOT_CLASS_MASK_BITS = 3;

































struct JSObject {
    



    friend class js::TraceRecorder;

    JSObjectMap *map;                       
    jsuword     classword;                  
    jsval       fslots[JS_INITIAL_NSLOTS];  
    jsval       *dslots;                    

    bool isNative() const { return map->ops->isNative(); }

    JSClass *getClass() const {
        return (JSClass *) (classword & ~JSSLOT_CLASS_MASK_BITS);
    }

    bool hasClass(const JSClass *clasp) const {
        return clasp == getClass();
    }

    inline JSScope *scope() const;
    inline uint32 shape() const;

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

    uint32 numSlots(void) const {
        return dslots ? (uint32)dslots[-1] : (uint32)JS_INITIAL_NSLOTS;
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

    jsval& getSlotRef(uintN slot) {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < (uint32)dslots[-1]),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    jsval getSlot(uintN slot) const {
        return (slot < JS_INITIAL_NSLOTS)
               ? fslots[slot]
               : (JS_ASSERT(slot < (uint32)dslots[-1]),
                  dslots[slot - JS_INITIAL_NSLOTS]);
    }

    void setSlot(uintN slot, jsval value) {
        if (slot < JS_INITIAL_NSLOTS) {
            fslots[slot] = value;
        } else {
            JS_ASSERT(slot < (uint32)dslots[-1]);
            dslots[slot - JS_INITIAL_NSLOTS] = value;
        }
    }

    inline jsval lockedGetSlot(uintN slot) const;
    inline void lockedSetSlot(uintN slot, jsval value);

    





    inline jsval getSlotMT(JSContext *cx, uintN slot);
    inline void setSlotMT(JSContext *cx, uintN slot, jsval value);

    inline jsval getReservedSlot(uintN index) const;

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
            JS_CALL_OBJECT_TRACER(trc, parent, "parent");
    }

    JSObject *getGlobal();

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

    



  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = JSSLOT_PRIVATE;

  public:
    inline jsval getPrimitiveThis() const;
    inline void setPrimitiveThis(jsval pthis);

    



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

    inline jsval getDenseArrayElement(uint32 i) const;
    inline jsval *addressOfDenseArrayElement(uint32 i);
    inline void setDenseArrayElement(uint32 i, jsval v);

    inline jsval *getDenseArrayElements() const;   
    bool resizeDenseArrayElements(JSContext *cx, uint32 oldcap, uint32 newcap,
                               bool initializeAllSlots = true);
    bool ensureDenseArrayElements(JSContext *cx, uint32 newcap,
                               bool initializeAllSlots = true);
    inline void freeDenseArrayElements(JSContext *cx);

    inline void voidDenseOnlyArraySlots();  

    JSBool makeDenseArraySlow(JSContext *cx);

    



  private:
    












    static const uint32 JSSLOT_ARGS_LENGTH = JSSLOT_PRIVATE + 1;
    static const uint32 JSSLOT_ARGS_CALLEE = JSSLOT_PRIVATE + 2;

  public:
    
    static const uint32 ARGS_FIXED_RESERVED_SLOTS = 2;

    inline uint32 getArgsLength() const;
    inline void setArgsLength(uint32 argc);
    inline void setArgsLengthOverridden();
    inline bool isArgsLengthOverridden() const;

    inline jsval getArgsCallee() const;
    inline void setArgsCallee(jsval callee);

    inline jsval getArgsElement(uint32 i) const;
    inline void setArgsElement(uint32 i, jsval v);

    



  private:
    
    static const uint32 JSSLOT_DATE_UTC_TIME   = JSSLOT_PRIVATE;
    static const uint32 JSSLOT_DATE_LOCAL_TIME = JSSLOT_PRIVATE + 1;

  public:
    static const uint32 DATE_FIXED_RESERVED_SLOTS = 2;

    inline jsval getDateLocalTime() const;
    inline jsval *addressOfDateLocalTime();
    inline void setDateLocalTime(jsval pthis);

    inline jsval getDateUTCTime() const;
    inline jsval *addressOfDateUTCTime();
    inline void setDateUTCTime(jsval pthis);

    



  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = JSSLOT_PRIVATE + 1;

  public:
    static const uint32 REGEXP_FIXED_RESERVED_SLOTS = 1;

    inline jsval getRegExpLastIndex() const;
    inline jsval *addressOfRegExpLastIndex();
    inline void zeroRegExpLastIndex();

    



    inline NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(NativeIterator *);

    



    






  private:
    static const uint32 JSSLOT_NAME_PREFIX          = JSSLOT_PRIVATE;       
    static const uint32 JSSLOT_NAME_URI             = JSSLOT_PRIVATE + 1;   

    static const uint32 JSSLOT_NAMESPACE_DECLARED   = JSSLOT_PRIVATE + 2;

    static const uint32 JSSLOT_QNAME_LOCAL_NAME     = JSSLOT_PRIVATE + 2;

  public:
    static const uint32 NAMESPACE_FIXED_RESERVED_SLOTS = 3;
    static const uint32 QNAME_FIXED_RESERVED_SLOTS     = 3;

    inline jsval getNamePrefix() const;
    inline void setNamePrefix(jsval prefix);

    inline jsval getNameURI() const;
    inline void setNameURI(jsval uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline jsval getQNameLocalName() const;
    inline void setQNameLocalName(jsval decl);

    



    inline jsval getProxyHandler() const;
    inline jsval getProxyPrivate() const;
    inline void setProxyPrivate(jsval priv);

    


    inline JSObject *getWithThis() const;
    inline void setWithThis(JSObject *thisp);

    



    inline bool isCallable();

    
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

    



    inline void initSharingEmptyScope(JSClass *clasp, JSObject *proto, JSObject *parent,
                                      jsval privateSlotValue);

    inline bool hasSlotsArray() const { return !!dslots; }

    
    inline void freeSlotsArray(JSContext *cx);

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

    JSBool getAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        return map->ops->getAttributes(cx, this, id, attrsp);
    }

    JSBool setAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        return map->ops->setAttributes(cx, this, id, attrsp);
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

    JSType typeOf(JSContext *cx) {
        return map->ops->typeOf(cx, this);
    }

    JSObject *wrappedObject(JSContext *cx) const;

    
    JSObject *thisObject(JSContext *cx) {
        return map->ops->thisObject ? map->ops->thisObject(cx, this) : this;
    }

    inline void dropProperty(JSContext *cx, JSProperty *prop);

    JSCompartment *getCompartment(JSContext *cx);

    void swap(JSObject *obj);

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
    inline bool isNamespace() const;
    inline bool isQName() const;

    inline bool isProxy() const;
    inline bool isObjectProxy() const;
    inline bool isFunctionProxy() const;

    inline bool unbrand(JSContext *cx);
};

#define JSSLOT_START(clasp) (((clasp)->flags & JSCLASS_HAS_PRIVATE)           \
                             ? JSSLOT_PRIVATE + 1                             \
                             : JSSLOT_PRIVATE)

#define JSSLOT_FREE(clasp)  (JSSLOT_START(clasp)                              \
                             + JSCLASS_RESERVED_SLOTS(clasp))






#define MAX_DSLOTS_LENGTH   (JS_MAX(~uint32(0), ~size_t(0)) / sizeof(jsval) - 1)
#define MAX_DSLOTS_LENGTH32 (~uint32(0) / sizeof(jsval) - 1)

#define OBJ_CHECK_SLOT(obj,slot)                                              \
    (JS_ASSERT((obj)->isNative()), JS_ASSERT(slot < (obj)->scope()->freeslot))

#ifdef JS_THREADSAFE






#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif 

#ifdef __cplusplus
inline void
OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj)
{
    JSClass *clasp = obj->getClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->innerObject)
            obj = xclasp->innerObject(cx, obj);
    }
}





inline void
OBJ_TO_OUTER_OBJECT(JSContext *cx, JSObject *&obj)
{
    JSClass *clasp = obj->getClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp = (JSExtendedClass *) clasp;
        if (xclasp->outerObject)
            obj = xclasp->outerObject(cx, obj);
    }
}
#endif

extern JSClass  js_ObjectClass;
extern JSClass  js_WithClass;
extern JSClass  js_BlockClass;
















static const uint32 JSSLOT_BLOCK_DEPTH = JSSLOT_PRIVATE + 1;

static inline bool
OBJ_IS_CLONED_BLOCK(JSObject *obj)
{
    return obj->getProto() != NULL;
}

static const uint32 JSSLOT_WITH_THIS = JSSLOT_PRIVATE + 2;

extern JSBool
js_DefineBlockVariable(JSContext *cx, JSObject *obj, jsid id, intN index);

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    ((OBJ_IS_CLONED_BLOCK(obj) ? obj->getProto() : obj)->scope()->entryCount)
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    JSVAL_TO_INT(obj->getSlot(JSSLOT_BLOCK_DEPTH))
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    obj->setSlot(JSSLOT_BLOCK_DEPTH, INT_TO_JSVAL(depth))









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
                        jsval *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_NewPropertyDescriptorObject(JSContext *cx, jsid id, uintN attrs, jsval getter, jsval setter, jsval value, jsval *vp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) js_obj_defineGetter(JSContext *cx, uintN argc, jsval *vp);
JS_FRIEND_API(JSBool) js_obj_defineSetter(JSContext *cx, uintN argc, jsval *vp);
#endif

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

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif













extern JSObject*
js_NewObjectWithClassProto(JSContext *cx, JSClass *clasp, JSObject *proto,
                           jsval privateSlotValue);

extern JSBool
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props);




extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject *cobj, JSObject *prototype);





extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key, jsval *vp,
                   JSClass *clasp = NULL);

extern JSObject *
js_ConstructObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, jsval *argv);

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
                     JSPropertyOp getter, JSPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);






extern JSScopeProperty *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             JSScopeProperty *sprop, uintN attrs, uintN mask,
                             JSPropertyOp getter, JSPropertyOp setter);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                  JSPropertyOp getter, JSPropertyOp setter, uintN attrs);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id, jsval descriptor, JSBool *bp);




const uintN JSDNP_CACHE_RESULT = 1; 
const uintN JSDNP_DONT_PURGE   = 2; 
const uintN JSDNP_SET_METHOD   = 4; 


const uintN JSDNP_UNQUALIFIED  = 8; 










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
    JS_ASSERT(obj->getParent());

    JSClass *clasp = obj->getClass();
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
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, jsval *vp);







extern JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, jsval propname);

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     jsval *vp);

extern JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);





extern JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, JSScopeProperty *sprop,
                       uintN attrs);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *rval);

extern JSBool
js_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp);

extern JSBool
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             jsval *statep, jsid *idp);

namespace js {

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            jsval *vp, uintN *attrsp);

}

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

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





extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, JSClass *clasp = NULL);

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

extern bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, jsval v);




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

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpValue(jsval val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, JSStackFrame *start = NULL);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);


JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JS_END_EXTERN_C

#endif 

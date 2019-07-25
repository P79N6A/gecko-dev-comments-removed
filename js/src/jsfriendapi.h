






































#ifndef jsfriendapi_h___
#define jsfriendapi_h___

#include "jsclass.h"
#include "jspubtd.h"
#include "jsprvtd.h"

#include "mozilla/GuardObjects.h"

JS_BEGIN_EXTERN_C

extern JS_FRIEND_API(void)
JS_SetGrayGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data);

extern JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt);

extern JS_FRIEND_API(JSObject *)
JS_FindCompilationScope(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSFunction *)
JS_GetObjectFunction(JSObject *obj);

extern JS_FRIEND_API(JSObject *)
JS_GetGlobalForFrame(JSStackFrame *fp);

extern JS_FRIEND_API(JSBool)
JS_SplicePrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_FRIEND_API(JSObject *)
JS_NewObjectWithUniqueType(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(uint32_t)
JS_ObjectCountDynamicSlots(JSObject *obj);

extern JS_FRIEND_API(void)
JS_ShrinkGCBuffers(JSRuntime *rt);

extern JS_FRIEND_API(size_t)
JS_GetE4XObjectsCreated(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx);

extern JS_FRIEND_API(JSBool)
JS_NondeterministicGetWeakMapKeys(JSContext *cx, JSObject *obj, JSObject **ret);






extern JS_FRIEND_API(void)
JS_TraceShapeCycleCollectorChildren(JSTracer *trc, void *shape);

enum {
    JS_TELEMETRY_GC_REASON,
    JS_TELEMETRY_GC_IS_COMPARTMENTAL,
    JS_TELEMETRY_GC_MS,
    JS_TELEMETRY_GC_MARK_MS,
    JS_TELEMETRY_GC_SWEEP_MS
};

typedef void
(* JSAccumulateTelemetryDataCallback)(int id, uint32_t sample);

extern JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime *rt, JSAccumulateTelemetryDataCallback callback);

typedef void
(* JSGCFinishedCallback)(JSRuntime *rt, JSCompartment *comp, const char *description);

extern JS_FRIEND_API(void)
JS_SetGCFinishedCallback(JSRuntime *rt, JSGCFinishedCallback callback);

extern JS_FRIEND_API(JSPrincipals *)
JS_GetCompartmentPrincipals(JSCompartment *compartment);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *obj);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSObject *)
JS_CloneObject(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *maybecx);

#ifdef DEBUG







extern JS_FRIEND_API(void)
js_DumpString(JSString *str);

extern JS_FRIEND_API(void)
js_DumpAtom(JSAtom *atom);

extern JS_FRIEND_API(void)
js_DumpObject(JSObject *obj);

extern JS_FRIEND_API(void)
js_DumpChars(const jschar *s, size_t n);
#endif

#ifdef __cplusplus

extern JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *target, JSObject *obj);

extern JS_FRIEND_API(JSBool)
JS_WrapPropertyDescriptor(JSContext *cx, js::PropertyDescriptor *desc);

extern JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSObject *obj, JSIterateOp enum_op, js::Value *statep, jsid *idp);

#endif

JS_END_EXTERN_C

#ifdef __cplusplus

struct PRLock;

namespace js {

struct ContextFriendFields {
    JSRuntime *const    runtime;

    ContextFriendFields(JSRuntime *rt)
      : runtime(rt) { }

    static const ContextFriendFields *get(const JSContext *cx) {
        return reinterpret_cast<const ContextFriendFields *>(cx);
    }
};

struct RuntimeFriendFields {
    



    volatile int32_t    interrupt;

    
    uintptr_t           nativeStackLimit;

    RuntimeFriendFields()
      : interrupt(0),
        nativeStackLimit(0) { }

    static const RuntimeFriendFields *get(const JSRuntime *rt) {
        return reinterpret_cast<const RuntimeFriendFields *>(rt);
    }
};

inline JSRuntime *
GetRuntime(const JSContext *cx)
{
    return ContextFriendFields::get(cx)->runtime;
}

typedef bool
(* PreserveWrapperCallback)(JSContext *cx, JSObject *obj);

#ifdef DEBUG
 



extern JS_FRIEND_API(void)
DumpHeapComplete(JSContext *cx, FILE *fp);

#endif

class JS_FRIEND_API(AutoPreserveCompartment) {
  private:
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoPreserveCompartment(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoPreserveCompartment();
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class JS_FRIEND_API(AutoSwitchCompartment) {
  private:
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoSwitchCompartment(JSContext *cx, JSCompartment *newCompartment
                          JS_GUARD_OBJECT_NOTIFIER_PARAM);
    AutoSwitchCompartment(JSContext *cx, JSObject *target JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoSwitchCompartment();
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) obj_defineGetter(JSContext *cx, uintN argc, js::Value *vp);
JS_FRIEND_API(JSBool) obj_defineSetter(JSContext *cx, uintN argc, js::Value *vp);
#endif

extern JS_FRIEND_API(bool)
IsSystemCompartment(const JSCompartment *compartment);

extern JS_FRIEND_API(bool)
IsAtomsCompartmentFor(const JSContext *cx, const JSCompartment *c);







extern JS_FRIEND_API(bool)
CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);

struct WeakMapTracer;







typedef void
(* WeakMapTraceCallback)(WeakMapTracer *trc, JSObject *m,
                         void *k, JSGCTraceKind kkind,
                         void *v, JSGCTraceKind vkind);

struct WeakMapTracer {
    JSContext            *context;
    WeakMapTraceCallback callback;

    WeakMapTracer(JSContext *cx, WeakMapTraceCallback cb)
        : context(cx), callback(cb) {}
};

extern JS_FRIEND_API(void)
TraceWeakMaps(WeakMapTracer *trc);







namespace shadow {

struct TypeObject {
    JSObject    *proto;
};

struct BaseShape {
    js::Class   *clasp;
    JSObject    *parent;
};

struct Shape {
    BaseShape   *base;
    jsid        _1;
    uint32_t    slotInfo;

    static const uint32_t FIXED_SLOTS_SHIFT = 27;
};

struct Object {
    Shape       *shape;
    TypeObject  *type;
    js::Value   *slots;
    js::Value   *_1;

    size_t numFixedSlots() const { return shape->slotInfo >> Shape::FIXED_SLOTS_SHIFT; }
    Value *fixedSlots() const {
        return (Value *)(uintptr_t(this) + sizeof(shadow::Object));
    }

    js::Value &slotRef(size_t slot) const {
        size_t nfixed = numFixedSlots();
        if (slot < nfixed)
            return fixedSlots()[slot];
        return slots[slot - nfixed];
    }
};

} 

extern JS_FRIEND_DATA(js::Class) AnyNameClass;
extern JS_FRIEND_DATA(js::Class) AttributeNameClass;
extern JS_FRIEND_DATA(js::Class) CallClass;
extern JS_FRIEND_DATA(js::Class) DeclEnvClass;
extern JS_FRIEND_DATA(js::Class) FunctionClass;
extern JS_FRIEND_DATA(js::Class) FunctionProxyClass;
extern JS_FRIEND_DATA(js::Class) NamespaceClass;
extern JS_FRIEND_DATA(js::Class) OuterWindowProxyClass;
extern JS_FRIEND_DATA(js::Class) ObjectProxyClass;
extern JS_FRIEND_DATA(js::Class) QNameClass;
extern JS_FRIEND_DATA(js::Class) XMLClass;
extern JS_FRIEND_DATA(js::Class) ObjectClass;

inline js::Class *
GetObjectClass(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->shape->base->clasp;
}

inline JSClass *
GetObjectJSClass(const JSObject *obj)
{
    return js::Jsvalify(GetObjectClass(obj));
}

JS_FRIEND_API(bool)
IsScopeObject(JSObject *obj);

inline JSObject *
GetObjectParent(JSObject *obj)
{
    JS_ASSERT(!IsScopeObject(obj));
    return reinterpret_cast<shadow::Object*>(obj)->shape->base->parent;
}

JS_FRIEND_API(JSObject *)
GetObjectParentMaybeScope(JSObject *obj);

JS_FRIEND_API(JSObject *)
GetGlobalForObjectCrossCompartment(JSObject *obj);

JS_FRIEND_API(bool)
IsOriginalScriptFunction(JSFunction *fun);

JS_FRIEND_API(JSFunction *)
DefineFunctionWithReserved(JSContext *cx, JSObject *obj, const char *name, JSNative call,
                           uintN nargs, uintN attrs);

JS_FRIEND_API(JSFunction *)
NewFunctionWithReserved(JSContext *cx, JSNative call, uintN nargs, uintN flags,
                        JSObject *parent, const char *name);

JS_FRIEND_API(JSFunction *)
NewFunctionByIdWithReserved(JSContext *cx, JSNative native, uintN nargs, uintN flags,
                            JSObject *parent, jsid id);

JS_FRIEND_API(JSObject *)
InitClassWithReserved(JSContext *cx, JSObject *obj, JSObject *parent_proto,
                      JSClass *clasp, JSNative constructor, uintN nargs,
                      JSPropertySpec *ps, JSFunctionSpec *fs,
                      JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

JS_FRIEND_API(const Value &)
GetFunctionNativeReserved(JSObject *fun, size_t which);

JS_FRIEND_API(void)
SetFunctionNativeReserved(JSObject *fun, size_t which, const Value &val);

inline JSObject *
GetObjectProto(JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->type->proto;
}

inline void *
GetObjectPrivate(JSObject *obj)
{
    const shadow::Object *nobj = reinterpret_cast<const shadow::Object*>(obj);
    void **addr = reinterpret_cast<void**>(&nobj->fixedSlots()[nobj->numFixedSlots()]);
    return *addr;
}





inline const Value &
GetReservedSlot(const JSObject *obj, size_t slot)
{
    JS_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

JS_FRIEND_API(void)
SetReservedSlotWithBarrier(JSObject *obj, size_t slot, const Value &value);

inline void
SetReservedSlot(JSObject *obj, size_t slot, const Value &value)
{
    JS_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    shadow::Object *sobj = reinterpret_cast<shadow::Object *>(obj);
    if (sobj->slotRef(slot).isMarkable())
        SetReservedSlotWithBarrier(obj, slot, value);
    else
        sobj->slotRef(slot) = value;
}

JS_FRIEND_API(uint32_t)
GetObjectSlotSpan(JSObject *obj);

inline const Value &
GetObjectSlot(JSObject *obj, size_t slot)
{
    JS_ASSERT(slot < GetObjectSlotSpan(obj));
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

inline Shape *
GetObjectShape(JSObject *obj)
{
    shadow::Shape *shape = reinterpret_cast<const shadow::Object*>(obj)->shape;
    return reinterpret_cast<Shape *>(shape);
}

static inline js::PropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(js::PropertyOp, object);
}

static inline js::StrictPropertyOp
CastAsJSStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(js::StrictPropertyOp, object);
}

JS_FRIEND_API(bool)
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector *props);

JS_FRIEND_API(bool)
StringIsArrayIndex(JSLinearString *str, jsuint *indexp);

JS_FRIEND_API(void)
SetPreserveWrapperCallback(JSRuntime *rt, PreserveWrapperCallback callback);

JS_FRIEND_API(bool)
IsObjectInContextCompartment(const JSObject *obj, const JSContext *cx);






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */

inline uintptr_t
GetContextStackLimit(const JSContext *cx)
{
    return RuntimeFriendFields::get(GetRuntime(cx))->nativeStackLimit;
}

#define JS_CHECK_RECURSION(cx, onerror)                                         \
    JS_BEGIN_MACRO                                                              \
        int stackDummy_;                                                        \
        if (!JS_CHECK_STACK_SIZE(js::GetContextStackLimit(cx), &stackDummy_)) { \
            js_ReportOverRecursed(cx);                                          \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

JS_FRIEND_API(void)
StartPCCountProfiling(JSContext *cx);

JS_FRIEND_API(void)
StopPCCountProfiling(JSContext *cx);

JS_FRIEND_API(void)
PurgePCCounts(JSContext *cx);

JS_FRIEND_API(size_t)
GetPCCountScriptCount(JSContext *cx);

JS_FRIEND_API(JSString *)
GetPCCountScriptSummary(JSContext *cx, size_t script);

JS_FRIEND_API(JSString *)
GetPCCountScriptContents(JSContext *cx, size_t script);

#ifdef JS_THREADSAFE
JS_FRIEND_API(void *)
GetOwnerThread(const JSContext *cx);

JS_FRIEND_API(unsigned)
GetContextOutstandingRequests(const JSContext *cx);

JS_FRIEND_API(PRLock *)
GetRuntimeGCLock(const JSRuntime *rt);

class JS_FRIEND_API(AutoSkipConservativeScan)
{
  public:
    AutoSkipConservativeScan(JSContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoSkipConservativeScan();

  private:
    JSContext *context;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

JS_FRIEND_API(JSCompartment *)
GetContextCompartment(const JSContext *cx);

JS_FRIEND_API(bool)
HasUnrootedGlobal(const JSContext *cx);

typedef void
(* ActivityCallback)(void *arg, JSBool active);







JS_FRIEND_API(void)
SetActivityCallback(JSRuntime *rt, ActivityCallback cb, void *arg);

class JS_FRIEND_API(AutoLockGC)
{
  public:
    explicit AutoLockGC(JSRuntime *rt = NULL
                        MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (rt)
            LockGC(rt);
    }

    ~AutoLockGC()
    {
        if (runtime)
            UnlockGC(runtime);
    }

    bool locked() const {
        return !!runtime;
    }
    void lock(JSRuntime *rt);

  private:
    static void LockGC(JSRuntime *rt);
    static void UnlockGC(JSRuntime *rt);

    JSRuntime *runtime;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

extern JS_FRIEND_API(const JSStructuredCloneCallbacks *)
GetContextStructuredCloneCallbacks(JSContext *cx);

extern JS_FRIEND_API(JSVersion)
VersionSetXML(JSVersion version, bool enable);

extern JS_FRIEND_API(bool)
CanCallContextDebugHandler(JSContext *cx);

extern JS_FRIEND_API(JSTrapStatus)
CallContextDebugHandler(JSContext *cx, JSScript *script, jsbytecode *bc, Value *rval);

extern JS_FRIEND_API(bool)
IsContextRunningJS(JSContext *cx);


extern JS_FRIEND_API(void)
TriggerOperationCallback(JSRuntime *rt);

class SystemAllocPolicy;
typedef Vector<JSCompartment*, 0, SystemAllocPolicy> CompartmentVector;
extern JS_FRIEND_API(const CompartmentVector&)
GetRuntimeCompartments(JSRuntime *rt);

extern JS_FRIEND_API(size_t)
SizeOfJSContext();

#define GCREASONS(D)                            \
    /* Reasons internal to the JS engine */     \
    D(API)                                      \
    D(MAYBEGC)                                  \
    D(LAST_CONTEXT)                             \
    D(DESTROY_CONTEXT)                          \
    D(LAST_DITCH)                               \
    D(TOO_MUCH_MALLOC)                          \
    D(ALLOC_TRIGGER)                            \
    D(UNUSED1) /* was CHUNK */                  \
    D(UNUSED2) /* was SHAPE */                  \
    D(UNUSED3) /* was REFILL */                 \
                                                \
    /* Reasons from Firefox */                  \
    D(DOM_WINDOW_UTILS)                         \
    D(COMPONENT_UTILS)                          \
    D(MEM_PRESSURE)                             \
    D(CC_WAITING)                               \
    D(CC_FORCED)                                \
    D(LOAD_END)                                 \
    D(POST_COMPARTMENT)                         \
    D(PAGE_HIDE)                                \
    D(NSJSCONTEXT_DESTROY)                      \
    D(SET_NEW_DOCUMENT)                         \
    D(SET_DOC_SHELL)                            \
    D(DOM_UTILS)                                \
    D(DOM_IPC)                                  \
    D(DOM_WORKER)                               \
    D(INTER_SLICE_GC)                           \
    D(REFRESH_FRAME)                            \
    D(FULL_GC_TIMER)

namespace gcreason {


enum Reason {
#define MAKE_REASON(name) name,
    GCREASONS(MAKE_REASON)
#undef MAKE_REASON
    NO_REASON,
    NUM_REASONS
};

} 

extern JS_FRIEND_API(void)
GCForReason(JSContext *cx, gcreason::Reason reason);

extern JS_FRIEND_API(void)
CompartmentGCForReason(JSContext *cx, JSCompartment *comp, gcreason::Reason reason);

extern JS_FRIEND_API(void)
ShrinkingGC(JSContext *cx, gcreason::Reason reason);

extern JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSRuntime *rt);

extern JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSContext *cx);

extern JS_FRIEND_API(void)
IncrementalReferenceBarrier(void *ptr);

extern JS_FRIEND_API(void)
IncrementalValueBarrier(const Value &v);

class ObjectPtr
{
    JSObject *value;

  public:
    ObjectPtr() : value(NULL) {}

    ObjectPtr(JSObject *obj) : value(obj) {}

    
    ~ObjectPtr() { JS_ASSERT(!value); }

    void finalize(JSRuntime *rt) {
        if (IsIncrementalBarrierNeeded(rt))
            IncrementalReferenceBarrier(value);
        value = NULL;
    }
    void finalize(JSContext *cx) { finalize(JS_GetRuntime(cx)); }

    void init(JSObject *obj) { value = obj; }

    JSObject *get() const { return value; }

    void writeBarrierPre(JSRuntime *rt) {
        IncrementalReferenceBarrier(value);
    }

    ObjectPtr &operator=(JSObject *obj) {
        IncrementalReferenceBarrier(value);
        value = obj;
        return *this;
    }

    JSObject &operator*() const { return *value; }
    JSObject *operator->() const { return value; }
    operator JSObject *() const { return value; }
};

} 





extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);

#endif







extern JS_FRIEND_API(JSBool)
js_DateIsValid(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(double)
js_DateGetMsecSinceEpoch(JSContext *cx, JSObject *obj);







typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);



extern JS_FRIEND_API(uint64_t)
js_GetSCOffset(JSStructuredCloneWriter* writer);

#endif

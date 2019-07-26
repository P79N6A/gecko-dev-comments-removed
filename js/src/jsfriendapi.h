





#ifndef jsfriendapi_h___
#define jsfriendapi_h___

#include "mozilla/GuardObjects.h"

#include "jsclass.h"
#include "jscpucfg.h"
#include "jspubtd.h"
#include "jsprvtd.h"






#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE_WITH_TOLERANCE(limit, sp, tolerance)  \
    ((uintptr_t)(sp) < (limit)+(tolerance))
#else
# define JS_CHECK_STACK_SIZE_WITH_TOLERANCE(limit, sp, tolerance)  \
    ((uintptr_t)(sp) > (limit)-(tolerance))
#endif

#define JS_CHECK_STACK_SIZE(limit, lval) JS_CHECK_STACK_SIZE_WITH_TOLERANCE(limit, lval, 0)

extern JS_FRIEND_API(void)
JS_SetGrayGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data);

extern JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt);

extern JS_FRIEND_API(JSObject *)
JS_FindCompilationScope(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(JSFunction *)
JS_GetObjectFunction(JSRawObject obj);

extern JS_FRIEND_API(JSBool)
JS_SplicePrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_FRIEND_API(JSObject *)
JS_NewObjectWithUniqueType(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(uint32_t)
JS_ObjectCountDynamicSlots(JSHandleObject obj);

extern JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx);

extern JS_FRIEND_API(JSBool)
JS_NondeterministicGetWeakMapKeys(JSContext *cx, JSObject *obj, JSObject **ret);







extern JS_FRIEND_API(JSBool)
JS_IsDeadWrapper(JSObject *obj);






extern JS_FRIEND_API(void)
JS_TraceShapeCycleCollectorChildren(JSTracer *trc, void *shape);

enum {
    JS_TELEMETRY_GC_REASON,
    JS_TELEMETRY_GC_IS_COMPARTMENTAL,
    JS_TELEMETRY_GC_MS,
    JS_TELEMETRY_GC_MAX_PAUSE_MS,
    JS_TELEMETRY_GC_MARK_MS,
    JS_TELEMETRY_GC_SWEEP_MS,
    JS_TELEMETRY_GC_MARK_ROOTS_MS,
    JS_TELEMETRY_GC_MARK_GRAY_MS,
    JS_TELEMETRY_GC_SLICE_MS,
    JS_TELEMETRY_GC_MMU_50,
    JS_TELEMETRY_GC_RESET,
    JS_TELEMETRY_GC_INCREMENTAL_DISABLED,
    JS_TELEMETRY_GC_NON_INCREMENTAL,
    JS_TELEMETRY_GC_SCC_SWEEP_TOTAL_MS,
    JS_TELEMETRY_GC_SCC_SWEEP_MAX_PAUSE_MS
};

typedef void
(* JSAccumulateTelemetryDataCallback)(int id, uint32_t sample);

extern JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime *rt, JSAccumulateTelemetryDataCallback callback);

extern JS_FRIEND_API(JSPrincipals *)
JS_GetCompartmentPrincipals(JSCompartment *compartment);

extern JS_FRIEND_API(void)
JS_SetCompartmentPrincipals(JSCompartment *compartment, JSPrincipals *principals);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *obj);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSObject *)
JS_CloneObject(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

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

extern JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *target, JSObject *obj);

extern JS_FRIEND_API(JSBool)
JS_WrapPropertyDescriptor(JSContext *cx, js::PropertyDescriptor *desc);

extern JS_FRIEND_API(JSBool)
JS_WrapAutoIdVector(JSContext *cx, JS::AutoIdVector &props);

extern JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op,
                  js::MutableHandleValue statep, js::MutableHandleId idp);

struct JSFunctionSpecWithHelp {
    const char      *name;
    JSNative        call;
    uint16_t        nargs;
    uint16_t        flags;
    const char      *usage;
    const char      *help;
};

#define JS_FN_HELP(name,call,nargs,flags,usage,help)                          \
    {name, call, nargs, (flags) | JSPROP_ENUMERATE | JSFUN_STUB_GSOPS, usage, help}
#define JS_FS_HELP_END                                                        \
    {NULL, NULL, 0, 0, NULL, NULL}

extern JS_FRIEND_API(bool)
JS_DefineFunctionsWithHelp(JSContext *cx, JSObject *obj, const JSFunctionSpecWithHelp *fs);

typedef bool (* JS_SourceHook)(JSContext *cx, JSScript *script, jschar **src, uint32_t *length);

extern JS_FRIEND_API(void)
JS_SetSourceHook(JSRuntime *rt, JS_SourceHook hook);

namespace js {

extern mozilla::ThreadLocal<PerThreadData *> TlsPerThreadData;

inline JSRuntime *
GetRuntime(const JSContext *cx)
{
    return ContextFriendFields::get(cx)->runtime;
}

inline JSCompartment *
GetContextCompartment(const JSContext *cx)
{
    return ContextFriendFields::get(cx)->compartment;
}

typedef bool
(* PreserveWrapperCallback)(JSContext *cx, JSObject *obj);

 



extern JS_FRIEND_API(void)
DumpHeapComplete(JSRuntime *rt, FILE *fp);

class JS_FRIEND_API(AutoSwitchCompartment) {
  private:
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoSwitchCompartment(JSContext *cx, JSCompartment *newCompartment
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    AutoSwitchCompartment(JSContext *cx, JSHandleObject target
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoSwitchCompartment();
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) obj_defineGetter(JSContext *cx, unsigned argc, js::Value *vp);
JS_FRIEND_API(JSBool) obj_defineSetter(JSContext *cx, unsigned argc, js::Value *vp);
#endif

extern JS_FRIEND_API(bool)
IsSystemCompartment(const JSCompartment *compartment);

extern JS_FRIEND_API(bool)
IsAtomsCompartment(const JSCompartment *c);








extern JS_FRIEND_API(bool)
ReportIfUndeclaredVarAssignment(JSContext *cx, HandleString propname);

struct WeakMapTracer;







typedef void
(* WeakMapTraceCallback)(WeakMapTracer *trc, JSObject *m,
                         void *k, JSGCTraceKind kkind,
                         void *v, JSGCTraceKind vkind);

struct WeakMapTracer {
    JSRuntime            *runtime;
    WeakMapTraceCallback callback;

    WeakMapTracer(JSRuntime *rt, WeakMapTraceCallback cb)
        : runtime(rt), callback(cb) {}
};

extern JS_FRIEND_API(void)
TraceWeakMaps(WeakMapTracer *trc);

extern JS_FRIEND_API(bool)
AreGCGrayBitsValid(JSRuntime *rt);

typedef void
(*GCThingCallback)(void *closure, void *gcthing);

extern JS_FRIEND_API(void)
VisitGrayWrapperTargets(JSCompartment *comp, GCThingCallback callback, void *closure);

extern JS_FRIEND_API(JSObject *)
GetWeakmapKeyDelegate(JSObject *key);

JS_FRIEND_API(JSGCTraceKind)
GCThingTraceKind(void *thing);




extern JS_FRIEND_API(void)
IterateGrayObjects(JSCompartment *compartment, GCThingCallback cellCallback, void *data);

#ifdef JS_HAS_CTYPES
extern JS_FRIEND_API(size_t)
SizeOfDataIfCDataObject(JSMallocSizeOfFun mallocSizeOf, JSObject *obj);
#endif







namespace shadow {

struct TypeObject {
    Class       *clasp;
    JSObject    *proto;
};

struct BaseShape {
    js::Class   *clasp;
    JSObject    *parent;
};

class Shape {
public:
    shadow::BaseShape *base;
    jsid              _1;
    uint32_t          slotInfo;

    static const uint32_t FIXED_SLOTS_SHIFT = 27;
};

struct Object {
    shadow::Shape      *shape;
    shadow::TypeObject *type;
    js::Value          *slots;
    js::Value          *_1;

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

struct Function {
    Object base;
    uint16_t nargs;
    uint16_t flags;
    
    Native native;
    const JSJitInfo *jitinfo;
    void *_1;
};

struct Atom {
    size_t _;
    const jschar *chars;
};

} 

extern JS_FRIEND_DATA(js::Class) CallClass;
extern JS_FRIEND_DATA(js::Class) DeclEnvClass;
extern JS_FRIEND_DATA(js::Class) FunctionClass;
extern JS_FRIEND_DATA(js::Class) FunctionProxyClass;
extern JS_FRIEND_DATA(js::Class) OuterWindowProxyClass;
extern JS_FRIEND_DATA(js::Class) ObjectProxyClass;
extern JS_FRIEND_DATA(js::Class) ObjectClass;

inline js::Class *
GetObjectClass(RawObject obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->type->clasp;
}

inline JSClass *
GetObjectJSClass(RawObject obj)
{
    return js::Jsvalify(GetObjectClass(obj));
}

inline bool
IsInnerObject(JSObject *obj) {
    return !!GetObjectClass(obj)->ext.outerObject;
}

inline bool
IsOuterObject(JSObject *obj) {
    return !!GetObjectClass(obj)->ext.innerObject;
}

JS_FRIEND_API(bool)
IsScopeObject(RawObject obj);

inline JSObject *
GetObjectParent(RawObject obj)
{
    JS_ASSERT(!IsScopeObject(obj));
    return reinterpret_cast<shadow::Object*>(obj)->shape->base->parent;
}

JS_FRIEND_API(JSObject *)
GetObjectParentMaybeScope(RawObject obj);

JS_FRIEND_API(JSObject *)
GetGlobalForObjectCrossCompartment(RawObject obj);

JS_FRIEND_API(void)
NotifyAnimationActivity(RawObject obj);

JS_FRIEND_API(bool)
IsOriginalScriptFunction(JSFunction *fun);










JS_FRIEND_API(JSScript *)
GetOutermostEnclosingFunctionOfScriptedCaller(JSContext *cx);

JS_FRIEND_API(JSFunction *)
DefineFunctionWithReserved(JSContext *cx, JSObject *obj, const char *name, JSNative call,
                           unsigned nargs, unsigned attrs);

JS_FRIEND_API(JSFunction *)
NewFunctionWithReserved(JSContext *cx, JSNative call, unsigned nargs, unsigned flags,
                        JSObject *parent, const char *name);

JS_FRIEND_API(JSFunction *)
NewFunctionByIdWithReserved(JSContext *cx, JSNative native, unsigned nargs, unsigned flags,
                            JSObject *parent, jsid id);

JS_FRIEND_API(JSObject *)
InitClassWithReserved(JSContext *cx, JSObject *obj, JSObject *parent_proto,
                      JSClass *clasp, JSNative constructor, unsigned nargs,
                      JSPropertySpec *ps, JSFunctionSpec *fs,
                      JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

JS_FRIEND_API(const Value &)
GetFunctionNativeReserved(RawObject fun, size_t which);

JS_FRIEND_API(void)
SetFunctionNativeReserved(RawObject fun, size_t which, const Value &val);

inline bool
GetObjectProto(JSContext *cx, JSObject *obj, JSObject **proto)
{
    js::Class *clasp = GetObjectClass(obj);
    if (clasp == &js::ObjectProxyClass ||
        clasp == &js::OuterWindowProxyClass ||
        clasp == &js::FunctionProxyClass)
    {
        return JS_GetPrototype(cx, obj, proto);
    }

    *proto = reinterpret_cast<const shadow::Object*>(obj)->type->proto;
    return true;
}

inline void *
GetObjectPrivate(RawObject obj)
{
    const shadow::Object *nobj = reinterpret_cast<const shadow::Object*>(obj);
    void **addr = reinterpret_cast<void**>(&nobj->fixedSlots()[nobj->numFixedSlots()]);
    return *addr;
}





inline const Value &
GetReservedSlot(RawObject obj, size_t slot)
{
    JS_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

JS_FRIEND_API(void)
SetReservedSlotWithBarrier(RawObject obj, size_t slot, const Value &value);

inline void
SetReservedSlot(RawObject obj, size_t slot, const Value &value)
{
    JS_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    shadow::Object *sobj = reinterpret_cast<shadow::Object *>(obj);
    if (sobj->slotRef(slot).isMarkable()
#ifdef JSGC_GENERATIONAL
        || value.isMarkable()
#endif
       )
    {
        SetReservedSlotWithBarrier(obj, slot, value);
    } else {
        sobj->slotRef(slot) = value;
    }
}

JS_FRIEND_API(uint32_t)
GetObjectSlotSpan(RawObject obj);

inline const Value &
GetObjectSlot(RawObject obj, size_t slot)
{
    JS_ASSERT(slot < GetObjectSlotSpan(obj));
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

inline const jschar *
GetAtomChars(JSAtom *atom)
{
    return reinterpret_cast<shadow::Atom *>(atom)->chars;
}

inline JSLinearString *
AtomToLinearString(JSAtom *atom)
{
    return reinterpret_cast<JSLinearString *>(atom);
}

static inline js::PropertyOp
CastAsJSPropertyOp(RawObject object)
{
    return JS_DATA_TO_FUNC_PTR(js::PropertyOp, object);
}

static inline js::StrictPropertyOp
CastAsJSStrictPropertyOp(RawObject object)
{
    return JS_DATA_TO_FUNC_PTR(js::StrictPropertyOp, object);
}

JS_FRIEND_API(bool)
GetPropertyNames(JSContext *cx, RawObject obj, unsigned flags, js::AutoIdVector *props);

JS_FRIEND_API(bool)
AppendUnique(JSContext *cx, AutoIdVector &base, AutoIdVector &others);

JS_FRIEND_API(bool)
GetGeneric(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

JS_FRIEND_API(bool)
StringIsArrayIndex(JSLinearString *str, uint32_t *indexp);

JS_FRIEND_API(void)
SetPreserveWrapperCallback(JSRuntime *rt, PreserveWrapperCallback callback);

JS_FRIEND_API(bool)
IsObjectInContextCompartment(RawObject obj, const JSContext *cx);






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */
#define JSITER_FOR_OF     0x20  /* harmony for-of loop */

inline uintptr_t
GetNativeStackLimit(const JSRuntime *rt)
{
    return PerThreadDataFriendFields::getMainThread(rt)->nativeStackLimit;
}







#define JS_CHECK_RECURSION(cx, onerror)                              \
    JS_BEGIN_MACRO                                                              \
        int stackDummy_;                                                        \
        if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(js::GetRuntime(cx)), &stackDummy_)) { \
            js_ReportOverRecursed(cx);                                          \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

#define JS_CHECK_CHROME_RECURSION(cx, onerror)                                  \
    JS_BEGIN_MACRO                                                              \
        int stackDummy_;                                                        \
        if (!JS_CHECK_STACK_SIZE_WITH_TOLERANCE(js::GetNativeStackLimit(js::GetRuntime(cx)), \
                                                &stackDummy_,                   \
                                                1024 * sizeof(size_t)))         \
        {                                                                       \
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







class ProfileEntry
{
    














    const char * volatile string; 
    void * volatile sp;           
    JSScript * volatile script_;  
    int32_t volatile idx;         

  public:
    






    bool js() volatile {
        JS_ASSERT_IF(sp == NULL, script_ != NULL);
        return sp == NULL;
    }

    uint32_t line() volatile { JS_ASSERT(!js()); return idx; }
    JSScript *script() volatile { JS_ASSERT(js()); return script_; }
    void *stackAddress() volatile { return sp; }
    const char *label() volatile { return string; }

    void setLine(uint32_t aLine) volatile { JS_ASSERT(!js()); idx = aLine; }
    void setLabel(const char *aString) volatile { string = aString; }
    void setStackAddress(void *aSp) volatile { sp = aSp; }
    void setScript(JSScript *aScript) volatile { script_ = aScript; }

    
    JS_FRIEND_API(jsbytecode *) pc() volatile;
    JS_FRIEND_API(void) setPC(jsbytecode *pc) volatile;

    static size_t offsetOfString() { return offsetof(ProfileEntry, string); }
    static size_t offsetOfStackAddress() { return offsetof(ProfileEntry, sp); }
    static size_t offsetOfPCIdx() { return offsetof(ProfileEntry, idx); }
    static size_t offsetOfScript() { return offsetof(ProfileEntry, script_); }

    




    static const int32_t NullPCIndex = -1;
};

JS_FRIEND_API(void)
SetRuntimeProfilingStack(JSRuntime *rt, ProfileEntry *stack, uint32_t *size,
                         uint32_t max);

JS_FRIEND_API(void)
EnableRuntimeProfilingStack(JSRuntime *rt, bool enabled);



JS_FRIEND_API(jsbytecode*)
ProfilingGetPC(JSRuntime *rt, RawScript script, void *ip);

#ifdef JS_THREADSAFE
JS_FRIEND_API(void *)
GetOwnerThread(const JSContext *cx);

JS_FRIEND_API(bool)
ContextHasOutstandingRequests(const JSContext *cx);
#endif

JS_FRIEND_API(bool)
HasUnrootedGlobal(const JSContext *cx);

typedef void
(* ActivityCallback)(void *arg, JSBool active);






JS_FRIEND_API(void)
SetActivityCallback(JSRuntime *rt, ActivityCallback cb, void *arg);

extern JS_FRIEND_API(const JSStructuredCloneCallbacks *)
GetContextStructuredCloneCallbacks(JSContext *cx);

extern JS_FRIEND_API(bool)
CanCallContextDebugHandler(JSContext *cx);

extern JS_FRIEND_API(JSTrapStatus)
CallContextDebugHandler(JSContext *cx, JSScript *script, jsbytecode *bc, Value *rval);

extern JS_FRIEND_API(bool)
IsContextRunningJS(JSContext *cx);

class SystemAllocPolicy;
typedef Vector<JSCompartment*, 0, SystemAllocPolicy> CompartmentVector;
extern JS_FRIEND_API(const CompartmentVector&)
GetRuntimeCompartments(JSRuntime *rt);

typedef void
(* AnalysisPurgeCallback)(JSRuntime *rt, JSFlatString *desc);

extern JS_FRIEND_API(AnalysisPurgeCallback)
SetAnalysisPurgeCallback(JSRuntime *rt, AnalysisPurgeCallback callback);

typedef JSBool
(* DOMInstanceClassMatchesProto)(JSHandleObject protoObject, uint32_t protoID,
                                 uint32_t depth);
struct JSDOMCallbacks {
    DOMInstanceClassMatchesProto instanceClassMatchesProto;
};
typedef struct JSDOMCallbacks DOMCallbacks;

extern JS_FRIEND_API(void)
SetDOMCallbacks(JSRuntime *rt, const DOMCallbacks *callbacks);

extern JS_FRIEND_API(const DOMCallbacks *)
GetDOMCallbacks(JSRuntime *rt);

extern JS_FRIEND_API(JSObject *)
GetTestingFunctions(JSContext *cx);






inline JSFreeOp *
CastToJSFreeOp(FreeOp *fop)
{
    return reinterpret_cast<JSFreeOp *>(fop);
}







extern JS_FRIEND_API(const jschar*)
GetErrorTypeName(JSContext* cx, int16_t exnType);

#ifdef DEBUG
extern JS_FRIEND_API(unsigned)
GetEnterCompartmentDepth(JSContext* cx);
#endif


typedef enum NukeReferencesToWindow {
    NukeWindowReferences,
    DontNukeWindowReferences
} NukeReferencesToWindow;





struct CompartmentFilter {
    virtual bool match(JSCompartment *c) const = 0;
};

struct AllCompartments : public CompartmentFilter {
    virtual bool match(JSCompartment *c) const { return true; }
};

struct ContentCompartmentsOnly : public CompartmentFilter {
    virtual bool match(JSCompartment *c) const {
        return !IsSystemCompartment(c);
    }
};

struct ChromeCompartmentsOnly : public CompartmentFilter {
    virtual bool match(JSCompartment *c) const {
        return IsSystemCompartment(c);
    }
};

struct SingleCompartment : public CompartmentFilter {
    JSCompartment *ours;
    SingleCompartment(JSCompartment *c) : ours(c) {}
    virtual bool match(JSCompartment *c) const { return c == ours; }
};

struct CompartmentsWithPrincipals : public CompartmentFilter {
    JSPrincipals *principals;
    CompartmentsWithPrincipals(JSPrincipals *p) : principals(p) {}
    virtual bool match(JSCompartment *c) const {
        return JS_GetCompartmentPrincipals(c) == principals;
    }
};

extern JS_FRIEND_API(JSBool)
NukeCrossCompartmentWrappers(JSContext* cx,
                             const CompartmentFilter& sourceFilter,
                             const CompartmentFilter& targetFilter,
                             NukeReferencesToWindow nukeReferencesToWindow);


JS_FRIEND_API(void)
SetListBaseInformation(void *listBaseHandlerFamily, uint32_t listBaseExpandoSlot);

void *GetListBaseHandlerFamily();
uint32_t GetListBaseExpandoSlot();

} 







extern JS_FRIEND_API(JSBool)
js_DateIsValid(JSObject* obj);

extern JS_FRIEND_API(double)
js_DateGetMsecSinceEpoch(JSRawObject obj);







typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const unsigned errorNumber);



extern JS_FRIEND_API(uint64_t)
js_GetSCOffset(JSStructuredCloneWriter* writer);



namespace js {
namespace ArrayBufferView {

enum ViewType {
    TYPE_INT8 = 0,
    TYPE_UINT8,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_FLOAT32,
    TYPE_FLOAT64,

    



    TYPE_UINT8_CLAMPED,

    



    TYPE_DATAVIEW,

    TYPE_MAX
};

} 
} 

typedef js::ArrayBufferView::ViewType JSArrayBufferViewType;







extern JS_FRIEND_API(JSObject *)
JS_NewInt8Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8ClampedArray(JSContext *cx, uint32_t nelements);

extern JS_FRIEND_API(JSObject *)
JS_NewInt16Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewUint16Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewInt32Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewUint32Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat32Array(JSContext *cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat64Array(JSContext *cx, uint32_t nelements);









extern JS_FRIEND_API(JSObject *)
JS_NewInt8ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8ClampedArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewInt16ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewUint16ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewInt32ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewUint32ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat32ArrayFromArray(JSContext *cx, JSObject *array);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat64ArrayFromArray(JSContext *cx, JSObject *array);








extern JS_FRIEND_API(JSObject *)
JS_NewInt8ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                          uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewUint8ClampedArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                                  uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewInt16ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewUint16ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                            uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewInt32ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewUint32ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                            uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat32ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                             uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject *)
JS_NewFloat64ArrayWithBuffer(JSContext *cx, JSObject *arrayBuffer,
                             uint32_t byteOffset, int32_t length);




extern JS_FRIEND_API(JSObject *)
JS_NewArrayBuffer(JSContext *cx, uint32_t nbytes);







extern JS_FRIEND_API(JSBool)
JS_IsTypedArrayObject(JSObject *obj);








extern JS_FRIEND_API(JSBool)
JS_IsArrayBufferViewObject(JSObject *obj);





extern JS_FRIEND_API(JSBool)
JS_IsInt8Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsUint8Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsUint8ClampedArray(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsInt16Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsUint16Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsInt32Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsUint32Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsFloat32Array(JSObject *obj);
extern JS_FRIEND_API(JSBool)
JS_IsFloat64Array(JSObject *obj);






extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsInt8Array(JSObject *obj, uint32_t *length, int8_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsUint8Array(JSObject *obj, uint32_t *length, uint8_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsUint8ClampedArray(JSObject *obj, uint32_t *length, uint8_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsInt16Array(JSObject *obj, uint32_t *length, int16_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsUint16Array(JSObject *obj, uint32_t *length, uint16_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsInt32Array(JSObject *obj, uint32_t *length, int32_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsUint32Array(JSObject *obj, uint32_t *length, uint32_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsFloat32Array(JSObject *obj, uint32_t *length, float **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsFloat64Array(JSObject *obj, uint32_t *length, double **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsArrayBufferView(JSObject *obj, uint32_t *length, uint8_t **data);
extern JS_FRIEND_API(JSObject *)
JS_GetObjectAsArrayBuffer(JSObject *obj, uint32_t *length, uint8_t **data);








extern JS_FRIEND_API(JSArrayBufferViewType)
JS_GetArrayBufferViewType(JSObject *obj);







extern JS_FRIEND_API(JSBool)
JS_IsArrayBufferObject(JSObject *obj);








extern JS_FRIEND_API(uint32_t)
JS_GetArrayBufferByteLength(JSObject *obj);










extern JS_FRIEND_API(uint8_t *)
JS_GetArrayBufferData(JSObject *obj);








extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayLength(JSObject *obj);









extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteOffset(JSObject *obj);








extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteLength(JSObject *obj);






extern JS_FRIEND_API(JSBool)
JS_IsArrayBufferViewObject(JSObject *obj);




extern JS_FRIEND_API(uint32_t)
JS_GetArrayBufferViewByteLength(JSObject *obj);











extern JS_FRIEND_API(int8_t *)
JS_GetInt8ArrayData(JSObject *obj);
extern JS_FRIEND_API(uint8_t *)
JS_GetUint8ArrayData(JSObject *obj);
extern JS_FRIEND_API(uint8_t *)
JS_GetUint8ClampedArrayData(JSObject *obj);
extern JS_FRIEND_API(int16_t *)
JS_GetInt16ArrayData(JSObject *obj);
extern JS_FRIEND_API(uint16_t *)
JS_GetUint16ArrayData(JSObject *obj);
extern JS_FRIEND_API(int32_t *)
JS_GetInt32ArrayData(JSObject *obj);
extern JS_FRIEND_API(uint32_t *)
JS_GetUint32ArrayData(JSObject *obj);
extern JS_FRIEND_API(float *)
JS_GetFloat32ArrayData(JSObject *obj);
extern JS_FRIEND_API(double *)
JS_GetFloat64ArrayData(JSObject *obj);





extern JS_FRIEND_API(void *)
JS_GetArrayBufferViewData(JSObject *obj);






extern JS_FRIEND_API(JSObject *)
JS_GetArrayBufferViewBuffer(JSObject *obj);




JS_FRIEND_API(JSBool)
JS_IsDataViewObject(JSObject *obj);









JS_FRIEND_API(uint32_t)
JS_GetDataViewByteOffset(JSObject *obj);









JS_FRIEND_API(uint32_t)
JS_GetDataViewByteLength(JSObject *obj);









JS_FRIEND_API(void *)
JS_GetDataViewData(JSObject *obj);






typedef bool
(* JSJitPropertyOp)(JSContext *cx, JSHandleObject thisObj,
                    void *specializedThis, JS::Value *vp);
typedef bool
(* JSJitMethodOp)(JSContext *cx, JSHandleObject thisObj,
                  void *specializedThis, unsigned argc, JS::Value *vp);

struct JSJitInfo {
    enum OpType {
        Getter,
        Setter,
        Method
    };

    JSJitPropertyOp op;
    uint32_t protoID;
    uint32_t depth;
    OpType type;
    bool isInfallible;      
    bool isConstant;        
    bool isPure;            


    JSValueType returnType; 
};

static JS_ALWAYS_INLINE const JSJitInfo *
FUNCTION_VALUE_TO_JITINFO(const JS::Value& v)
{
    JS_ASSERT(js::GetObjectClass(&v.toObject()) == &js::FunctionClass);
    return reinterpret_cast<js::shadow::Function *>(&v.toObject())->jitinfo;
}


static const unsigned JS_FUNCTION_INTERPRETED_BIT = 0x1;

static JS_ALWAYS_INLINE void
SET_JITINFO(JSFunction * func, const JSJitInfo *info)
{
    js::shadow::Function *fun = reinterpret_cast<js::shadow::Function *>(func);
    JS_ASSERT(!(fun->flags & JS_FUNCTION_INTERPRETED_BIT));
    fun->jitinfo = info;
}






static JS_ALWAYS_INLINE jsid
JSID_FROM_BITS(size_t bits)
{
    jsid id;
    JSID_BITS(id) = bits;
    return id;
}






















static JS_ALWAYS_INLINE jsid
NON_INTEGER_ATOM_TO_JSID(JSAtom *atom)
{
    JS_ASSERT(((size_t)atom & 0x7) == 0);
    jsid id = JSID_FROM_BITS((size_t)atom);
    JS_ASSERT(id == INTERNED_STRING_TO_JSID(NULL, (JSString*)atom));
    return id;
}


static JS_ALWAYS_INLINE JSBool
JSID_IS_ATOM(jsid id)
{
    return JSID_IS_STRING(id);
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_ATOM(jsid id, JSAtom *atom)
{
    return id == JSID_FROM_BITS((size_t)atom);
}

static JS_ALWAYS_INLINE JSAtom *
JSID_TO_ATOM(jsid id)
{
    return (JSAtom *)JSID_TO_STRING(id);
}

JS_STATIC_ASSERT(sizeof(jsid) == JS_BYTES_PER_WORD);

namespace js {

static JS_ALWAYS_INLINE Value
IdToValue(jsid id)
{
    if (JSID_IS_STRING(id))
        return StringValue(JSID_TO_STRING(id));
    if (JS_LIKELY(JSID_IS_INT(id)))
        return Int32Value(JSID_TO_INT(id));
    if (JS_LIKELY(JSID_IS_OBJECT(id)))
        return ObjectValue(*JSID_TO_OBJECT(id));
    JS_ASSERT(JSID_IS_VOID(id));
    return UndefinedValue();
}

static JS_ALWAYS_INLINE jsval
IdToJsval(jsid id)
{
    return IdToValue(id);
}

extern JS_FRIEND_API(bool)
IsReadOnlyDateMethod(JS::IsAcceptableThis test, JS::NativeImpl method);

extern JS_FRIEND_API(bool)
IsTypedArrayThisCheck(JS::IsAcceptableThis test);

enum CTypesActivityType {
    CTYPES_CALL_BEGIN,
    CTYPES_CALL_END,
    CTYPES_CALLBACK_BEGIN,
    CTYPES_CALLBACK_END
};

typedef void
(* CTypesActivityCallback)(JSContext *cx, CTypesActivityType type);





JS_FRIEND_API(void)
SetCTypesActivityCallback(JSRuntime *rt, CTypesActivityCallback cb);

class JS_FRIEND_API(AutoCTypesActivityCallback) {
  private:
    JSContext *cx;
    CTypesActivityCallback callback;
    CTypesActivityType endType;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCTypesActivityCallback(JSContext *cx, CTypesActivityType beginType,
                               CTypesActivityType endType
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoCTypesActivityCallback() {
        DoEndCallback();
    }
    void DoEndCallback() {
        if (callback) {
            callback(cx, endType);
            callback = NULL;
        }
    }
};

#ifdef DEBUG
extern JS_FRIEND_API(void)
assertEnteredPolicy(JSContext *cx, JSObject *obj, jsid id);
#else
inline JS_FRIEND_API(void) assertEnteredPolicy(JSContext *cx, JSObject *obj, jsid id) {};
#endif

} 

#endif 

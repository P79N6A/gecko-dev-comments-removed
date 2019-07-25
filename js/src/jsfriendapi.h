






































#ifndef jsfriendapi_h___
#define jsfriendapi_h___

#include "jsclass.h"
#include "jspubtd.h"
#include "jsprvtd.h"

#ifdef __cplusplus
#include "jsatom.h"
#include "js/Vector.h"
#endif

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

extern JS_FRIEND_API(uint32)
JS_ObjectCountDynamicSlots(JSObject *obj);

extern JS_FRIEND_API(size_t)
JS_GetE4XObjectsCreated(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx);

extern JS_FRIEND_API(JSBool)
JS_NondeterministicGetWeakMapKeys(JSContext *cx, JSObject *obj, JSObject **ret);

enum {
    JS_TELEMETRY_GC_REASON,
    JS_TELEMETRY_GC_IS_COMPARTMENTAL,
    JS_TELEMETRY_GC_IS_SHAPE_REGEN,
    JS_TELEMETRY_GC_MS,
    JS_TELEMETRY_GC_MARK_MS,
    JS_TELEMETRY_GC_SWEEP_MS
};

typedef void
(* JSAccumulateTelemetryDataCallback)(int id, JSUint32 sample);

extern JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime *rt, JSAccumulateTelemetryDataCallback callback);


typedef struct TypeInferenceMemoryStats
{
    int64 scripts;
    int64 objects;
    int64 tables;
    int64 temporary;
    int64 emptyShapes;
} TypeInferenceMemoryStats;

extern JS_FRIEND_API(void)
JS_GetTypeInferenceMemoryStats(JSContext *cx, JSCompartment *compartment,
                               TypeInferenceMemoryStats *stats);

extern JS_FRIEND_API(void)
JS_GetTypeInferenceObjectStats( void *object,
                               TypeInferenceMemoryStats *stats);

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

namespace js {

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
CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);

typedef js::Vector<JSCompartment *, 0, js::SystemAllocPolicy> CompartmentVector;







namespace shadow {

struct TypeObject {
    JSObject    *proto;
};

struct Object {
    void        *_1;
    js::Class   *clasp;
    uint32      flags;
    uint32      objShape;
    void        *_2;
    JSObject    *parent;
    void        *privateData;
    jsuword     capacity;
    js::Value   *slots;
    TypeObject  *type;

    static const uint32 FIXED_SLOTS_SHIFT = 27;

    js::Value &slotRef(size_t slot) const {
        size_t nfixed = flags >> FIXED_SLOTS_SHIFT;
        if (slot < nfixed)
            return ((Value *)((jsuword) this + sizeof(shadow::Object)))[slot];
        return slots[slot - nfixed];
    }
};

struct Context {
    JS::AutoGCRooter *autoGCRooters;
    void *data;
    void *data2;
#ifdef JS_THREADSAFE
    JSThread *thread_;
#endif
    JSCompartment *compartment;
    JSObject *globalObject;
    JSRuntime *runtime;
    jsuword stackLimit;
#ifdef JS_THREADSAFE
    unsigned outstandingRequests;
#endif
    JSDebugHooks *debugHooks;
};

struct Thread {
    void *id;
};

struct Runtime {
    JSAtomState atomState;
    JSCompartment *atomsCompartment;
    js::CompartmentVector compartments;
    JSStructuredCloneCallbacks *structuredCloneCallbacks;
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
extern JS_FRIEND_DATA(js::Class) ScriptClass;
extern JS_FRIEND_DATA(js::Class) XMLClass;

inline js::Class *
GetObjectClass(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->clasp;
}

inline JSClass *
GetObjectJSClass(const JSObject *obj)
{
    return js::Jsvalify(GetObjectClass(obj));
}

inline JSObject *
GetObjectParent(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->parent;
}

inline JSObject *
GetObjectProto(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->type->proto;
}

inline void *
GetObjectPrivate(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->privateData;
}

inline JSObject *
GetObjectGlobal(JSObject *obj)
{
    while (JSObject *parent = GetObjectParent(obj))
        obj = parent;
    return obj;
}

#ifdef DEBUG
extern JS_FRIEND_API(void) CheckReservedSlot(const JSObject *obj, size_t slot);
extern JS_FRIEND_API(void) CheckSlot(const JSObject *obj, size_t slot);
#else
inline void CheckReservedSlot(const JSObject *obj, size_t slot) {}
inline void CheckSlot(const JSObject *obj, size_t slot) {}
#endif





inline const Value &
GetReservedSlot(const JSObject *obj, size_t slot)
{
    CheckReservedSlot(obj, slot);
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

inline void
SetReservedSlot(JSObject *obj, size_t slot, const Value &value)
{
    CheckReservedSlot(obj, slot);
    reinterpret_cast<shadow::Object *>(obj)->slotRef(slot) = value;
}

inline uint32
GetNumSlots(const JSObject *obj)
{
    return uint32(reinterpret_cast<const shadow::Object *>(obj)->capacity);
}

inline const Value &
GetSlot(const JSObject *obj, size_t slot)
{
    CheckSlot(obj, slot);
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

inline uint32
GetObjectShape(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->objShape;
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






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */


#define JSFUN_TRCINFO     0x2000

static inline void *
GetContextPrivate(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->data;
}

static inline void *
GetContextPrivate2(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->data2;
}

static inline void
SetContextPrivate2(JSContext *cx, void *data)
{
    reinterpret_cast<js::shadow::Context *>(cx)->data2 = data;
}

#ifdef JS_THREADSAFE
static inline JSThread *
GetContextThread(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->thread_;
}

static inline void *
GetContextThreadId(JSContext *cx)
{
    JSThread *th = GetContextThread(cx);
    return reinterpret_cast<js::shadow::Thread *>(th)->id;
}
#endif

static inline JSCompartment *
GetContextCompartment(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->compartment;
}

static inline JSObject *
GetContextGlobalObject(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->globalObject;
}

static inline void
SetContextGlobalObject(JSContext *cx, JSObject *obj)
{
    reinterpret_cast<js::shadow::Context *>(cx)->globalObject = obj;
}

static inline JSRuntime *
GetContextRuntime(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->runtime;
}

static inline jsuword
GetContextStackLimit(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->stackLimit;
}

#ifdef JS_THREADSAFE
static inline unsigned
GetContextOutstandingRequests(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->outstandingRequests;
}
#endif

static inline JSDebugHooks *
GetContextDebugHooks(JSContext *cx)
{
    return reinterpret_cast<js::shadow::Context *>(cx)->debugHooks;
}

static inline JSString *
GetEmptyAtom(JSContext *cx)
{
    JSRuntime *rt = js::GetContextRuntime(cx);
    return (JSString *)reinterpret_cast<js::shadow::Runtime *>(rt)->atomState.emptyAtom;
}

static inline CompartmentVector &
GetRuntimeCompartments(JSRuntime *rt)
{
    return reinterpret_cast<js::shadow::Runtime *>(rt)->compartments;
}

static inline JSStructuredCloneCallbacks *
GetRuntimeStructuredCloneCallbacks(JSRuntime *rt)
{
    return reinterpret_cast<js::shadow::Runtime *>(rt)->structuredCloneCallbacks;
}

#define JS_CHECK_RECURSION(cx, onerror)                                       \
    JS_BEGIN_MACRO                                                            \
        int stackDummy_;                                                      \
                                                                              \
        if (!JS_CHECK_STACK_SIZE(js::GetContextStackLimit(cx), &stackDummy_)) { \
            js_ReportOverRecursed(cx);                                        \
            onerror;                                                          \
        }                                                                     \
    JS_END_MACRO

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *vec, size_t len)
{
    JS_ASSERT(vec <= vec + len);
    PodZero(vec, len);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(Value *beg, Value *end)
{
    JS_ASSERT(beg <= end);
    PodZero(beg, end - beg);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *beg, jsid *end)
{
    JS_ASSERT(beg <= end);
    for (jsid *id = beg; id < end; ++id)
        *id = INT_TO_JSID(0);
}

static JS_ALWAYS_INLINE void
MakeRangeGCSafe(jsid *vec, size_t len)
{
    MakeRangeGCSafe(vec, vec + len);
}

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
  public:
    explicit AutoVectorRooter(JSContext *cx
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(const T &v) { return vector.append(v); }

    
    void infallibleAppend(const T &v) { vector.infallibleAppend(v); }

    void popBack() { vector.popBack(); }
    T popCopy() { return vector.popCopy(); }

    bool growBy(size_t inc) {
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool resize(size_t newLength) {
        size_t oldLength = vector.length();
        if (newLength <= oldLength) {
            vector.shrinkBy(oldLength - newLength);
            return true;
        }
        if (!vector.growByUninitialized(newLength - oldLength))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    void clear() { vector.clear(); }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    T &operator[](size_t i) { return vector[i]; }
    const T &operator[](size_t i) const { return vector[i]; }

    const T *begin() const { return vector.begin(); }
    T *begin() { return vector.begin(); }

    const T *end() const { return vector.end(); }
    T *end() { return vector.end(); }

    const T &back() const { return vector.back(); }

  protected:
    typedef Vector<T, 8> VectorImpl;
    VectorImpl vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Value>(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    const jsval *jsval_begin() const { return begin(); }
    jsval *jsval_begin() { return begin(); }

    const jsval *jsval_end() const { return end(); }
    jsval *jsval_end() { return end(); }

  protected:
    virtual JS_FRIEND_API(void) trace(JSTracer *trc);

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectVector : public AutoVectorRooter<JSObject *>
{
  public:
    explicit AutoObjectVector(JSContext *cx
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSObject *>(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

  protected:
    virtual JS_FRIEND_API(void) trace(JSTracer *trc);

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : public AutoVectorRooter<jsid>
{
  public:
    explicit AutoIdVector(JSContext *cx
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

  protected:
    virtual JS_FRIEND_API(void) trace(JSTracer *trc);

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef JS_THREADSAFE
class JS_FRIEND_API(AutoSkipConservativeScan)
{
  public:
    AutoSkipConservativeScan(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoSkipConservativeScan();

  private:
    JSContext *context;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

extern JS_FORCES_STACK JS_FRIEND_API(void)
LeaveTrace(JSContext *cx);

} 
#endif





typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);





extern JS_FRIEND_API(JSBool)
JS_DateIsValid(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(jsdouble)
JS_DateGetMsecSinceEpoch(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSBool)
JS_WasLastGCCompartmental(JSContext *cx);

extern JS_FRIEND_API(JSUint64)
JS_GetSCOffset(JSStructuredCloneWriter* writer);

extern JS_FRIEND_API(JSVersion)
JS_VersionSetXML(JSVersion version, JSBool enable);

extern JS_FRIEND_API(JSBool)
JS_IsContextRunningJS(JSContext *cx);

#endif

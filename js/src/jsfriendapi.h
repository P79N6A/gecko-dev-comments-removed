





#ifndef jsfriendapi_h
#define jsfriendapi_h

#include "mozilla/Casting.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/UniquePtr.h"

#include "jsapi.h" 
#include "jsbytecode.h"
#include "jspubtd.h"

#include "js/CallArgs.h"
#include "js/CallNonGenericMethod.h"
#include "js/Class.h"

#if JS_STACK_GROWTH_DIRECTION > 0
# define JS_CHECK_STACK_SIZE(limit, sp) (MOZ_LIKELY(((uintptr_t)(sp) < (limit)))
#else
# define JS_CHECK_STACK_SIZE(limit, sp) (MOZ_LIKELY((uintptr_t)(sp) > (limit)))
#endif

class JSAtom;
struct JSErrorFormatString;
class JSLinearString;
struct JSJitInfo;
class JSErrorReport;

namespace JS {
template <class T>
class Heap;
} 

namespace js {
class JS_FRIEND_API(BaseProxyHandler);
class InterpreterFrame;
} 

extern JS_FRIEND_API(void)
JS_SetGrayGCRootsTracer(JSRuntime* rt, JSTraceDataOp traceOp, void* data);

extern JS_FRIEND_API(JSString*)
JS_GetAnonymousString(JSRuntime* rt);

extern JS_FRIEND_API(JSObject*)
JS_FindCompilationScope(JSContext* cx, JS::HandleObject obj);

extern JS_FRIEND_API(JSFunction*)
JS_GetObjectFunction(JSObject* obj);

extern JS_FRIEND_API(bool)
JS_SplicePrototype(JSContext* cx, JS::HandleObject obj, JS::HandleObject proto);

extern JS_FRIEND_API(JSObject*)
JS_NewObjectWithUniqueType(JSContext* cx, const JSClass* clasp, JS::HandleObject proto);





extern JS_FRIEND_API(JSObject*)
JS_NewObjectWithoutMetadata(JSContext* cx, const JSClass* clasp, JS::Handle<JSObject*> proto);

extern JS_FRIEND_API(uint32_t)
JS_ObjectCountDynamicSlots(JS::HandleObject obj);

extern JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext* cx);

extern JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext* cx);

extern JS_FRIEND_API(bool)
JS_NondeterministicGetWeakMapKeys(JSContext* cx, JS::HandleObject obj, JS::MutableHandleObject ret);


extern JS_FRIEND_API(unsigned)
JS_PCToLineNumber(JSScript* script, jsbytecode* pc, unsigned* columnp = nullptr);







extern JS_FRIEND_API(bool)
JS_IsDeadWrapper(JSObject* obj);






extern JS_FRIEND_API(void)
JS_TraceShapeCycleCollectorChildren(JSTracer* trc, JS::GCCellPtr shape);

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
    JS_TELEMETRY_GC_SCC_SWEEP_MAX_PAUSE_MS,
    JS_TELEMETRY_DEPRECATED_LANGUAGE_EXTENSIONS_IN_CONTENT,
    JS_TELEMETRY_ADDON_EXCEPTIONS
};

typedef void
(*JSAccumulateTelemetryDataCallback)(int id, uint32_t sample, const char* key);

extern JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime* rt, JSAccumulateTelemetryDataCallback callback);

extern JS_FRIEND_API(JSPrincipals*)
JS_GetCompartmentPrincipals(JSCompartment* compartment);

extern JS_FRIEND_API(void)
JS_SetCompartmentPrincipals(JSCompartment* compartment, JSPrincipals* principals);

extern JS_FRIEND_API(JSPrincipals*)
JS_GetScriptPrincipals(JSScript* script);

extern JS_FRIEND_API(bool)
JS_ScriptHasMutedErrors(JSScript* script);


extern JS_FRIEND_API(JSObject*)
JS_ObjectToInnerObject(JSContext* cx, JS::HandleObject obj);


extern JS_FRIEND_API(JSObject*)
JS_ObjectToOuterObject(JSContext* cx, JS::HandleObject obj);

extern JS_FRIEND_API(JSObject*)
JS_CloneObject(JSContext* cx, JS::HandleObject obj, JS::HandleObject proto);














extern JS_FRIEND_API(bool)
JS_InitializePropertiesFromCompatibleNativeObject(JSContext* cx,
                                                  JS::HandleObject dst,
                                                  JS::HandleObject src);

extern JS_FRIEND_API(JSString*)
JS_BasicObjectToString(JSContext* cx, JS::HandleObject obj);

namespace js {

JS_FRIEND_API(bool)
ObjectClassIs(JSContext* cx, JS::HandleObject obj, ESClassValue classValue);

JS_FRIEND_API(const char*)
ObjectClassName(JSContext* cx, JS::HandleObject obj);

JS_FRIEND_API(void)
ReportOverRecursed(JSContext* maybecx);

JS_FRIEND_API(bool)
AddRawValueRoot(JSContext* cx, JS::Value* vp, const char* name);

JS_FRIEND_API(void)
RemoveRawValueRoot(JSContext* cx, JS::Value* vp);

JS_FRIEND_API(JSAtom*)
GetPropertyNameFromPC(JSScript* script, jsbytecode* pc);

#ifdef JS_DEBUG







extern JS_FRIEND_API(void)
DumpString(JSString* str);

extern JS_FRIEND_API(void)
DumpAtom(JSAtom* atom);

extern JS_FRIEND_API(void)
DumpObject(JSObject* obj);

extern JS_FRIEND_API(void)
DumpChars(const char16_t* s, size_t n);

extern JS_FRIEND_API(void)
DumpValue(const JS::Value& val);

extern JS_FRIEND_API(void)
DumpId(jsid id);

extern JS_FRIEND_API(void)
DumpInterpreterFrame(JSContext* cx, InterpreterFrame* start = nullptr);

extern JS_FRIEND_API(bool)
DumpPC(JSContext* cx);

extern JS_FRIEND_API(bool)
DumpScript(JSContext* cx, JSScript* scriptArg);

#endif

extern JS_FRIEND_API(void)
DumpBacktrace(JSContext* cx);

} 

namespace JS {


extern JS_FRIEND_API(char*)
FormatStackDump(JSContext* cx, char* buf, bool showArgs, bool showLocals, bool showThisProps);

} 








extern JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext* cx, JS::HandleObject target, JS::HandleObject obj);










typedef enum  {
    MakeNonConfigurableIntoConfigurable,
    CopyNonConfigurableAsIs
} PropertyCopyBehavior;

extern JS_FRIEND_API(bool)
JS_CopyPropertyFrom(JSContext* cx, JS::HandleId id, JS::HandleObject target,
                    JS::HandleObject obj,
                    PropertyCopyBehavior copyBehavior = CopyNonConfigurableAsIs);

extern JS_FRIEND_API(bool)
JS_WrapPropertyDescriptor(JSContext* cx, JS::MutableHandle<JSPropertyDescriptor> desc);

struct JSFunctionSpecWithHelp {
    const char*     name;
    JSNative        call;
    uint16_t        nargs;
    uint16_t        flags;
    const char*     usage;
    const char*     help;
};

#define JS_FN_HELP(name,call,nargs,flags,usage,help)                          \
    {name, call, nargs, (flags) | JSPROP_ENUMERATE | JSFUN_STUB_GSOPS, usage, help}
#define JS_FS_HELP_END                                                        \
    {nullptr, nullptr, 0, 0, nullptr, nullptr}

extern JS_FRIEND_API(bool)
JS_DefineFunctionsWithHelp(JSContext* cx, JS::HandleObject obj, const JSFunctionSpecWithHelp* fs);

namespace js {







#define PROXY_MAKE_EXT(outerObject, innerObject, isWrappedNative,       \
                       objectMoved)                                     \
    {                                                                   \
        outerObject,                                                    \
        innerObject,                                                    \
        isWrappedNative,                                                \
        js::proxy_WeakmapKeyDelegate,                                   \
        objectMoved                                                     \
    }

#define PROXY_CLASS_WITH_EXT(name, flags, ext)                                          \
    {                                                                                   \
        name,                                                                           \
        js::Class::NON_NATIVE |                                                         \
            JSCLASS_IS_PROXY |                                                          \
            JSCLASS_IMPLEMENTS_BARRIERS |                                               \
            flags,                                                                      \
        nullptr,                 /* addProperty */                                      \
        nullptr,                 /* delProperty */                                      \
        nullptr,                 /* getProperty */                                      \
        nullptr,                 /* setProperty */                                      \
        nullptr,                 /* enumerate */                                        \
        nullptr,                 /* resolve */                                          \
        js::proxy_Convert,                                                              \
        js::proxy_Finalize,      /* finalize    */                                      \
        nullptr,                 /* call        */                                      \
        js::proxy_HasInstance,   /* hasInstance */                                      \
        nullptr,                 /* construct   */                                      \
        js::proxy_Trace,         /* trace       */                                      \
        JS_NULL_CLASS_SPEC,                                                             \
        ext,                                                                            \
        {                                                                               \
            js::proxy_LookupProperty,                                                   \
            js::proxy_DefineProperty,                                                   \
            js::proxy_HasProperty,                                                      \
            js::proxy_GetProperty,                                                      \
            js::proxy_SetProperty,                                                      \
            js::proxy_GetOwnPropertyDescriptor,                                         \
            js::proxy_DeleteProperty,                                                   \
            js::proxy_Watch, js::proxy_Unwatch,                                         \
            js::proxy_GetElements,                                                      \
            nullptr,             /* enumerate       */                                  \
            nullptr,             /* thisObject      */                                  \
        }                                                                               \
    }

#define PROXY_CLASS_DEF(name, flags)                                    \
  PROXY_CLASS_WITH_EXT(name, flags,                                     \
                       PROXY_MAKE_EXT(                                  \
                         nullptr, /* outerObject */                     \
                         nullptr, /* innerObject */                     \
                         false,   /* isWrappedNative */                 \
                         js::proxy_ObjectMoved                          \
                       ))







extern JS_FRIEND_API(bool)
proxy_LookupProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleObject objp,
                    JS::MutableHandle<Shape*> propp);
extern JS_FRIEND_API(bool)
proxy_DefineProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::Handle<JSPropertyDescriptor> desc,
                     JS::ObjectOpResult& result);
extern JS_FRIEND_API(bool)
proxy_HasProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp);
extern JS_FRIEND_API(bool)
proxy_GetProperty(JSContext* cx, JS::HandleObject obj, JS::HandleObject receiver, JS::HandleId id,
                  JS::MutableHandleValue vp);
extern JS_FRIEND_API(bool)
proxy_SetProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue bp,
                  JS::HandleValue receiver, JS::ObjectOpResult& result);
extern JS_FRIEND_API(bool)
proxy_GetOwnPropertyDescriptor(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                               JS::MutableHandle<JSPropertyDescriptor> desc);
extern JS_FRIEND_API(bool)
proxy_DeleteProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                     JS::ObjectOpResult& result);

extern JS_FRIEND_API(void)
proxy_Trace(JSTracer* trc, JSObject* obj);
extern JS_FRIEND_API(JSObject*)
proxy_WeakmapKeyDelegate(JSObject* obj);
extern JS_FRIEND_API(bool)
proxy_Convert(JSContext* cx, JS::HandleObject proxy, JSType hint, JS::MutableHandleValue vp);
extern JS_FRIEND_API(void)
proxy_Finalize(FreeOp* fop, JSObject* obj);
extern JS_FRIEND_API(void)
proxy_ObjectMoved(JSObject* obj, const JSObject* old);
extern JS_FRIEND_API(bool)
proxy_HasInstance(JSContext* cx, JS::HandleObject proxy, JS::MutableHandleValue v, bool* bp);
extern JS_FRIEND_API(bool)
proxy_Call(JSContext* cx, unsigned argc, JS::Value* vp);
extern JS_FRIEND_API(bool)
proxy_Construct(JSContext* cx, unsigned argc, JS::Value* vp);
extern JS_FRIEND_API(JSObject*)
proxy_innerObject(JSObject* obj);
extern JS_FRIEND_API(bool)
proxy_Watch(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);
extern JS_FRIEND_API(bool)
proxy_Unwatch(JSContext* cx, JS::HandleObject obj, JS::HandleId id);
extern JS_FRIEND_API(bool)
proxy_GetElements(JSContext* cx, JS::HandleObject proxy, uint32_t begin, uint32_t end,
                  ElementAdder* adder);












class SourceHook {
  public:
    virtual ~SourceHook() { }

    




    virtual bool load(JSContext* cx, const char* filename, char16_t** src, size_t* length) = 0;
};







extern JS_FRIEND_API(void)
SetSourceHook(JSRuntime* rt, mozilla::UniquePtr<SourceHook> hook);


extern JS_FRIEND_API(mozilla::UniquePtr<SourceHook>)
ForgetSourceHook(JSRuntime* rt);

extern JS_FRIEND_API(JS::Zone*)
GetCompartmentZone(JSCompartment* comp);

typedef bool
(* PreserveWrapperCallback)(JSContext* cx, JSObject* obj);

typedef enum  {
    CollectNurseryBeforeDump,
    IgnoreNurseryObjects
} DumpHeapNurseryBehaviour;

 



extern JS_FRIEND_API(void)
DumpHeapComplete(JSRuntime* rt, FILE* fp, DumpHeapNurseryBehaviour nurseryBehaviour);

#ifdef JS_OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(bool) obj_defineGetter(JSContext* cx, unsigned argc, JS::Value* vp);
JS_FRIEND_API(bool) obj_defineSetter(JSContext* cx, unsigned argc, JS::Value* vp);
#endif

extern JS_FRIEND_API(bool)
IsSystemCompartment(JSCompartment* comp);

extern JS_FRIEND_API(bool)
IsSystemZone(JS::Zone* zone);

extern JS_FRIEND_API(bool)
IsAtomsCompartment(JSCompartment* comp);







extern JS_FRIEND_API(bool)
IsInNonStrictPropertySet(JSContext* cx);

struct WeakMapTracer;









typedef void
(* WeakMapTraceCallback)(WeakMapTracer* trc, JSObject* m, JS::GCCellPtr key, JS::GCCellPtr value);

struct WeakMapTracer {
    JSRuntime*           runtime;
    WeakMapTraceCallback callback;

    WeakMapTracer(JSRuntime* rt, WeakMapTraceCallback cb)
        : runtime(rt), callback(cb) {}
};

extern JS_FRIEND_API(void)
TraceWeakMaps(WeakMapTracer* trc);

extern JS_FRIEND_API(bool)
AreGCGrayBitsValid(JSRuntime* rt);

extern JS_FRIEND_API(bool)
ZoneGlobalsAreAllGray(JS::Zone* zone);

typedef void
(*GCThingCallback)(void* closure, JS::GCCellPtr thing);

extern JS_FRIEND_API(void)
VisitGrayWrapperTargets(JS::Zone* zone, GCThingCallback callback, void* closure);

extern JS_FRIEND_API(JSObject*)
GetWeakmapKeyDelegate(JSObject* key);

JS_FRIEND_API(JSGCTraceKind)
GCThingTraceKind(void* thing);




extern JS_FRIEND_API(void)
IterateGrayObjects(JS::Zone* zone, GCThingCallback cellCallback, void* data);

#ifdef JS_HAS_CTYPES
extern JS_FRIEND_API(size_t)
SizeOfDataIfCDataObject(mozilla::MallocSizeOf mallocSizeOf, JSObject* obj);
#endif

extern JS_FRIEND_API(JSCompartment*)
GetAnyCompartmentInZone(JS::Zone* zone);







namespace shadow {

struct ObjectGroup {
    const Class* clasp;
    JSObject*   proto;
    JSCompartment* compartment;
};

struct BaseShape {
    const js::Class* clasp_;
    JSObject* parent;
};

class Shape {
public:
    shadow::BaseShape* base;
    jsid              _1;
    uint32_t          slotInfo;

    static const uint32_t FIXED_SLOTS_SHIFT = 27;
};




struct Object {
    shadow::ObjectGroup* group;
    shadow::Shape*      shape;
    JS::Value*          slots;
    void*               _1;

    size_t numFixedSlots() const { return shape->slotInfo >> Shape::FIXED_SLOTS_SHIFT; }
    JS::Value* fixedSlots() const {
        return (JS::Value*)(uintptr_t(this) + sizeof(shadow::Object));
    }

    JS::Value& slotRef(size_t slot) const {
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
    
    JSNative native;
    const JSJitInfo* jitinfo;
    void* _1;
};

struct String
{
    static const uint32_t INLINE_CHARS_BIT = JS_BIT(2);
    static const uint32_t LATIN1_CHARS_BIT = JS_BIT(6);
    static const uint32_t ROPE_FLAGS       = 0;
    static const uint32_t TYPE_FLAGS_MASK  = JS_BIT(6) - 1;
    uint32_t flags;
    uint32_t length;
    union {
        const JS::Latin1Char* nonInlineCharsLatin1;
        const char16_t* nonInlineCharsTwoByte;
        JS::Latin1Char inlineStorageLatin1[1];
        char16_t inlineStorageTwoByte[1];
    };
};

} 



extern JS_FRIEND_DATA(const js::Class* const) ObjectClassPtr;

inline const js::Class*
GetObjectClass(JSObject* obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->group->clasp;
}

inline const JSClass*
GetObjectJSClass(JSObject* obj)
{
    return js::Jsvalify(GetObjectClass(obj));
}

JS_FRIEND_API(const Class*)
ProtoKeyToClass(JSProtoKey key);





inline bool
StandardClassIsDependent(JSProtoKey key)
{
    const Class* clasp = ProtoKeyToClass(key);
    return clasp->spec.defined() && clasp->spec.dependent();
}








inline JSProtoKey
ParentKeyForStandardClass(JSProtoKey key)
{
    
    if (key == JSProto_Object)
        return JSProto_Null;

    
    if (StandardClassIsDependent(key))
        return ProtoKeyToClass(key)->spec.parentKey();

    
    return JSProto_Object;
}

inline bool
IsInnerObject(JSObject* obj) {
    return !!GetObjectClass(obj)->ext.outerObject;
}

inline bool
IsOuterObject(JSObject* obj) {
    return !!GetObjectClass(obj)->ext.innerObject;
}

JS_FRIEND_API(bool)
IsFunctionObject(JSObject* obj);

JS_FRIEND_API(bool)
IsScopeObject(JSObject* obj);

JS_FRIEND_API(bool)
IsCallObject(JSObject* obj);

JS_FRIEND_API(bool)
CanAccessObjectShape(JSObject* obj);

static MOZ_ALWAYS_INLINE JSCompartment*
GetObjectCompartment(JSObject* obj)
{
    return reinterpret_cast<shadow::Object*>(obj)->group->compartment;
}

JS_FRIEND_API(JSObject*)
GetGlobalForObjectCrossCompartment(JSObject* obj);

JS_FRIEND_API(JSObject*)
GetPrototypeNoProxy(JSObject* obj);



JS_FRIEND_API(void)
SetPendingExceptionCrossContext(JSContext* cx, JS::HandleValue v);

JS_FRIEND_API(void)
AssertSameCompartment(JSContext* cx, JSObject* obj);

#ifdef JS_DEBUG
JS_FRIEND_API(void)
AssertSameCompartment(JSObject* objA, JSObject* objB);
#else
inline void AssertSameCompartment(JSObject* objA, JSObject* objB) {}
#endif

JS_FRIEND_API(void)
NotifyAnimationActivity(JSObject* obj);










JS_FRIEND_API(JSFunction*)
GetOutermostEnclosingFunctionOfScriptedCaller(JSContext* cx);

JS_FRIEND_API(JSFunction*)
DefineFunctionWithReserved(JSContext* cx, JSObject* obj, const char* name, JSNative call,
                           unsigned nargs, unsigned attrs);

JS_FRIEND_API(JSFunction*)
NewFunctionWithReserved(JSContext* cx, JSNative call, unsigned nargs, unsigned flags,
                        const char* name);

JS_FRIEND_API(JSFunction*)
NewFunctionByIdWithReserved(JSContext* cx, JSNative native, unsigned nargs, unsigned flags,
                            jsid id);

JS_FRIEND_API(const JS::Value&)
GetFunctionNativeReserved(JSObject* fun, size_t which);

JS_FRIEND_API(void)
SetFunctionNativeReserved(JSObject* fun, size_t which, const JS::Value& val);

JS_FRIEND_API(bool)
FunctionHasNativeReserved(JSObject* fun);

JS_FRIEND_API(bool)
GetObjectProto(JSContext* cx, JS::HandleObject obj, JS::MutableHandleObject proto);

JS_FRIEND_API(bool)
GetOriginalEval(JSContext* cx, JS::HandleObject scope,
                JS::MutableHandleObject eval);

inline void*
GetObjectPrivate(JSObject* obj)
{
    MOZ_ASSERT(GetObjectClass(obj)->flags & JSCLASS_HAS_PRIVATE);
    const shadow::Object* nobj = reinterpret_cast<const shadow::Object*>(obj);
    void** addr = reinterpret_cast<void**>(&nobj->fixedSlots()[nobj->numFixedSlots()]);
    return *addr;
}

inline const JS::Value&
GetReservedSlot(JSObject* obj, size_t slot)
{
    MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    return reinterpret_cast<const shadow::Object*>(obj)->slotRef(slot);
}

JS_FRIEND_API(void)
SetReservedOrProxyPrivateSlotWithBarrier(JSObject* obj, size_t slot, const JS::Value& value);

inline void
SetReservedSlot(JSObject* obj, size_t slot, const JS::Value& value)
{
    MOZ_ASSERT(slot < JSCLASS_RESERVED_SLOTS(GetObjectClass(obj)));
    shadow::Object* sobj = reinterpret_cast<shadow::Object*>(obj);
    if (sobj->slotRef(slot).isMarkable() || value.isMarkable())
        SetReservedOrProxyPrivateSlotWithBarrier(obj, slot, value);
    else
        sobj->slotRef(slot) = value;
}

JS_FRIEND_API(uint32_t)
GetObjectSlotSpan(JSObject* obj);

inline const JS::Value&
GetObjectSlot(JSObject* obj, size_t slot)
{
    MOZ_ASSERT(slot < GetObjectSlotSpan(obj));
    return reinterpret_cast<const shadow::Object*>(obj)->slotRef(slot);
}

MOZ_ALWAYS_INLINE size_t
GetAtomLength(JSAtom* atom)
{
    return reinterpret_cast<shadow::String*>(atom)->length;
}

static const uint32_t MaxStringLength = (1 << 28) - 1;

MOZ_ALWAYS_INLINE size_t
GetStringLength(JSString* s)
{
    return reinterpret_cast<shadow::String*>(s)->length;
}

MOZ_ALWAYS_INLINE size_t
GetFlatStringLength(JSFlatString* s)
{
    return reinterpret_cast<shadow::String*>(s)->length;
}

MOZ_ALWAYS_INLINE size_t
GetLinearStringLength(JSLinearString* s)
{
    return reinterpret_cast<shadow::String*>(s)->length;
}

MOZ_ALWAYS_INLINE bool
LinearStringHasLatin1Chars(JSLinearString* s)
{
    return reinterpret_cast<shadow::String*>(s)->flags & shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE bool
AtomHasLatin1Chars(JSAtom* atom)
{
    return reinterpret_cast<shadow::String*>(atom)->flags & shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE bool
StringHasLatin1Chars(JSString* s)
{
    return reinterpret_cast<shadow::String*>(s)->flags & shadow::String::LATIN1_CHARS_BIT;
}

MOZ_ALWAYS_INLINE const JS::Latin1Char*
GetLatin1LinearStringChars(const JS::AutoCheckCannotGC& nogc, JSLinearString* linear)
{
    MOZ_ASSERT(LinearStringHasLatin1Chars(linear));

    using shadow::String;
    String* s = reinterpret_cast<String*>(linear);
    if (s->flags & String::INLINE_CHARS_BIT)
        return s->inlineStorageLatin1;
    return s->nonInlineCharsLatin1;
}

MOZ_ALWAYS_INLINE const char16_t*
GetTwoByteLinearStringChars(const JS::AutoCheckCannotGC& nogc, JSLinearString* linear)
{
    MOZ_ASSERT(!LinearStringHasLatin1Chars(linear));

    using shadow::String;
    String* s = reinterpret_cast<String*>(linear);
    if (s->flags & String::INLINE_CHARS_BIT)
        return s->inlineStorageTwoByte;
    return s->nonInlineCharsTwoByte;
}

MOZ_ALWAYS_INLINE JSLinearString*
AtomToLinearString(JSAtom* atom)
{
    return reinterpret_cast<JSLinearString*>(atom);
}

MOZ_ALWAYS_INLINE JSFlatString*
AtomToFlatString(JSAtom* atom)
{
    return reinterpret_cast<JSFlatString*>(atom);
}

MOZ_ALWAYS_INLINE JSLinearString*
FlatStringToLinearString(JSFlatString* s)
{
    return reinterpret_cast<JSLinearString*>(s);
}

MOZ_ALWAYS_INLINE const JS::Latin1Char*
GetLatin1AtomChars(const JS::AutoCheckCannotGC& nogc, JSAtom* atom)
{
    return GetLatin1LinearStringChars(nogc, AtomToLinearString(atom));
}

MOZ_ALWAYS_INLINE const char16_t*
GetTwoByteAtomChars(const JS::AutoCheckCannotGC& nogc, JSAtom* atom)
{
    return GetTwoByteLinearStringChars(nogc, AtomToLinearString(atom));
}

JS_FRIEND_API(JSLinearString*)
StringToLinearStringSlow(JSContext* cx, JSString* str);

MOZ_ALWAYS_INLINE JSLinearString*
StringToLinearString(JSContext* cx, JSString* str)
{
    using shadow::String;
    String* s = reinterpret_cast<String*>(str);
    if (MOZ_UNLIKELY((s->flags & String::TYPE_FLAGS_MASK) == String::ROPE_FLAGS))
        return StringToLinearStringSlow(cx, str);
    return reinterpret_cast<JSLinearString*>(str);
}

MOZ_ALWAYS_INLINE void
CopyLinearStringChars(char16_t* dest, JSLinearString* s, size_t len)
{
    JS::AutoCheckCannotGC nogc;
    if (LinearStringHasLatin1Chars(s)) {
        const JS::Latin1Char* src = GetLatin1LinearStringChars(nogc, s);
        for (size_t i = 0; i < len; i++)
            dest[i] = src[i];
    } else {
        const char16_t* src = GetTwoByteLinearStringChars(nogc, s);
        mozilla::PodCopy(dest, src, len);
    }
}

inline bool
CopyStringChars(JSContext* cx, char16_t* dest, JSString* s, size_t len)
{
    JSLinearString* linear = StringToLinearString(cx, s);
    if (!linear)
        return false;

    CopyLinearStringChars(dest, linear, len);
    return true;
}

inline void
CopyFlatStringChars(char16_t* dest, JSFlatString* s, size_t len)
{
    CopyLinearStringChars(dest, FlatStringToLinearString(s), len);
}

JS_FRIEND_API(bool)
GetPropertyKeys(JSContext* cx, JS::HandleObject obj, unsigned flags, JS::AutoIdVector* props);

JS_FRIEND_API(bool)
AppendUnique(JSContext* cx, JS::AutoIdVector& base, JS::AutoIdVector& others);

JS_FRIEND_API(bool)
GetGeneric(JSContext* cx, JSObject* obj, JSObject* receiver, jsid id, JS::Value* vp);

JS_FRIEND_API(bool)
StringIsArrayIndex(JSLinearString* str, uint32_t* indexp);

JS_FRIEND_API(void)
SetPreserveWrapperCallback(JSRuntime* rt, PreserveWrapperCallback callback);

JS_FRIEND_API(bool)
IsObjectInContextCompartment(JSObject* obj, const JSContext* cx);






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* get obj[key] for each property */
#define JSITER_KEYVALUE   0x4   /* obsolete destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */
#define JSITER_SYMBOLS    0x20  /* also include symbol property keys */
#define JSITER_SYMBOLSONLY 0x40 /* exclude string property keys */

JS_FRIEND_API(bool)
RunningWithTrustedPrincipals(JSContext* cx);

inline uintptr_t
GetNativeStackLimit(JSContext* cx, StackKind kind, int extraAllowance = 0)
{
    PerThreadDataFriendFields* mainThread =
      PerThreadDataFriendFields::getMainThread(GetRuntime(cx));
    uintptr_t limit = mainThread->nativeStackLimit[kind];
#if JS_STACK_GROWTH_DIRECTION > 0
    limit += extraAllowance;
#else
    limit -= extraAllowance;
#endif
    return limit;
}

inline uintptr_t
GetNativeStackLimit(JSContext* cx, int extraAllowance = 0)
{
    StackKind kind = RunningWithTrustedPrincipals(cx) ? StackForTrustedScript
                                                      : StackForUntrustedScript;
    return GetNativeStackLimit(cx, kind, extraAllowance);
}










#define JS_CHECK_RECURSION_LIMIT(cx, limit, onerror)                            \
    JS_BEGIN_MACRO                                                              \
        int stackDummy_;                                                        \
        if (!JS_CHECK_STACK_SIZE(limit, &stackDummy_)) {                        \
            js::ReportOverRecursed(cx);                                         \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

#define JS_CHECK_RECURSION(cx, onerror)                                         \
    JS_CHECK_RECURSION_LIMIT(cx, js::GetNativeStackLimit(cx), onerror)

#define JS_CHECK_RECURSION_DONT_REPORT(cx, onerror)                             \
    JS_BEGIN_MACRO                                                              \
        int stackDummy_;                                                        \
        if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(cx), &stackDummy_)) {  \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

#define JS_CHECK_RECURSION_WITH_SP_DONT_REPORT(cx, sp, onerror)                 \
    JS_BEGIN_MACRO                                                              \
        if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(cx), sp)) {            \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

#define JS_CHECK_RECURSION_WITH_SP(cx, sp, onerror)                             \
    JS_BEGIN_MACRO                                                              \
        if (!JS_CHECK_STACK_SIZE(js::GetNativeStackLimit(cx), sp)) {            \
            js::ReportOverRecursed(cx);                                         \
            onerror;                                                            \
        }                                                                       \
    JS_END_MACRO

#define JS_CHECK_SYSTEM_RECURSION(cx, onerror)                                  \
    JS_CHECK_RECURSION_LIMIT(cx, js::GetNativeStackLimit(cx, js::StackForSystemCode), onerror)

#define JS_CHECK_RECURSION_CONSERVATIVE(cx, onerror)                            \
    JS_CHECK_RECURSION_LIMIT(cx, js::GetNativeStackLimit(cx, js::StackForUntrustedScript, -1024 * int(sizeof(size_t))), onerror)

JS_FRIEND_API(void)
StartPCCountProfiling(JSContext* cx);

JS_FRIEND_API(void)
StopPCCountProfiling(JSContext* cx);

JS_FRIEND_API(void)
PurgePCCounts(JSContext* cx);

JS_FRIEND_API(size_t)
GetPCCountScriptCount(JSContext* cx);

JS_FRIEND_API(JSString*)
GetPCCountScriptSummary(JSContext* cx, size_t script);

JS_FRIEND_API(JSString*)
GetPCCountScriptContents(JSContext* cx, size_t script);

JS_FRIEND_API(bool)
ContextHasOutstandingRequests(const JSContext* cx);

typedef void
(* ActivityCallback)(void* arg, bool active);






JS_FRIEND_API(void)
SetActivityCallback(JSRuntime* rt, ActivityCallback cb, void* arg);

extern JS_FRIEND_API(const JSStructuredCloneCallbacks*)
GetContextStructuredCloneCallbacks(JSContext* cx);

extern JS_FRIEND_API(bool)
IsContextRunningJS(JSContext* cx);

typedef bool
(* DOMInstanceClassHasProtoAtDepth)(const Class* instanceClass,
                                    uint32_t protoID, uint32_t depth);
struct JSDOMCallbacks {
    DOMInstanceClassHasProtoAtDepth instanceClassMatchesProto;
};
typedef struct JSDOMCallbacks DOMCallbacks;

extern JS_FRIEND_API(void)
SetDOMCallbacks(JSRuntime* rt, const DOMCallbacks* callbacks);

extern JS_FRIEND_API(const DOMCallbacks*)
GetDOMCallbacks(JSRuntime* rt);

extern JS_FRIEND_API(JSObject*)
GetTestingFunctions(JSContext* cx);






inline JSFreeOp*
CastToJSFreeOp(FreeOp* fop)
{
    return reinterpret_cast<JSFreeOp*>(fop);
}







extern JS_FRIEND_API(JSFlatString*)
GetErrorTypeName(JSRuntime* rt, int16_t exnType);

#ifdef JS_DEBUG
extern JS_FRIEND_API(unsigned)
GetEnterCompartmentDepth(JSContext* cx);
#endif

class RegExpGuard;
extern JS_FRIEND_API(bool)
RegExpToSharedNonInline(JSContext* cx, JS::HandleObject regexp, RegExpGuard* shared);


typedef enum NukeReferencesToWindow {
    NukeWindowReferences,
    DontNukeWindowReferences
} NukeReferencesToWindow;





struct CompartmentFilter {
    virtual bool match(JSCompartment* c) const = 0;
};

struct AllCompartments : public CompartmentFilter {
    virtual bool match(JSCompartment* c) const override { return true; }
};

struct ContentCompartmentsOnly : public CompartmentFilter {
    virtual bool match(JSCompartment* c) const override {
        return !IsSystemCompartment(c);
    }
};

struct ChromeCompartmentsOnly : public CompartmentFilter {
    virtual bool match(JSCompartment* c) const override {
        return IsSystemCompartment(c);
    }
};

struct SingleCompartment : public CompartmentFilter {
    JSCompartment* ours;
    explicit SingleCompartment(JSCompartment* c) : ours(c) {}
    virtual bool match(JSCompartment* c) const override { return c == ours; }
};

struct CompartmentsWithPrincipals : public CompartmentFilter {
    JSPrincipals* principals;
    explicit CompartmentsWithPrincipals(JSPrincipals* p) : principals(p) {}
    virtual bool match(JSCompartment* c) const override {
        return JS_GetCompartmentPrincipals(c) == principals;
    }
};

extern JS_FRIEND_API(bool)
NukeCrossCompartmentWrappers(JSContext* cx,
                             const CompartmentFilter& sourceFilter,
                             const CompartmentFilter& targetFilter,
                             NukeReferencesToWindow nukeReferencesToWindow);


























struct ExpandoAndGeneration {
  ExpandoAndGeneration()
    : expando(JS::UndefinedValue()),
      generation(0)
  {}

  void Unlink()
  {
      ++generation;
      expando.setUndefined();
  }

  static size_t offsetOfExpando()
  {
      return offsetof(ExpandoAndGeneration, expando);
  }

  static size_t offsetOfGeneration()
  {
      return offsetof(ExpandoAndGeneration, generation);
  }

  JS::Heap<JS::Value> expando;
  uint32_t generation;
};

typedef enum DOMProxyShadowsResult {
  ShadowCheckFailed,
  Shadows,
  DoesntShadow,
  DoesntShadowUnique,
  ShadowsViaDirectExpando,
  ShadowsViaIndirectExpando
} DOMProxyShadowsResult;
typedef DOMProxyShadowsResult
(* DOMProxyShadowsCheck)(JSContext* cx, JS::HandleObject object, JS::HandleId id);
JS_FRIEND_API(void)
SetDOMProxyInformation(const void* domProxyHandlerFamily, uint32_t domProxyExpandoSlot,
                       DOMProxyShadowsCheck domProxyShadowsCheck);

const void* GetDOMProxyHandlerFamily();
uint32_t GetDOMProxyExpandoSlot();
DOMProxyShadowsCheck GetDOMProxyShadowsCheck();
inline bool DOMProxyIsShadowing(DOMProxyShadowsResult result) {
    return result == Shadows ||
           result == ShadowsViaDirectExpando ||
           result == ShadowsViaIndirectExpando;
}







extern JS_FRIEND_API(bool)
DateIsValid(JSContext* cx, JSObject* obj);

extern JS_FRIEND_API(double)
DateGetMsecSinceEpoch(JSContext* cx, JSObject* obj);

} 







typedef enum JSErrNum {
#define MSG_DEF(name, count, exception, format) \
    name,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

namespace js {

extern JS_FRIEND_API(const JSErrorFormatString*)
GetErrorMessage(void* userRef, const unsigned errorNumber);













class MOZ_STACK_CLASS AutoStableStringChars
{
    
    JS::RootedString s_;
    union {
        const char16_t* twoByteChars_;
        const JS::Latin1Char* latin1Chars_;
    };
    enum State { Uninitialized, Latin1, TwoByte };
    State state_;
    bool ownsChars_;

  public:
    explicit AutoStableStringChars(JSContext* cx)
      : s_(cx), state_(Uninitialized), ownsChars_(false)
    {}
    ~AutoStableStringChars();

    bool init(JSContext* cx, JSString* s);

    
    bool initTwoByte(JSContext* cx, JSString* s);

    bool isLatin1() const { return state_ == Latin1; }
    bool isTwoByte() const { return state_ == TwoByte; }

    const char16_t* twoByteChars() const {
        MOZ_ASSERT(state_ == TwoByte);
        return twoByteChars_;
    }

    mozilla::Range<const JS::Latin1Char> latin1Range() const {
        MOZ_ASSERT(state_ == Latin1);
        return mozilla::Range<const JS::Latin1Char>(latin1Chars_,
                                                    GetStringLength(s_));
    }

    mozilla::Range<const char16_t> twoByteRange() const {
        MOZ_ASSERT(state_ == TwoByte);
        return mozilla::Range<const char16_t>(twoByteChars_,
                                            GetStringLength(s_));
    }

    
    bool maybeGiveOwnershipToCaller() {
        MOZ_ASSERT(state_ != Uninitialized);
        if (!ownsChars_)
            return false;
        state_ = Uninitialized;
        ownsChars_ = false;
        return true;
    }

  private:
    AutoStableStringChars(const AutoStableStringChars& other) = delete;
    void operator=(const AutoStableStringChars& other) = delete;
};



extern JS_FRIEND_API(JSString*)
ErrorReportToString(JSContext* cx, JSErrorReport* reportp);

struct MOZ_STACK_CLASS JS_FRIEND_API(ErrorReport)
{
    explicit ErrorReport(JSContext* cx);
    ~ErrorReport();

    bool init(JSContext* cx, JS::HandleValue exn);

    JSErrorReport* report()
    {
        return reportp;
    }

    const char* message()
    {
        return message_;
    }

  private:
    
    
    
    
    
    
    bool populateUncaughtExceptionReport(JSContext* cx, ...);
    bool populateUncaughtExceptionReportVA(JSContext* cx, va_list ap);

    
    JSErrorReport* reportp;

    
    const char* message_;

    
    JSErrorReport ownedReport;

    
    
    char* ownedMessage;

    
    
    JS::RootedString str;

    
    AutoStableStringChars strChars;

    
    JS::RootedObject exnObject;

    
    JSAutoByteString bytesStorage;

    
    JSAutoByteString filename;

    
    bool ownsMessageAndReport;
};


extern JS_FRIEND_API(uint64_t)
GetSCOffset(JSStructuredCloneWriter* writer);

namespace Scalar {






enum Type {
    Int8 = 0,
    Uint8,
    Int16,
    Uint16,
    Int32,
    Uint32,
    Float32,
    Float64,

    



    Uint8Clamped,

    


    MaxTypedArrayViewType,

    Float32x4,
    Int32x4
};

static inline size_t
byteSize(Type atype)
{
    switch (atype) {
      case Int8:
      case Uint8:
      case Uint8Clamped:
        return 1;
      case Int16:
      case Uint16:
        return 2;
      case Int32:
      case Uint32:
      case Float32:
        return 4;
      case Float64:
        return 8;
      case Int32x4:
      case Float32x4:
        return 16;
      default:
        MOZ_CRASH("invalid scalar type");
    }
}

static inline bool
isSignedIntType(Type atype) {
    switch (atype) {
      case Int8:
      case Int16:
      case Int32:
      case Int32x4:
        return true;
      case Uint8:
      case Uint8Clamped:
      case Uint16:
      case Uint32:
      case Float32:
      case Float64:
      case Float32x4:
        return false;
      default:
        MOZ_CRASH("invalid scalar type");
    }
}

static inline bool
isSimdType(Type atype) {
    switch (atype) {
      case Int8:
      case Uint8:
      case Uint8Clamped:
      case Int16:
      case Uint16:
      case Int32:
      case Uint32:
      case Float32:
      case Float64:
        return false;
      case Int32x4:
      case Float32x4:
        return true;
      case MaxTypedArrayViewType:
        break;
    }
    MOZ_CRASH("invalid scalar type");
}

static inline size_t
scalarByteSize(Type atype) {
    switch (atype) {
      case Int32x4:
      case Float32x4:
        return 4;
      case Int8:
      case Uint8:
      case Uint8Clamped:
      case Int16:
      case Uint16:
      case Int32:
      case Uint32:
      case Float32:
      case Float64:
      case MaxTypedArrayViewType:
        break;
    }
    MOZ_CRASH("invalid simd type");
}

} 
} 







extern JS_FRIEND_API(JSObject*)
JS_NewInt8Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8ClampedArray(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewInt16Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewUint16Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewInt32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewUint32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat64Array(JSContext* cx, uint32_t nelements);

extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt8Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint8Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint8ClampedArray(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt16Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint16Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedFloat32Array(JSContext* cx, uint32_t nelements);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedFloat64Array(JSContext* cx, uint32_t nelements);









extern JS_FRIEND_API(JSObject*)
JS_NewInt8ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8ClampedArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewInt16ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewUint16ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewInt32ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewUint32ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat32ArrayFromArray(JSContext* cx, JS::HandleObject array);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat64ArrayFromArray(JSContext* cx, JS::HandleObject array);







extern JS_FRIEND_API(JSObject*)
JS_NewInt8ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                          uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewUint8ClampedArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                  uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewInt16ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewUint16ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                            uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewInt32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                           uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewUint32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                            uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                             uint32_t byteOffset, int32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewFloat64ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                             uint32_t byteOffset, int32_t length);




extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt8ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint8ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                 uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint8ClampedArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                        uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt16ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                 uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint16ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                  uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedInt32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                 uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedUint32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                  uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedFloat32ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                   uint32_t byteOffset, uint32_t length);
extern JS_FRIEND_API(JSObject*)
JS_NewSharedFloat64ArrayWithBuffer(JSContext* cx, JS::HandleObject arrayBuffer,
                                   uint32_t byteOffset, uint32_t length);




extern JS_FRIEND_API(JSObject*)
JS_NewArrayBuffer(JSContext* cx, uint32_t nbytes);







extern JS_FRIEND_API(bool)
JS_IsTypedArrayObject(JSObject* obj);




extern JS_FRIEND_API(bool)
JS_IsSharedTypedArrayObject(JSObject* obj);








extern JS_FRIEND_API(bool)
JS_IsArrayBufferViewObject(JSObject* obj);





extern JS_FRIEND_API(bool)
JS_IsInt8Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsUint8Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsUint8ClampedArray(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsInt16Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsUint16Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsInt32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsUint32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsFloat32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsFloat64Array(JSObject* obj);

extern JS_FRIEND_API(bool)
JS_IsSharedInt8Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedUint8Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedUint8ClampedArray(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedInt16Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedUint16Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedInt32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedUint32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedFloat32Array(JSObject* obj);
extern JS_FRIEND_API(bool)
JS_IsSharedFloat64Array(JSObject* obj);






namespace js {

extern JS_FRIEND_API(JSObject*)
UnwrapInt8Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapUint8Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapUint8ClampedArray(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapInt16Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapUint16Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapInt32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapUint32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapFloat32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapFloat64Array(JSObject* obj);

extern JS_FRIEND_API(JSObject*)
UnwrapArrayBuffer(JSObject* obj);

extern JS_FRIEND_API(JSObject*)
UnwrapArrayBufferView(JSObject* obj);

extern JS_FRIEND_API(JSObject*)
UnwrapSharedInt8Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedUint8Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedUint8ClampedArray(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedInt16Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedUint16Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedInt32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedUint32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedFloat32Array(JSObject* obj);
extern JS_FRIEND_API(JSObject*)
UnwrapSharedFloat64Array(JSObject* obj);

namespace detail {

extern JS_FRIEND_DATA(const Class* const) Int8ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Uint8ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Uint8ClampedArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Int16ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Uint16ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Int32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Uint32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Float32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) Float64ArrayClassPtr;

extern JS_FRIEND_DATA(const Class* const) SharedInt8ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedUint8ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedUint8ClampedArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedInt16ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedUint16ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedInt32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedUint32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedFloat32ArrayClassPtr;
extern JS_FRIEND_DATA(const Class* const) SharedFloat64ArrayClassPtr;

const size_t TypedArrayLengthSlot = 1;

} 






#define JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Type, type) \
inline void \
Get ## Type ## ArrayLengthAndData(JSObject* obj, uint32_t* length, type** data) \
{ \
    MOZ_ASSERT(GetObjectClass(obj) == detail::Type ## ArrayClassPtr); \
    const JS::Value& slot = GetReservedSlot(obj, detail::TypedArrayLengthSlot); \
    *length = mozilla::AssertedCast<uint32_t>(slot.toInt32()); \
    *data = static_cast<type*>(GetObjectPrivate(obj)); \
}

JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Int8, int8_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Uint8, uint8_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Uint8Clamped, uint8_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Int16, int16_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Uint16, uint16_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Int32, int32_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Uint32, uint32_t)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Float32, float)
JS_DEFINE_DATA_AND_LENGTH_ACCESSOR(Float64, double)

#undef JS_DEFINE_DATA_AND_LENGTH_ACCESSOR



extern JS_FRIEND_API(void)
GetArrayBufferViewLengthAndData(JSObject* obj, uint32_t* length, uint8_t** data);



extern JS_FRIEND_API(void)
GetArrayBufferLengthAndData(JSObject* obj, uint32_t* length, uint8_t** data);

} 






extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsInt8Array(JSObject* obj, uint32_t* length, int8_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsUint8Array(JSObject* obj, uint32_t* length, uint8_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsUint8ClampedArray(JSObject* obj, uint32_t* length, uint8_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsInt16Array(JSObject* obj, uint32_t* length, int16_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsUint16Array(JSObject* obj, uint32_t* length, uint16_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsInt32Array(JSObject* obj, uint32_t* length, int32_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsUint32Array(JSObject* obj, uint32_t* length, uint32_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsFloat32Array(JSObject* obj, uint32_t* length, float** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsFloat64Array(JSObject* obj, uint32_t* length, double** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsArrayBufferView(JSObject* obj, uint32_t* length, uint8_t** data);
extern JS_FRIEND_API(JSObject*)
JS_GetObjectAsArrayBuffer(JSObject* obj, uint32_t* length, uint8_t** data);








extern JS_FRIEND_API(js::Scalar::Type)
JS_GetArrayBufferViewType(JSObject* obj);







extern JS_FRIEND_API(bool)
JS_IsArrayBufferObject(JSObject* obj);

extern JS_FRIEND_API(bool)
JS_IsSharedArrayBufferObject(JSObject* obj);








extern JS_FRIEND_API(uint32_t)
JS_GetArrayBufferByteLength(JSObject* obj);









extern JS_FRIEND_API(bool)
JS_ArrayBufferHasData(JSObject* obj);






extern JS_FRIEND_API(bool)
JS_IsMappedArrayBufferObject(JSObject* obj);








extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayLength(JSObject* obj);









extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteOffset(JSObject* obj);








extern JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteLength(JSObject* obj);






extern JS_FRIEND_API(bool)
JS_IsArrayBufferViewObject(JSObject* obj);




extern JS_FRIEND_API(uint32_t)
JS_GetArrayBufferViewByteLength(JSObject* obj);













extern JS_FRIEND_API(uint8_t*)
JS_GetArrayBufferData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(int8_t*)
JS_GetInt8ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(uint8_t*)
JS_GetUint8ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(uint8_t*)
JS_GetUint8ClampedArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(int16_t*)
JS_GetInt16ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(uint16_t*)
JS_GetUint16ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(int32_t*)
JS_GetInt32ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(uint32_t*)
JS_GetUint32ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(float*)
JS_GetFloat32ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);
extern JS_FRIEND_API(double*)
JS_GetFloat64ArrayData(JSObject* obj, const JS::AutoCheckCannotGC&);





extern JS_FRIEND_API(void*)
JS_GetArrayBufferViewData(JSObject* obj, const JS::AutoCheckCannotGC&);






extern JS_FRIEND_API(JSObject*)
JS_GetArrayBufferViewBuffer(JSContext* cx, JS::HandleObject obj);

typedef enum {
    ChangeData,
    KeepData
} NeuterDataDisposition;










extern JS_FRIEND_API(bool)
JS_NeuterArrayBuffer(JSContext* cx, JS::HandleObject obj,
                     NeuterDataDisposition changeData);






extern JS_FRIEND_API(bool)
JS_IsNeuteredArrayBufferObject(JSObject* obj);




JS_FRIEND_API(bool)
JS_IsDataViewObject(JSObject* obj);









JS_FRIEND_API(uint32_t)
JS_GetDataViewByteOffset(JSObject* obj);









JS_FRIEND_API(uint32_t)
JS_GetDataViewByteLength(JSObject* obj);









JS_FRIEND_API(void*)
JS_GetDataViewData(JSObject* obj, const JS::AutoCheckCannotGC&);

namespace js {










extern JS_FRIEND_API(bool)
WatchGuts(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);









extern JS_FRIEND_API(bool)
UnwatchGuts(JSContext* cx, JS::HandleObject obj, JS::HandleId id);

} 





class JSJitGetterCallArgs : protected JS::MutableHandleValue
{
  public:
    explicit JSJitGetterCallArgs(const JS::CallArgs& args)
      : JS::MutableHandleValue(args.rval())
    {}

    explicit JSJitGetterCallArgs(JS::RootedValue* rooted)
      : JS::MutableHandleValue(rooted)
    {}

    JS::MutableHandleValue rval() {
        return *this;
    }
};





class JSJitSetterCallArgs : protected JS::MutableHandleValue
{
  public:
    explicit JSJitSetterCallArgs(const JS::CallArgs& args)
      : JS::MutableHandleValue(args[0])
    {}

    JS::MutableHandleValue operator[](unsigned i) {
        MOZ_ASSERT(i == 0);
        return *this;
    }

    unsigned length() const { return 1; }

    
};

struct JSJitMethodCallArgsTraits;





class JSJitMethodCallArgs : protected JS::detail::CallArgsBase<JS::detail::NoUsedRval>
{
  private:
    typedef JS::detail::CallArgsBase<JS::detail::NoUsedRval> Base;
    friend struct JSJitMethodCallArgsTraits;

  public:
    explicit JSJitMethodCallArgs(const JS::CallArgs& args) {
        argv_ = args.array();
        argc_ = args.length();
    }

    JS::MutableHandleValue rval() const {
        return Base::rval();
    }

    unsigned length() const { return Base::length(); }

    JS::MutableHandleValue operator[](unsigned i) const {
        return Base::operator[](i);
    }

    bool hasDefined(unsigned i) const {
        return Base::hasDefined(i);
    }

    JSObject& callee() const {
        
        
        return argv_[-2].toObject();
    }

    
};

struct JSJitMethodCallArgsTraits
{
    static const size_t offsetOfArgv = offsetof(JSJitMethodCallArgs, argv_);
    static const size_t offsetOfArgc = offsetof(JSJitMethodCallArgs, argc_);
};






typedef bool
(* JSJitGetterOp)(JSContext* cx, JS::HandleObject thisObj,
                  void* specializedThis, JSJitGetterCallArgs args);
typedef bool
(* JSJitSetterOp)(JSContext* cx, JS::HandleObject thisObj,
                  void* specializedThis, JSJitSetterCallArgs args);
typedef bool
(* JSJitMethodOp)(JSContext* cx, JS::HandleObject thisObj,
                  void* specializedThis, const JSJitMethodCallArgs& args);

struct JSJitInfo {
    enum OpType {
        Getter,
        Setter,
        Method,
        StaticMethod,
        
        OpTypeCount
    };

    enum ArgType {
        
        String = (1 << 0),
        Integer = (1 << 1), 
        Double = (1 << 2), 
        Boolean = (1 << 3),
        Object = (1 << 4),
        Null = (1 << 5),

        
        Numeric = Integer | Double,
        
        
        Primitive = Numeric | Boolean | Null | String,
        ObjectOrNull = Object | Null,
        Any = ObjectOrNull | Primitive,

        
        ArgTypeListEnd = (1 << 31)
    };

    static_assert(Any & String, "Any must include String.");
    static_assert(Any & Integer, "Any must include Integer.");
    static_assert(Any & Double, "Any must include Double.");
    static_assert(Any & Boolean, "Any must include Boolean.");
    static_assert(Any & Object, "Any must include Object.");
    static_assert(Any & Null, "Any must include Null.");

    enum AliasSet {
        
        
        

        
        
        AliasNone,

        
        
        AliasDOMSets,

        
        
        AliasEverything,

        
        AliasSetCount
    };

    bool needsOuterizedThisObject() const
    {
        return type() != Getter && type() != Setter;
    }

    bool isTypedMethodJitInfo() const
    {
        return isTypedMethod;
    }

    OpType type() const
    {
        return OpType(type_);
    }

    AliasSet aliasSet() const
    {
        return AliasSet(aliasSet_);
    }

    JSValueType returnType() const
    {
        return JSValueType(returnType_);
    }

    union {
        JSJitGetterOp getter;
        JSJitSetterOp setter;
        JSJitMethodOp method;
        
        JSNative staticMethod;
    };

    uint16_t protoID;
    uint16_t depth;

    
    
    

#define JITINFO_OP_TYPE_BITS 4
#define JITINFO_ALIAS_SET_BITS 4
#define JITINFO_RETURN_TYPE_BITS 8

    
    uint32_t type_ : JITINFO_OP_TYPE_BITS;

    
    
    
    
    uint32_t aliasSet_ : JITINFO_ALIAS_SET_BITS;

    
    uint32_t returnType_ : JITINFO_RETURN_TYPE_BITS;

    static_assert(OpTypeCount <= (1 << JITINFO_OP_TYPE_BITS),
                  "Not enough space for OpType");
    static_assert(AliasSetCount <= (1 << JITINFO_ALIAS_SET_BITS),
                  "Not enough space for AliasSet");
    static_assert((sizeof(JSValueType) * 8) <= JITINFO_RETURN_TYPE_BITS,
                  "Not enough space for JSValueType");

#undef JITINFO_RETURN_TYPE_BITS
#undef JITINFO_ALIAS_SET_BITS
#undef JITINFO_OP_TYPE_BITS

    uint32_t isInfallible : 1; 
    uint32_t isMovable : 1;    



    
    uint32_t isAlwaysInSlot : 1; 


    uint32_t isLazilyCachedInSlot : 1; 



    uint32_t isTypedMethod : 1; 

    uint32_t slotIndex : 11;   


};

static_assert(sizeof(JSJitInfo) == (sizeof(void*) + 2 * sizeof(uint32_t)),
              "There are several thousand instances of JSJitInfo stored in "
              "a binary. Please don't increase its space requirements without "
              "verifying that there is no other way forward (better packing, "
              "smaller datatypes for fields, subclassing, etc.).");

struct JSTypedMethodJitInfo
{
    
    
    
    
    
    
    
    
    JSJitInfo base;

    const JSJitInfo::ArgType* const argTypes; 





};

namespace js {

static MOZ_ALWAYS_INLINE shadow::Function*
FunctionObjectToShadowFunction(JSObject* fun)
{
    MOZ_ASSERT(GetObjectClass(fun) == FunctionClassPtr);
    return reinterpret_cast<shadow::Function*>(fun);
}


static const unsigned JS_FUNCTION_INTERPRETED_BITS = 0x401;


static MOZ_ALWAYS_INLINE bool
FunctionObjectIsNative(JSObject* fun)
{
    return !(FunctionObjectToShadowFunction(fun)->flags & JS_FUNCTION_INTERPRETED_BITS);
}

static MOZ_ALWAYS_INLINE JSNative
GetFunctionObjectNative(JSObject* fun)
{
    MOZ_ASSERT(FunctionObjectIsNative(fun));
    return FunctionObjectToShadowFunction(fun)->native;
}

} 

static MOZ_ALWAYS_INLINE const JSJitInfo*
FUNCTION_VALUE_TO_JITINFO(const JS::Value& v)
{
    MOZ_ASSERT(js::FunctionObjectIsNative(&v.toObject()));
    return js::FunctionObjectToShadowFunction(&v.toObject())->jitinfo;
}

static MOZ_ALWAYS_INLINE void
SET_JITINFO(JSFunction * func, const JSJitInfo* info)
{
    js::shadow::Function* fun = reinterpret_cast<js::shadow::Function*>(func);
    MOZ_ASSERT(!(fun->flags & js::JS_FUNCTION_INTERPRETED_BITS));
    fun->jitinfo = info;
}






static MOZ_ALWAYS_INLINE jsid
JSID_FROM_BITS(size_t bits)
{
    jsid id;
    JSID_BITS(id) = bits;
    return id;
}

namespace js {
namespace detail {
bool IdMatchesAtom(jsid id, JSAtom* atom);
}
}






















static MOZ_ALWAYS_INLINE jsid
NON_INTEGER_ATOM_TO_JSID(JSAtom* atom)
{
    MOZ_ASSERT(((size_t)atom & 0x7) == 0);
    jsid id = JSID_FROM_BITS((size_t)atom);
    MOZ_ASSERT(js::detail::IdMatchesAtom(id, atom));
    return id;
}


static MOZ_ALWAYS_INLINE bool
JSID_IS_ATOM(jsid id)
{
    return JSID_IS_STRING(id);
}

static MOZ_ALWAYS_INLINE bool
JSID_IS_ATOM(jsid id, JSAtom* atom)
{
    return id == JSID_FROM_BITS((size_t)atom);
}

static MOZ_ALWAYS_INLINE JSAtom*
JSID_TO_ATOM(jsid id)
{
    return (JSAtom*)JSID_TO_STRING(id);
}

JS_STATIC_ASSERT(sizeof(jsid) == sizeof(void*));

namespace js {

static MOZ_ALWAYS_INLINE JS::Value
IdToValue(jsid id)
{
    if (JSID_IS_STRING(id))
        return JS::StringValue(JSID_TO_STRING(id));
    if (JSID_IS_INT(id))
        return JS::Int32Value(JSID_TO_INT(id));
    if (JSID_IS_SYMBOL(id))
        return JS::SymbolValue(JSID_TO_SYMBOL(id));
    MOZ_ASSERT(JSID_IS_VOID(id));
    return JS::UndefinedValue();
}






extern JS_FRIEND_API(JSContext*)
DefaultJSContext(JSRuntime* rt);

typedef JSContext*
(* DefaultJSContextCallback)(JSRuntime* rt);

JS_FRIEND_API(void)
SetDefaultJSContextCallback(JSRuntime* rt, DefaultJSContextCallback cb);







#ifdef DEBUG
JS_FRIEND_API(void)
Debug_SetActiveJSContext(JSRuntime* rt, JSContext* cx);
#else
inline void
Debug_SetActiveJSContext(JSRuntime* rt, JSContext* cx) {}
#endif

enum CTypesActivityType {
    CTYPES_CALL_BEGIN,
    CTYPES_CALL_END,
    CTYPES_CALLBACK_BEGIN,
    CTYPES_CALLBACK_END
};

typedef void
(* CTypesActivityCallback)(JSContext* cx, CTypesActivityType type);





JS_FRIEND_API(void)
SetCTypesActivityCallback(JSRuntime* rt, CTypesActivityCallback cb);

class JS_FRIEND_API(AutoCTypesActivityCallback) {
  private:
    JSContext* cx;
    CTypesActivityCallback callback;
    CTypesActivityType endType;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoCTypesActivityCallback(JSContext* cx, CTypesActivityType beginType,
                               CTypesActivityType endType
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoCTypesActivityCallback() {
        DoEndCallback();
    }
    void DoEndCallback() {
        if (callback) {
            callback(cx, endType);
            callback = nullptr;
        }
    }
};

typedef JSObject*
(* ObjectMetadataCallback)(JSContext* cx);






JS_FRIEND_API(void)
SetObjectMetadataCallback(JSContext* cx, ObjectMetadataCallback callback);


JS_FRIEND_API(JSObject*)
GetObjectMetadata(JSObject* obj);

JS_FRIEND_API(bool)
GetElementsWithAdder(JSContext* cx, JS::HandleObject obj, JS::HandleObject receiver,
                     uint32_t begin, uint32_t end, js::ElementAdder* adder);

JS_FRIEND_API(bool)
ForwardToNative(JSContext* cx, JSNative native, const JS::CallArgs& args);





















JS_FRIEND_API(bool)
SetPropertyIgnoringNamedGetter(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                               JS::HandleValue v, JS::HandleValue receiver,
                               JS::Handle<JSPropertyDescriptor> ownDesc,
                               JS::ObjectOpResult& result);

JS_FRIEND_API(void)
ReportErrorWithId(JSContext* cx, const char* msg, JS::HandleId id);


extern JS_FRIEND_API(bool)
ExecuteInGlobalAndReturnScope(JSContext* cx, JS::HandleObject obj, JS::HandleScript script,
                              JS::MutableHandleObject scope);

#if defined(XP_WIN) && defined(_WIN64)


typedef long
(*JitExceptionHandler)(void* exceptionRecord,  
                       void* context);         


















extern JS_FRIEND_API(void)
SetJitExceptionHandler(JitExceptionHandler handler);
#endif







extern JS_FRIEND_API(JSObject*)
GetObjectEnvironmentObjectForFunction(JSFunction* fun);






extern JS_FRIEND_API(JSPrincipals*)
GetSavedFramePrincipals(JS::HandleObject savedFrame);









extern JS_FRIEND_API(JSObject*)
GetFirstSubsumedSavedFrame(JSContext* cx, JS::HandleObject savedFrame);

extern JS_FRIEND_API(bool)
ReportIsNotFunction(JSContext* cx, JS::HandleValue v);

extern JS_FRIEND_API(bool)
DefineOwnProperty(JSContext* cx, JSObject* objArg, jsid idArg,
                  JS::Handle<JSPropertyDescriptor> descriptor, JS::ObjectOpResult& result);

} 

extern JS_FRIEND_API(void)
JS_StoreObjectPostBarrierCallback(JSContext* cx,
                                  void (*callback)(JSTracer* trc, JSObject* key, void* data),
                                  JSObject* key, void* data);

extern JS_FRIEND_API(void)
JS_StoreStringPostBarrierCallback(JSContext* cx,
                                  void (*callback)(JSTracer* trc, JSString* key, void* data),
                                  JSString* key, void* data);

#endif 

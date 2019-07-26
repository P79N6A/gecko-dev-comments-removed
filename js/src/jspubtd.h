





#ifndef jspubtd_h
#define jspubtd_h





#include "mozilla/PodOperations.h"

#include "jsprototypes.h"
#include "jstypes.h"

#include "js/TypeDecls.h"

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING) || defined(DEBUG)
# define JSGC_TRACK_EXACT_ROOTS
#endif

namespace JS {

class AutoIdVector;
class CallArgs;

template <typename T>
class Rooted;

class JS_PUBLIC_API(AutoGCRooter);

class JS_PUBLIC_API(CompileOptions);
class JS_PUBLIC_API(CompartmentOptions);

struct Zone;

} 






typedef enum JSVersion {
    JSVERSION_ECMA_3  = 148,
    JSVERSION_1_6     = 160,
    JSVERSION_1_7     = 170,
    JSVERSION_1_8     = 180,
    JSVERSION_ECMA_5  = 185,
    JSVERSION_DEFAULT = 0,
    JSVERSION_UNKNOWN = -1,
    JSVERSION_LATEST  = JSVERSION_ECMA_5
} JSVersion;


typedef enum JSType {
    JSTYPE_VOID,                
    JSTYPE_OBJECT,              
    JSTYPE_FUNCTION,            
    JSTYPE_STRING,              
    JSTYPE_NUMBER,              
    JSTYPE_BOOLEAN,             
    JSTYPE_NULL,                
    JSTYPE_LIMIT
} JSType;


typedef enum JSProtoKey {
#define PROTOKEY_AND_INITIALIZER(name,code,init) JSProto_##name = code,
    JS_FOR_EACH_PROTOTYPE(PROTOKEY_AND_INITIALIZER)
#undef PROTOKEY_AND_INITIALIZER
    JSProto_LIMIT
} JSProtoKey;


typedef enum JSAccessMode {
    JSACC_PROTO  = 0,           

                                




                                




    JSACC_WATCH  = 3,           
    JSACC_READ   = 4,           
    JSACC_WRITE  = 8,           
    JSACC_LIMIT
} JSAccessMode;

#define JSACC_TYPEMASK          (JSACC_WRITE - 1)





typedef enum JSIterateOp {
    
    JSENUMERATE_INIT,

    
    JSENUMERATE_INIT_ALL,

    
    JSENUMERATE_NEXT,

    
    JSENUMERATE_DESTROY
} JSIterateOp;


typedef enum {
    JSTRACE_OBJECT,
    JSTRACE_STRING,
    JSTRACE_SCRIPT,

    



    JSTRACE_LAZY_SCRIPT,
    JSTRACE_IONCODE,
    JSTRACE_SHAPE,
    JSTRACE_BASE_SHAPE,
    JSTRACE_TYPE_OBJECT,
    JSTRACE_LAST = JSTRACE_TYPE_OBJECT
} JSGCTraceKind;


typedef struct JSClass                      JSClass;
typedef struct JSCompartment                JSCompartment;
typedef struct JSConstDoubleSpec            JSConstDoubleSpec;
typedef struct JSCrossCompartmentCall       JSCrossCompartmentCall;
typedef struct JSErrorReport                JSErrorReport;
typedef struct JSExceptionState             JSExceptionState;
typedef struct JSFunctionSpec               JSFunctionSpec;
typedef struct JSIdArray                    JSIdArray;
typedef struct JSLocaleCallbacks            JSLocaleCallbacks;
typedef struct JSObjectMap                  JSObjectMap;
typedef struct JSPrincipals                 JSPrincipals;
typedef struct JSPropertyDescriptor         JSPropertyDescriptor;
typedef struct JSPropertyName               JSPropertyName;
typedef struct JSPropertySpec               JSPropertySpec;
typedef struct JSRuntime                    JSRuntime;
typedef struct JSSecurityCallbacks          JSSecurityCallbacks;
typedef struct JSStructuredCloneCallbacks   JSStructuredCloneCallbacks;
typedef struct JSStructuredCloneReader      JSStructuredCloneReader;
typedef struct JSStructuredCloneWriter      JSStructuredCloneWriter;
typedef struct JSTracer                     JSTracer;

class                                       JSFlatString;
class                                       JSStableString;  

#ifdef JS_THREADSAFE
typedef struct PRCallOnceType   JSCallOnceType;
#else
typedef bool                    JSCallOnceType;
#endif
typedef bool                    (*JSInitCallback)(void);





typedef void
(* JSTraceDataOp)(JSTracer *trc, void *data);

namespace js {
namespace gc {
class StoreBuffer;
}
}

namespace JS {

typedef void (*OffThreadCompileCallback)(void *token, void *callbackData);

namespace shadow {

struct Runtime
{
    
    bool needsBarrier_;

#ifdef JSGC_GENERATIONAL
    
    uintptr_t gcNurseryStart_;
    uintptr_t gcNurseryEnd_;

  private:
    js::gc::StoreBuffer *gcStoreBufferPtr_;
#endif

  public:
    Runtime(
#ifdef JSGC_GENERATIONAL
        js::gc::StoreBuffer *storeBuffer
#endif
    )
      : needsBarrier_(false)
#ifdef JSGC_GENERATIONAL
      , gcNurseryStart_(0)
      , gcNurseryEnd_(0)
      , gcStoreBufferPtr_(storeBuffer)
#endif
    {}

    bool needsBarrier() const {
        return needsBarrier_;
    }

#ifdef JSGC_GENERATIONAL
    js::gc::StoreBuffer *gcStoreBufferPtr() { return gcStoreBufferPtr_; }
#endif

    static JS::shadow::Runtime *asShadowRuntime(JSRuntime *rt) {
        return reinterpret_cast<JS::shadow::Runtime*>(rt);
    }
};

} 
} 

namespace js {







enum ParallelResult { TP_SUCCESS, TP_RETRY_SEQUENTIALLY, TP_RETRY_AFTER_GC, TP_FATAL };

struct ThreadSafeContext;
struct ForkJoinSlice;
class ExclusiveContext;

class Allocator;

class SkipRoot;

enum ThingRootKind
{
    THING_ROOT_OBJECT,
    THING_ROOT_SHAPE,
    THING_ROOT_BASE_SHAPE,
    THING_ROOT_TYPE_OBJECT,
    THING_ROOT_STRING,
    THING_ROOT_ION_CODE,
    THING_ROOT_SCRIPT,
    THING_ROOT_ID,
    THING_ROOT_PROPERTY_ID,
    THING_ROOT_VALUE,
    THING_ROOT_TYPE,
    THING_ROOT_BINDINGS,
    THING_ROOT_PROPERTY_DESCRIPTOR,
    THING_ROOT_LIMIT
};






enum StackKind
{
    StackForSystemCode,      
    StackForTrustedScript,   
    StackForUntrustedScript, 
    StackKindCount
};

template <typename T>
struct RootKind;






template<typename T, ThingRootKind Kind>
struct SpecificRootKind
{
    static ThingRootKind rootKind() { return Kind; }
};

template <> struct RootKind<JSObject *> : SpecificRootKind<JSObject *, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSFlatString *> : SpecificRootKind<JSFlatString *, THING_ROOT_STRING> {};
template <> struct RootKind<JSFunction *> : SpecificRootKind<JSFunction *, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSString *> : SpecificRootKind<JSString *, THING_ROOT_STRING> {};
template <> struct RootKind<JSScript *> : SpecificRootKind<JSScript *, THING_ROOT_SCRIPT> {};
template <> struct RootKind<jsid> : SpecificRootKind<jsid, THING_ROOT_ID> {};
template <> struct RootKind<JS::Value> : SpecificRootKind<JS::Value, THING_ROOT_VALUE> {};

struct ContextFriendFields
{
  protected:
    JSRuntime *const     runtime_;

    
    JSCompartment       *compartment_;

    
    JS::Zone            *zone_;

  public:
    explicit ContextFriendFields(JSRuntime *rt)
      : runtime_(rt), compartment_(NULL), zone_(NULL), autoGCRooters(NULL)
    {
#ifdef JSGC_TRACK_EXACT_ROOTS
        mozilla::PodArrayZero(thingGCRooters);
#endif
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
        skipGCRooters = NULL;
#endif
    }

    static const ContextFriendFields *get(const JSContext *cx) {
        return reinterpret_cast<const ContextFriendFields *>(cx);
    }

    static ContextFriendFields *get(JSContext *cx) {
        return reinterpret_cast<ContextFriendFields *>(cx);
    }

#ifdef JSGC_TRACK_EXACT_ROOTS
    



    JS::Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    







    SkipRoot *skipGCRooters;
#endif

    
    JS::AutoGCRooter   *autoGCRooters;

    friend JSRuntime *GetRuntime(const JSContext *cx);
    friend JSCompartment *GetContextCompartment(const JSContext *cx);
    friend JS::Zone *GetContextZone(const JSContext *cx);
};

class PerThreadData;

struct PerThreadDataFriendFields
{
  private:
    
    
    
    struct RuntimeDummy : JS::shadow::Runtime
    {
        struct PerThreadDummy {
            void *field1;
            uintptr_t field2;
#ifdef DEBUG
            uint64_t field3;
#endif
        } mainThread;
    };

  public:

    PerThreadDataFriendFields();

#ifdef JSGC_TRACK_EXACT_ROOTS
    



    JS::Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    







    SkipRoot *skipGCRooters;
#endif

    
    uintptr_t nativeStackLimit[StackKindCount];

    static const size_t RuntimeMainThreadOffset = offsetof(RuntimeDummy, mainThread);

    static inline PerThreadDataFriendFields *get(js::PerThreadData *pt) {
        return reinterpret_cast<PerThreadDataFriendFields *>(pt);
    }

    static inline PerThreadDataFriendFields *getMainThread(JSRuntime *rt) {
        
        
        return reinterpret_cast<PerThreadDataFriendFields *>(
            reinterpret_cast<char*>(rt) + RuntimeMainThreadOffset);
    }

    static inline const PerThreadDataFriendFields *getMainThread(const JSRuntime *rt) {
        
        
        return reinterpret_cast<const PerThreadDataFriendFields *>(
            reinterpret_cast<const char*>(rt) + RuntimeMainThreadOffset);
    }
};

} 

#endif 







#ifndef jspubtd_h___
#define jspubtd_h___





#include "jsprototypes.h"
#include "jstypes.h"

#ifdef __cplusplus

namespace JS {





class Value;

template <typename T>
class Rooted;

} 

#endif 
















#ifdef __cplusplus

# if defined(DEBUG) && !defined(JS_NO_JSVAL_JSID_STRUCT_TYPES)
#  define JS_USE_JSID_STRUCT_TYPES
# endif

# ifdef JS_USE_JSID_STRUCT_TYPES
struct jsid
{
    size_t asBits;
    bool operator==(jsid rhs) const { return asBits == rhs.asBits; }
    bool operator!=(jsid rhs) const { return asBits != rhs.asBits; }
};
#  define JSID_BITS(id) (id.asBits)
# else  
typedef ptrdiff_t jsid;
#  define JSID_BITS(id) (id)
# endif  
#else  
typedef ptrdiff_t jsid;
# define JSID_BITS(id) (id)
#endif

#ifdef WIN32
typedef wchar_t   jschar;
#else
typedef uint16_t  jschar;
#endif






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
#undef JS_PROTO
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

    



    JSTRACE_IONCODE,
    JSTRACE_SHAPE,
    JSTRACE_BASE_SHAPE,
    JSTRACE_TYPE_OBJECT,
    JSTRACE_LAST = JSTRACE_TYPE_OBJECT
} JSGCTraceKind;


typedef struct JSClass                      JSClass;
typedef struct JSCompartment                JSCompartment;
typedef struct JSConstDoubleSpec            JSConstDoubleSpec;
typedef struct JSContext                    JSContext;
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

#ifdef __cplusplus
class                                       JSFlatString;
class                                       JSFunction;
class                                       JSObject;
class                                       JSScript;
class                                       JSStableString;  
class                                       JSString;
#else
typedef struct JSFlatString                 JSFlatString;
typedef struct JSFunction                   JSFunction;
typedef struct JSObject                     JSObject;
typedef struct JSScript                     JSScript;
typedef struct JSString                     JSString;
#endif 

#ifdef JS_THREADSAFE
typedef struct PRCallOnceType    JSCallOnceType;
#else
typedef JSBool                   JSCallOnceType;
#endif
typedef JSBool                 (*JSInitCallback)(void);

#ifdef __cplusplus

namespace js {

class Allocator;

class SkipRoot;

enum ThingRootKind
{
    THING_ROOT_OBJECT,
    THING_ROOT_SHAPE,
    THING_ROOT_BASE_SHAPE,
    THING_ROOT_TYPE_OBJECT,
    THING_ROOT_STRING,
    THING_ROOT_SCRIPT,
    THING_ROOT_ID,
    THING_ROOT_PROPERTY_ID,
    THING_ROOT_VALUE,
    THING_ROOT_TYPE,
    THING_ROOT_BINDINGS,
    THING_ROOT_LIMIT
};

template <typename T>
struct RootKind;






template<typename T, ThingRootKind Kind>
struct SpecificRootKind
{
    static ThingRootKind rootKind() { return Kind; }
};

template <> struct RootKind<JSObject *> : SpecificRootKind<JSObject *, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSFunction *> : SpecificRootKind<JSFunction *, THING_ROOT_OBJECT> {};
template <> struct RootKind<JSString *> : SpecificRootKind<JSString *, THING_ROOT_STRING> {};
template <> struct RootKind<JSScript *> : SpecificRootKind<JSScript *, THING_ROOT_SCRIPT> {};
template <> struct RootKind<jsid> : SpecificRootKind<jsid, THING_ROOT_ID> {};
template <> struct RootKind<JS::Value> : SpecificRootKind<JS::Value, THING_ROOT_VALUE> {};

struct ContextFriendFields {
    JSRuntime *const    runtime;

    
    JSCompartment       *compartment;

    explicit ContextFriendFields(JSRuntime *rt)
      : runtime(rt), compartment(NULL)
    { }

    static const ContextFriendFields *get(const JSContext *cx) {
        return reinterpret_cast<const ContextFriendFields *>(cx);
    }

    static ContextFriendFields *get(JSContext *cx) {
        return reinterpret_cast<ContextFriendFields *>(cx);
    }

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    



    JS::Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    







    SkipRoot *skipGCRooters;
#endif
};

struct RuntimeFriendFields {
    



    volatile int32_t    interrupt;

    RuntimeFriendFields()
      : interrupt(0) { }

    static const RuntimeFriendFields *get(const JSRuntime *rt) {
        return reinterpret_cast<const RuntimeFriendFields *>(rt);
    }
};

class PerThreadData;

struct PerThreadDataFriendFields
{
  private:
    
    
    
    struct RuntimeDummy : RuntimeFriendFields
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

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    



    JS::Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    







    SkipRoot *skipGCRooters;
#endif

    
    uintptr_t nativeStackLimit;

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

#endif 

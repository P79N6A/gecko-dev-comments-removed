





#ifndef jspubtd_h___
#define jspubtd_h___




#include "jstypes.h"





#ifdef __cplusplus
namespace JS { class Value; }
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

JS_BEGIN_EXTERN_C

#ifdef WIN32
typedef wchar_t   jschar;
#else
typedef uint16_t  jschar;
#endif






typedef enum JSVersion {
    JSVERSION_1_0     = 100,
    JSVERSION_1_1     = 110,
    JSVERSION_1_2     = 120,
    JSVERSION_1_3     = 130,
    JSVERSION_1_4     = 140,
    JSVERSION_ECMA_3  = 148,
    JSVERSION_1_5     = 150,
    JSVERSION_1_6     = 160,
    JSVERSION_1_7     = 170,
    JSVERSION_1_8     = 180,
    JSVERSION_ECMA_5  = 185,
    JSVERSION_DEFAULT = 0,
    JSVERSION_UNKNOWN = -1,
    JSVERSION_LATEST  = JSVERSION_ECMA_5
} JSVersion;

#define JSVERSION_IS_ECMA(version) \
    ((version) == JSVERSION_DEFAULT || (version) >= JSVERSION_1_3)


typedef enum JSType {
    JSTYPE_VOID,                
    JSTYPE_OBJECT,              
    JSTYPE_FUNCTION,            
    JSTYPE_STRING,              
    JSTYPE_NUMBER,              
    JSTYPE_BOOLEAN,             
    JSTYPE_NULL,                
    JSTYPE_XML,                 
    JSTYPE_LIMIT
} JSType;


typedef enum JSProtoKey {
#define JS_PROTO(name,code,init) JSProto_##name = code,
#include "jsproto.tbl"
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
#if JS_HAS_XML_SUPPORT
    JSTRACE_XML,
#endif
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
typedef struct JSFunction                   JSFunction;
typedef struct JSFunctionSpec               JSFunctionSpec;
typedef struct JSIdArray                    JSIdArray;
typedef struct JSLocaleCallbacks            JSLocaleCallbacks;
typedef struct JSObject                     JSObject;
typedef struct JSObjectMap                  JSObjectMap;
typedef struct JSPrincipals                 JSPrincipals;
typedef struct JSPropertyDescriptor         JSPropertyDescriptor;
typedef struct JSPropertyName               JSPropertyName;
typedef struct JSPropertySpec               JSPropertySpec;
typedef struct JSRuntime                    JSRuntime;
typedef struct JSSecurityCallbacks          JSSecurityCallbacks;
typedef struct JSStackFrame                 JSStackFrame;
typedef struct JSScript          JSScript;
typedef struct JSStructuredCloneCallbacks   JSStructuredCloneCallbacks;
typedef struct JSStructuredCloneReader      JSStructuredCloneReader;
typedef struct JSStructuredCloneWriter      JSStructuredCloneWriter;
typedef struct JSTracer                     JSTracer;

#ifdef __cplusplus
class                                       JSFlatString;
class                                       JSString;
#else
typedef struct JSFlatString                 JSFlatString;
typedef struct JSString                     JSString;
#endif 

#ifdef JS_THREADSAFE
typedef struct PRCallOnceType    JSCallOnceType;
#else
typedef JSBool                   JSCallOnceType;
#endif
typedef JSBool                 (*JSInitCallback)(void);

JS_END_EXTERN_C

#ifdef __cplusplus

namespace JS {

template <typename T>
class Rooted;

class SkipRoot;

enum ThingRootKind
{
    THING_ROOT_OBJECT,
    THING_ROOT_SHAPE,
    THING_ROOT_BASE_SHAPE,
    THING_ROOT_TYPE_OBJECT,
    THING_ROOT_STRING,
    THING_ROOT_SCRIPT,
    THING_ROOT_XML,
    THING_ROOT_ID,
    THING_ROOT_PROPERTY_ID,
    THING_ROOT_VALUE,
    THING_ROOT_TYPE,
    THING_ROOT_LIMIT
};

template <typename T>
struct RootKind;






template <> struct RootKind<JSObject *> { static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }; };
template <> struct RootKind<JSFunction *> { static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }; };
template <> struct RootKind<JSString *> { static ThingRootKind rootKind() { return THING_ROOT_STRING; }; };
template <> struct RootKind<JSScript *> { static ThingRootKind rootKind() { return THING_ROOT_SCRIPT; }; };
template <> struct RootKind<jsid> { static ThingRootKind rootKind() { return THING_ROOT_ID; }; };
template <> struct RootKind<Value> { static ThingRootKind rootKind() { return THING_ROOT_VALUE; }; };

struct ContextFriendFields {
    JSRuntime *const    runtime;

    ContextFriendFields(JSRuntime *rt)
      : runtime(rt) { }

    static const ContextFriendFields *get(const JSContext *cx) {
        return reinterpret_cast<const ContextFriendFields *>(cx);
    }

    static ContextFriendFields *get(JSContext *cx) {
        return reinterpret_cast<ContextFriendFields *>(cx);
    }

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    



    Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    







    SkipRoot *skipGCRooters;
#endif
};

struct RuntimeFriendFields {
    



    volatile int32_t    interrupt;

    
    uintptr_t           nativeStackLimit;

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    



    Rooted<void*> *thingGCRooters[THING_ROOT_LIMIT];
#endif

    RuntimeFriendFields()
      : interrupt(0),
        nativeStackLimit(0) { }

    static const RuntimeFriendFields *get(const JSRuntime *rt) {
        return reinterpret_cast<const RuntimeFriendFields *>(rt);
    }
};

} 

#endif 

#endif 

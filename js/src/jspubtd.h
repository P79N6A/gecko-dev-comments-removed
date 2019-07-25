






































#ifndef jspubtd_h___
#define jspubtd_h___



#include "jstypes.h"
#include "jscompat.h"





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


typedef JSInt32   jsint;
typedef JSUint32  jsuint;
typedef float64   jsdouble;
typedef JSInt32   jsrefcount;   

#ifdef WIN32
typedef wchar_t   jschar;
#else
typedef JSUint16  jschar;
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
    JSACC_PARENT = 1,           

                                




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

    



#if JS_HAS_XML_SUPPORT
    JSTRACE_XML,
#endif
    JSTRACE_SHAPE,
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
typedef struct JSXDRState                   JSXDRState;

#ifdef __cplusplus
class                                       JSFlatString;
class                                       JSString;
#else
typedef struct JSFlatString                 JSFlatString;
typedef struct JSString                     JSString;
#endif

JS_END_EXTERN_C

#endif 

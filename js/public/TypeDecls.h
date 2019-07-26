















#ifndef js_TypeDecls_h
#define js_TypeDecls_h

#include <stddef.h>
#include <stdint.h>

struct JSContext;
class JSFunction;
class JSObject;
class JSScript;
class JSString;















#if defined(DEBUG) && !defined(JS_NO_JSVAL_JSID_STRUCT_TYPES)
# define JS_USE_JSID_STRUCT_TYPES
#endif

#ifdef JS_USE_JSID_STRUCT_TYPES
struct jsid;
#else
typedef ptrdiff_t jsid;
#endif

#ifdef WIN32
typedef wchar_t  jschar;
#else
typedef uint16_t jschar;
#endif

namespace JS {

class Value;
template <typename T> class Handle;
template <typename T> class MutableHandle;

typedef Handle<JSFunction*> HandleFunction;
typedef Handle<jsid>        HandleId;
typedef Handle<JSObject*>   HandleObject;
typedef Handle<JSScript*>   HandleScript;
typedef Handle<JSString*>   HandleString;
typedef Handle<Value>       HandleValue;

typedef MutableHandle<JSFunction*> MutableHandleFunction;
typedef MutableHandle<jsid>        MutableHandleId;
typedef MutableHandle<JSObject*>   MutableHandleObject;
typedef MutableHandle<JSScript*>   MutableHandleScript;
typedef MutableHandle<JSString*>   MutableHandleString;
typedef MutableHandle<Value>       MutableHandleValue;

} 

#endif 

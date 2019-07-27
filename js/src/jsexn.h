









#ifndef jsexn_h
#define jsexn_h

#include "jsapi.h"
#include "NamespaceImports.h"

namespace js {
class ErrorObject;

JSErrorReport*
CopyErrorReport(JSContext* cx, JSErrorReport* report);

JSString*
ComputeStackString(JSContext* cx);



























extern bool
ErrorToException(JSContext* cx, const char* message, JSErrorReport* reportp,
                 JSErrorCallback callback, void* userRef);

















extern bool
ReportUncaughtException(JSContext* cx);

extern JSErrorReport*
ErrorFromException(JSContext* cx, HandleObject obj);








extern JSObject*
CopyErrorObject(JSContext* cx, JS::Handle<ErrorObject*> errobj);

static_assert(JSEXN_ERR == 0 &&
              JSProto_Error + JSEXN_INTERNALERR == JSProto_InternalError &&
              JSProto_Error + JSEXN_EVALERR == JSProto_EvalError &&
              JSProto_Error + JSEXN_RANGEERR == JSProto_RangeError &&
              JSProto_Error + JSEXN_REFERENCEERR == JSProto_ReferenceError &&
              JSProto_Error + JSEXN_SYNTAXERR == JSProto_SyntaxError &&
              JSProto_Error + JSEXN_TYPEERR == JSProto_TypeError &&
              JSProto_Error + JSEXN_URIERR == JSProto_URIError &&
              JSEXN_URIERR + 1 == JSEXN_LIMIT,
              "GetExceptionProtoKey and ExnTypeFromProtoKey require that "
              "each corresponding JSExnType and JSProtoKey value be separated "
              "by the same constant value");

static inline JSProtoKey
GetExceptionProtoKey(JSExnType exn)
{
    MOZ_ASSERT(JSEXN_ERR <= exn);
    MOZ_ASSERT(exn < JSEXN_LIMIT);
    return JSProtoKey(JSProto_Error + int(exn));
}

static inline JSExnType
ExnTypeFromProtoKey(JSProtoKey key)
{
    JSExnType type = static_cast<JSExnType>(key - JSProto_Error);
    MOZ_ASSERT(type >= JSEXN_ERR);
    MOZ_ASSERT(type < JSEXN_LIMIT);
    return type;
}

class AutoClearPendingException
{
    JSContext* cx;

  public:
    explicit AutoClearPendingException(JSContext* cxArg)
      : cx(cxArg)
    { }

    ~AutoClearPendingException() {
        JS_ClearPendingException(cx);
    }
};

extern const char*
ValueToSourceForError(JSContext* cx, HandleValue val, JSAutoByteString& bytes);

} 

#endif 

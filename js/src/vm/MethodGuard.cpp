






































#include "jsproxy.h"

#include "MethodGuard.h"
#include "Stack.h"

#include "jsfuninlines.h"
#include "jsobjinlines.h"

using namespace js;

void
js::ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp)
{
    Value &thisv = call.thisv();

#ifdef DEBUG
    if (thisv.isObject()) {
        JS_ASSERT(thisv.toObject().getClass() != clasp ||
                  !thisv.toObject().getProto() ||
                  thisv.toObject().getProto()->getClass() != clasp);
    } else if (thisv.isString()) {
        JS_ASSERT(clasp != &StringClass);
    } else if (thisv.isNumber()) {
        JS_ASSERT(clasp != &NumberClass);
    } else if (thisv.isBoolean()) {
        JS_ASSERT(clasp != &BooleanClass);
    } else {
        JS_ASSERT(thisv.isUndefined() || thisv.isNull());
    }
#endif

    if (JSFunction *fun = js_ValueToFunction(cx, &call.calleev(), 0)) {
        JSAutoByteString funNameBytes;
        if (const char *funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                                 clasp->name, funName, InformalValueTypeName(thisv));
        }
    }
}

bool
js::HandleNonGenericMethodClassMismatch(JSContext *cx, CallArgs args, Native native, Class *clasp)
{
    if (args.thisv().isObject()) {
        JSObject &thisObj = args.thisv().toObject();
        if (thisObj.isProxy())
            return Proxy::nativeCall(cx, &thisObj, clasp, native, args);
    }

    ReportIncompatibleMethod(cx, args, clasp);
    return false;
}

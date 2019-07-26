









#include "jsexn.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/PodOperations.h"

#include <string.h>

#include "jsapi.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jswrapper.h"

#include "gc/Marking.h"
#include "vm/ErrorObject.h"
#include "vm/GlobalObject.h"
#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

#include "vm/ErrorObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::ArrayLength;
using mozilla::PodArrayZero;
using mozilla::PodZero;

static void
exn_finalize(FreeOp *fop, JSObject *obj);

const Class ErrorObject::class_ = {
    js_Error_str,
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Error) |
    JSCLASS_HAS_RESERVED_SLOTS(ErrorObject::RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    exn_finalize,
    nullptr,                 
    nullptr,                 
    nullptr                  
};

JSErrorReport *
js::CopyErrorReport(JSContext *cx, JSErrorReport *report)
{
    












    JS_STATIC_ASSERT(sizeof(JSErrorReport) % sizeof(const char *) == 0);
    JS_STATIC_ASSERT(sizeof(const char *) % sizeof(jschar) == 0);

    size_t filenameSize;
    size_t linebufSize;
    size_t uclinebufSize;
    size_t ucmessageSize;
    size_t i, argsArraySize, argsCopySize, argSize;
    size_t mallocSize;
    JSErrorReport *copy;
    uint8_t *cursor;

#define JS_CHARS_SIZE(jschars) ((js_strlen(jschars) + 1) * sizeof(jschar))

    filenameSize = report->filename ? strlen(report->filename) + 1 : 0;
    linebufSize = report->linebuf ? strlen(report->linebuf) + 1 : 0;
    uclinebufSize = report->uclinebuf ? JS_CHARS_SIZE(report->uclinebuf) : 0;
    ucmessageSize = 0;
    argsArraySize = 0;
    argsCopySize = 0;
    if (report->ucmessage) {
        ucmessageSize = JS_CHARS_SIZE(report->ucmessage);
        if (report->messageArgs) {
            for (i = 0; report->messageArgs[i]; ++i)
                argsCopySize += JS_CHARS_SIZE(report->messageArgs[i]);

            
            JS_ASSERT(i != 0);
            argsArraySize = (i + 1) * sizeof(const jschar *);
        }
    }

    



    mallocSize = sizeof(JSErrorReport) + argsArraySize + argsCopySize +
                 ucmessageSize + uclinebufSize + linebufSize + filenameSize;
    cursor = cx->pod_malloc<uint8_t>(mallocSize);
    if (!cursor)
        return nullptr;

    copy = (JSErrorReport *)cursor;
    memset(cursor, 0, sizeof(JSErrorReport));
    cursor += sizeof(JSErrorReport);

    if (argsArraySize != 0) {
        copy->messageArgs = (const jschar **)cursor;
        cursor += argsArraySize;
        for (i = 0; report->messageArgs[i]; ++i) {
            copy->messageArgs[i] = (const jschar *)cursor;
            argSize = JS_CHARS_SIZE(report->messageArgs[i]);
            js_memcpy(cursor, report->messageArgs[i], argSize);
            cursor += argSize;
        }
        copy->messageArgs[i] = nullptr;
        JS_ASSERT(cursor == (uint8_t *)copy->messageArgs[0] + argsCopySize);
    }

    if (report->ucmessage) {
        copy->ucmessage = (const jschar *)cursor;
        js_memcpy(cursor, report->ucmessage, ucmessageSize);
        cursor += ucmessageSize;
    }

    if (report->uclinebuf) {
        copy->uclinebuf = (const jschar *)cursor;
        js_memcpy(cursor, report->uclinebuf, uclinebufSize);
        cursor += uclinebufSize;
        if (report->uctokenptr) {
            copy->uctokenptr = copy->uclinebuf + (report->uctokenptr -
                                                  report->uclinebuf);
        }
    }

    if (report->linebuf) {
        copy->linebuf = (const char *)cursor;
        js_memcpy(cursor, report->linebuf, linebufSize);
        cursor += linebufSize;
        if (report->tokenptr) {
            copy->tokenptr = copy->linebuf + (report->tokenptr -
                                              report->linebuf);
        }
    }

    if (report->filename) {
        copy->filename = (const char *)cursor;
        js_memcpy(cursor, report->filename, filenameSize);
    }
    JS_ASSERT(cursor + filenameSize == (uint8_t *)copy + mallocSize);

    
    copy->originPrincipals = report->originPrincipals;

    
    copy->lineno = report->lineno;
    copy->column = report->column;
    copy->errorNumber = report->errorNumber;
    copy->exnType = report->exnType;

    
    copy->flags = report->flags;

#undef JS_CHARS_SIZE
    return copy;
}

struct SuppressErrorsGuard
{
    JSContext *cx;
    JSErrorReporter prevReporter;
    JS::AutoSaveExceptionState prevState;

    explicit SuppressErrorsGuard(JSContext *cx)
      : cx(cx),
        prevReporter(JS_SetErrorReporter(cx, nullptr)),
        prevState(cx)
    {}

    ~SuppressErrorsGuard()
    {
        JS_SetErrorReporter(cx, prevReporter);
    }
};

JSString *
js::ComputeStackString(JSContext *cx)
{
    StringBuffer sb(cx);

    {
        RootedAtom atom(cx);
        SuppressErrorsGuard seg(cx);
        for (NonBuiltinFrameIter i(cx, FrameIter::ALL_CONTEXTS, FrameIter::GO_THROUGH_SAVED,
                                   cx->compartment()->principals);
             !i.done();
             ++i)
        {
            
            if (i.isNonEvalFunctionFrame())
                atom = i.functionDisplayAtom();
            else
                atom = nullptr;
            if (atom && !sb.append(atom))
                return nullptr;

            
            if (!sb.append('@'))
                return nullptr;

            
            const char *cfilename = i.scriptFilename();
            if (!cfilename)
                cfilename = "";
            if (!sb.appendInflated(cfilename, strlen(cfilename)))
                return nullptr;

            uint32_t column = 0;
            uint32_t line = i.computeLine(&column);
            
            if (!sb.append(':') || !NumberValueToStringBuffer(cx, NumberValue(line), sb))
                return nullptr;

            
            
            if (!sb.append(':') || !NumberValueToStringBuffer(cx, NumberValue(column + 1), sb) ||
                !sb.append('\n'))
            {
                return nullptr;
            }

            



            const size_t MaxReportedStackDepth = 1u << 20;
            if (sb.length() > MaxReportedStackDepth)
                break;
        }
    }

    return sb.finishString();
}

static void
exn_finalize(FreeOp *fop, JSObject *obj)
{
    if (JSErrorReport *report = obj->as<ErrorObject>().getErrorReport()) {
        
        if (JSPrincipals *prin = report->originPrincipals)
            JS_DropPrincipals(fop->runtime(), prin);
        fop->free_(report);
    }
}

JSErrorReport *
js_ErrorFromException(JSContext *cx, HandleObject objArg)
{
    
    
    
    
    
    
    RootedObject obj(cx, UncheckedUnwrap(objArg));
    if (!obj->is<ErrorObject>())
        return nullptr;

    return obj->as<ErrorObject>().getOrCreateErrorReport(cx);
}

static bool
Error(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString message(cx, nullptr);
    if (args.hasDefined(0)) {
        message = ToString<CanGC>(cx, args[0]);
        if (!message)
            return false;
    }

    
    NonBuiltinFrameIter iter(cx);

    
    RootedString fileName(cx);
    if (args.length() > 1) {
        fileName = ToString<CanGC>(cx, args[1]);
    } else {
        fileName = cx->runtime()->emptyString;
        if (!iter.done()) {
            if (const char *cfilename = iter.scriptFilename())
                fileName = JS_NewStringCopyZ(cx, cfilename);
        }
    }
    if (!fileName)
        return false;

    
    uint32_t lineNumber, columnNumber = 0;
    if (args.length() > 2) {
        if (!ToUint32(cx, args[2], &lineNumber))
            return false;
    } else {
        lineNumber = iter.done() ? 0 : iter.computeLine(&columnNumber);
    }

    Rooted<JSString*> stack(cx, ComputeStackString(cx));
    if (!stack)
        return false;

    





    JSExnType exnType = JSExnType(args.callee().as<JSFunction>().getExtendedSlot(0).toInt32());

    RootedObject obj(cx, ErrorObject::create(cx, exnType, stack, fileName,
                                             lineNumber, columnNumber, nullptr, message));
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}


static bool
exn_toString(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!args.thisv().isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_PROTOTYPE, "Error");
        return false;
    }

    
    RootedObject obj(cx, &args.thisv().toObject());

    
    RootedValue nameVal(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().name, &nameVal))
        return false;

    
    RootedString name(cx);
    if (nameVal.isUndefined()) {
        name = cx->names().Error;
    } else {
        name = ToString<CanGC>(cx, nameVal);
        if (!name)
            return false;
    }

    
    RootedValue msgVal(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().message, &msgVal))
        return false;

    
    RootedString message(cx);
    if (msgVal.isUndefined()) {
        message = cx->runtime()->emptyString;
    } else {
        message = ToString<CanGC>(cx, msgVal);
        if (!message)
            return false;
    }

    
    if (name->empty() && message->empty()) {
        args.rval().setString(cx->names().Error);
        return true;
    }

    
    if (name->empty()) {
        args.rval().setString(message);
        return true;
    }

    
    if (message->empty()) {
        args.rval().setString(name);
        return true;
    }

    
    StringBuffer sb(cx);
    if (!sb.append(name) || !sb.append(": ") || !sb.append(message))
        return false;

    JSString *str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

#if JS_HAS_TOSOURCE



static bool
exn_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedValue nameVal(cx);
    RootedString name(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().name, &nameVal) ||
        !(name = ToString<CanGC>(cx, nameVal)))
    {
        return false;
    }

    RootedValue messageVal(cx);
    RootedString message(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().message, &messageVal) ||
        !(message = ValueToSource(cx, messageVal)))
    {
        return false;
    }

    RootedValue filenameVal(cx);
    RootedString filename(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().fileName, &filenameVal) ||
        !(filename = ValueToSource(cx, filenameVal)))
    {
        return false;
    }

    RootedValue linenoVal(cx);
    uint32_t lineno;
    if (!JSObject::getProperty(cx, obj, obj, cx->names().lineNumber, &linenoVal) ||
        !ToUint32(cx, linenoVal, &lineno))
    {
        return false;
    }

    StringBuffer sb(cx);
    if (!sb.append("(new ") || !sb.append(name) || !sb.append("("))
        return false;

    if (!sb.append(message))
        return false;

    if (!filename->empty()) {
        if (!sb.append(", ") || !sb.append(filename))
            return false;
    }
    if (lineno != 0) {
        
        if (filename->empty() && !sb.append(", \"\""))
                return false;

        JSString *linenumber = ToString<CanGC>(cx, linenoVal);
        if (!linenumber)
            return false;
        if (!sb.append(", ") || !sb.append(linenumber))
            return false;
    }

    if (!sb.append("))"))
        return false;

    JSString *str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}
#endif

static const JSFunctionSpec exception_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,   exn_toSource,           0,0),
#endif
    JS_FN(js_toString_str,   exn_toString,           0,0),
    JS_FS_END
};


JS_STATIC_ASSERT(JSEXN_ERR == 0);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_INTERNALERR  == JSProto_InternalError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_EVALERR      == JSProto_EvalError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_RANGEERR     == JSProto_RangeError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_REFERENCEERR == JSProto_ReferenceError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_SYNTAXERR    == JSProto_SyntaxError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_TYPEERR      == JSProto_TypeError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_URIERR       == JSProto_URIError);

 ErrorObject *
ErrorObject::createProto(JSContext *cx, JS::Handle<GlobalObject*> global, JSExnType type,
                         JS::HandleObject proto)
{
    RootedObject errorProto(cx);
    errorProto = global->createBlankPrototypeInheriting(cx, &ErrorObject::class_, *proto);
    if (!errorProto)
        return nullptr;

    Rooted<ErrorObject*> err(cx, &errorProto->as<ErrorObject>());
    RootedString emptyStr(cx, cx->names().empty);
    if (!ErrorObject::init(cx, err, type, nullptr, emptyStr, emptyStr, 0, 0, emptyStr))
        return nullptr;

    
    
    JSProtoKey key = GetExceptionProtoKey(type);
    RootedPropertyName name(cx, ClassName(key, cx));
    RootedValue nameValue(cx, StringValue(name));
    if (!JSObject::defineProperty(cx, err, cx->names().name, nameValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
        return nullptr;
    }

    
    RootedFunction ctor(cx, global->createConstructor(cx, Error, name, 1,
                                                      JSFunction::ExtendedFinalizeKind));
    if (!ctor)
        return nullptr;
    ctor->setExtendedSlot(0, Int32Value(int32_t(type)));

    if (!LinkConstructorAndPrototype(cx, ctor, err))
        return nullptr;

    if (!GlobalObject::initBuiltinConstructor(cx, global, key, ctor, err))
        return nullptr;

    return err;
}

JSObject *
js_InitExceptionClasses(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;

    
    RootedObject errorProto(cx, ErrorObject::createProto(cx, global, JSEXN_ERR, objProto));
    if (!errorProto)
        return nullptr;

    
    if (!DefinePropertiesAndBrand(cx, errorProto, nullptr, exception_methods))
        return nullptr;

    
    for (int i = JSEXN_ERR + 1; i < JSEXN_LIMIT; i++) {
        if (!ErrorObject::createProto(cx, global, JSExnType(i), errorProto))
            return nullptr;
    }

    return errorProto;
}

const JSErrorFormatString*
js_GetLocalizedErrorMessage(ExclusiveContext *cx, void *userRef, const char *locale,
                            const unsigned errorNumber)
{
    const JSErrorFormatString *errorString = nullptr;

    
    
    
    if (cx->isJSContext() &&
        cx->asJSContext()->runtime()->localeCallbacks &&
        cx->asJSContext()->runtime()->localeCallbacks->localeGetErrorMessage)
    {
        JSLocaleCallbacks *callbacks = cx->asJSContext()->runtime()->localeCallbacks;
        errorString = callbacks->localeGetErrorMessage(userRef, locale, errorNumber);
    }

    if (!errorString)
        errorString = js_GetErrorMessage(userRef, locale, errorNumber);
    return errorString;
}

JS_FRIEND_API(const jschar*)
js::GetErrorTypeName(JSRuntime* rt, int16_t exnType)
{
    



    if (exnType <= JSEXN_NONE || exnType >= JSEXN_LIMIT ||
        exnType == JSEXN_INTERNALERR)
    {
        return nullptr;
    }
    JSProtoKey key = GetExceptionProtoKey(JSExnType(exnType));
    return ClassName(key, rt)->chars();
}

bool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef)
{
    
    JS_ASSERT(reportp);
    if (JSREPORT_IS_WARNING(reportp->flags))
        return false;

    
    JSErrNum errorNumber = static_cast<JSErrNum>(reportp->errorNumber);
    const JSErrorFormatString *errorString;
    if (!callback || callback == js_GetErrorMessage)
        errorString = js_GetLocalizedErrorMessage(cx, nullptr, nullptr, errorNumber);
    else
        errorString = callback(userRef, nullptr, errorNumber);
    JSExnType exnType = errorString ? static_cast<JSExnType>(errorString->exnType) : JSEXN_NONE;
    MOZ_ASSERT(exnType < JSEXN_LIMIT);

    
    
    if (exnType == JSEXN_NONE)
        return false;

    
    if (cx->generatingError)
        return false;
    AutoScopedAssign<bool> asa(&cx->generatingError, true);

    
    RootedString messageStr(cx, reportp->ucmessage ? JS_NewUCStringCopyZ(cx, reportp->ucmessage)
                                                   : JS_NewStringCopyZ(cx, message));
    if (!messageStr)
        return cx->isExceptionPending();

    RootedString fileName(cx, JS_NewStringCopyZ(cx, reportp->filename));
    if (!fileName)
        return cx->isExceptionPending();

    uint32_t lineNumber = reportp->lineno;
    uint32_t columnNumber = reportp->column;

    RootedString stack(cx, ComputeStackString(cx));
    if (!stack)
        return cx->isExceptionPending();

    js::ScopedJSFreePtr<JSErrorReport> report(CopyErrorReport(cx, reportp));
    if (!report)
        return cx->isExceptionPending();

    RootedObject errObject(cx, ErrorObject::create(cx, exnType, stack, fileName,
                                                   lineNumber, columnNumber, &report, messageStr));
    if (!errObject)
        return cx->isExceptionPending();

    
    RootedValue errValue(cx, ObjectValue(*errObject));
    JS_SetPendingException(cx, errValue);

    
    reportp->flags |= JSREPORT_EXCEPTION;
    return true;
}

static bool
IsDuckTypedErrorObject(JSContext *cx, HandleObject exnObject, const char **filename_strp)
{
    bool found;
    if (!JS_HasProperty(cx, exnObject, js_message_str, &found) || !found)
        return false;

    const char *filename_str = *filename_strp;
    if (!JS_HasProperty(cx, exnObject, filename_str, &found) || !found) {
        
        filename_str = js_fileName_str;
        if (!JS_HasProperty(cx, exnObject, filename_str, &found) || !found)
            return false;
    }

    if (!JS_HasProperty(cx, exnObject, js_lineNumber_str, &found) || !found)
        return false;

    *filename_strp = filename_str;
    return true;
}

JS_FRIEND_API(JSString *)
js::ErrorReportToString(JSContext *cx, JSErrorReport *reportp)
{
    JSExnType type = static_cast<JSExnType>(reportp->exnType);
    RootedString str(cx, cx->runtime()->emptyString);
    if (type != JSEXN_NONE)
        str = ClassName(GetExceptionProtoKey(type), cx);
    RootedString toAppend(cx, JS_NewUCStringCopyN(cx, MOZ_UTF16(": "), 2));
    if (!str || !toAppend)
        return nullptr;
    str = ConcatStrings<CanGC>(cx, str, toAppend);
    if (!str)
        return nullptr;
    toAppend = JS_NewUCStringCopyZ(cx, reportp->ucmessage);
    if (toAppend)
        str = ConcatStrings<CanGC>(cx, str, toAppend);
    return str;
}

bool
js_ReportUncaughtException(JSContext *cx)
{
    if (!cx->isExceptionPending())
        return true;

    RootedValue exn(cx);
    if (!cx->getPendingException(&exn))
        return false;

    





    RootedObject exnObject(cx);
    if (exn.isPrimitive()) {
        exnObject = nullptr;
    } else {
        exnObject = exn.toObjectOrNull();
    }

    JS_ClearPendingException(cx);
    JSErrorReport *reportp = exnObject ? js_ErrorFromException(cx, exnObject)
                                       : nullptr;

    
    
    
    RootedString str(cx);
    if (reportp)
        str = ErrorReportToString(cx, reportp);
    else
        str = ToString<CanGC>(cx, exn);

    JSErrorReport report;

    
    
    
    
    
    
    
    
    const char *filename_str = "filename";
    JSAutoByteString filename;
    if (!reportp && exnObject && IsDuckTypedErrorObject(cx, exnObject, &filename_str))
    {
        
        RootedValue val(cx);

        RootedString name(cx);
        if (JS_GetProperty(cx, exnObject, js_name_str, &val) && val.isString())
            name = val.toString();

        RootedString msg(cx);
        if (JS_GetProperty(cx, exnObject, js_message_str, &val) && val.isString())
            msg = val.toString();

        
        
        
        
        
        
        
        if (name && msg) {
            RootedString colon(cx, JS_NewStringCopyZ(cx, ": "));
            if (!colon)
                return false;
            RootedString nameColon(cx, ConcatStrings<CanGC>(cx, name, colon));
            if (!nameColon)
                return false;
            str = ConcatStrings<CanGC>(cx, nameColon, msg);
            if (!str)
                return false;
        } else if (name) {
            str = name;
        } else if (msg) {
            str = msg;
        }

        if (JS_GetProperty(cx, exnObject, filename_str, &val)) {
            JSString *tmp = ToString<CanGC>(cx, val);
            if (tmp)
                filename.encodeLatin1(cx, tmp);
        }

        uint32_t lineno;
        if (!JS_GetProperty(cx, exnObject, js_lineNumber_str, &val) ||
            !ToUint32(cx, val, &lineno))
        {
            lineno = 0;
        }

        uint32_t column;
        if (!JS_GetProperty(cx, exnObject, js_columnNumber_str, &val) ||
            !ToUint32(cx, val, &column))
        {
            column = 0;
        }

        reportp = &report;
        PodZero(&report);
        report.filename = filename.ptr();
        report.lineno = (unsigned) lineno;
        report.exnType = int16_t(JSEXN_NONE);
        report.column = (unsigned) column;
        if (str) {
            
            
            
            
            
            
            
            if (JSFlatString *flat = str->ensureFlat(cx))
                report.ucmessage = flat->chars();
        }
    }

    JSAutoByteString bytesStorage;
    const char *bytes = nullptr;
    if (str)
        bytes = bytesStorage.encodeLatin1(cx, str);
    if (!bytes)
        bytes = "unknown (can't convert to string)";

    if (!reportp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_UNCAUGHT_EXCEPTION, bytes);
    } else {
        
        reportp->flags |= JSREPORT_EXCEPTION;

        
        JS_SetPendingException(cx, exn);
        CallErrorReporter(cx, bytes, reportp);
    }

    JS_ClearPendingException(cx);
    return true;
}

JSObject *
js_CopyErrorObject(JSContext *cx, Handle<ErrorObject*> err, HandleObject scope)
{
    assertSameCompartment(cx, scope);

    js::ScopedJSFreePtr<JSErrorReport> copyReport;
    if (JSErrorReport *errorReport = err->getErrorReport()) {
        copyReport = CopyErrorReport(cx, errorReport);
        if (!copyReport)
            return nullptr;
    }

    RootedString message(cx, err->getMessage());
    if (message && !cx->compartment()->wrap(cx, message.address()))
        return nullptr;
    RootedString fileName(cx, err->fileName(cx));
    if (!cx->compartment()->wrap(cx, fileName.address()))
        return nullptr;
    RootedString stack(cx, err->stack(cx));
    if (!cx->compartment()->wrap(cx, stack.address()))
        return nullptr;
    uint32_t lineNumber = err->lineNumber();
    uint32_t columnNumber = err->columnNumber();
    JSExnType errorType = err->type();

    
    return ErrorObject::create(cx, errorType, stack, fileName,
                               lineNumber, columnNumber, &copyReport, message);
}

JS_PUBLIC_API(bool)
JS::CreateTypeError(JSContext *cx, HandleString stack, HandleString fileName,
                    uint32_t lineNumber, uint32_t columnNumber, JSErrorReport *report,
                    HandleString message, MutableHandleValue rval)
{
    assertSameCompartment(cx, stack, fileName, message);
    js::ScopedJSFreePtr<JSErrorReport> rep;
    if (report)
        rep = CopyErrorReport(cx, report);

    RootedObject obj(cx,
        js::ErrorObject::create(cx, JSEXN_TYPEERR, stack, fileName,
                                lineNumber, columnNumber, &rep, message));
    if (!obj)
        return false;

    rval.setObject(*obj);
    return true;
}

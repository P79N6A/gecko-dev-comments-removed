









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

bool
Error(JSContext *cx, unsigned argc, Value *vp);

static bool
exn_toSource(JSContext *cx, unsigned argc, Value *vp);

static const JSFunctionSpec exception_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, exn_toSource, 0, 0),
#endif
    JS_SELF_HOSTED_FN(js_toString_str, "ErrorToString", 0,0),
    JS_FS_END
};

#define IMPLEMENT_ERROR_SUBCLASS(name) \
    { \
        js_Error_str, /* yes, really */ \
        JSCLASS_IMPLEMENTS_BARRIERS | \
        JSCLASS_HAS_CACHED_PROTO(JSProto_##name) | \
        JSCLASS_HAS_RESERVED_SLOTS(ErrorObject::RESERVED_SLOTS), \
        JS_PropertyStub,         /* addProperty */ \
        JS_DeletePropertyStub,   /* delProperty */ \
        JS_PropertyStub,         /* getProperty */ \
        JS_StrictPropertyStub,   /* setProperty */ \
        JS_EnumerateStub, \
        JS_ResolveStub, \
        JS_ConvertStub, \
        exn_finalize, \
        nullptr,                 /* call        */ \
        nullptr,                 /* hasInstance */ \
        nullptr,                 /* construct   */ \
        nullptr,                 /* trace       */ \
        { \
            ErrorObject::createConstructor, \
            ErrorObject::createProto, \
            nullptr, \
            exception_methods, \
            nullptr, \
            nullptr, \
            JSProto_Error \
        } \
    }

const Class
ErrorObject::classes[JSEXN_LIMIT] = {
    {
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
        nullptr,                 
        nullptr,                 
        {
            ErrorObject::createConstructor,
            ErrorObject::createProto,
            nullptr,
            exception_methods,
            0
        }
    },
    IMPLEMENT_ERROR_SUBCLASS(InternalError),
    IMPLEMENT_ERROR_SUBCLASS(EvalError),
    IMPLEMENT_ERROR_SUBCLASS(RangeError),
    IMPLEMENT_ERROR_SUBCLASS(ReferenceError),
    IMPLEMENT_ERROR_SUBCLASS(SyntaxError),
    IMPLEMENT_ERROR_SUBCLASS(TypeError),
    IMPLEMENT_ERROR_SUBCLASS(URIError)
};

JSErrorReport *
js::CopyErrorReport(JSContext *cx, JSErrorReport *report)
{
    












    JS_STATIC_ASSERT(sizeof(JSErrorReport) % sizeof(const char *) == 0);
    JS_STATIC_ASSERT(sizeof(const char *) % sizeof(char16_t) == 0);

    size_t filenameSize;
    size_t linebufSize;
    size_t uclinebufSize;
    size_t ucmessageSize;
    size_t i, argsArraySize, argsCopySize, argSize;
    size_t mallocSize;
    JSErrorReport *copy;
    uint8_t *cursor;

#define JS_CHARS_SIZE(chars) ((js_strlen(chars) + 1) * sizeof(char16_t))

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
            argsArraySize = (i + 1) * sizeof(const char16_t *);
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
        copy->messageArgs = (const char16_t **)cursor;
        cursor += argsArraySize;
        for (i = 0; report->messageArgs[i]; ++i) {
            copy->messageArgs[i] = (const char16_t *)cursor;
            argSize = JS_CHARS_SIZE(report->messageArgs[i]);
            js_memcpy(cursor, report->messageArgs[i], argSize);
            cursor += argSize;
        }
        copy->messageArgs[i] = nullptr;
        JS_ASSERT(cursor == (uint8_t *)copy->messageArgs[0] + argsCopySize);
    }

    if (report->ucmessage) {
        copy->ucmessage = (const char16_t *)cursor;
        js_memcpy(cursor, report->ucmessage, ucmessageSize);
        cursor += ucmessageSize;
    }

    if (report->uclinebuf) {
        copy->uclinebuf = (const char16_t *)cursor;
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
        prevReporter(JS_SetErrorReporter(cx->runtime(), nullptr)),
        prevState(cx)
    {}

    ~SuppressErrorsGuard()
    {
        JS_SetErrorReporter(cx->runtime(), prevReporter);
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
            if (!sb.append(cfilename, strlen(cfilename)))
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

bool
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


JS_STATIC_ASSERT(JSEXN_ERR == 0);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_INTERNALERR  == JSProto_InternalError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_EVALERR      == JSProto_EvalError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_RANGEERR     == JSProto_RangeError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_REFERENCEERR == JSProto_ReferenceError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_SYNTAXERR    == JSProto_SyntaxError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_TYPEERR      == JSProto_TypeError);
JS_STATIC_ASSERT(JSProto_Error + JSEXN_URIERR       == JSProto_URIError);

 JSObject *
ErrorObject::createProto(JSContext *cx, JSProtoKey key)
{
    RootedObject errorProto(cx, GenericCreatePrototype(cx, key));
    if (!errorProto)
        return nullptr;

    Rooted<ErrorObject*> err(cx, &errorProto->as<ErrorObject>());
    RootedString emptyStr(cx, cx->names().empty);
    JSExnType type = ExnTypeFromProtoKey(key);
    if (!ErrorObject::init(cx, err, type, nullptr, emptyStr, emptyStr, 0, 0, emptyStr))
        return nullptr;

    
    
    RootedPropertyName name(cx, ClassName(key, cx));
    RootedValue nameValue(cx, StringValue(name));
    if (!JSObject::defineProperty(cx, err, cx->names().name, nameValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
        return nullptr;
    }

    return errorProto;
}

 JSObject *
ErrorObject::createConstructor(JSContext *cx, JSProtoKey key)
{
    RootedObject ctor(cx);
    ctor = GenericCreateConstructor<Error, 1, JSFunction::ExtendedFinalizeKind>(cx, key);
    if (!ctor)
        return nullptr;

    ctor->as<JSFunction>().setExtendedSlot(0, Int32Value(ExnTypeFromProtoKey(key)));
    return ctor;
}

JS_FRIEND_API(JSFlatString *)
js::GetErrorTypeName(JSRuntime *rt, int16_t exnType)
{
    



    if (exnType <= JSEXN_NONE || exnType >= JSEXN_LIMIT ||
        exnType == JSEXN_INTERNALERR)
    {
        return nullptr;
    }
    JSProtoKey key = GetExceptionProtoKey(JSExnType(exnType));
    return ClassName(key, rt);
}

bool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef)
{
    
    JS_ASSERT(reportp);
    if (JSREPORT_IS_WARNING(reportp->flags))
        return false;

    
    JSErrNum errorNumber = static_cast<JSErrNum>(reportp->errorNumber);
    if (!callback)
        callback = js_GetErrorMessage;
    const JSErrorFormatString *errorString = callback(userRef, errorNumber);
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

    cx->clearPendingException();

    ErrorReport err(cx);
    if (!err.init(cx, exn)) {
        cx->clearPendingException();
        return false;
    }

    cx->setPendingException(exn);
    CallErrorReporter(cx, err.message(), err.report());
    cx->clearPendingException();
    return true;
}

ErrorReport::ErrorReport(JSContext *cx)
  : reportp(nullptr),
    message_(nullptr),
    ownedMessage(nullptr),
    str(cx),
    strChars(cx),
    exnObject(cx)
{
}

ErrorReport::~ErrorReport()
{
    if (!ownedMessage)
        return;

    js_free(ownedMessage);
    if (ownedReport.messageArgs) {
        




        size_t i = 0;
        while (ownedReport.messageArgs[i])
            js_free(const_cast<char16_t*>(ownedReport.messageArgs[i++]));
        js_free(ownedReport.messageArgs);
    }
    js_free(const_cast<char16_t*>(ownedReport.ucmessage));
}

bool
ErrorReport::init(JSContext *cx, HandleValue exn)
{
    MOZ_ASSERT(!cx->isExceptionPending());

    



    if (exn.isObject()) {
        exnObject = &exn.toObject();
        reportp = js_ErrorFromException(cx, exnObject);
    }

    
    
    
    if (reportp)
        str = ErrorReportToString(cx, reportp);
    else
        str = ToString<CanGC>(cx, exn);

    if (!str)
        cx->clearPendingException();

    
    
    
    
    
    
    
    
    const char *filename_str = "filename";
    if (!reportp && exnObject && IsDuckTypedErrorObject(cx, exnObject, &filename_str))
    {
        
        RootedValue val(cx);

        RootedString name(cx);
        if (JS_GetProperty(cx, exnObject, js_name_str, &val) && val.isString())
            name = val.toString();
        else
            cx->clearPendingException();

        RootedString msg(cx);
        if (JS_GetProperty(cx, exnObject, js_message_str, &val) && val.isString())
            msg = val.toString();
        else
            cx->clearPendingException();

        
        
        
        
        
        
        
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
            else
                cx->clearPendingException();
        } else {
            cx->clearPendingException();
        }

        uint32_t lineno;
        if (!JS_GetProperty(cx, exnObject, js_lineNumber_str, &val) ||
            !ToUint32(cx, val, &lineno))
        {
            cx->clearPendingException();
            lineno = 0;
        }

        uint32_t column;
        if (!JS_GetProperty(cx, exnObject, js_columnNumber_str, &val) ||
            !ToUint32(cx, val, &column))
        {
            cx->clearPendingException();
            column = 0;
        }

        reportp = &ownedReport;
        PodZero(&ownedReport);
        ownedReport.filename = filename.ptr();
        ownedReport.lineno = lineno;
        ownedReport.exnType = int16_t(JSEXN_NONE);
        ownedReport.column = column;
        if (str) {
            
            
            
            
            
            
            
            if (str->ensureFlat(cx) && strChars.initTwoByte(cx, str))
                ownedReport.ucmessage = strChars.twoByteChars();
        }
    }

    if (str)
        message_ = bytesStorage.encodeLatin1(cx, str);
    if (!message_)
        message_ = "unknown (can't convert to string)";

    if (!reportp) {
        
        
        
        
        
        
        
        populateUncaughtExceptionReport(cx, message_);
    } else {
        
        reportp->flags |= JSREPORT_EXCEPTION;
    }

    return true;
}

void
ErrorReport::populateUncaughtExceptionReport(JSContext *cx, ...)
{
    va_list ap;
    va_start(ap, cx);
    populateUncaughtExceptionReportVA(cx, ap);
    va_end(ap);
}

void
ErrorReport::populateUncaughtExceptionReportVA(JSContext *cx, va_list ap)
{
    PodZero(&ownedReport);
    ownedReport.flags = JSREPORT_ERROR;
    ownedReport.errorNumber = JSMSG_UNCAUGHT_EXCEPTION;
    
    
    
    NonBuiltinFrameIter iter(cx);
    if (!iter.done()) {
        ownedReport.filename = iter.scriptFilename();
        ownedReport.lineno = iter.computeLine(&ownedReport.column);
        ownedReport.originPrincipals = iter.originPrincipals();
    }

    if (!js_ExpandErrorArguments(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_UNCAUGHT_EXCEPTION, &ownedMessage,
                                 &ownedReport, ArgumentsAreASCII, ap)) {
        return;
    }

    reportp = &ownedReport;
    message_ = ownedMessage;
    ownsMessageAndReport = true;
}

JSObject *
js_CopyErrorObject(JSContext *cx, Handle<ErrorObject*> err)
{
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
JS::CreateError(JSContext *cx, JSExnType type, HandleString stack, HandleString fileName,
                    uint32_t lineNumber, uint32_t columnNumber, JSErrorReport *report,
                    HandleString message, MutableHandleValue rval)
{
    assertSameCompartment(cx, stack, fileName, message);
    js::ScopedJSFreePtr<JSErrorReport> rep;
    if (report)
        rep = CopyErrorReport(cx, report);

    RootedObject obj(cx,
        js::ErrorObject::create(cx, type, stack, fileName,
                                lineNumber, columnNumber, &rep, message));
    if (!obj)
        return false;

    rval.setObject(*obj);
    return true;
}

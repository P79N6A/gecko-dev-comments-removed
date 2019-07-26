









#include "jsexn.h"

#include "mozilla/PodOperations.h"
#include "mozilla/Util.h"

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

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::ArrayLength;
using mozilla::PodArrayZero;
using mozilla::PodZero;


static bool
Exception(JSContext *cx, unsigned argc, Value *vp);

static void
exn_trace(JSTracer *trc, JSObject *obj);

static void
exn_finalize(FreeOp *fop, JSObject *obj);

static bool
exn_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
            MutableHandleObject objp);

const Class ErrorObject::class_ = {
    js_Error_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Error),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    (JSResolveOp)exn_resolve,
    JS_ConvertStub,
    exn_finalize,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    exn_trace
};

template <typename T>
struct JSStackTraceElemImpl
{
    T                   funName;
    const char          *filename;
    unsigned            ulineno;
};

typedef JSStackTraceElemImpl<HeapPtrString> JSStackTraceElem;
typedef JSStackTraceElemImpl<JSString *>    JSStackTraceStackElem;

struct JSExnPrivate
{
    
    JSErrorReport       *errorReport;
    js::HeapPtrString   message;
    js::HeapPtrString   filename;
    unsigned            lineno;
    unsigned            column;
    size_t              stackDepth;
    int                 exnType;
    JSStackTraceElem    stackElems[1];
};

static JSString *
StackTraceToString(JSContext *cx, JSExnPrivate *priv);

static JSErrorReport *
CopyErrorReport(JSContext *cx, JSErrorReport *report)
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
    JSExceptionState *prevState;

    SuppressErrorsGuard(JSContext *cx)
      : cx(cx),
        prevReporter(JS_SetErrorReporter(cx, nullptr)),
        prevState(JS_SaveExceptionState(cx))
    {}

    ~SuppressErrorsGuard()
    {
        JS_RestoreExceptionState(cx, prevState);
        JS_SetErrorReporter(cx, prevReporter);
    }
};

static void
SetExnPrivate(ErrorObject &exnObject, JSExnPrivate *priv);

static bool
InitExnPrivate(JSContext *cx, HandleObject exnObject, HandleString message,
               HandleString filename, unsigned lineno, unsigned column,
               JSErrorReport *report, int exnType)
{
    JS_ASSERT(exnObject->is<ErrorObject>());
    JS_ASSERT(!exnObject->getPrivate());

    JSCheckAccessOp checkAccess = cx->runtime()->securityCallbacks->checkObjectAccess;

    Vector<JSStackTraceStackElem> frames(cx);
    {
        SuppressErrorsGuard seg(cx);
        for (NonBuiltinScriptFrameIter i(cx); !i.done(); ++i) {

            
            if (checkAccess && i.isNonEvalFunctionFrame()) {
                RootedValue v(cx);
                RootedId callerid(cx, NameToId(cx->names().caller));
                RootedObject obj(cx, i.callee());
                if (!checkAccess(cx, obj, callerid, JSACC_READ, &v))
                    break;
            }

            if (!frames.growBy(1))
                return false;
            JSStackTraceStackElem &frame = frames.back();
            if (i.isNonEvalFunctionFrame()) {
                JSAtom *atom = i.callee()->displayAtom();
                if (atom == nullptr)
                    atom = cx->runtime()->emptyString;
                frame.funName = atom;
            } else {
                frame.funName = nullptr;
            }
            RootedScript script(cx, i.script());
            const char *cfilename = script->filename();
            if (!cfilename)
                cfilename = "";
            frame.filename = cfilename;
            frame.ulineno = PCToLineNumber(script, i.pc());
        }
    }

    
    JS_STATIC_ASSERT(sizeof(JSStackTraceElem) <= sizeof(StackFrame));

    size_t nbytes = offsetof(JSExnPrivate, stackElems) +
                    frames.length() * sizeof(JSStackTraceElem);

    JSExnPrivate *priv = (JSExnPrivate *)cx->malloc_(nbytes);
    if (!priv)
        return false;

    
    memset(priv, 0, nbytes);

    if (report) {
        





        priv->errorReport = CopyErrorReport(cx, report);
        if (!priv->errorReport) {
            js_free(priv);
            return false;
        }
    } else {
        priv->errorReport = nullptr;
    }

    priv->message.init(message);
    priv->filename.init(filename);
    priv->lineno = lineno;
    priv->column = column;
    priv->stackDepth = frames.length();
    priv->exnType = exnType;
    for (size_t i = 0; i < frames.length(); ++i) {
        priv->stackElems[i].funName.init(frames[i].funName);
        priv->stackElems[i].filename = JS_strdup(cx, frames[i].filename);
        if (!priv->stackElems[i].filename)
            return false;
        priv->stackElems[i].ulineno = frames[i].ulineno;
    }

    SetExnPrivate(exnObject->as<ErrorObject>(), priv);
    return true;
}

static void
exn_trace(JSTracer *trc, JSObject *obj)
{
    if (JSExnPrivate *priv = obj->as<ErrorObject>().getExnPrivate()) {
        if (priv->message)
            MarkString(trc, &priv->message, "exception message");
        if (priv->filename)
            MarkString(trc, &priv->filename, "exception filename");

        for (size_t i = 0; i != priv->stackDepth; ++i) {
            JSStackTraceElem &elem = priv->stackElems[i];
            if (elem.funName)
                MarkString(trc, &elem.funName, "stack trace function name");
        }
    }
}


static void
SetExnPrivate(ErrorObject &exnObject, JSExnPrivate *priv)
{
    JS_ASSERT(!exnObject.getExnPrivate());
    if (JSErrorReport *report = priv->errorReport) {
        if (JSPrincipals *prin = report->originPrincipals)
            JS_HoldPrincipals(prin);
    }
    exnObject.setPrivate(priv);
}

static void
exn_finalize(FreeOp *fop, JSObject *obj)
{
    if (JSExnPrivate *priv = obj->as<ErrorObject>().getExnPrivate()) {
        if (JSErrorReport *report = priv->errorReport) {
            
            if (JSPrincipals *prin = report->originPrincipals)
                JS_DropPrincipals(fop->runtime(), prin);
            fop->free_(report);
        }
        for (size_t i = 0; i < priv->stackDepth; i++)
            js_free(const_cast<char *>(priv->stackElems[i].filename));
        fop->free_(priv);
    }
}

static bool
exn_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
            MutableHandleObject objp)
{
    JSExnPrivate *priv;
    const char *prop;
    jsval v;
    unsigned attrs;

    objp.set(nullptr);
    priv = obj->as<ErrorObject>().getExnPrivate();
    if (priv && JSID_IS_ATOM(id)) {
        RootedString str(cx, JSID_TO_STRING(id));

        RootedAtom atom(cx, cx->names().message);
        if (str == atom) {
            prop = js_message_str;

            




            if (!priv->message)
                return true;

            v = STRING_TO_JSVAL(priv->message);
            attrs = 0;
            goto define;
        }

        atom = cx->names().fileName;
        if (str == atom) {
            prop = js_fileName_str;
            v = STRING_TO_JSVAL(priv->filename);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }

        atom = cx->names().lineNumber;
        if (str == atom) {
            prop = js_lineNumber_str;
            v = UINT_TO_JSVAL(priv->lineno);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }

        atom = cx->names().columnNumber;
        if (str == atom) {
            prop = js_columnNumber_str;
            v = UINT_TO_JSVAL(priv->column);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }

        atom = cx->names().stack;
        if (str == atom) {
            JSString *stack = StackTraceToString(cx, priv);
            if (!stack)
                return false;

            prop = js_stack_str;
            v = STRING_TO_JSVAL(stack);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }
    }
    return true;

  define:
    if (!JS_DefineProperty(cx, obj, prop, v, nullptr, nullptr, attrs))
        return false;
    objp.set(obj);
    return true;
}

JSErrorReport *
js_ErrorFromException(jsval exn)
{
    if (JSVAL_IS_PRIMITIVE(exn))
        return nullptr;

    
    
    
    
    
    
    JSObject *obj = UncheckedUnwrap(JSVAL_TO_OBJECT(exn));
    if (!obj->is<ErrorObject>())
        return nullptr;

    JSExnPrivate *priv = obj->as<ErrorObject>().getExnPrivate();
    if (!priv)
        return nullptr;

    return priv->errorReport;
}

static JSString *
StackTraceToString(JSContext *cx, JSExnPrivate *priv)
{
    StringBuffer sb(cx);

    JSStackTraceElem *element = priv->stackElems, *end = element + priv->stackDepth;
    for (; element < end; element++) {
        
        size_t length = ((element->funName ? element->funName->length() : 0) +
                         (element->filename ? strlen(element->filename) * 2 : 0) +
                         13); 

        if (!sb.reserve(length) || sb.length() > JS_BIT(20))
            break; 

        if (element->funName) {
            if (!sb.append(element->funName))
                return nullptr;
        }
        if (!sb.append('@'))
            return nullptr;
        if (element->filename) {
            if (!sb.appendInflated(element->filename, strlen(element->filename)))
                return nullptr;
        }
        if (!sb.append(':') || !NumberValueToStringBuffer(cx, NumberValue(element->ulineno), sb) || 
            !sb.append('\n'))
        {
            return nullptr;
        }
    }

    return sb.finishString();
}



static JSString *
FilenameToString(JSContext *cx, const char *filename)
{
    return JS_NewStringCopyZ(cx, filename);
}

static bool
Exception(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    






    RootedObject callee(cx, &args.callee());
    RootedValue protov(cx);
    if (!JSObject::getProperty(cx, callee, callee, cx->names().classPrototype, &protov))
        return false;

    if (!protov.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_PROTOTYPE, "Error");
        return false;
    }

    RootedObject obj(cx, NewObjectWithGivenProto(cx, &ErrorObject::class_, &protov.toObject(),
                     nullptr));
    if (!obj)
        return false;

    
    RootedString message(cx);
    if (args.hasDefined(0)) {
        message = ToString<CanGC>(cx, args[0]);
        if (!message)
            return false;
        args[0].setString(message);
    } else {
        message = nullptr;
    }

    
    NonBuiltinScriptFrameIter iter(cx);

    
    RootedScript script(cx, iter.done() ? nullptr : iter.script());
    RootedString filename(cx);
    if (args.length() > 1) {
        filename = ToString<CanGC>(cx, args[1]);
        if (!filename)
            return false;
        args[1].setString(filename);
    } else {
        filename = cx->runtime()->emptyString;
        if (!iter.done()) {
            if (const char *cfilename = script->filename()) {
                filename = FilenameToString(cx, cfilename);
                if (!filename)
                    return false;
            }
        }
    }

    
    uint32_t lineno, column = 0;
    if (args.length() > 2) {
        if (!ToUint32(cx, args[2], &lineno))
            return false;
    } else {
        lineno = iter.done() ? 0 : PCToLineNumber(script, iter.pc(), &column);
    }

    int exnType = args.callee().as<JSFunction>().getExtendedSlot(0).toInt32();
    if (!InitExnPrivate(cx, obj, message, filename, lineno, column, nullptr, exnType))
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

static JSObject *
InitErrorClass(JSContext *cx, Handle<GlobalObject*> global, int type, HandleObject proto)
{
    JSProtoKey key = GetExceptionProtoKey(type);
    RootedAtom name(cx, ClassName(key, cx));
    RootedObject errorProto(cx, global->createBlankPrototypeInheriting(cx, &ErrorObject::class_,
                            *proto));
    if (!errorProto)
        return nullptr;

    RootedValue nameValue(cx, StringValue(name));
    RootedValue zeroValue(cx, Int32Value(0));
    RootedValue empty(cx, StringValue(cx->runtime()->emptyString));
    RootedId nameId(cx, NameToId(cx->names().name));
    RootedId messageId(cx, NameToId(cx->names().message));
    RootedId fileNameId(cx, NameToId(cx->names().fileName));
    RootedId lineNumberId(cx, NameToId(cx->names().lineNumber));
    RootedId columnNumberId(cx, NameToId(cx->names().columnNumber));
    if (!DefineNativeProperty(cx, errorProto, nameId, nameValue,
                              JS_PropertyStub, JS_StrictPropertyStub, 0, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, messageId, empty,
                              JS_PropertyStub, JS_StrictPropertyStub, 0, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, fileNameId, empty,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, lineNumberId, zeroValue,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, columnNumberId, zeroValue,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0))
    {
        return nullptr;
    }

    
    RootedFunction ctor(cx, global->createConstructor(cx, Exception, name, 1,
                                                      JSFunction::ExtendedFinalizeKind));
    if (!ctor)
        return nullptr;
    ctor->setExtendedSlot(0, Int32Value(int32_t(type)));

    if (!LinkConstructorAndPrototype(cx, ctor, errorProto))
        return nullptr;

    if (!DefineConstructorAndPrototype(cx, global, key, ctor, errorProto))
        return nullptr;

    JS_ASSERT(!errorProto->getPrivate());

    return errorProto;
}

JSObject *
js_InitExceptionClasses(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    RootedObject objectProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objectProto)
        return nullptr;

    
    RootedObject errorProto(cx, InitErrorClass(cx, global, JSEXN_ERR, objectProto));
    if (!errorProto)
        return nullptr;

    
    if (!DefinePropertiesAndBrand(cx, errorProto, nullptr, exception_methods))
        return nullptr;

    
    for (int i = JSEXN_ERR + 1; i < JSEXN_LIMIT; i++) {
        if (!InitErrorClass(cx, global, i, errorProto))
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
    JSProtoKey key = GetExceptionProtoKey(exnType);
    return ClassName(key, rt)->chars();
}

#if defined ( DEBUG_mccabe ) && defined ( PRINTNAMES )

static const struct exnname { char *name; char *exception; } errortoexnname[] = {
#define MSG_DEF(name, number, count, exception, format) \
    {#name, #exception},
#include "js.msg"
#undef MSG_DEF
};
#endif 

bool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef)
{
    JSErrNum errorNumber;
    const JSErrorFormatString *errorString;
    JSExnType exn;
    jsval tv[4];

    


    JS_ASSERT(reportp);
    if (JSREPORT_IS_WARNING(reportp->flags))
        return false;

    
    errorNumber = (JSErrNum) reportp->errorNumber;
    if (!callback || callback == js_GetErrorMessage)
        errorString = js_GetLocalizedErrorMessage(cx, nullptr, nullptr, errorNumber);
    else
        errorString = callback(userRef, nullptr, errorNumber);
    exn = errorString ? (JSExnType) errorString->exnType : JSEXN_NONE;
    JS_ASSERT(exn < JSEXN_LIMIT);

#if defined( DEBUG_mccabe ) && defined ( PRINTNAMES )
    
    fprintf(stderr, "%s\t%s\n",
            errortoexnname[errorNumber].name,
            errortoexnname[errorNumber].exception);
#endif

    



    if (exn == JSEXN_NONE)
        return false;

    
    if (cx->generatingError)
        return false;
    AutoScopedAssign<bool> asa(&cx->generatingError, true);

    
    PodArrayZero(tv);
    AutoArrayRooter tvr(cx, ArrayLength(tv), tv);

    




    RootedObject errProto(cx);
    if (!js_GetClassPrototype(cx, GetExceptionProtoKey(exn), &errProto))
        return false;
    tv[0] = OBJECT_TO_JSVAL(errProto);

    RootedObject errObject(cx, NewObjectWithGivenProto(cx, &ErrorObject::class_,
                                                       errProto, nullptr));
    if (!errObject)
        return false;
    tv[1] = OBJECT_TO_JSVAL(errObject);

    RootedString messageStr(cx, reportp->ucmessage ? JS_NewUCStringCopyZ(cx, reportp->ucmessage)
                                                   : JS_NewStringCopyZ(cx, message));
    if (!messageStr)
        return false;
    tv[2] = STRING_TO_JSVAL(messageStr);

    RootedString filenameStr(cx, JS_NewStringCopyZ(cx, reportp->filename));
    if (!filenameStr)
        return false;
    tv[3] = STRING_TO_JSVAL(filenameStr);

    if (!InitExnPrivate(cx, errObject, messageStr, filenameStr,
                        reportp->lineno, reportp->column, reportp, exn)) {
        return false;
    }

    RootedValue errValue(cx, OBJECT_TO_JSVAL(errObject));
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
        
        filename_str = "filename";
        if (!JS_HasProperty(cx, exnObject, filename_str, &found) || !found)
            return false;
    }

    if (!JS_HasProperty(cx, exnObject, js_lineNumber_str, &found) || !found)
        return false;

    *filename_strp = filename_str;
    return true;
}

bool
js_ReportUncaughtException(JSContext *cx)
{
    JSErrorReport *reportp, report;

    if (!cx->isExceptionPending())
        return true;

    RootedValue exn(cx, cx->getPendingException());
    AutoValueVector roots(cx);
    roots.resize(6);

    





    RootedObject exnObject(cx);
    if (JSVAL_IS_PRIMITIVE(exn)) {
        exnObject = nullptr;
    } else {
        exnObject = JSVAL_TO_OBJECT(exn);
        roots[0] = exn;
    }

    JS_ClearPendingException(cx);
    reportp = js_ErrorFromException(exn);

    
    RootedString str(cx, ToString<CanGC>(cx, exn));
    if (str)
        roots[1] = StringValue(str);

    const char *filename_str = js_fileName_str;
    JSAutoByteString filename;
    if (!reportp && exnObject &&
        (exnObject->is<ErrorObject>() || IsDuckTypedErrorObject(cx, exnObject, &filename_str)))
    {
        RootedString name(cx);
        if (JS_GetProperty(cx, exnObject, js_name_str, roots.handleAt(2)) && roots[2].isString())
            name = roots[2].toString();

        RootedString msg(cx);
        if (JS_GetProperty(cx, exnObject, js_message_str, roots.handleAt(3)) && roots[3].isString())
            msg = roots[3].toString();

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

        if (JS_GetProperty(cx, exnObject, filename_str, roots.handleAt(4))) {
            JSString *tmp = ToString<CanGC>(cx, roots.handleAt(4));
            if (tmp)
                filename.encodeLatin1(cx, tmp);
        }

        uint32_t lineno;
        if (!JS_GetProperty(cx, exnObject, js_lineNumber_str, roots.handleAt(5)) ||
            !ToUint32(cx, roots.handleAt(5), &lineno))
        {
            lineno = 0;
        }

        uint32_t column;
        if (!JS_GetProperty(cx, exnObject, js_columnNumber_str, roots.handleAt(5)) ||
            !ToUint32(cx, roots.handleAt(5), &column))
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
            if (JSStableString *stable = str->ensureStable(cx))
                report.ucmessage = stable->chars().get();
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
        js_ReportErrorAgain(cx, bytes, reportp);
    }

    JS_ClearPendingException(cx);
    return true;
}

extern JSObject *
js_CopyErrorObject(JSContext *cx, HandleObject errobj, HandleObject scope)
{
    assertSameCompartment(cx, scope);
    JSExnPrivate *priv = errobj->as<ErrorObject>().getExnPrivate();

    size_t size = offsetof(JSExnPrivate, stackElems) +
                  priv->stackDepth * sizeof(JSStackTraceElem);

    ScopedJSFreePtr<JSExnPrivate> copy(static_cast<JSExnPrivate *>(cx->malloc_(size)));
    if (!copy)
        return nullptr;

    if (priv->errorReport) {
        copy->errorReport = CopyErrorReport(cx, priv->errorReport);
        if (!copy->errorReport)
            return nullptr;
    } else {
        copy->errorReport = nullptr;
    }
    ScopedJSFreePtr<JSErrorReport> autoFreeErrorReport(copy->errorReport);

    copy->message.init(priv->message);
    if (!cx->compartment()->wrap(cx, &copy->message))
        return nullptr;
    JS::Anchor<JSString *> messageAnchor(copy->message);
    copy->filename.init(priv->filename);
    if (!cx->compartment()->wrap(cx, &copy->filename))
        return nullptr;
    JS::Anchor<JSString *> filenameAnchor(copy->filename);
    copy->lineno = priv->lineno;
    copy->column = priv->column;
    copy->stackDepth = 0;
    copy->exnType = priv->exnType;

    
    RootedObject proto(cx, scope->global().getOrCreateCustomErrorPrototype(cx, copy->exnType));
    if (!proto)
        return nullptr;
    RootedObject copyobj(cx, NewObjectWithGivenProto(cx, &ErrorObject::class_, proto, nullptr));
    if (!copyobj)
        return nullptr;
    SetExnPrivate(copyobj->as<ErrorObject>(), copy);
    copy.forget();
    autoFreeErrorReport.forget();
    return copyobj;
}

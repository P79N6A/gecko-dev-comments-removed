










































#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jswrapper.h"

#include "vm/GlobalObject.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;
using namespace js::types;


static JSBool
Exception(JSContext *cx, uintN argc, Value *vp);

static void
exn_trace(JSTracer *trc, JSObject *obj);

static void
exn_finalize(JSContext *cx, JSObject *obj);

static JSBool
exn_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
            JSObject **objp);

Class js::ErrorClass = {
    js_Error_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Error),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    (JSResolveOp)exn_resolve,
    JS_ConvertStub,
    exn_finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    exn_trace
};

typedef struct JSStackTraceElem {
    js::HeapPtrString   funName;
    size_t              argc;
    const char          *filename;
    uintN               ulineno;
} JSStackTraceElem;

typedef struct JSExnPrivate {
    
    JSErrorReport       *errorReport;
    js::HeapPtrString   message;
    js::HeapPtrString   filename;
    uintN               lineno;
    size_t              stackDepth;
    intN                exnType;
    JSStackTraceElem    stackElems[1];
} JSExnPrivate;

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
    cursor = (uint8_t *)cx->malloc_(mallocSize);
    if (!cursor)
        return NULL;

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
        copy->messageArgs[i] = NULL;
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
    copy->errorNumber = report->errorNumber;

    
    copy->flags = report->flags;

#undef JS_CHARS_SIZE
    return copy;
}

static HeapValue *
GetStackTraceValueBuffer(JSExnPrivate *priv)
{
    





    JS_STATIC_ASSERT(sizeof(JSStackTraceElem) % sizeof(jsval) == 0);

    return reinterpret_cast<HeapValue *>(priv->stackElems + priv->stackDepth);
}

struct SuppressErrorsGuard
{
    JSContext *cx;
    JSErrorReporter prevReporter;
    JSExceptionState *prevState;

    SuppressErrorsGuard(JSContext *cx)
      : cx(cx),
        prevReporter(JS_SetErrorReporter(cx, NULL)),
        prevState(JS_SaveExceptionState(cx))
    {}

    ~SuppressErrorsGuard()
    {
        JS_RestoreExceptionState(cx, prevState);
        JS_SetErrorReporter(cx, prevReporter);
    }
};

struct AppendArg {
    Vector<Value> &values;
    AppendArg(Vector<Value> &values) : values(values) {}
    bool operator()(uintN, Value *vp) {
        return values.append(*vp);
    }
};

static void
SetExnPrivate(JSContext *cx, JSObject *exnObject, JSExnPrivate *priv);

static bool
InitExnPrivate(JSContext *cx, JSObject *exnObject, JSString *message,
               JSString *filename, uintN lineno, JSErrorReport *report, intN exnType)
{
    JS_ASSERT(exnObject->isError());
    JS_ASSERT(!exnObject->getPrivate());

    JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);
    JSCheckAccessOp checkAccess = callbacks ? callbacks->checkObjectAccess : NULL;

    Vector<JSStackTraceElem> frames(cx);
    Vector<Value> values(cx);
    {
        SuppressErrorsGuard seg(cx);
        for (FrameRegsIter i(cx); !i.done(); ++i) {
            






            StackFrame *fp = i.fp();
            if (fp->compartment() != cx->compartment)
                break;
            if (checkAccess && fp->isNonEvalFunctionFrame()) {
                Value v = NullValue();
                jsid callerid = ATOM_TO_JSID(cx->runtime->atomState.callerAtom);
                if (!checkAccess(cx, &fp->callee(), callerid, JSACC_READ, &v))
                    break;
            }

            if (!frames.growBy(1))
                return false;
            JSStackTraceElem &frame = frames.back();
            if (fp->isNonEvalFunctionFrame()) {
                frame.funName.init(fp->fun()->atom ? fp->fun()->atom : cx->runtime->emptyString);
                frame.argc = fp->numActualArgs();
                if (!fp->forEachCanonicalActualArg(AppendArg(values)))
                    return false;
            } else {
                frame.funName.init(NULL);
                frame.argc = 0;
            }
            if (fp->isScriptFrame()) {
                frame.filename = fp->script()->filename;
                frame.ulineno = js_PCToLineNumber(cx, fp->script(), i.pc());
            } else {
                frame.ulineno = 0;
                frame.filename = NULL;
            }
        }
    }

    
    JS_STATIC_ASSERT(sizeof(JSStackTraceElem) <= sizeof(StackFrame));

    size_t nbytes = offsetof(JSExnPrivate, stackElems) +
                    frames.length() * sizeof(JSStackTraceElem) +
                    values.length() * sizeof(HeapValue);

    JSExnPrivate *priv = (JSExnPrivate *)cx->malloc_(nbytes);
    if (!priv)
        return false;

    
    memset(priv, 0, nbytes);

    if (report) {
        





        priv->errorReport = CopyErrorReport(cx, report);
        if (!priv->errorReport) {
            cx->free_(priv);
            return false;
        }
    } else {
        priv->errorReport = NULL;
    }

    priv->message.init(message);
    priv->filename.init(filename);
    priv->lineno = lineno;
    priv->stackDepth = frames.length();
    priv->exnType = exnType;

    JSStackTraceElem *framesDest = priv->stackElems;
    HeapValue *valuesDest = reinterpret_cast<HeapValue *>(framesDest + frames.length());
    JS_ASSERT(valuesDest == GetStackTraceValueBuffer(priv));

    PodCopy(framesDest, frames.begin(), frames.length());
    for (size_t i = 0; i < values.length(); ++i)
        valuesDest[i].init(cx->compartment, values[i]);

    SetExnPrivate(cx, exnObject, priv);
    return true;
}

static inline JSExnPrivate *
GetExnPrivate(JSObject *obj)
{
    JS_ASSERT(obj->isError());
    return (JSExnPrivate *) obj->getPrivate();
}

static void
exn_trace(JSTracer *trc, JSObject *obj)
{
    JSExnPrivate *priv;
    JSStackTraceElem *elem;
    size_t vcount, i;
    HeapValue *vp;

    priv = GetExnPrivate(obj);
    if (priv) {
        if (priv->message)
            MarkString(trc, priv->message, "exception message");
        if (priv->filename)
            MarkString(trc, priv->filename, "exception filename");

        elem = priv->stackElems;
        for (vcount = i = 0; i != priv->stackDepth; ++i, ++elem) {
            if (elem->funName)
                MarkString(trc, elem->funName, "stack trace function name");
            if (IS_GC_MARKING_TRACER(trc) && elem->filename)
                js_MarkScriptFilename(elem->filename);
            vcount += elem->argc;
        }
        vp = GetStackTraceValueBuffer(priv);
        for (i = 0; i != vcount; ++i, ++vp) {
            MarkValue(trc, *vp, "stack trace argument");
        }
    }
}


static void
SetExnPrivate(JSContext *cx, JSObject *exnObject, JSExnPrivate *priv)
{
    JS_ASSERT(!exnObject->getPrivate());
    JS_ASSERT(exnObject->isError());
    if (JSErrorReport *report = priv->errorReport) {
        if (JSPrincipals *prin = report->originPrincipals)
            JSPRINCIPALS_HOLD(cx, prin);
    }
    exnObject->setPrivate(priv);
}

static void
exn_finalize(JSContext *cx, JSObject *obj)
{
    if (JSExnPrivate *priv = GetExnPrivate(obj)) {
        if (JSErrorReport *report = priv->errorReport) {
            
            if (JSPrincipals *prin = report->originPrincipals)
                JSPRINCIPALS_DROP(cx, prin);
            cx->free_(report);
        }
        cx->free_(priv);
    }
}

static JSBool
exn_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
            JSObject **objp)
{
    JSExnPrivate *priv;
    JSString *str;
    JSAtom *atom;
    JSString *stack;
    const char *prop;
    jsval v;
    uintN attrs;

    *objp = NULL;
    priv = GetExnPrivate(obj);
    if (priv && JSID_IS_ATOM(id)) {
        str = JSID_TO_STRING(id);

        atom = cx->runtime->atomState.messageAtom;
        if (str == atom) {
            prop = js_message_str;

            




            if (!priv->message)
                return true;

            v = STRING_TO_JSVAL(priv->message);
            attrs = 0;
            goto define;
        }

        atom = cx->runtime->atomState.fileNameAtom;
        if (str == atom) {
            prop = js_fileName_str;
            v = STRING_TO_JSVAL(priv->filename);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }

        atom = cx->runtime->atomState.lineNumberAtom;
        if (str == atom) {
            prop = js_lineNumber_str;
            v = INT_TO_JSVAL(priv->lineno);
            attrs = JSPROP_ENUMERATE;
            goto define;
        }

        atom = cx->runtime->atomState.stackAtom;
        if (str == atom) {
            stack = StackTraceToString(cx, priv);
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
    if (!JS_DefineProperty(cx, obj, prop, v, NULL, NULL, attrs))
        return false;
    *objp = obj;
    return true;
}

JSErrorReport *
js_ErrorFromException(JSContext *cx, jsval exn)
{
    JSObject *obj;
    JSExnPrivate *priv;

    if (JSVAL_IS_PRIMITIVE(exn))
        return NULL;
    obj = JSVAL_TO_OBJECT(exn);
    if (!obj->isError())
        return NULL;
    priv = GetExnPrivate(obj);
    if (!priv)
        return NULL;
    return priv->errorReport;
}

static JSString *
ValueToShortSource(JSContext *cx, const Value &v)
{
    JSString *str;

    
    if (!v.isObject())
        return js_ValueToSource(cx, v);

    JSObject *obj = &v.toObject();
    AutoCompartment ac(cx, obj);
    if (!ac.enter())
        return NULL;

    if (obj->isFunction()) {
        


        str = JS_GetFunctionId(obj->toFunction());
        if (!str && !(str = js_ValueToSource(cx, v))) {
            



            JS_ClearPendingException(cx);
            str = JS_NewStringCopyZ(cx, "[unknown function]");
        }
    } else {
        



        char buf[100];
        JS_snprintf(buf, sizeof buf, "[object %s]", obj->getClass()->name);
        str = JS_NewStringCopyZ(cx, buf);
    }

    ac.leave();

    if (!str || !cx->compartment->wrap(cx, &str))
        return NULL;
    return str;
}

static JSString *
StackTraceToString(JSContext *cx, JSExnPrivate *priv)
{
    jschar *stackbuf;
    size_t stacklen, stackmax;
    JSStackTraceElem *elem, *endElem;
    HeapValue *values;
    size_t i;
    JSString *str;
    const char *cp;
    char ulnbuf[11];

    
    stackbuf = NULL;
    stacklen = stackmax = 0;


#define STACK_LENGTH_LIMIT JS_BIT(20)

#define APPEND_CHAR_TO_STACK(c)                                               \
    JS_BEGIN_MACRO                                                            \
        if (stacklen == stackmax) {                                           \
            void *ptr_;                                                       \
            if (stackmax >= STACK_LENGTH_LIMIT)                               \
                goto done;                                                    \
            stackmax = stackmax ? 2 * stackmax : 64;                          \
            ptr_ = cx->realloc_(stackbuf, (stackmax+1) * sizeof(jschar));      \
            if (!ptr_)                                                        \
                goto bad;                                                     \
            stackbuf = (jschar *) ptr_;                                       \
        }                                                                     \
        stackbuf[stacklen++] = (c);                                           \
    JS_END_MACRO

#define APPEND_STRING_TO_STACK(str)                                           \
    JS_BEGIN_MACRO                                                            \
        JSString *str_ = str;                                                 \
        size_t length_ = str_->length();                                      \
        const jschar *chars_ = str_->getChars(cx);                            \
        if (!chars_)                                                          \
            goto bad;                                                         \
                                                                              \
        if (length_ > stackmax - stacklen) {                                  \
            void *ptr_;                                                       \
            if (stackmax >= STACK_LENGTH_LIMIT ||                             \
                length_ >= STACK_LENGTH_LIMIT - stacklen) {                   \
                goto done;                                                    \
            }                                                                 \
            stackmax = RoundUpPow2(stacklen + length_);                       \
            ptr_ = cx->realloc_(stackbuf, (stackmax+1) * sizeof(jschar));     \
            if (!ptr_)                                                        \
                goto bad;                                                     \
            stackbuf = (jschar *) ptr_;                                       \
        }                                                                     \
        js_strncpy(stackbuf + stacklen, chars_, length_);                     \
        stacklen += length_;                                                  \
    JS_END_MACRO

    values = GetStackTraceValueBuffer(priv);
    elem = priv->stackElems;
    for (endElem = elem + priv->stackDepth; elem != endElem; elem++) {
        if (elem->funName) {
            APPEND_STRING_TO_STACK(elem->funName);
            APPEND_CHAR_TO_STACK('(');
            for (i = 0; i != elem->argc; i++, values++) {
                if (i > 0)
                    APPEND_CHAR_TO_STACK(',');
                str = ValueToShortSource(cx, *values);
                if (!str)
                    goto bad;
                APPEND_STRING_TO_STACK(str);
            }
            APPEND_CHAR_TO_STACK(')');
        }
        APPEND_CHAR_TO_STACK('@');
        if (elem->filename) {
            for (cp = elem->filename; *cp; cp++)
                APPEND_CHAR_TO_STACK(*cp);
        }
        APPEND_CHAR_TO_STACK(':');
        JS_snprintf(ulnbuf, sizeof ulnbuf, "%u", elem->ulineno);
        for (cp = ulnbuf; *cp; cp++)
            APPEND_CHAR_TO_STACK(*cp);
        APPEND_CHAR_TO_STACK('\n');
    }
#undef APPEND_CHAR_TO_STACK
#undef APPEND_STRING_TO_STACK
#undef STACK_LENGTH_LIMIT

  done:
    if (stacklen == 0) {
        JS_ASSERT(!stackbuf);
        return cx->runtime->emptyString;
    }
    if (stacklen < stackmax) {
        




        void *shrunk = cx->realloc_(stackbuf, (stacklen+1) * sizeof(jschar));
        if (shrunk)
            stackbuf = (jschar *) shrunk;
    }

    stackbuf[stacklen] = 0;
    str = js_NewString(cx, stackbuf, stacklen);
    if (str)
        return str;

  bad:
    if (stackbuf)
        cx->free_(stackbuf);
    return NULL;
}



static JSString *
FilenameToString(JSContext *cx, const char *filename)
{
    return JS_NewStringCopyZ(cx, filename);
}

static JSBool
Exception(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    






    Value protov;
    if (!args.callee().getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &protov))
        return false;

    if (!protov.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_PROTOTYPE, "Error");
        return false;
    }

    JSObject *errProto = &protov.toObject();
    JSObject *obj = NewObjectWithGivenProto(cx, &ErrorClass, errProto, NULL);
    if (!obj)
        return false;

    
    JSString *message;
    if (args.length() != 0 && !args[0].isUndefined()) {
        message = ToString(cx, args[0]);
        if (!message)
            return false;
        args[0].setString(message);
    } else {
        message = NULL;
    }

    
    FrameRegsIter iter(cx);
    while (!iter.done() && !iter.fp()->isScriptFrame())
        ++iter;

    
    JSString *filename;
    if (args.length() > 1) {
        filename = ToString(cx, args[1]);
        if (!filename)
            return false;
        args[1].setString(filename);
    } else {
        if (!iter.done()) {
            filename = FilenameToString(cx, iter.fp()->script()->filename);
            if (!filename)
                return false;
        } else {
            filename = cx->runtime->emptyString;
        }
    }

    
    uint32_t lineno;
    if (args.length() > 2) {
        if (!ToUint32(cx, args[2], &lineno))
            return false;
    } else {
        lineno = iter.done() ? 0 : js_PCToLineNumber(cx, iter.fp()->script(), iter.pc());
    }

    intN exnType = args.callee().toFunction()->getExtendedSlot(0).toInt32();
    if (!InitExnPrivate(cx, obj, message, filename, lineno, NULL, exnType))
        return false;

    args.rval().setObject(*obj);
    return true;
}


static JSBool
exn_toString(JSContext *cx, uintN argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!args.thisv().isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_PROTOTYPE, "Error");
        return false;
    }

    
    JSObject &obj = args.thisv().toObject();

    
    Value nameVal;
    if (!obj.getProperty(cx, cx->runtime->atomState.nameAtom, &nameVal))
        return false;

    
    JSString *name;
    if (nameVal.isUndefined()) {
        name = CLASS_ATOM(cx, Error);
    } else {
        name = ToString(cx, nameVal);
        if (!name)
            return false;
    }

    
    Value msgVal;
    if (!obj.getProperty(cx, cx->runtime->atomState.messageAtom, &msgVal))
        return false;

    
    JSString *message;
    if (msgVal.isUndefined()) {
        message = cx->runtime->emptyString;
    } else {
        message = ToString(cx, msgVal);
        if (!message)
            return false;
    }

    
    if (name->empty() && message->empty()) {
        args.rval().setString(CLASS_ATOM(cx, Error));
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



static JSBool
exn_toSource(JSContext *cx, uintN argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);
    CallArgs args = CallArgsFromVp(argc, vp);

    JSObject *obj = ToObject(cx, &args.thisv());
    if (!obj)
        return false;

    Value nameVal;
    JSString *name;
    if (!obj->getProperty(cx, cx->runtime->atomState.nameAtom, &nameVal) ||
        !(name = ToString(cx, nameVal)))
    {
        return false;
    }

    Value messageVal;
    JSString *message;
    if (!obj->getProperty(cx, cx->runtime->atomState.messageAtom, &messageVal) ||
        !(message = js_ValueToSource(cx, messageVal)))
    {
        return false;
    }

    Value filenameVal;
    JSString *filename;
    if (!obj->getProperty(cx, cx->runtime->atomState.fileNameAtom, &filenameVal) ||
        !(filename = js_ValueToSource(cx, filenameVal)))
    {
        return false;
    }

    Value linenoVal;
    uint32_t lineno;
    if (!obj->getProperty(cx, cx->runtime->atomState.lineNumberAtom, &linenoVal) ||
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

        JSString *linenumber = ToString(cx, linenoVal);
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

static JSFunctionSpec exception_methods[] = {
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

static JS_INLINE JSProtoKey
GetExceptionProtoKey(intN exn)
{
    JS_ASSERT(JSEXN_ERR <= exn);
    JS_ASSERT(exn < JSEXN_LIMIT);
    return JSProtoKey(JSProto_Error + exn);
}

static JSObject *
InitErrorClass(JSContext *cx, GlobalObject *global, intN type, JSObject &proto)
{
    JSProtoKey key = GetExceptionProtoKey(type);
    JSAtom *name = cx->runtime->atomState.classAtoms[key];
    JSObject *errorProto = global->createBlankPrototypeInheriting(cx, &ErrorClass, proto);
    if (!errorProto)
        return NULL;

    Value empty = StringValue(cx->runtime->emptyString);
    jsid nameId = ATOM_TO_JSID(cx->runtime->atomState.nameAtom);
    jsid messageId = ATOM_TO_JSID(cx->runtime->atomState.messageAtom);
    jsid fileNameId = ATOM_TO_JSID(cx->runtime->atomState.fileNameAtom);
    jsid lineNumberId = ATOM_TO_JSID(cx->runtime->atomState.lineNumberAtom);
    if (!DefineNativeProperty(cx, errorProto, nameId, StringValue(name),
                              JS_PropertyStub, JS_StrictPropertyStub, 0, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, messageId, empty,
                              JS_PropertyStub, JS_StrictPropertyStub, 0, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, fileNameId, empty,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0) ||
        !DefineNativeProperty(cx, errorProto, lineNumberId, Int32Value(0),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0))
    {
        return NULL;
    }

    
    JSFunction *ctor = global->createConstructor(cx, Exception, &ErrorClass, name, 1,
                                                 JSFunction::ExtendedFinalizeKind);
    if (!ctor)
        return NULL;
    ctor->setExtendedSlot(0, Int32Value(int32_t(type)));

    if (!LinkConstructorAndPrototype(cx, ctor, errorProto))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, key, ctor, errorProto))
        return NULL;

    JS_ASSERT(!errorProto->getPrivate());

    return errorProto;
}

JSObject *
js_InitExceptionClasses(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isGlobal());
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *objectProto;
    if (!js_GetClassPrototype(cx, global, JSProto_Object, &objectProto))
        return NULL;

    
    JSObject *errorProto = InitErrorClass(cx, global, JSEXN_ERR, *objectProto);
    if (!errorProto)
        return NULL;

    
    if (!DefinePropertiesAndBrand(cx, errorProto, NULL, exception_methods))
        return NULL;

    
    for (intN i = JSEXN_ERR + 1; i < JSEXN_LIMIT; i++) {
        if (!InitErrorClass(cx, global, i, *errorProto))
            return NULL;
    }

    return errorProto;
}

const JSErrorFormatString*
js_GetLocalizedErrorMessage(JSContext* cx, void *userRef, const char *locale,
                            const uintN errorNumber)
{
    const JSErrorFormatString *errorString = NULL;

    if (cx->localeCallbacks && cx->localeCallbacks->localeGetErrorMessage) {
        errorString = cx->localeCallbacks
                        ->localeGetErrorMessage(userRef, locale, errorNumber);
    }
    if (!errorString)
        errorString = js_GetErrorMessage(userRef, locale, errorNumber);
    return errorString;
}

#if defined ( DEBUG_mccabe ) && defined ( PRINTNAMES )

static struct exnname { char *name; char *exception; } errortoexnname[] = {
#define MSG_DEF(name, number, count, exception, format) \
    {#name, #exception},
#include "js.msg"
#undef MSG_DEF
};
#endif 

JSBool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef)
{
    JSErrNum errorNumber;
    const JSErrorFormatString *errorString;
    JSExnType exn;
    jsval tv[4];
    JSBool ok;
    JSObject *errProto, *errObject;
    JSString *messageStr, *filenameStr;

    


    JS_ASSERT(reportp);
    if (JSREPORT_IS_WARNING(reportp->flags))
        return JS_FALSE;

    
    errorNumber = (JSErrNum) reportp->errorNumber;
    if (!callback || callback == js_GetErrorMessage)
        errorString = js_GetLocalizedErrorMessage(cx, NULL, NULL, errorNumber);
    else
        errorString = callback(userRef, NULL, errorNumber);
    exn = errorString ? (JSExnType) errorString->exnType : JSEXN_NONE;
    JS_ASSERT(exn < JSEXN_LIMIT);

#if defined( DEBUG_mccabe ) && defined ( PRINTNAMES )
    
    fprintf(stderr, "%s\t%s\n",
            errortoexnname[errorNumber].name,
            errortoexnname[errorNumber].exception);
#endif

    



    if (exn == JSEXN_NONE)
        return JS_FALSE;

    





    if (cx->generatingError)
        return JS_FALSE;

    MUST_FLOW_THROUGH("out");
    cx->generatingError = JS_TRUE;

    
    PodArrayZero(tv);
    AutoArrayRooter tvr(cx, ArrayLength(tv), tv);

    




    ok = js_GetClassPrototype(cx, NULL, GetExceptionProtoKey(exn), &errProto);
    if (!ok)
        goto out;
    tv[0] = OBJECT_TO_JSVAL(errProto);

    errObject = NewObjectWithGivenProto(cx, &ErrorClass, errProto, NULL);
    if (!errObject) {
        ok = JS_FALSE;
        goto out;
    }
    tv[1] = OBJECT_TO_JSVAL(errObject);

    messageStr = JS_NewStringCopyZ(cx, message);
    if (!messageStr) {
        ok = JS_FALSE;
        goto out;
    }
    tv[2] = STRING_TO_JSVAL(messageStr);

    filenameStr = JS_NewStringCopyZ(cx, reportp->filename);
    if (!filenameStr) {
        ok = JS_FALSE;
        goto out;
    }
    tv[3] = STRING_TO_JSVAL(filenameStr);

    ok = InitExnPrivate(cx, errObject, messageStr, filenameStr,
                        reportp->lineno, reportp, exn);
    if (!ok)
        goto out;

    JS_SetPendingException(cx, OBJECT_TO_JSVAL(errObject));

    
    reportp->flags |= JSREPORT_EXCEPTION;

out:
    cx->generatingError = JS_FALSE;
    return ok;
}

JSBool
js_ReportUncaughtException(JSContext *cx)
{
    jsval exn;
    JSObject *exnObject;
    jsval roots[5];
    JSErrorReport *reportp, report;
    JSString *str;
    const char *bytes;

    if (!JS_IsExceptionPending(cx))
        return true;

    if (!JS_GetPendingException(cx, &exn))
        return false;

    PodArrayZero(roots);
    AutoArrayRooter tvr(cx, ArrayLength(roots), roots);

    





    if (JSVAL_IS_PRIMITIVE(exn)) {
        exnObject = NULL;
    } else {
        exnObject = JSVAL_TO_OBJECT(exn);
        roots[0] = exn;
    }

    JS_ClearPendingException(cx);
    reportp = js_ErrorFromException(cx, exn);

    
    str = ToString(cx, exn);
    JSAutoByteString bytesStorage;
    if (!str) {
        bytes = "unknown (can't convert to string)";
    } else {
        roots[1] = StringValue(str);
        if (!bytesStorage.encode(cx, str))
            return false;
        bytes = bytesStorage.ptr();
    }

    JSAutoByteString filename;
    if (!reportp && exnObject && exnObject->isError()) {
        if (!JS_GetProperty(cx, exnObject, js_message_str, &roots[2]))
            return false;
        if (JSVAL_IS_STRING(roots[2])) {
            bytesStorage.clear();
            if (!bytesStorage.encode(cx, str))
                return false;
            bytes = bytesStorage.ptr();
        }

        if (!JS_GetProperty(cx, exnObject, js_fileName_str, &roots[3]))
            return false;
        str = ToString(cx, roots[3]);
        if (!str || !filename.encode(cx, str))
            return false;

        if (!JS_GetProperty(cx, exnObject, js_lineNumber_str, &roots[4]))
            return false;
        uint32_t lineno;
        if (!ToUint32(cx, roots[4], &lineno))
            return false;

        reportp = &report;
        PodZero(&report);
        report.filename = filename.ptr();
        report.lineno = (uintN) lineno;
        if (JSVAL_IS_STRING(roots[2])) {
            JSFixedString *fixed = JSVAL_TO_STRING(roots[2])->ensureFixed(cx);
            if (!fixed)
                return false;
            report.ucmessage = fixed->chars();
        }
    }

    if (!reportp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNCAUGHT_EXCEPTION, bytes);
    } else {
        
        reportp->flags |= JSREPORT_EXCEPTION;

        
        JS_SetPendingException(cx, exn);
        js_ReportErrorAgain(cx, bytes, reportp);
        JS_ClearPendingException(cx);
    }

    return true;
}

extern JSObject *
js_CopyErrorObject(JSContext *cx, JSObject *errobj, JSObject *scope)
{
    assertSameCompartment(cx, scope);
    JSExnPrivate *priv = GetExnPrivate(errobj);

    uint32_t stackDepth = priv->stackDepth;
    size_t valueCount = 0;
    for (uint32_t i = 0; i < stackDepth; i++)
        valueCount += priv->stackElems[i].argc;

    size_t size = offsetof(JSExnPrivate, stackElems) +
                  stackDepth * sizeof(JSStackTraceElem) +
                  valueCount * sizeof(jsval);

    JSExnPrivate *copy = (JSExnPrivate *)cx->malloc_(size);
    if (!copy)
        return NULL;

    struct AutoFree {
        JSContext *cx;
        JSExnPrivate *p;
        ~AutoFree() {
            if (p) {
                cx->free_(p->errorReport);
                cx->free_(p);
            }
        }
    } autoFree = {cx, copy};

    
    if (priv->errorReport) {
        copy->errorReport = CopyErrorReport(cx, priv->errorReport);
        if (!copy->errorReport)
            return NULL;
    } else {
        copy->errorReport = NULL;
    }
    copy->message.init(priv->message);
    if (!cx->compartment->wrap(cx, &copy->message))
        return NULL;
    JS::Anchor<JSString *> messageAnchor(copy->message);
    copy->filename.init(priv->filename);
    if (!cx->compartment->wrap(cx, &copy->filename))
        return NULL;
    JS::Anchor<JSString *> filenameAnchor(copy->filename);
    copy->lineno = priv->lineno;
    copy->stackDepth = 0;
    copy->exnType = priv->exnType;

    
    JSObject *proto;
    if (!js_GetClassPrototype(cx, &scope->global(), GetExceptionProtoKey(copy->exnType), &proto))
        return NULL;
    JSObject *copyobj = NewObjectWithGivenProto(cx, &ErrorClass, proto, NULL);
    SetExnPrivate(cx, copyobj, copy);
    autoFree.p = NULL;
    return copyobj;
}

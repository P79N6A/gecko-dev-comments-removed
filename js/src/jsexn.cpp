











































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jsutil.h" 
#include "jsprf.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"


static JSBool
Exception(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static void
exn_trace(JSTracer *trc, JSObject *obj);

static void
exn_finalize(JSContext *cx, JSObject *obj);

static JSBool
exn_enumerate(JSContext *cx, JSObject *obj);

static JSBool
exn_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp);

JSClass js_ErrorClass = {
    js_Error_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_MARK_IS_TRACE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Error),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    exn_enumerate,    (JSResolveOp)exn_resolve, JS_ConvertStub, exn_finalize,
    NULL,             NULL,             NULL,             Exception,
    NULL,             NULL,             JS_CLASS_TRACE(exn_trace), NULL
};

typedef struct JSStackTraceElem {
    JSString            *funName;
    size_t              argc;
    const char          *filename;
    uintN               ulineno;
} JSStackTraceElem;

typedef struct JSExnPrivate {
    
    JSErrorReport       *errorReport;
    JSString            *message;
    JSString            *filename;
    uintN               lineno;
    size_t              stackDepth;
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
    uint8 *cursor;

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
    cursor = (uint8 *)cx->malloc(mallocSize);
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
            memcpy(cursor, report->messageArgs[i], argSize);
            cursor += argSize;
        }
        copy->messageArgs[i] = NULL;
        JS_ASSERT(cursor == (uint8 *)copy->messageArgs[0] + argsCopySize);
    }

    if (report->ucmessage) {
        copy->ucmessage = (const jschar *)cursor;
        memcpy(cursor, report->ucmessage, ucmessageSize);
        cursor += ucmessageSize;
    }

    if (report->uclinebuf) {
        copy->uclinebuf = (const jschar *)cursor;
        memcpy(cursor, report->uclinebuf, uclinebufSize);
        cursor += uclinebufSize;
        if (report->uctokenptr) {
            copy->uctokenptr = copy->uclinebuf + (report->uctokenptr -
                                                  report->uclinebuf);
        }
    }

    if (report->linebuf) {
        copy->linebuf = (const char *)cursor;
        memcpy(cursor, report->linebuf, linebufSize);
        cursor += linebufSize;
        if (report->tokenptr) {
            copy->tokenptr = copy->linebuf + (report->tokenptr -
                                              report->linebuf);
        }
    }

    if (report->filename) {
        copy->filename = (const char *)cursor;
        memcpy(cursor, report->filename, filenameSize);
    }
    JS_ASSERT(cursor + filenameSize == (uint8 *)copy + mallocSize);

    
    copy->lineno = report->lineno;
    copy->errorNumber = report->errorNumber;

    
    copy->flags = report->flags;

#undef JS_CHARS_SIZE
    return copy;
}

static jsval *
GetStackTraceValueBuffer(JSExnPrivate *priv)
{
    





    JS_STATIC_ASSERT(sizeof(JSStackTraceElem) % sizeof(jsval) == 0);

    return (jsval *)(priv->stackElems + priv->stackDepth);
}

static JSBool
InitExnPrivate(JSContext *cx, JSObject *exnObject, JSString *message,
               JSString *filename, uintN lineno, JSErrorReport *report)
{
    JSSecurityCallbacks *callbacks;
    JSCheckAccessOp checkAccess;
    JSErrorReporter older;
    JSExceptionState *state;
    jsval callerid, v;
    JSStackFrame *fp, *fpstop;
    size_t stackDepth, valueCount, size;
    JSBool overflow;
    JSExnPrivate *priv;
    JSStackTraceElem *elem;
    jsval *values;

    JS_ASSERT(OBJ_GET_CLASS(cx, exnObject) == &js_ErrorClass);

    






    callbacks = JS_GetSecurityCallbacks(cx);
    checkAccess = callbacks
                  ? callbacks->checkObjectAccess
                  : NULL;
    older = JS_SetErrorReporter(cx, NULL);
    state = JS_SaveExceptionState(cx);

    callerid = ATOM_KEY(cx->runtime->atomState.callerAtom);
    stackDepth = 0;
    valueCount = 0;
    for (fp = js_GetTopStackFrame(cx); fp; fp = fp->down) {
        if (fp->fun && fp->argv) {
            v = JSVAL_NULL;
            if (checkAccess &&
                !checkAccess(cx, fp->callee(), callerid, JSACC_READ, &v)) {
                break;
            }
            valueCount += fp->argc;
        }
        ++stackDepth;
    }
    JS_RestoreExceptionState(cx, state);
    JS_SetErrorReporter(cx, older);
    fpstop = fp;

    size = offsetof(JSExnPrivate, stackElems);
    overflow = (stackDepth > ((size_t)-1 - size) / sizeof(JSStackTraceElem));
    size += stackDepth * sizeof(JSStackTraceElem);
    overflow |= (valueCount > ((size_t)-1 - size) / sizeof(jsval));
    size += valueCount * sizeof(jsval);
    if (overflow) {
        js_ReportAllocationOverflow(cx);
        return JS_FALSE;
    }
    priv = (JSExnPrivate *)cx->malloc(size);
    if (!priv)
        return JS_FALSE;

    




    priv->errorReport = NULL;
    priv->message = message;
    priv->filename = filename;
    priv->lineno = lineno;
    priv->stackDepth = stackDepth;

    values = GetStackTraceValueBuffer(priv);
    elem = priv->stackElems;
    for (fp = js_GetTopStackFrame(cx); fp != fpstop; fp = fp->down) {
        if (!fp->fun) {
            elem->funName = NULL;
            elem->argc = 0;
        } else {
            elem->funName = fp->fun->atom
                            ? ATOM_TO_STRING(fp->fun->atom)
                            : cx->runtime->emptyString;
            elem->argc = fp->argc;
            memcpy(values, fp->argv, fp->argc * sizeof(jsval));
            values += fp->argc;
        }
        elem->ulineno = 0;
        elem->filename = NULL;
        if (fp->script) {
            elem->filename = fp->script->filename;
            if (fp->regs)
                elem->ulineno = js_FramePCToLineNumber(cx, fp);
        }
        ++elem;
    }
    JS_ASSERT(priv->stackElems + stackDepth == elem);
    JS_ASSERT(GetStackTraceValueBuffer(priv) + valueCount == values);

    exnObject->setPrivate(priv);

    if (report) {
        





        priv->errorReport = CopyErrorReport(cx, report);
        if (!priv->errorReport) {
            
            return JS_FALSE;
        }
    }

    return JS_TRUE;
}

static inline JSExnPrivate *
GetExnPrivate(JSContext *cx, JSObject *obj)
{
    return (JSExnPrivate *) obj->getPrivate();
}

static void
exn_trace(JSTracer *trc, JSObject *obj)
{
    JSExnPrivate *priv;
    JSStackTraceElem *elem;
    size_t vcount, i;
    jsval *vp, v;

    priv = GetExnPrivate(trc->context, obj);
    if (priv) {
        if (priv->message)
            JS_CALL_STRING_TRACER(trc, priv->message, "exception message");
        if (priv->filename)
            JS_CALL_STRING_TRACER(trc, priv->filename, "exception filename");

        elem = priv->stackElems;
        for (vcount = i = 0; i != priv->stackDepth; ++i, ++elem) {
            if (elem->funName) {
                JS_CALL_STRING_TRACER(trc, elem->funName,
                                      "stack trace function name");
            }
            if (IS_GC_MARKING_TRACER(trc) && elem->filename)
                js_MarkScriptFilename(elem->filename);
            vcount += elem->argc;
        }
        vp = GetStackTraceValueBuffer(priv);
        for (i = 0; i != vcount; ++i, ++vp) {
            v = *vp;
            JS_CALL_VALUE_TRACER(trc, v, "stack trace argument");
        }
    }
}

static void
exn_finalize(JSContext *cx, JSObject *obj)
{
    JSExnPrivate *priv;

    priv = GetExnPrivate(cx, obj);
    if (priv) {
        if (priv->errorReport)
            cx->free(priv->errorReport);
        cx->free(priv);
    }
}

static JSBool
exn_enumerate(JSContext *cx, JSObject *obj)
{
    JSAtomState *atomState;
    uintN i;
    JSAtom *atom;
    JSObject *pobj;
    JSProperty *prop;

    JS_STATIC_ASSERT(sizeof(JSAtomState) <= (size_t)(uint16)-1);
    static const uint16 offsets[] = {
        (uint16)offsetof(JSAtomState, messageAtom),
        (uint16)offsetof(JSAtomState, fileNameAtom),
        (uint16)offsetof(JSAtomState, lineNumberAtom),
        (uint16)offsetof(JSAtomState, stackAtom),
    };

    atomState = &cx->runtime->atomState;
    for (i = 0; i != JS_ARRAY_LENGTH(offsets); ++i) {
        atom = *(JSAtom **)((uint8 *)atomState + offsets[i]);
        if (!js_LookupProperty(cx, obj, ATOM_TO_JSID(atom), &pobj, &prop))
            return JS_FALSE;
        if (prop)
            pobj->dropProperty(cx, prop);
    }
    return JS_TRUE;
}

static JSBool
exn_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp)
{
    JSExnPrivate *priv;
    JSString *str;
    JSAtom *atom;
    JSString *stack;
    const char *prop;
    jsval v;

    *objp = NULL;
    priv = GetExnPrivate(cx, obj);
    if (priv && JSVAL_IS_STRING(id)) {
        str = JSVAL_TO_STRING(id);

        atom = cx->runtime->atomState.messageAtom;
        if (str == ATOM_TO_STRING(atom)) {
            prop = js_message_str;
            v = STRING_TO_JSVAL(priv->message);
            goto define;
        }

        atom = cx->runtime->atomState.fileNameAtom;
        if (str == ATOM_TO_STRING(atom)) {
            prop = js_fileName_str;
            v = STRING_TO_JSVAL(priv->filename);
            goto define;
        }

        atom = cx->runtime->atomState.lineNumberAtom;
        if (str == ATOM_TO_STRING(atom)) {
            prop = js_lineNumber_str;
            v = INT_TO_JSVAL(priv->lineno);
            goto define;
        }

        atom = cx->runtime->atomState.stackAtom;
        if (str == ATOM_TO_STRING(atom)) {
            stack = StackTraceToString(cx, priv);
            if (!stack)
                return JS_FALSE;

            
            priv->stackDepth = 0;
            prop = js_stack_str;
            v = STRING_TO_JSVAL(stack);
            goto define;
        }
    }
    return JS_TRUE;

  define:
    if (!JS_DefineProperty(cx, obj, prop, v, NULL, NULL, JSPROP_ENUMERATE))
        return JS_FALSE;
    *objp = obj;
    return JS_TRUE;
}

JSErrorReport *
js_ErrorFromException(JSContext *cx, jsval exn)
{
    JSObject *obj;
    JSExnPrivate *priv;

    if (JSVAL_IS_PRIMITIVE(exn))
        return NULL;
    obj = JSVAL_TO_OBJECT(exn);
    if (OBJ_GET_CLASS(cx, obj) != &js_ErrorClass)
        return NULL;
    priv = GetExnPrivate(cx, obj);
    if (!priv)
        return NULL;
    return priv->errorReport;
}

static JSString *
ValueToShortSource(JSContext *cx, jsval v)
{
    JSString *str;

    
    if (JSVAL_IS_PRIMITIVE(v)) {
        str = js_ValueToSource(cx, v);
    } else if (VALUE_IS_FUNCTION(cx, v)) {
        


        str = JS_GetFunctionId(JS_ValueToFunction(cx, v));
        if (!str && !(str = js_ValueToSource(cx, v))) {
            



            JS_ClearPendingException(cx);
            str = JS_NewStringCopyZ(cx, "[unknown function]");
        }
    } else {
        



        char buf[100];
        JS_snprintf(buf, sizeof buf, "[object %s]",
                    OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v))->name);
        str = JS_NewStringCopyZ(cx, buf);
    }
    return str;
}

static JSString *
StackTraceToString(JSContext *cx, JSExnPrivate *priv)
{
    jschar *stackbuf;
    size_t stacklen, stackmax;
    JSStackTraceElem *elem, *endElem;
    jsval *values;
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
            ptr_ = cx->realloc(stackbuf, (stackmax+1) * sizeof(jschar));      \
            if (!ptr_)                                                        \
                goto bad;                                                     \
            stackbuf = (jschar *) ptr_;                                       \
        }                                                                     \
        stackbuf[stacklen++] = (c);                                           \
    JS_END_MACRO

#define APPEND_STRING_TO_STACK(str)                                           \
    JS_BEGIN_MACRO                                                            \
        JSString *str_ = str;                                                 \
        const jschar *chars_;                                                 \
        size_t length_;                                                       \
                                                                              \
        str_->getCharsAndLength(chars_, length_);                             \
        if (length_ > stackmax - stacklen) {                                  \
            void *ptr_;                                                       \
            if (stackmax >= STACK_LENGTH_LIMIT ||                             \
                length_ >= STACK_LENGTH_LIMIT - stacklen) {                   \
                goto done;                                                    \
            }                                                                 \
            stackmax = JS_BIT(JS_CeilingLog2(stacklen + length_));            \
            ptr_ = cx->realloc(stackbuf, (stackmax+1) * sizeof(jschar));      \
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
        




        void *shrunk = cx->realloc(stackbuf, (stacklen+1) * sizeof(jschar));
        if (shrunk)
            stackbuf = (jschar *) shrunk;
    }

    stackbuf[stacklen] = 0;
    str = js_NewString(cx, stackbuf, stacklen);
    if (str)
        return str;

  bad:
    if (stackbuf)
        cx->free(stackbuf);
    return NULL;
}



static JSString *
FilenameToString(JSContext *cx, const char *filename)
{
    return JS_NewStringCopyZ(cx, filename);
}

static const char *
StringToFilename(JSContext *cx, JSString *str)
{
    return js_GetStringBytes(cx, str);
}

static JSBool
Exception(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uint32 lineno;
    JSString *message, *filename;
    JSStackFrame *fp;

    if (!JS_IsConstructing(cx)) {
        






        if (!JSVAL_TO_OBJECT(argv[-2])->getProperty(cx,
                                                    ATOM_TO_JSID(cx->runtime->atomState
                                                                 .classPrototypeAtom),
                                                    rval)) {
            return JS_FALSE;
        }
        obj = js_NewObject(cx, &js_ErrorClass, JSVAL_TO_OBJECT(*rval), NULL);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    }

    



    if (OBJ_GET_CLASS(cx, obj) == &js_ErrorClass)
        obj->setPrivate(NULL);

    
    if (argc != 0) {
        message = js_ValueToString(cx, argv[0]);
        if (!message)
            return JS_FALSE;
        argv[0] = STRING_TO_JSVAL(message);
    } else {
        message = cx->runtime->emptyString;
    }

    
    if (argc > 1) {
        filename = js_ValueToString(cx, argv[1]);
        if (!filename)
            return JS_FALSE;
        argv[1] = STRING_TO_JSVAL(filename);
        fp = NULL;
    } else {
        fp = js_GetScriptedCaller(cx, NULL);
        if (fp) {
            filename = FilenameToString(cx, fp->script->filename);
            if (!filename)
                return JS_FALSE;
        } else {
            filename = cx->runtime->emptyString;
        }
    }

    
    if (argc > 2) {
        lineno = js_ValueToECMAUint32(cx, &argv[2]);
        if (JSVAL_IS_NULL(argv[2]))
            return JS_FALSE;
    } else {
        if (!fp)
            fp = js_GetScriptedCaller(cx, NULL);
        lineno = (fp && fp->regs) ? js_FramePCToLineNumber(cx, fp) : 0;
    }

    return (OBJ_GET_CLASS(cx, obj) != &js_ErrorClass) ||
            InitExnPrivate(cx, obj, message, filename, lineno, NULL);
}








static JSBool
exn_toString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval v;
    JSString *name, *message, *result;
    jschar *chars, *cp;
    size_t name_length, message_length, length;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.nameAtom), &v))
        return JS_FALSE;
    name = JSVAL_IS_STRING(v) ? JSVAL_TO_STRING(v) : cx->runtime->emptyString;
    *vp = STRING_TO_JSVAL(name);

    if (!JS_GetProperty(cx, obj, js_message_str, &v))
        return JS_FALSE;
    message = JSVAL_IS_STRING(v) ? JSVAL_TO_STRING(v)
                                 : cx->runtime->emptyString;

    if (message->length() != 0) {
        name_length = name->length();
        message_length = message->length();
        length = (name_length ? name_length + 2 : 0) + message_length;
        cp = chars = (jschar *) cx->malloc((length + 1) * sizeof(jschar));
        if (!chars)
            return JS_FALSE;

        if (name_length) {
            js_strncpy(cp, name->chars(), name_length);
            cp += name_length;
            *cp++ = ':'; *cp++ = ' ';
        }
        js_strncpy(cp, message->chars(), message_length);
        cp += message_length;
        *cp = 0;

        result = js_NewString(cx, chars, length);
        if (!result) {
            cx->free(chars);
            return JS_FALSE;
        }
    } else {
        result = name;
    }

    *vp = STRING_TO_JSVAL(result);
    return JS_TRUE;
}

#if JS_HAS_TOSOURCE



static JSBool
exn_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    JSString *name, *message, *filename, *lineno_as_str, *result;
    jsval localroots[3] = {JSVAL_NULL, JSVAL_NULL, JSVAL_NULL};
    JSTempValueRooter tvr;
    JSBool ok;
    uint32 lineno;
    size_t lineno_length, name_length, message_length, filename_length, length;
    jschar *chars, *cp;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.nameAtom), vp))
        return JS_FALSE;
    name = js_ValueToString(cx, *vp);
    if (!name)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(name);

    MUST_FLOW_THROUGH("out");
    JS_PUSH_TEMP_ROOT(cx, 3, localroots, &tvr);

#ifdef __GNUC__
    message = filename = NULL;
#endif
    ok = JS_GetProperty(cx, obj, js_message_str, &localroots[0]) &&
         (message = js_ValueToSource(cx, localroots[0]));
    if (!ok)
        goto out;
    localroots[0] = STRING_TO_JSVAL(message);

    ok = JS_GetProperty(cx, obj, js_fileName_str, &localroots[1]) &&
         (filename = js_ValueToSource(cx, localroots[1]));
    if (!ok)
        goto out;
    localroots[1] = STRING_TO_JSVAL(filename);

    ok = JS_GetProperty(cx, obj, js_lineNumber_str, &localroots[2]);
    if (!ok)
        goto out;
    lineno = js_ValueToECMAUint32 (cx, &localroots[2]);
    ok = !JSVAL_IS_NULL(localroots[2]);
    if (!ok)
        goto out;

    if (lineno != 0) {
        lineno_as_str = js_ValueToString(cx, localroots[2]);
        if (!lineno_as_str) {
            ok = JS_FALSE;
            goto out;
        }
        lineno_length = lineno_as_str->length();
    } else {
        lineno_as_str = NULL;
        lineno_length = 0;
    }

    
    name_length = name->length();
    message_length = message->length();
    length = 8 + name_length + message_length;

    filename_length = filename->length();
    if (filename_length != 0) {
        
        length += 2 + filename_length;
        if (lineno_as_str) {
            
            length += 2 + lineno_length;
        }
    } else {
        if (lineno_as_str) {
            



            length += 6 + lineno_length;
        }
    }

    cp = chars = (jschar *) cx->malloc((length + 1) * sizeof(jschar));
    if (!chars) {
        ok = JS_FALSE;
        goto out;
    }

    *cp++ = '('; *cp++ = 'n'; *cp++ = 'e'; *cp++ = 'w'; *cp++ = ' ';
    js_strncpy(cp, name->chars(), name_length);
    cp += name_length;
    *cp++ = '(';
    if (message_length != 0) {
        js_strncpy(cp, message->chars(), message_length);
        cp += message_length;
    }

    if (filename_length != 0) {
        
        *cp++ = ','; *cp++ = ' ';
        js_strncpy(cp, filename->chars(), filename_length);
        cp += filename_length;
    } else {
        if (lineno_as_str) {
            



            *cp++ = ','; *cp++ = ' '; *cp++ = '"'; *cp++ = '"';
        }
    }
    if (lineno_as_str) {
        
        *cp++ = ','; *cp++ = ' ';
        js_strncpy(cp, lineno_as_str->chars(), lineno_length);
        cp += lineno_length;
    }

    *cp++ = ')'; *cp++ = ')'; *cp = 0;

    result = js_NewString(cx, chars, length);
    if (!result) {
        cx->free(chars);
        ok = JS_FALSE;
        goto out;
    }
    *vp = STRING_TO_JSVAL(result);
    ok = JS_TRUE;

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
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
    return (JSProtoKey) (JSProto_Error + exn);
}

JSObject *
js_InitExceptionClasses(JSContext *cx, JSObject *obj)
{
    jsval roots[3];
    JSObject *obj_proto, *error_proto;
    jsval empty;

    









    if (!js_GetClassPrototype(cx, obj, INT_TO_JSID(JSProto_Object),
                              &obj_proto)) {
        return NULL;
    }

    memset(roots, 0, sizeof(roots));
    JSAutoTempValueRooter tvr(cx, JS_ARRAY_LENGTH(roots), roots);

#ifdef __GNUC__
    error_proto = NULL;   
#endif

    
    for (intN i = JSEXN_ERR; i != JSEXN_LIMIT; i++) {
        JSObject *proto;
        JSProtoKey protoKey;
        JSAtom *atom;
        JSFunction *fun;

        
        proto = js_NewObject(cx, &js_ErrorClass,
                             (i != JSEXN_ERR) ? error_proto : obj_proto,
                             obj);
        if (!proto)
            return NULL;
        if (i == JSEXN_ERR) {
            error_proto = proto;
            roots[0] = OBJECT_TO_JSVAL(proto);
        } else {
            
            
            roots[1] = OBJECT_TO_JSVAL(proto);
        }

        
        proto->setPrivate(NULL);

        
        protoKey = GetExceptionProtoKey(i);
        atom = cx->runtime->atomState.classAtoms[protoKey];
        fun = js_DefineFunction(cx, obj, atom, Exception, 3, 0);
        if (!fun)
            return NULL;
        roots[2] = OBJECT_TO_JSVAL(FUN_OBJECT(fun));

        
        FUN_CLASP(fun) = &js_ErrorClass;

        
        if (!js_SetClassPrototype(cx, FUN_OBJECT(fun), proto,
                                  JSPROP_READONLY | JSPROP_PERMANENT)) {
            return NULL;
        }

        
        if (!JS_DefineProperty(cx, proto, js_name_str, ATOM_KEY(atom),
                               NULL, NULL, JSPROP_ENUMERATE)) {
            return NULL;
        }

        
        if (!js_SetClassObject(cx, obj, protoKey, FUN_OBJECT(fun)))
            return NULL;
    }

    



    empty = STRING_TO_JSVAL(cx->runtime->emptyString);
    if (!JS_DefineProperty(cx, error_proto, js_message_str, empty,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, error_proto, js_fileName_str, empty,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, error_proto, js_lineNumber_str, JSVAL_ZERO,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineFunctions(cx, error_proto, exception_methods)) {
        return NULL;
    }

    return error_proto;
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
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JSErrNum errorNumber;
    const JSErrorFormatString *errorString;
    JSExnType exn;
    jsval tv[4];
    JSTempValueRooter tvr;
    JSBool ok;
    JSObject *errProto, *errObject;
    JSString *messageStr, *filenameStr;

    


    JS_ASSERT(reportp);
    if (JSREPORT_IS_WARNING(reportp->flags))
        return JS_FALSE;

    
    errorNumber = (JSErrNum) reportp->errorNumber;
    errorString = js_GetLocalizedErrorMessage(cx, NULL, NULL, errorNumber);
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

    
    memset(tv, 0, sizeof tv);
    JS_PUSH_TEMP_ROOT(cx, JS_ARRAY_LENGTH(tv), tv, &tvr);

    




    ok = js_GetClassPrototype(cx, NULL, INT_TO_JSID(GetExceptionProtoKey(exn)),
                              &errProto);
    if (!ok)
        goto out;
    tv[0] = OBJECT_TO_JSVAL(errProto);

    errObject = js_NewObject(cx, &js_ErrorClass, errProto, NULL);
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
                        reportp->lineno, reportp);
    if (!ok)
        goto out;

    JS_SetPendingException(cx, OBJECT_TO_JSVAL(errObject));

    
    reportp->flags |= JSREPORT_EXCEPTION;

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    cx->generatingError = JS_FALSE;
    return ok;
}

JSBool
js_ReportUncaughtException(JSContext *cx)
{
    jsval exn;
    JSObject *exnObject;
    jsval roots[5];
    JSTempValueRooter tvr;
    JSErrorReport *reportp, report;
    JSString *str;
    const char *bytes;
    JSBool ok;

    if (!JS_IsExceptionPending(cx))
        return JS_TRUE;

    if (!JS_GetPendingException(cx, &exn))
        return JS_FALSE;

    memset(roots, 0, sizeof roots);
    JS_PUSH_TEMP_ROOT(cx, JS_ARRAY_LENGTH(roots), roots, &tvr);

    





    if (JSVAL_IS_PRIMITIVE(exn)) {
        exnObject = NULL;
    } else {
        exnObject = JSVAL_TO_OBJECT(exn);
        roots[0] = exn;
    }

    JS_ClearPendingException(cx);
    reportp = js_ErrorFromException(cx, exn);

    
    str = js_ValueToString(cx, exn);
    if (!str) {
        bytes = "unknown (can't convert to string)";
    } else {
        roots[1] = STRING_TO_JSVAL(str);
        bytes = js_GetStringBytes(cx, str);
        if (!bytes) {
            ok = JS_FALSE;
            goto out;
        }
    }
    ok = JS_TRUE;

    if (!reportp &&
        exnObject &&
        OBJ_GET_CLASS(cx, exnObject) == &js_ErrorClass) {
        const char *filename;
        uint32 lineno;

        ok = JS_GetProperty(cx, exnObject, js_message_str, &roots[2]);
        if (!ok)
            goto out;
        if (JSVAL_IS_STRING(roots[2])) {
            bytes = js_GetStringBytes(cx, JSVAL_TO_STRING(roots[2]));
            if (!bytes) {
                ok = JS_FALSE;
                goto out;
            }
        }

        ok = JS_GetProperty(cx, exnObject, js_fileName_str, &roots[3]);
        if (!ok)
            goto out;
        str = js_ValueToString(cx, roots[3]);
        if (!str) {
            ok = JS_FALSE;
            goto out;
        }
        filename = StringToFilename(cx, str);
        if (!filename) {
            ok = JS_FALSE;
            goto out;
        }

        ok = JS_GetProperty(cx, exnObject, js_lineNumber_str, &roots[4]);
        if (!ok)
            goto out;
        lineno = js_ValueToECMAUint32 (cx, &roots[4]);
        ok = !JSVAL_IS_NULL(roots[4]);
        if (!ok)
            goto out;

        reportp = &report;
        memset(&report, 0, sizeof report);
        report.filename = filename;
        report.lineno = (uintN) lineno;
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

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

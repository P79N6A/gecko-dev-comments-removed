










































#include "jsstddef.h"
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" 
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscript.h"
#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif

#if JS_HAS_SCRIPT_OBJECT

static const char js_script_exec[] = "Script.prototype.exec";
static const char js_script_compile[] = "Script.prototype.compile";




static jsint
GetScriptExecDepth(JSContext *cx, JSObject *obj)
{
    jsval v;

    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, obj));
    v = LOCKED_OBJ_GET_SLOT(obj, JSSLOT_START(&js_ScriptClass));
    return JSVAL_TO_INT(v);
}

static void
AdjustScriptExecDepth(JSContext *cx, JSObject *obj, jsint delta)
{
    jsint execDepth;

    JS_LOCK_OBJ(cx, obj);
    execDepth = GetScriptExecDepth(cx, obj);
    LOCKED_OBJ_SET_SLOT(obj, JSSLOT_START(&js_ScriptClass),
                        INT_TO_JSVAL(execDepth + delta));
    JS_UNLOCK_OBJ(cx, obj);
}

#if JS_HAS_TOSOURCE
static JSBool
script_toSource(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    uint32 indent;
    JSScript *script;
    size_t i, j, k, n;
    char buf[16];
    jschar *s, *t;
    JSString *str;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    indent = 0;
    if (argc && !js_ValueToECMAUint32(cx, argv[0], &indent))
        return JS_FALSE;

    script = (JSScript *) JS_GetPrivate(cx, obj);

    
    j = JS_snprintf(buf, sizeof buf, "(new %s(", js_ScriptClass.name);
    n = j + 2;
    if (!script) {
        
        k = 0;
        s = NULL;               
    } else {
        str = JS_DecompileScript(cx, script, "Script.prototype.toSource",
                                 (uintN)indent);
        if (!str)
            return JS_FALSE;
        str = js_QuoteString(cx, str, '\'');
        if (!str)
            return JS_FALSE;
        s = JSSTRING_CHARS(str);
        k = JSSTRING_LENGTH(str);
        n += k;
    }

    
    t = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!t)
        return JS_FALSE;
    for (i = 0; i < j; i++)
        t[i] = buf[i];
    for (j = 0; j < k; i++, j++)
        t[i] = s[j];
    t[i++] = ')';
    t[i++] = ')';
    t[i] = 0;

    
    str = JS_NewUCString(cx, t, n);
    if (!str) {
        JS_free(cx, t);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#endif 

static JSBool
script_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
    uint32 indent;
    JSScript *script;
    JSString *str;

    indent = 0;
    if (argc && !js_ValueToECMAUint32(cx, argv[0], &indent))
        return JS_FALSE;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script) {
        *rval = STRING_TO_JSVAL(cx->runtime->emptyString);
        return JS_TRUE;
    }

    str = JS_DecompileScript(cx, script, "Script.prototype.toString",
                             (uintN)indent);
    if (!str)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
script_compile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    JSString *str;
    JSObject *scopeobj;
    jsval v;
    JSScript *script, *oldscript;
    JSStackFrame *fp, *caller;
    const char *file;
    uintN line;
    JSPrincipals *principals;
    jsint execDepth;

    
    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    
    if (argc == 0)
        goto out;

    
    str = js_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(str);

    scopeobj = NULL;
    if (argc >= 2) {
        if (!js_ValueToObject(cx, argv[1], &scopeobj))
            return JS_FALSE;
        argv[1] = OBJECT_TO_JSVAL(scopeobj);
    }

    
    fp = cx->fp;
    caller = JS_GetScriptedCaller(cx, fp);
    JS_ASSERT(!caller || fp->scopeChain == caller->scopeChain);

    if (caller) {
        if (!scopeobj) {
            scopeobj = js_GetScopeChain(cx, caller);
            if (!scopeobj)
                return JS_FALSE;
            fp->scopeChain = scopeobj;  
        }

        file = caller->script->filename;
        line = js_PCToLineNumber(cx, caller->script, caller->pc);
        principals = JS_EvalFramePrincipals(cx, fp, caller);
    } else {
        file = NULL;
        line = 0;
        principals = NULL;
    }

    
    scopeobj = js_CheckScopeChainValidity(cx, scopeobj, js_script_compile);
    if (!scopeobj)
        return JS_FALSE;

    







    fp->flags |= JSFRAME_SCRIPT_OBJECT;
    script = JS_CompileUCScriptForPrincipals(cx, scopeobj, principals,
                                             JSSTRING_CHARS(str),
                                             JSSTRING_LENGTH(str),
                                             file, line);
    if (!script)
        return JS_FALSE;

    JS_LOCK_OBJ(cx, obj);
    execDepth = GetScriptExecDepth(cx, obj);

    



    if (execDepth > 0) {
        JS_UNLOCK_OBJ(cx, obj);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_COMPILE_EXECED_SCRIPT);
        return JS_FALSE;
    }

    
    v = LOCKED_OBJ_GET_SLOT(obj, JSSLOT_PRIVATE);
    oldscript = !JSVAL_IS_VOID(v) ? JSVAL_TO_PRIVATE(v) : NULL;
    LOCKED_OBJ_SET_SLOT(obj, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(script));
    JS_UNLOCK_OBJ(cx, obj);

    if (oldscript)
        js_DestroyScript(cx, oldscript);

    script->object = obj;
    js_CallNewScriptHook(cx, script, NULL);

out:
    
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
script_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *scopeobj, *parent;
    JSStackFrame *fp, *caller;
    JSPrincipals *principals;
    JSScript *script;
    JSBool ok;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    scopeobj = NULL;
    if (argc) {
        if (!js_ValueToObject(cx, argv[0], &scopeobj))
            return JS_FALSE;
        argv[0] = OBJECT_TO_JSVAL(scopeobj);
    }

    











    fp = cx->fp;
    caller = JS_GetScriptedCaller(cx, fp);
    if (caller && !caller->varobj) {
        
        JS_ASSERT(caller->fun && !JSFUN_HEAVYWEIGHT_TEST(caller->fun->flags));

        
        parent = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(caller->argv[-2]));
        if (!js_GetCallObject(cx, caller, parent))
            return JS_FALSE;
    }

    if (!scopeobj) {
        
        if (caller) {
            



            scopeobj = js_GetScopeChain(cx, caller);
            if (!scopeobj)
                return JS_FALSE;
        } else {
            







            scopeobj = cx->globalObject;
        }
    }

    scopeobj = js_CheckScopeChainValidity(cx, scopeobj, js_script_exec);
    if (!scopeobj)
        return JS_FALSE;

    
    AdjustScriptExecDepth(cx, obj, 1);

    
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script) {
        ok = JS_FALSE;
        goto out;
    }

    
    principals = script->principals;
    ok = js_CheckPrincipalsAccess(cx, scopeobj, principals,
                                  CLASS_ATOM(cx, Script));
    if (!ok)
        goto out;

    ok = js_Execute(cx, scopeobj, script, caller, JSFRAME_EVAL, rval);
   
out:
    AdjustScriptExecDepth(cx, obj, -1); 
    return ok;
}

#endif 

#if JS_HAS_XDR

static JSBool
XDRAtomMap(JSXDRState *xdr, JSAtomMap *map)
{
    JSContext *cx;
    uint32 natoms, i, index;
    JSAtom **atoms;

    cx = xdr->cx;

    if (xdr->mode == JSXDR_ENCODE)
        natoms = (uint32)map->length;

    if (!JS_XDRUint32(xdr, &natoms))
        return JS_FALSE;

    if (xdr->mode == JSXDR_ENCODE) {
        atoms = map->vector;
    } else {
        if (natoms == 0) {
            atoms = NULL;
        } else {
            atoms = (JSAtom **) JS_malloc(cx, (size_t)natoms * sizeof *atoms);
            if (!atoms)
                return JS_FALSE;
#ifdef DEBUG
            memset(atoms, 0, (size_t)natoms * sizeof *atoms);
#endif
        }

        map->vector = atoms;
        map->length = natoms;
    }

    for (i = 0; i != natoms; ++i) {
        if (xdr->mode == JSXDR_ENCODE)
            index = i;
        if (!JS_XDRUint32(xdr, &index))
            goto bad;

        



        JS_ASSERT(index < natoms);
        JS_ASSERT(xdr->mode == JSXDR_ENCODE || !atoms[index]);
        if (!js_XDRAtom(xdr, &atoms[index]))
            goto bad;
    }

    return JS_TRUE;

  bad:
    if (xdr->mode == JSXDR_DECODE) {
        JS_free(cx, atoms);
        map->vector = NULL;
        map->length = 0;
    }

    return JS_FALSE;
}

JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, JSBool *hasMagic)
{
    JSContext *cx;
    JSScript *script, *newscript, *oldscript;
    uint32 length, lineno, depth, magic, nsrcnotes, ntrynotes;
    uint32 prologLength, version;
    JSBool filenameWasSaved;
    jssrcnote *notes, *sn;

    cx = xdr->cx;
    script = *scriptp;
    nsrcnotes = ntrynotes = 0;
    filenameWasSaved = JS_FALSE;
    notes = NULL;

    















    if (xdr->mode == JSXDR_ENCODE)
        magic = JSXDR_MAGIC_SCRIPT_CURRENT;
    if (!JS_XDRUint32(xdr, &magic))
        return JS_FALSE;
    JS_ASSERT((uint32)JSXDR_MAGIC_SCRIPT_5 - (uint32)JSXDR_MAGIC_SCRIPT_1 == 4);
    if (magic - (uint32)JSXDR_MAGIC_SCRIPT_1 > 4) {
        if (!hasMagic) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_SCRIPT_MAGIC);
            return JS_FALSE;
        }
        *hasMagic = JS_FALSE;
        return JS_TRUE;
    }
    if (hasMagic)
        *hasMagic = JS_TRUE;

    if (xdr->mode == JSXDR_ENCODE) {
        length = script->length;
        prologLength = PTRDIFF(script->main, script->code, jsbytecode);
        JS_ASSERT((int16)script->version != JSVERSION_UNKNOWN);
        version = (uint32)script->version | (script->numGlobalVars << 16);
        lineno = (uint32)script->lineno;
        depth = (uint32)script->depth;

        
        notes = SCRIPT_NOTES(script);
        for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn))
            continue;
        nsrcnotes = PTRDIFF(sn, notes, jssrcnote);
        nsrcnotes++;            

        
        if (script->trynotes) {
            while (script->trynotes[ntrynotes].catchStart)
                ntrynotes++;
            ntrynotes++;        
        }
    }

    if (!JS_XDRUint32(xdr, &length))
        return JS_FALSE;
    if (magic >= JSXDR_MAGIC_SCRIPT_2) {
        if (!JS_XDRUint32(xdr, &prologLength))
            return JS_FALSE;
        if (!JS_XDRUint32(xdr, &version))
            return JS_FALSE;

        
        if (magic >= JSXDR_MAGIC_SCRIPT_4) {
            if (!JS_XDRUint32(xdr, &nsrcnotes))
                return JS_FALSE;
            if (!JS_XDRUint32(xdr, &ntrynotes))
                return JS_FALSE;
        }
    }

    if (xdr->mode == JSXDR_DECODE) {
        size_t alloclength = length;
        if (magic < JSXDR_MAGIC_SCRIPT_5)
            ++alloclength;      

        script = js_NewScript(cx, alloclength, nsrcnotes, ntrynotes);
        if (!script)
            return JS_FALSE;
        if (magic >= JSXDR_MAGIC_SCRIPT_2) {
            script->main += prologLength;
            script->version = (JSVersion) (version & 0xffff);
            script->numGlobalVars = (uint16) (version >> 16);

            
            if (magic >= JSXDR_MAGIC_SCRIPT_4)
                notes = SCRIPT_NOTES(script);
        }
        *scriptp = script;
    }

    





    oldscript = xdr->script;
    xdr->script = script;
    if (!JS_XDRBytes(xdr, (char *)script->code, length * sizeof(jsbytecode)) ||
        !XDRAtomMap(xdr, &script->atomMap)) {
        goto error;
    }

    if (magic < JSXDR_MAGIC_SCRIPT_5) {
        if (xdr->mode == JSXDR_DECODE) {
            




            script->code[length++] = JSOP_STOP;
        }

        if (magic < JSXDR_MAGIC_SCRIPT_4) {
            if (!JS_XDRUint32(xdr, &nsrcnotes))
                goto error;
            if (xdr->mode == JSXDR_DECODE) {
                notes = (jssrcnote *)
                        JS_malloc(cx, nsrcnotes * sizeof(jssrcnote));
                if (!notes)
                    goto error;
            }
        }
    }

    if (!JS_XDRBytes(xdr, (char *)notes, nsrcnotes * sizeof(jssrcnote)) ||
        !JS_XDRCStringOrNull(xdr, (char **)&script->filename) ||
        !JS_XDRUint32(xdr, &lineno) ||
        !JS_XDRUint32(xdr, &depth) ||
        (magic < JSXDR_MAGIC_SCRIPT_4 && !JS_XDRUint32(xdr, &ntrynotes))) {
        goto error;
    }

    
    if (magic >= JSXDR_MAGIC_SCRIPT_3) {
        JSPrincipals *principals;
        uint32 encodeable;

        if (xdr->mode == JSXDR_ENCODE) {
            principals = script->principals;
            encodeable = (cx->runtime->principalsTranscoder != NULL);
            if (!JS_XDRUint32(xdr, &encodeable))
                goto error;
            if (encodeable &&
                !cx->runtime->principalsTranscoder(xdr, &principals)) {
                goto error;
            }
        } else {
            if (!JS_XDRUint32(xdr, &encodeable))
                goto error;
            if (encodeable) {
                if (!cx->runtime->principalsTranscoder) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_CANT_DECODE_PRINCIPALS);
                    goto error;
                }
                if (!cx->runtime->principalsTranscoder(xdr, &principals))
                    goto error;
                script->principals = principals;
            }
        }
    }

    if (xdr->mode == JSXDR_DECODE) {
        const char *filename = script->filename;
        if (filename) {
            filename = js_SaveScriptFilename(cx, filename);
            if (!filename)
                goto error;
            JS_free(cx, (void *) script->filename);
            script->filename = filename;
            filenameWasSaved = JS_TRUE;
        }
        script->lineno = (uintN)lineno;
        script->depth = (uintN)depth;

        if (magic < JSXDR_MAGIC_SCRIPT_4) {
            





            uint32 osrcnotes = nsrcnotes;

            if (ntrynotes)
                nsrcnotes += JSTRYNOTE_ALIGNMASK;
            newscript = (JSScript *) JS_realloc(cx, script,
                                                sizeof(JSScript) +
                                                length * sizeof(jsbytecode) +
                                                nsrcnotes * sizeof(jssrcnote) +
                                                ntrynotes * sizeof(JSTryNote));
            if (!newscript)
                goto error;

            *scriptp = script = newscript;
            script->code = (jsbytecode *)(script + 1);
            script->main = script->code + prologLength;
            memcpy(script->code + length, notes, osrcnotes * sizeof(jssrcnote));
            JS_free(cx, (void *) notes);
            notes = NULL;
            if (ntrynotes) {
                script->trynotes = (JSTryNote *)
                                   ((jsword)(SCRIPT_NOTES(script) + nsrcnotes) &
                                    ~(jsword)JSTRYNOTE_ALIGNMASK);
                memset(script->trynotes, 0, ntrynotes * sizeof(JSTryNote));
            }
        }
    }

    while (ntrynotes) {
        JSTryNote *tn = &script->trynotes[--ntrynotes];
        uint32 start = (uint32) tn->start,
               catchLength = (uint32) tn->length,
               catchStart = (uint32) tn->catchStart;

        if (!JS_XDRUint32(xdr, &start) ||
            !JS_XDRUint32(xdr, &catchLength) ||
            !JS_XDRUint32(xdr, &catchStart)) {
            goto error;
        }
        tn->start = (ptrdiff_t) start;
        tn->length = (ptrdiff_t) catchLength;
        tn->catchStart = (ptrdiff_t) catchStart;
    }

    xdr->script = oldscript;
    return JS_TRUE;

  error:
    if (xdr->mode == JSXDR_DECODE) {
        if (script->filename && !filenameWasSaved) {
            JS_free(cx, (void *) script->filename);
            script->filename = NULL;
        }
        if (notes && magic < JSXDR_MAGIC_SCRIPT_4)
            JS_free(cx, (void *) notes);
        js_DestroyScript(cx, script);
        *scriptp = NULL;
    }
    return JS_FALSE;
}

#if JS_HAS_SCRIPT_OBJECT && JS_HAS_XDR_FREEZE_THAW






static JSBool
script_freeze(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    JSXDRState *xdr;
    JSScript *script;
    JSBool ok, hasMagic;
    uint32 len;
    void *buf;
    JSString *str;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;
    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (!script)
        return JS_TRUE;

    
    xdr = JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (!xdr)
        return JS_FALSE;

    
    ok = js_XDRScript(xdr, &script, &hasMagic);
    if (!ok)
        goto out;
    if (!hasMagic) {
        *rval = JSVAL_VOID;
        goto out;
    }

    buf = JS_XDRMemGetData(xdr, &len);
    if (!buf) {
        ok = JS_FALSE;
        goto out;
    }

    JS_ASSERT((jsword)buf % sizeof(jschar) == 0);
    len /= sizeof(jschar);
#if IS_BIG_ENDIAN
  {
    jschar *chars;
    uint32 i;

    
    chars = (jschar *)buf;
    for (i = 0; i < len; i++)
        chars[i] = JSXDR_SWAB16(chars[i]);
  }
#endif
    str = JS_NewUCStringCopyN(cx, (jschar *)buf, len);
    if (!str) {
        ok = JS_FALSE;
        goto out;
    }

    *rval = STRING_TO_JSVAL(str);

out:
    JS_XDRDestroy(xdr);
    return ok;
}

static JSBool
script_thaw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval)
{
    JSXDRState *xdr;
    JSString *str;
    void *buf;
    uint32 len;
    jsval v;
    JSScript *script, *oldscript;
    JSBool ok, hasMagic;

    if (!JS_InstanceOf(cx, obj, &js_ScriptClass, argv))
        return JS_FALSE;

    if (argc == 0)
        return JS_TRUE;
    str = js_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(str);

    
    xdr = JS_XDRNewMem(cx, JSXDR_DECODE);
    if (!xdr)
        return JS_FALSE;

    buf = JSSTRING_CHARS(str);
    len = JSSTRING_LENGTH(str);
#if IS_BIG_ENDIAN
  {
    jschar *from, *to;
    uint32 i;

    
    from = (jschar *)buf;
    to = (jschar *) JS_malloc(cx, len * sizeof(jschar));
    if (!to) {
        JS_XDRDestroy(xdr);
        return JS_FALSE;
    }
    for (i = 0; i < len; i++)
        to[i] = JSXDR_SWAB16(from[i]);
    buf = (char *)to;
  }
#endif
    len *= sizeof(jschar);
    JS_XDRMemSetData(xdr, buf, len);

    
    ok = js_XDRScript(xdr, &script, &hasMagic);
    if (!ok)
        goto out;
    if (!hasMagic) {
        *rval = JSVAL_FALSE;
        goto out;
    }

    JS_LOCK_OBJ(cx, obj);
    execDepth = GetScriptExecDepth(cx, obj);

    



    if (execDepth > 0) {
        JS_UNLOCK_OBJ(cx, obj);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_COMPILE_EXECED_SCRIPT);
        goto out;
    }

    
    v = LOCKED_OBJ_GET_SLOT(obj, JSSLOT_PRIVATE);
    oldscript = !JSVAL_IS_VOID(v) ? JSVAL_TO_PRIVATE(v) : NULL;
    LOCKED_OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(script));
    JS_UNLOCK_OBJ(cx, obj);

    if (oldscript)
        js_DestroyScript(cx, oldscript);

    script->object = obj;
    js_CallNewScriptHook(cx, script, NULL);

out:
    



    JS_XDRMemSetData(xdr, NULL, 0);
    JS_XDRDestroy(xdr);
#if IS_BIG_ENDIAN
    JS_free(cx, buf);
#endif
    *rval = JSVAL_TRUE;
    return ok;
}

static const char js_thaw_str[] = "thaw";

#endif 
#endif 

#if JS_HAS_SCRIPT_OBJECT

static JSFunctionSpec script_methods[] = {
#if JS_HAS_TOSOURCE
    {js_toSource_str,   script_toSource,        0,0,0},
#endif
    {js_toString_str,   script_toString,        0,0,0},
    {"compile",         script_compile,         2,0,0},
    {"exec",            script_exec,            1,0,0},
#if JS_HAS_XDR_FREEZE_THAW
    {"freeze",          script_freeze,          0,0,0},
    {js_thaw_str,       script_thaw,            1,0,0},
#endif 
    {0,0,0,0,0}
};

#endif 

static void
script_finalize(JSContext *cx, JSObject *obj)
{
    JSScript *script;

    script = (JSScript *) JS_GetPrivate(cx, obj);
    if (script)
        js_DestroyScript(cx, script);
}

static JSBool
script_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#if JS_HAS_SCRIPT_OBJECT
    return script_exec(cx, JSVAL_TO_OBJECT(argv[-2]), argc, argv, rval);
#else
    return JS_FALSE;
#endif
}

static void
script_trace(JSTracer *trc, JSObject *obj)
{
    JSScript *script;

    script = (JSScript *) JS_GetPrivate(trc->context, obj);
    if (script)
        js_TraceScript(trc, script);
}

#if !JS_HAS_SCRIPT_OBJECT
#define JSProto_Script  JSProto_Object
#endif

JS_FRIEND_DATA(JSClass) js_ScriptClass = {
    js_Script_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_MARK_IS_TRACE | JSCLASS_HAS_CACHED_PROTO(JSProto_Script),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   script_finalize,
    NULL,             NULL,             script_call,      NULL,
    NULL,             NULL,             JS_CLASS_TRACE(script_trace), NULL
};

#if JS_HAS_SCRIPT_OBJECT

static JSBool
Script(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_ScriptClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;

        



        *rval = OBJECT_TO_JSVAL(obj);
    }

    if (!JS_SetReservedSlot(cx, obj, 0, INT_TO_JSVAL(0)))
        return JS_FALSE;

    return script_compile(cx, obj, argc, argv, rval);
}

#if JS_HAS_SCRIPT_OBJECT && JS_HAS_XDR_FREEZE_THAW

static JSBool
script_static_thaw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
    obj = js_NewObject(cx, &js_ScriptClass, NULL, NULL);
    if (!obj)
        return JS_FALSE;
    if (!script_thaw(cx, obj, argc, argv, rval))
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSFunctionSpec script_static_methods[] = {
    {js_thaw_str,       script_static_thaw,     1,0,0},
    {0,0,0,0,0}
};

#else  

#define script_static_methods   NULL

#endif 

JSObject *
js_InitScriptClass(JSContext *cx, JSObject *obj)
{
    return JS_InitClass(cx, obj, NULL, &js_ScriptClass, Script, 1,
                        NULL, script_methods, NULL, script_static_methods);
}

#endif 




JS_STATIC_DLL_CALLBACK(int)
js_compare_strings(const void *k1, const void *k2)
{
    return strcmp(k1, k2) == 0;
}


extern void * JS_DLL_CALLBACK
js_alloc_table_space(void *priv, size_t size);

extern void JS_DLL_CALLBACK
js_free_table_space(void *priv, void *item);


typedef struct ScriptFilenameEntry {
    JSHashEntry         *next;          
    JSHashNumber        keyHash;        
    const void          *key;           
    uint32              flags;          
    JSPackedBool        mark;           
    char                filename[3];    
} ScriptFilenameEntry;

JS_STATIC_DLL_CALLBACK(JSHashEntry *)
js_alloc_sftbl_entry(void *priv, const void *key)
{
    size_t nbytes = offsetof(ScriptFilenameEntry, filename) + strlen(key) + 1;

    return (JSHashEntry *) malloc(JS_MAX(nbytes, sizeof(JSHashEntry)));
}

JS_STATIC_DLL_CALLBACK(void)
js_free_sftbl_entry(void *priv, JSHashEntry *he, uintN flag)
{
    if (flag != HT_FREE_ENTRY)
        return;
    free(he);
}

static JSHashAllocOps sftbl_alloc_ops = {
    js_alloc_table_space,   js_free_table_space,
    js_alloc_sftbl_entry,   js_free_sftbl_entry
};

JSBool
js_InitRuntimeScriptState(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(!rt->scriptFilenameTableLock);
    rt->scriptFilenameTableLock = JS_NEW_LOCK();
    if (!rt->scriptFilenameTableLock)
        return JS_FALSE;
#endif
    JS_ASSERT(!rt->scriptFilenameTable);
    rt->scriptFilenameTable =
        JS_NewHashTable(16, JS_HashString, js_compare_strings, NULL,
                        &sftbl_alloc_ops, NULL);
    if (!rt->scriptFilenameTable) {
        js_FinishRuntimeScriptState(rt);    
        return JS_FALSE;
    }
    JS_INIT_CLIST(&rt->scriptFilenamePrefixes);
    return JS_TRUE;
}

typedef struct ScriptFilenamePrefix {
    JSCList     links;      
    const char  *name;      
    size_t      length;     
    uint32      flags;      
} ScriptFilenamePrefix;

void
js_FinishRuntimeScriptState(JSRuntime *rt)
{
    if (rt->scriptFilenameTable) {
        JS_HashTableDestroy(rt->scriptFilenameTable);
        rt->scriptFilenameTable = NULL;
    }
#ifdef JS_THREADSAFE
    if (rt->scriptFilenameTableLock) {
        JS_DESTROY_LOCK(rt->scriptFilenameTableLock);
        rt->scriptFilenameTableLock = NULL;
    }
#endif
}

void
js_FreeRuntimeScriptState(JSRuntime *rt)
{
    ScriptFilenamePrefix *sfp;

    if (!rt->scriptFilenameTable)
        return;

    while (!JS_CLIST_IS_EMPTY(&rt->scriptFilenamePrefixes)) {
        sfp = (ScriptFilenamePrefix *) rt->scriptFilenamePrefixes.next;
        JS_REMOVE_LINK(&sfp->links);
        free(sfp);
    }
    js_FinishRuntimeScriptState(rt);
}

#ifdef DEBUG_brendan
#define DEBUG_SFTBL
#endif
#ifdef DEBUG_SFTBL
size_t sftbl_savings = 0;
#endif

static ScriptFilenameEntry *
SaveScriptFilename(JSRuntime *rt, const char *filename, uint32 flags)
{
    JSHashTable *table;
    JSHashNumber hash;
    JSHashEntry **hep;
    ScriptFilenameEntry *sfe;
    size_t length;
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    table = rt->scriptFilenameTable;
    hash = JS_HashString(filename);
    hep = JS_HashTableRawLookup(table, hash, filename);
    sfe = (ScriptFilenameEntry *) *hep;
#ifdef DEBUG_SFTBL
    if (sfe)
        sftbl_savings += strlen(sfe->filename);
#endif

    if (!sfe) {
        sfe = (ScriptFilenameEntry *)
              JS_HashTableRawAdd(table, hep, hash, filename, NULL);
        if (!sfe)
            return NULL;
        sfe->key = strcpy(sfe->filename, filename);
        sfe->flags = 0;
        sfe->mark = JS_FALSE;
    }

    
    if (flags != 0) {
        
        sfp = NULL;
        length = strlen(filename);
        for (head = link = &rt->scriptFilenamePrefixes;
             link->next != head;
             link = link->next) {
            
            sfp = (ScriptFilenamePrefix *) link->next;
            if (!strcmp(sfp->name, filename))
                break;
            if (sfp->length <= length) {
                sfp = NULL;
                break;
            }
            sfp = NULL;
        }

        if (!sfp) {
            
            sfp = (ScriptFilenamePrefix *) malloc(sizeof(ScriptFilenamePrefix));
            if (!sfp)
                return NULL;
            JS_INSERT_AFTER(&sfp->links, link);
            sfp->name = sfe->filename;
            sfp->length = length;
            sfp->flags = 0;
        }

        




        sfe->flags |= flags;
        sfp->flags |= flags;
    }

    return sfe;
}

const char *
js_SaveScriptFilename(JSContext *cx, const char *filename)
{
    JSRuntime *rt;
    ScriptFilenameEntry *sfe;
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    rt = cx->runtime;
    JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
    sfe = SaveScriptFilename(rt, filename, 0);
    if (!sfe) {
        JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    




    for (head = &rt->scriptFilenamePrefixes, link = head->next;
         link != head;
         link = link->next) {
        sfp = (ScriptFilenamePrefix *) link;
        if (!strncmp(sfp->name, filename, sfp->length)) {
            sfe->flags |= sfp->flags;
            break;
        }
    }
    JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
    return sfe->filename;
}

const char *
js_SaveScriptFilenameRT(JSRuntime *rt, const char *filename, uint32 flags)
{
    ScriptFilenameEntry *sfe;

    
    if (!rt->scriptFilenameTable && !js_InitRuntimeScriptState(rt))
        return NULL;

    JS_ACQUIRE_LOCK(rt->scriptFilenameTableLock);
    sfe = SaveScriptFilename(rt, filename, flags);
    JS_RELEASE_LOCK(rt->scriptFilenameTableLock);
    if (!sfe)
        return NULL;

    return sfe->filename;
}




#define FILENAME_TO_SFE(fn) \
    ((ScriptFilenameEntry *) ((fn) - offsetof(ScriptFilenameEntry, filename)))







#define ASSERT_VALID_SFE(sfe)   JS_ASSERT((sfe)->key == (sfe)->filename)

uint32
js_GetScriptFilenameFlags(const char *filename)
{
    ScriptFilenameEntry *sfe;

    sfe = FILENAME_TO_SFE(filename);
    ASSERT_VALID_SFE(sfe);
    return sfe->flags;
}

void
js_MarkScriptFilename(const char *filename)
{
    ScriptFilenameEntry *sfe;

    sfe = FILENAME_TO_SFE(filename);
    ASSERT_VALID_SFE(sfe);
    sfe->mark = JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(intN)
js_script_filename_marker(JSHashEntry *he, intN i, void *arg)
{
    ScriptFilenameEntry *sfe = (ScriptFilenameEntry *) he;

    sfe->mark = JS_TRUE;
    return HT_ENUMERATE_NEXT;
}

void
js_MarkScriptFilenames(JSRuntime *rt, JSBool keepAtoms)
{
    JSCList *head, *link;
    ScriptFilenamePrefix *sfp;

    if (!rt->scriptFilenameTable)
        return;

    if (keepAtoms) {
        JS_HashTableEnumerateEntries(rt->scriptFilenameTable,
                                     js_script_filename_marker,
                                     rt);
    }
    for (head = &rt->scriptFilenamePrefixes, link = head->next;
         link != head;
         link = link->next) {
        sfp = (ScriptFilenamePrefix *) link;
        js_MarkScriptFilename(sfp->name);
    }
}

JS_STATIC_DLL_CALLBACK(intN)
js_script_filename_sweeper(JSHashEntry *he, intN i, void *arg)
{
    ScriptFilenameEntry *sfe = (ScriptFilenameEntry *) he;

    if (!sfe->mark)
        return HT_ENUMERATE_REMOVE;
    sfe->mark = JS_FALSE;
    return HT_ENUMERATE_NEXT;
}

void
js_SweepScriptFilenames(JSRuntime *rt)
{
    if (!rt->scriptFilenameTable)
        return;

    JS_HashTableEnumerateEntries(rt->scriptFilenameTable,
                                 js_script_filename_sweeper,
                                 rt);
#ifdef DEBUG_notme
#ifdef DEBUG_SFTBL
    printf("script filename table savings so far: %u\n", sftbl_savings);
#endif
#endif
}

JSScript *
js_NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 ntrynotes)
{
    JSScript *script;

    
    if (ntrynotes)
        nsrcnotes += JSTRYNOTE_ALIGNMASK;
    script = (JSScript *) JS_malloc(cx,
                                    sizeof(JSScript) +
                                    length * sizeof(jsbytecode) +
                                    nsrcnotes * sizeof(jssrcnote) +
                                    ntrynotes * sizeof(JSTryNote));
    if (!script)
        return NULL;
    memset(script, 0, sizeof(JSScript));
    script->code = script->main = (jsbytecode *)(script + 1);
    script->length = length;
    script->version = cx->version;
    if (ntrynotes) {
        script->trynotes = (JSTryNote *)
                           ((jsword)(SCRIPT_NOTES(script) + nsrcnotes) &
                            ~(jsword)JSTRYNOTE_ALIGNMASK);
        memset(script->trynotes, 0, ntrynotes * sizeof(JSTryNote));
    }
    return script;
}

JS_FRIEND_API(JSScript *)
js_NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg, JSFunction *fun)
{
    uint32 mainLength, prologLength, nsrcnotes, ntrynotes;
    JSScript *script;
    const char *filename;

    mainLength = CG_OFFSET(cg);
    prologLength = CG_PROLOG_OFFSET(cg);
    CG_COUNT_FINAL_SRCNOTES(cg, nsrcnotes);
    CG_COUNT_FINAL_TRYNOTES(cg, ntrynotes);
    script = js_NewScript(cx, prologLength + mainLength, nsrcnotes, ntrynotes);
    if (!script)
        return NULL;

    
    script->main += prologLength;
    memcpy(script->code, CG_PROLOG_BASE(cg), prologLength * sizeof(jsbytecode));
    memcpy(script->main, CG_BASE(cg), mainLength * sizeof(jsbytecode));
    script->numGlobalVars = cg->treeContext.numGlobalVars;
    if (!js_InitAtomMap(cx, &script->atomMap, &cg->atomList))
        goto bad;

    filename = cg->filename;
    if (filename) {
        script->filename = js_SaveScriptFilename(cx, filename);
        if (!script->filename)
            goto bad;
    }
    script->lineno = cg->firstLine;
    script->depth = cg->maxStackDepth;
    if (cg->principals) {
        script->principals = cg->principals;
        JSPRINCIPALS_HOLD(cx, script->principals);
    }

    if (!js_FinishTakingSrcNotes(cx, cg, SCRIPT_NOTES(script)))
        goto bad;
    if (script->trynotes)
        js_FinishTakingTryNotes(cx, cg, script->trynotes);

    



    if (fun) {
        JS_ASSERT(FUN_INTERPRETED(fun) && !FUN_SCRIPT(fun));
        fun->u.i.script = script;
        if (cg->treeContext.flags & TCF_FUN_HEAVYWEIGHT)
            fun->flags |= JSFUN_HEAVYWEIGHT;
    }

    
    js_CallNewScriptHook(cx, script, fun);
    return script;

bad:
    js_DestroyScript(cx, script);
    return NULL;
}

JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun)
{
    JSRuntime *rt;
    JSNewScriptHook hook;

    rt = cx->runtime;
    hook = rt->newScriptHook;
    if (hook) {
        JS_KEEP_ATOMS(rt);
        hook(cx, script->filename, script->lineno, script, fun,
             rt->newScriptHookData);
        JS_UNKEEP_ATOMS(rt);
    }
}

JS_FRIEND_API(void)
js_CallDestroyScriptHook(JSContext *cx, JSScript *script)
{
    JSRuntime *rt;
    JSDestroyScriptHook hook;

    rt = cx->runtime;
    hook = rt->destroyScriptHook;
    if (hook)
        hook(cx, script, rt->destroyScriptHookData);
}

void
js_DestroyScript(JSContext *cx, JSScript *script)
{
    js_CallDestroyScriptHook(cx, script);

    JS_ClearScriptTraps(cx, script);
    js_FreeAtomMap(cx, &script->atomMap);
    if (script->principals)
        JSPRINCIPALS_DROP(cx, script->principals);
    if (JS_GSN_CACHE(cx).script == script)
        JS_CLEAR_GSN_CACHE(cx);
    JS_free(cx, script);
}

void
js_TraceScript(JSTracer *trc, JSScript *script)
{
    JSAtomMap *map;
    uintN i, length;
    JSAtom **vector;

    map = &script->atomMap;
    length = map->length;
    vector = map->vector;
    for (i = 0; i < length; i++)
        JS_CALL_TRACER(trc, vector[i], JSTRACE_ATOM, "atom_table");

    if (IS_GC_MARKING_TRACER(trc) && script->filename)
        js_MarkScriptFilename(script->filename);
}

typedef struct GSNCacheEntry {
    JSDHashEntryHdr     hdr;
    jsbytecode          *pc;
    jssrcnote           *sn;
} GSNCacheEntry;

#define GSN_CACHE_THRESHOLD     100

jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    ptrdiff_t target, offset;
    GSNCacheEntry *entry;
    jssrcnote *sn, *result;
    uintN nsrcnotes;


    target = PTRDIFF(pc, script->code, jsbytecode);
    if ((uint32)target >= script->length)
        return NULL;

    if (JS_GSN_CACHE(cx).script == script) {
        JS_METER_GSN_CACHE(cx, hits);
        entry = (GSNCacheEntry *)
                JS_DHashTableOperate(&JS_GSN_CACHE(cx).table, pc,
                                     JS_DHASH_LOOKUP);
        return entry->sn;
    }

    JS_METER_GSN_CACHE(cx, misses);
    offset = 0;
    for (sn = SCRIPT_NOTES(script); ; sn = SN_NEXT(sn)) {
        if (SN_IS_TERMINATOR(sn)) {
            result = NULL;
            break;
        }
        offset += SN_DELTA(sn);
        if (offset == target && SN_IS_GETTABLE(sn)) {
            result = sn;
            break;
        }
    }

    if (JS_GSN_CACHE(cx).script != script &&
        script->length >= GSN_CACHE_THRESHOLD) {
        JS_CLEAR_GSN_CACHE(cx);
        nsrcnotes = 0;
        for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn);
             sn = SN_NEXT(sn)) {
            if (SN_IS_GETTABLE(sn))
                ++nsrcnotes;
        }
        if (!JS_DHashTableInit(&JS_GSN_CACHE(cx).table, JS_DHashGetStubOps(),
                               NULL, sizeof(GSNCacheEntry),
                               JS_DHASH_DEFAULT_CAPACITY(nsrcnotes))) {
            JS_GSN_CACHE(cx).table.ops = NULL;
        } else {
            pc = script->code;
            for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn);
                 sn = SN_NEXT(sn)) {
                pc += SN_DELTA(sn);
                if (SN_IS_GETTABLE(sn)) {
                    entry = (GSNCacheEntry *)
                            JS_DHashTableOperate(&JS_GSN_CACHE(cx).table, pc,
                                                 JS_DHASH_ADD);
                    entry->pc = pc;
                    entry->sn = sn;
                }
            }
            JS_GSN_CACHE(cx).script = script;
            JS_METER_GSN_CACHE(cx, fills);
        }
    }

    return result;
}

uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSAtom *atom;
    JSFunction *fun;
    uintN lineno;
    ptrdiff_t offset, target;
    jssrcnote *sn;
    JSSrcNoteType type;

    
    if (!pc)
        return 0;

    



    if (js_CodeSpec[*pc].format & JOF_ATOMBASE)
        pc += js_CodeSpec[*pc].length;
    if (*pc == JSOP_DEFFUN) {
        atom = js_GetAtomFromBytecode(script, pc, 0);
        fun = (JSFunction *) JS_GetPrivate(cx, ATOM_TO_OBJECT(atom));
        JS_ASSERT(FUN_INTERPRETED(fun));
        return fun->u.i.script->lineno;
    }

    




    lineno = script->lineno;
    offset = 0;
    target = PTRDIFF(pc, script->code, jsbytecode);
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            if (offset <= target)
                lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            if (offset <= target)
                lineno++;
        }
        if (offset > target)
            break;
    }
    return lineno;
}


#define SN_LINE_LIMIT   (SN_3BYTE_OFFSET_FLAG << 16)

jsbytecode *
js_LineNumberToPC(JSScript *script, uintN target)
{
    ptrdiff_t offset, best;
    uintN lineno, bestdiff, diff;
    jssrcnote *sn;
    JSSrcNoteType type;

    offset = 0;
    best = -1;
    lineno = script->lineno;
    bestdiff = SN_LINE_LIMIT;
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        



        if (lineno == target && script->code + offset >= script->main)
            goto out;
        if (lineno >= target) {
            diff = lineno - target;
            if (diff < bestdiff) {
                bestdiff = diff;
                best = offset;
            }
        }
        offset += SN_DELTA(sn);
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    if (best >= 0)
        offset = best;
out:
    return script->code + offset;
}

JS_FRIEND_API(uintN)
js_GetScriptLineExtent(JSScript *script)
{
    uintN lineno;
    jssrcnote *sn;
    JSSrcNoteType type;

    lineno = script->lineno;
    for (sn = SCRIPT_NOTES(script); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        type = (JSSrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE) {
            lineno = (uintN) js_GetSrcNoteOffset(sn, 0);
        } else if (type == SRC_NEWLINE) {
            lineno++;
        }
    }
    return 1 + lineno - script->lineno;
}

#if JS_HAS_GENERATORS

jsbytecode *
js_FindFinallyHandler(JSScript *script, jsbytecode *pc)
{
    JSTryNote *tn;
    ptrdiff_t off;
    JSOp op2;

    tn = script->trynotes;
    if (!tn)
        return NULL;

    off = pc - script->main;
    if (off < 0)
        return NULL;

    JS_ASSERT(tn->catchStart != 0);
    do {
        if ((jsuword)(off - tn->start) < (jsuword)tn->length) {
            





            pc = script->main + tn->catchStart;
            JS_ASSERT(*pc == JSOP_SETSP);
            op2 = pc[JSOP_SETSP_LENGTH];
            if (op2 != JSOP_ENTERBLOCK) {
                JS_ASSERT(op2 == JSOP_GOSUB || op2 == JSOP_EXCEPTION);
                return pc;
            }
        }
    } while ((++tn)->catchStart != 0);
    return NULL;
}

#endif

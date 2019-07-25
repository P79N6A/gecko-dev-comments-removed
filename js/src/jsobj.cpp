










































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h" 
#include "jsbit.h"
#include "jsutil.h" 
#include "jshash.h" 
#include "jsdhash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstdint.h"
#include "jsstr.h"
#include "jstracer.h"
#include "jsdbgapi.h"
#include "json.h"

#include "jsinterpinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"

#if JS_HAS_GENERATORS
#include "jsiter.h"
#endif

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif

#include "jsprobes.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jsautooplen.h"

using namespace js;

JS_FRIEND_DATA(const JSObjectMap) JSObjectMap::sharedNonNative(JSObjectMap::SHAPELESS);

Class js_ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub
};

JS_FRIEND_API(JSObject *)
js_ObjectToOuterObject(JSContext *cx, JSObject *obj)
{
    OBJ_TO_OUTER_OBJECT(cx, obj);
    return obj;
}

#if JS_HAS_OBJ_PROTO_PROP

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp);

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, Value *vp);

static JSPropertySpec object_props[] = {
    {js_proto_str, 0, JSPROP_PERMANENT|JSPROP_SHARED, Jsvalify(obj_getProto), Jsvalify(obj_setProto)},
    {0,0,0,0,0}
};

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    
    uintN attrs;
    id = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    return CheckAccess(cx, obj, id, JSACC_PROTO, vp, &attrs);
}

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!vp->isObjectOrNull())
        return JS_TRUE;

    JSObject *pobj = vp->toObjectOrNull();
    if (pobj) {
        




        OBJ_TO_INNER_OBJECT(cx, pobj);
        if (!pobj)
            return JS_FALSE;
    }

    uintN attrs;
    id = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    if (!CheckAccess(cx, obj, id, JSAccessMode(JSACC_PROTO|JSACC_WRITE), vp, &attrs))
        return JS_FALSE;

    return SetProto(cx, obj, pobj, JS_TRUE);
}

#else  

#define object_props NULL

#endif 

static JSHashNumber
js_hash_object(const void *key)
{
    return JSHashNumber(uintptr_t(key) >> JS_GCTHING_ALIGN);
}

static JSHashEntry *
MarkSharpObjects(JSContext *cx, JSObject *obj, JSIdArray **idap)
{
    JSSharpObjectMap *map;
    JSHashTable *table;
    JSHashNumber hash;
    JSHashEntry **hep, *he;
    jsatomid sharpid;
    JSIdArray *ida;
    JSBool ok;
    jsint i, length;
    jsid id;
    JSObject *obj2;
    JSProperty *prop;

    JS_CHECK_RECURSION(cx, return NULL);

    map = &cx->sharpObjectMap;
    JS_ASSERT(map->depth >= 1);
    table = map->table;
    hash = js_hash_object(obj);
    hep = JS_HashTableRawLookup(table, hash, obj);
    he = *hep;
    if (!he) {
        sharpid = 0;
        he = JS_HashTableRawAdd(table, hep, hash, obj, (void *) sharpid);
        if (!he) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }

        ida = JS_Enumerate(cx, obj);
        if (!ida)
            return NULL;

        ok = JS_TRUE;
        for (i = 0, length = ida->length; i < length; i++) {
            id = ida->vector[i];
            ok = obj->lookupProperty(cx, id, &obj2, &prop);
            if (!ok)
                break;
            if (!prop)
                continue;
            bool hasGetter, hasSetter;
            AutoValueRooter v(cx);
            AutoValueRooter setter(cx);
            if (obj2->isNative()) {
                const Shape *shape = (Shape *) prop;
                hasGetter = shape->hasGetterValue();
                hasSetter = shape->hasSetterValue();
                if (hasGetter)
                    v.set(shape->getterValue());
                if (hasSetter)
                    setter.set(shape->setterValue());
                JS_UNLOCK_OBJ(cx, obj2);
            } else {
                hasGetter = hasSetter = false;
            }
            if (hasSetter) {
                
                if (hasGetter && v.value().isObject()) {
                    ok = !!MarkSharpObjects(cx, &v.value().toObject(), NULL);
                    if (!ok)
                        break;
                }
                v.set(setter.value());
            } else if (!hasGetter) {
                ok = obj->getProperty(cx, id, v.addr());
                if (!ok)
                    break;
            }
            if (v.value().isObject() &&
                !MarkSharpObjects(cx, &v.value().toObject(), NULL)) {
                ok = JS_FALSE;
                break;
            }
        }
        if (!ok || !idap)
            JS_DestroyIdArray(cx, ida);
        if (!ok)
            return NULL;
    } else {
        sharpid = uintptr_t(he->value);
        if (sharpid == 0) {
            sharpid = ++map->sharpgen << SHARP_ID_SHIFT;
            he->value = (void *) sharpid;
        }
        ida = NULL;
    }
    if (idap)
        *idap = ida;
    return he;
}

JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp)
{
    JSSharpObjectMap *map;
    JSHashTable *table;
    JSIdArray *ida;
    JSHashNumber hash;
    JSHashEntry *he, **hep;
    jsatomid sharpid;
    char buf[20];
    size_t len;

    if (!JS_CHECK_OPERATION_LIMIT(cx))
        return NULL;

    
    *sp = NULL;
    map = &cx->sharpObjectMap;
    table = map->table;
    if (!table) {
        table = JS_NewHashTable(8, js_hash_object, JS_CompareValues,
                                JS_CompareValues, NULL, NULL);
        if (!table) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
        map->table = table;
        JS_KEEP_ATOMS(cx->runtime);
    }

    
    ida = NULL;
    if (map->depth == 0) {
        











        ++map->depth;
        he = MarkSharpObjects(cx, obj, &ida);
        --map->depth;
        if (!he)
            goto bad;
        JS_ASSERT((uintptr_t(he->value) & SHARP_BIT) == 0);
        if (!idap) {
            JS_DestroyIdArray(cx, ida);
            ida = NULL;
        }
    } else {
        hash = js_hash_object(obj);
        hep = JS_HashTableRawLookup(table, hash, obj);
        he = *hep;

        






        if (!he) {
            he = JS_HashTableRawAdd(table, hep, hash, obj, NULL);
            if (!he) {
                JS_ReportOutOfMemory(cx);
                goto bad;
            }
            sharpid = 0;
            goto out;
        }
    }

    sharpid = uintptr_t(he->value);
    if (sharpid != 0) {
        len = JS_snprintf(buf, sizeof buf, "#%u%c",
                          sharpid >> SHARP_ID_SHIFT,
                          (sharpid & SHARP_BIT) ? '#' : '=');
        *sp = js_InflateString(cx, buf, &len);
        if (!*sp) {
            if (ida)
                JS_DestroyIdArray(cx, ida);
            goto bad;
        }
    }

out:
    JS_ASSERT(he);
    if ((sharpid & SHARP_BIT) == 0) {
        if (idap && !ida) {
            ida = JS_Enumerate(cx, obj);
            if (!ida) {
                if (*sp) {
                    cx->free(*sp);
                    *sp = NULL;
                }
                goto bad;
            }
        }
        map->depth++;
    }

    if (idap)
        *idap = ida;
    return he;

bad:
    
    if (map->depth == 0) {
        JS_UNKEEP_ATOMS(cx->runtime);
        map->sharpgen = 0;
        JS_HashTableDestroy(map->table);
        map->table = NULL;
    }
    return NULL;
}

void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap)
{
    JSSharpObjectMap *map;
    JSIdArray *ida;

    map = &cx->sharpObjectMap;
    JS_ASSERT(map->depth > 0);
    if (--map->depth == 0) {
        JS_UNKEEP_ATOMS(cx->runtime);
        map->sharpgen = 0;
        JS_HashTableDestroy(map->table);
        map->table = NULL;
    }
    if (idap) {
        ida = *idap;
        if (ida) {
            JS_DestroyIdArray(cx, ida);
            *idap = NULL;
        }
    }
}

static intN
gc_sharp_table_entry_marker(JSHashEntry *he, intN i, void *arg)
{
    JS_CALL_OBJECT_TRACER((JSTracer *)arg, (JSObject *)he->key,
                          "sharp table entry");
    return JS_DHASH_NEXT;
}

void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map)
{
    JS_ASSERT(map->depth > 0);
    JS_ASSERT(map->table);

    



















    JS_HashTableEnumerateEntries(map->table, gc_sharp_table_entry_marker, trc);
}

#if JS_HAS_TOSOURCE
static JSBool
obj_toSource(JSContext *cx, uintN argc, Value *vp)
{
    JSBool ok, outermost;
    JSObject *obj;
    JSHashEntry *he;
    JSIdArray *ida;
    jschar *chars, *ochars, *vsharp;
    const jschar *idstrchars, *vchars;
    size_t nchars, idstrlength, gsoplength, vlength, vsharplength, curlen;
    const char *comma;
    JSObject *obj2;
    JSProperty *prop;
    Value *val;
    JSString *gsop[2];
    JSString *idstr, *valstr, *str;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    Value localroot[4];
    PodArrayZero(localroot);
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(localroot), localroot);

    
    outermost = (cx->sharpObjectMap.depth == 0);
    obj = ComputeThisFromVp(cx, vp);
    if (!obj || !(he = js_EnterSharpObject(cx, obj, &ida, &chars))) {
        ok = JS_FALSE;
        goto out;
    }
    if (IS_SHARP(he)) {
        




        JS_ASSERT(!ida);
#if JS_HAS_SHARP_VARS
        nchars = js_strlen(chars);
#else
        chars[0] = '{';
        chars[1] = '}';
        chars[2] = 0;
        nchars = 2;
#endif
        goto make_string;
    }
    JS_ASSERT(ida);
    ok = JS_TRUE;

    if (!chars) {
        
        chars = (jschar *) cx->runtime->malloc(((outermost ? 4 : 2) + 1) * sizeof(jschar));
        nchars = 0;
        if (!chars)
            goto error;
        if (outermost)
            chars[nchars++] = '(';
    } else {
        
        MAKE_SHARP(he);
        nchars = js_strlen(chars);
        chars = (jschar *)
            js_realloc((ochars = chars), (nchars + 2 + 1) * sizeof(jschar));
        if (!chars) {
            js_free(ochars);
            goto error;
        }
        if (outermost) {
            




            outermost = JS_FALSE;
        }
    }

    chars[nchars++] = '{';

    comma = NULL;

    




    val = localroot + 2;

    for (jsint i = 0, length = ida->length; i < length; i++) {
        
        jsid id = ida->vector[i];

        ok = obj->lookupProperty(cx, id, &obj2, &prop);
        if (!ok)
            goto error;

        



        idstr = js_ValueToString(cx, IdToValue(id));
        if (!idstr) {
            ok = JS_FALSE;
            obj2->dropProperty(cx, prop);
            goto error;
        }
        vp->setString(idstr);                           

        jsint valcnt = 0;
        if (prop) {
            bool doGet = true;
            if (obj2->isNative()) {
                const Shape *shape = (Shape *) prop;
                unsigned attrs = shape->attributes();
                if (attrs & JSPROP_GETTER) {
                    doGet = false;
                    val[valcnt] = shape->getterValue();
                    gsop[valcnt] = ATOM_TO_STRING(cx->runtime->atomState.getAtom);
                    valcnt++;
                }
                if (attrs & JSPROP_SETTER) {
                    doGet = false;
                    val[valcnt] = shape->setterValue();
                    gsop[valcnt] = ATOM_TO_STRING(cx->runtime->atomState.setAtom);
                    valcnt++;
                }
                JS_UNLOCK_OBJ(cx, obj2);
            }
            if (doGet) {
                valcnt = 1;
                gsop[0] = NULL;
                ok = obj->getProperty(cx, id, &val[0]);
                if (!ok)
                    goto error;
            }
        }

        



        bool idIsLexicalIdentifier = !!js_IsIdentifier(idstr);
        if (JSID_IS_ATOM(id)
            ? !idIsLexicalIdentifier
            : (!JSID_IS_INT(id) || JSID_TO_INT(id) < 0)) {
            idstr = js_QuoteString(cx, idstr, jschar('\''));
            if (!idstr) {
                ok = JS_FALSE;
                goto error;
            }
            vp->setString(idstr);                       
        }
        idstr->getCharsAndLength(idstrchars, idstrlength);

        for (jsint j = 0; j < valcnt; j++) {
            



            if (gsop[j] && val[j].isUndefined())
                continue;

            
            valstr = js_ValueToSource(cx, val[j]);
            if (!valstr) {
                ok = JS_FALSE;
                goto error;
            }
            localroot[j].setString(valstr);             
            valstr->getCharsAndLength(vchars, vlength);

            




            vsharp = NULL;
            vsharplength = 0;
#if JS_HAS_SHARP_VARS
            if (!gsop[j] && val[j].isObject() && vchars[0] != '#') {
                he = js_EnterSharpObject(cx, &val[j].toObject(), NULL, &vsharp);
                if (!he) {
                    ok = JS_FALSE;
                    goto error;
                }
                if (IS_SHARP(he)) {
                    vchars = vsharp;
                    vlength = js_strlen(vchars);
                } else {
                    if (vsharp) {
                        vsharplength = js_strlen(vsharp);
                        MAKE_SHARP(he);
                    }
                    js_LeaveSharpObject(cx, NULL);
                }
            }
#endif

            



            if (gsop[j] && IsFunctionObject(val[j])) {
                const jschar *start = vchars;
                const jschar *end = vchars + vlength;

                uint8 parenChomp = 0;
                if (vchars[0] == '(') {
                    vchars++;
                    parenChomp = 1;
                }

                
                if (vchars)
                    vchars = js_strchr_limit(vchars, ' ', end);

                



                if (vchars)
                    vchars = js_strchr_limit(vchars, '(', end);

                if (vchars) {
                    if (*vchars == ' ')
                        vchars++;
                    vlength = end - vchars - parenChomp;
                } else {
                    gsop[j] = NULL;
                    vchars = start;
                }
            }

#define SAFE_ADD(n)                                                          \
    JS_BEGIN_MACRO                                                           \
        size_t n_ = (n);                                                     \
        curlen += n_;                                                        \
        if (curlen < n_)                                                     \
            goto overflow;                                                   \
    JS_END_MACRO

            curlen = nchars;
            if (comma)
                SAFE_ADD(2);
            SAFE_ADD(idstrlength + 1);
            if (gsop[j])
                SAFE_ADD(gsop[j]->length() + 1);
            SAFE_ADD(vsharplength);
            SAFE_ADD(vlength);
            
            SAFE_ADD((outermost ? 2 : 1) + 1);
#undef SAFE_ADD

            if (curlen > size_t(-1) / sizeof(jschar))
                goto overflow;

            
            chars = (jschar *) js_realloc((ochars = chars), curlen * sizeof(jschar));
            if (!chars) {
                
                cx->free(vsharp);
                js_free(ochars);
                goto error;
            }

            if (comma) {
                chars[nchars++] = comma[0];
                chars[nchars++] = comma[1];
            }
            comma = ", ";

            if (gsop[j]) {
                gsoplength = gsop[j]->length();
                js_strncpy(&chars[nchars], gsop[j]->chars(),
                            gsoplength);
                nchars += gsoplength;
                chars[nchars++] = ' ';
            }
            js_strncpy(&chars[nchars], idstrchars, idstrlength);
            nchars += idstrlength;
            
            chars[nchars++] = gsop[j] ? ' ' : ':';

            if (vsharplength) {
                js_strncpy(&chars[nchars], vsharp, vsharplength);
                nchars += vsharplength;
            }
            js_strncpy(&chars[nchars], vchars, vlength);
            nchars += vlength;

            if (vsharp)
                cx->free(vsharp);
        }
    }

    chars[nchars++] = '}';
    if (outermost)
        chars[nchars++] = ')';
    chars[nchars] = 0;

  error:
    js_LeaveSharpObject(cx, &ida);

    if (!ok) {
        if (chars)
            js_free(chars);
        goto out;
    }

    if (!chars) {
        JS_ReportOutOfMemory(cx);
        ok = JS_FALSE;
        goto out;
    }
  make_string:
    str = js_NewString(cx, chars, nchars);
    if (!str) {
        js_free(chars);
        ok = JS_FALSE;
        goto out;
    }
    vp->setString(str);
    ok = JS_TRUE;
  out:
    return ok;

  overflow:
    cx->free(vsharp);
    js_free(chars);
    chars = NULL;
    goto error;
}
#endif 

namespace js {

JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj)
{
    if (obj->isProxy())
        return JSProxy::obj_toString(cx, obj);

    const char *clazz = obj->wrappedObject(cx)->getClass()->name;
    size_t nchars = 9 + strlen(clazz); 
    jschar *chars = (jschar *) cx->malloc((nchars + 1) * sizeof(jschar));
    if (!chars)
        return NULL;

    const char *prefix = "[object ";
    nchars = 0;
    while ((chars[nchars] = (jschar)*prefix) != 0)
        nchars++, prefix++;
    while ((chars[nchars] = (jschar)*clazz) != 0)
        nchars++, clazz++;
    chars[nchars++] = ']';
    chars[nchars] = 0;

    JSString *str = js_NewString(cx, chars, nchars);
    if (!str)
        cx->free(chars);
    return str;
}

}

static JSBool
obj_toString(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return false;

    JSString *str = js::obj_toStringHelper(cx, obj);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}

static JSBool
obj_toLocaleString(JSContext *cx, uintN argc, Value *vp)
{
    if (!ComputeThisFromVp(cx, vp))
        return JS_FALSE;

    JSString *str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;

    vp->setString(str);
    return JS_TRUE;
}

static JSBool
obj_valueOf(JSContext *cx, uintN argc, Value *vp)
{
    if (!ComputeThisFromVp(cx, vp))
        return JS_FALSE;
    *vp = vp[1];
    return JS_TRUE;
}





JSBool
js_CheckContentSecurityPolicy(JSContext *cx)
{
    JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);

    
    
    if (callbacks && callbacks->contentSecurityPolicyAllows)
        return callbacks->contentSecurityPolicyAllows(cx);

    return JS_TRUE;
}






JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller)
{
    JSSecurityCallbacks *callbacks;
    JSPrincipals *scopePrincipals;
    const char *callerstr;

    callbacks = JS_GetSecurityCallbacks(cx);
    if (callbacks && callbacks->findObjectPrincipals) {
        scopePrincipals = callbacks->findObjectPrincipals(cx, scopeobj);
        if (!principals || !scopePrincipals ||
            !principals->subsume(principals, scopePrincipals)) {
            callerstr = js_AtomToPrintableString(cx, caller);
            if (!callerstr)
                return JS_FALSE;
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_INDIRECT_CALL, callerstr);
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSObject *
CheckScopeChainValidity(JSContext *cx, JSObject *scopeobj, const char *caller)
{
    JSObject *inner;

    if (!scopeobj)
        goto bad;

    OBJ_TO_INNER_OBJECT(cx, scopeobj);
    if (!scopeobj)
        return NULL;

    
    inner = scopeobj;
    while (scopeobj) {
        JSObjectOp op = scopeobj->getClass()->ext.innerObject;
        if (op && op(cx, scopeobj) != scopeobj)
            goto bad;
        scopeobj = scopeobj->getParent();
    }

    return inner;

bad:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_BAD_INDIRECT_CALL, caller);
    return NULL;
}

const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop)
{
    uint32 flags;
#ifdef DEBUG
    JSSecurityCallbacks *callbacks = JS_GetSecurityCallbacks(cx);
#endif

    JS_ASSERT(principals || !(callbacks  && callbacks->findObjectPrincipals));
    flags = JS_GetScriptFilenameFlags(caller->script());
    if ((flags & JSFILENAME_PROTECTED) &&
        principals &&
        strcmp(principals->codebase, "[System Principal]")) {
        *linenop = 0;
        return principals->codebase;
    }

    jsbytecode *pc = caller->pc(cx);
    if (pc && js_GetOpcode(cx, caller->script(), pc) == JSOP_EVAL) {
        JS_ASSERT(js_GetOpcode(cx, caller->script(), pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        *linenop = GET_UINT16(pc + JSOP_EVAL_LENGTH);
    } else {
        *linenop = js_FramePCToLineNumber(cx, caller);
    }
    return caller->script()->filename;
}

#ifndef EVAL_CACHE_CHAIN_LIMIT
# define EVAL_CACHE_CHAIN_LIMIT 4
#endif

static inline JSScript **
EvalCacheHash(JSContext *cx, JSString *str)
{
    const jschar *s;
    size_t n;
    uint32 h;

    str->getCharsAndLength(s, n);
    if (n > 100)
        n = 100;
    for (h = 0; n; s++, n--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;

    h *= JS_GOLDEN_RATIO;
    h >>= 32 - JS_EVAL_CACHE_SHIFT;
    return &JS_SCRIPTS_TO_GC(cx)[h];
}

static JSBool
obj_eval(JSContext *cx, uintN argc, Value *vp)
{
    if (argc < 1) {
        vp->setUndefined();
        return JS_TRUE;
    }

    JSStackFrame *caller = js_GetScriptedCaller(cx, NULL);
    if (!caller) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_INDIRECT_CALL, js_eval_str);
        return JS_FALSE;
    }

    jsbytecode *callerPC = caller->pc(cx);
    bool indirectCall = (callerPC && *callerPC != JSOP_EVAL);

    








    Value *argv = JS_ARGV(cx, vp);
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    obj = obj->wrappedObject(cx);

    OBJ_TO_INNER_OBJECT(cx, obj);
    if (!obj)
        return JS_FALSE;

    



    {
        JSObject *parent = obj->getParent();
        if (indirectCall || parent) {
            uintN flags = parent
                          ? JSREPORT_ERROR
                          : JSREPORT_STRICT | JSREPORT_WARNING;
            if (!JS_ReportErrorFlagsAndNumber(cx, flags, js_GetErrorMessage, NULL,
                                              JSMSG_BAD_INDIRECT_CALL,
                                              js_eval_str)) {
                return JS_FALSE;
            }
        }
    }

    if (!argv[0].isString()) {
        *vp = argv[0];
        return JS_TRUE;
    }

    




    if (argc > 1 && !caller->script()->warnedAboutTwoArgumentEval) {
        static const char TWO_ARGUMENT_WARNING[] =
            "Support for eval(code, scopeObject) has been removed. "
            "Use |with (scopeObject) eval(code);| instead.";
        if (!JS_ReportWarning(cx, TWO_ARGUMENT_WARNING))
            return JS_FALSE;
        caller->script()->warnedAboutTwoArgumentEval = true;
    }

    
    MUST_FLOW_THROUGH("out");
    uintN staticLevel = caller->script()->staticLevel + 1;

    



    JSObject *callerScopeChain = js_GetScopeChain(cx, caller);
    if (!callerScopeChain)
        return JS_FALSE;

    JSObject *scopeobj = NULL;

#if JS_HAS_EVAL_THIS_SCOPE
    




    if (indirectCall) {
        
        staticLevel = 0;

        if (!js_CheckPrincipalsAccess(cx, obj,
                                      js_StackFramePrincipals(cx, caller),
                                      cx->runtime->atomState.evalAtom)) {
            return JS_FALSE;
        }

        
        JS_ASSERT(!obj->getParent());
        scopeobj = obj;
    } else {
        




        JS_ASSERT_IF(caller->isFunctionFrame(), caller->hasCallObj());
        scopeobj = callerScopeChain;
    }
#endif

    
    JSObject *result = CheckScopeChainValidity(cx, scopeobj, js_eval_str);
    JS_ASSERT_IF(result, result == scopeobj);
    if (!result)
        return JS_FALSE;

    
    
    if (!js_CheckContentSecurityPolicy(cx)) {
        JS_ReportError(cx, "call to eval() blocked by CSP");
        return  JS_FALSE;
    }

    JSObject *callee = &vp[0].toObject();
    JSPrincipals *principals = js_EvalFramePrincipals(cx, callee, caller);
    uintN line;
    const char *file = js_ComputeFilename(cx, caller, principals, &line);

    JSString *str = argv[0].toString();
    JSScript *script = NULL;

    const jschar *chars;
    size_t length;
    str->getCharsAndLength(chars, length);

    





    if (length > 2 && chars[0] == '(' && chars[length-1] == ')') {
        JSONParser *jp = js_BeginJSONParse(cx, vp, true);
        JSBool ok = jp != NULL;
        if (ok) {
            
            ok = js_ConsumeJSONText(cx, jp, chars+1, length-2);
            ok &= js_FinishJSONParse(cx, jp, NullValue());
            if (ok)
                return JS_TRUE;
        }
    }

    








    JSScript **bucket = EvalCacheHash(cx, str);
    if (!indirectCall && caller->isFunctionFrame()) {
        uintN count = 0;
        JSScript **scriptp = bucket;

        EVAL_CACHE_METER(probe);
        JSVersion version = cx->findVersion();
        while ((script = *scriptp) != NULL) {
            if (script->savedCallerFun &&
                script->staticLevel == staticLevel &&
                script->version == version &&
                (script->principals == principals ||
                 (principals->subsume(principals, script->principals) &&
                  script->principals->subsume(script->principals, principals)))) {
                



                JSFunction *fun = script->getFunction(0);

                if (fun == caller->fun()) {
                    



                    JSString *src = ATOM_TO_STRING(script->atomMap.vector[0]);

                    if (src == str || js_EqualStrings(src, str)) {
                        






                        JSObjectArray *objarray = script->objects();
                        int i = 1;

                        if (objarray->length == 1) {
                            if (script->regexpsOffset != 0) {
                                objarray = script->regexps();
                                i = 0;
                            } else {
                                EVAL_CACHE_METER(noscope);
                                i = -1;
                            }
                        }
                        if (i < 0 ||
                            objarray->vector[i]->getParent() == scopeobj) {
                            JS_ASSERT(staticLevel == script->staticLevel);
                            EVAL_CACHE_METER(hit);
                            *scriptp = script->u.nextToGC;
                            script->u.nextToGC = NULL;
                            break;
                        }
                    }
                }
            }

            if (++count == EVAL_CACHE_CHAIN_LIMIT) {
                script = NULL;
                break;
            }
            EVAL_CACHE_METER(step);
            scriptp = &script->u.nextToGC;
        }
    }

    




    JSStackFrame *callerFrame = (staticLevel != 0) ? caller : NULL;
    if (!script) {
        uint32 tcflags = TCF_COMPILE_N_GO | TCF_NEED_MUTABLE_SCRIPT | TCF_COMPILE_FOR_EVAL;
        script = Compiler::compileScript(cx, scopeobj, callerFrame,
                                         principals, tcflags,
                                         chars, length,
                                         NULL, file, line, str, staticLevel);
        if (!script)
            return JS_FALSE;
    }

    



    JSBool ok = js_CheckPrincipalsAccess(cx, scopeobj, principals,
                                         cx->runtime->atomState.evalAtom) &&
                Execute(cx, scopeobj, script, callerFrame, JSFRAME_EVAL, vp);

    script->u.nextToGC = *bucket;
    *bucket = script;
#ifdef CHECK_SCRIPT_OWNER
    script->owner = NULL;
#endif

    return ok;
}

#if JS_HAS_OBJ_WATCHPOINT

static JSBool
obj_watch_handler(JSContext *cx, JSObject *obj, jsid id, jsval old,
                  jsval *nvp, void *closure)
{
    JSObject *callable;
    JSSecurityCallbacks *callbacks;
    JSStackFrame *caller;
    JSPrincipals *subject, *watcher;
    JSResolvingKey key;
    JSResolvingEntry *entry;
    uint32 generation;
    Value argv[3];
    JSBool ok;

    callable = (JSObject *) closure;

    callbacks = JS_GetSecurityCallbacks(cx);
    if (callbacks && callbacks->findObjectPrincipals) {
        
        caller = js_GetScriptedCaller(cx, NULL);
        if (caller) {
            



            watcher = callbacks->findObjectPrincipals(cx, callable);
            subject = js_StackFramePrincipals(cx, caller);

            if (watcher && subject && !watcher->subsume(watcher, subject)) {
                
                return JS_TRUE;
            }
        }
    }

    
    key.obj = obj;
    key.id = id;
    if (!js_StartResolving(cx, &key, JSRESFLAG_WATCH, &entry))
        return JS_FALSE;
    if (!entry)
        return JS_TRUE;
    generation = cx->resolvingTable->generation;

    argv[0] = IdToValue(id);
    argv[1] = Valueify(old);
    argv[2] = Valueify(*nvp);
    ok = ExternalInvoke(cx, obj, ObjectOrNullValue(callable), 3, argv, Valueify(nvp));
    js_StopResolving(cx, &key, JSRESFLAG_WATCH, entry, generation);
    return ok;
}

static JSBool
obj_watch(JSContext *cx, uintN argc, Value *vp)
{
    if (argc <= 1) {
        js_ReportMissingArg(cx, *vp, 1);
        return JS_FALSE;
    }

    JSObject *callable = js_ValueToCallableObject(cx, &vp[3], 0);
    if (!callable)
        return JS_FALSE;

    
    jsid propid;
    if (!ValueToId(cx, vp[2], &propid))
        return JS_FALSE;

    JSObject *obj = ComputeThisFromVp(cx, vp);
    Value tmp;
    uintN attrs;
    if (!obj || !CheckAccess(cx, obj, propid, JSACC_WATCH, &tmp, &attrs))
        return JS_FALSE;

    vp->setUndefined();

    if (attrs & JSPROP_READONLY)
        return JS_TRUE;
    if (obj->isDenseArray() && !obj->makeDenseArraySlow(cx))
        return JS_FALSE;
    return JS_SetWatchPoint(cx, obj, propid, obj_watch_handler, callable);
}

static JSBool
obj_unwatch(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    vp->setUndefined();
    jsid id;
    if (argc != 0) {
        if (!ValueToId(cx, vp[2], &id))
            return JS_FALSE;
    } else {
        id = JSID_VOID;
    }
    return JS_ClearWatchPoint(cx, obj, id, NULL, NULL);
}

#endif 







static JSBool
obj_hasOwnProperty(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    return obj &&
           js_HasOwnPropertyHelper(cx, obj->getOps()->lookupProperty, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;

    JSObject *obj = ComputeThisFromVp(cx, vp);
    JSObject *obj2;
    JSProperty *prop;
    if (!obj)
        return false;
    if (obj->isProxy()) {
        bool has;
        if (!JSProxy::hasOwn(cx, obj, id, &has))
            return false;
        vp->setBoolean(has);
        return true;
    }
    if (!js_HasOwnProperty(cx, lookup, obj, id, &obj2, &prop))
        return JS_FALSE;
    if (prop) {
        vp->setBoolean(true);
        obj2->dropProperty(cx, prop);
    } else {
        vp->setBoolean(false);
    }
    return JS_TRUE;
}

JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED | JSRESOLVE_DETECTING);
    if (!(lookup ? lookup : js_LookupProperty)(cx, obj, id, objp, propp))
        return false;
    if (!*propp)
        return true;

    if (*objp == obj)
        return true;

    Class *clasp = (*objp)->getClass();
    JSObject *outer = NULL;
    if (JSObjectOp op = (*objp)->getClass()->ext.outerObject) {
        outer = op(cx, *objp);
        if (!outer)
            return false;
    }

    if (outer != *objp) {
        if ((*objp)->isNative() && obj->getClass() == clasp) {
            














            Shape *shape = reinterpret_cast<Shape *>(*propp);
            if (shape->isSharedPermanent())
                return true;
        }

        (*objp)->dropProperty(cx, *propp);
        *propp = NULL;
    }
    return true;
}


static JSBool
obj_isPrototypeOf(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    const Value &v = argc != 0 ? vp[2] : UndefinedValue();
    vp->setBoolean(js_IsDelegate(cx, obj, v));
    return JS_TRUE;
}


static JSBool
obj_propertyIsEnumerable(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;

    JSObject *obj = ComputeThisFromVp(cx, vp);
    return obj && js_PropertyIsEnumerable(cx, obj, id, vp);
}

JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupProperty(cx, id, &pobj, &prop))
        return JS_FALSE;

    if (!prop) {
        vp->setBoolean(false);
        return JS_TRUE;
    }

    










    bool shared;
    uintN attrs;
    if (pobj->isNative()) {
        Shape *shape = (Shape *) prop;
        shared = shape->isSharedPermanent();
        attrs = shape->attributes();
        JS_UNLOCK_OBJ(cx, pobj);
    } else {
        shared = false;
        if (!pobj->getAttributes(cx, id, &attrs))
            return false;
    }
    if (pobj != obj && !shared) {
        vp->setBoolean(false);
        return true;
    }
    vp->setBoolean((attrs & JSPROP_ENUMERATE) != 0);
    return true;
}

#if OLD_GETTER_SETTER_METHODS

const char js_defineGetter_str[] = "__defineGetter__";
const char js_defineSetter_str[] = "__defineSetter__";
const char js_lookupGetter_str[] = "__lookupGetter__";
const char js_lookupSetter_str[] = "__lookupSetter__";

JS_FRIEND_API(JSBool)
js_obj_defineGetter(JSContext *cx, uintN argc, Value *vp)
{
    if (argc <= 1 || !js_IsCallable(vp[3])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_getter_str);
        return JS_FALSE;
    }
    PropertyOp getter = CastAsPropertyOp(&vp[3].toObject());

    jsid id;
    if (!ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj || !CheckRedeclaration(cx, obj, id, JSPROP_GETTER, NULL, NULL))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    vp->setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), getter, PropertyStub,
                               JSPROP_ENUMERATE | JSPROP_GETTER | JSPROP_SHARED);
}

JS_FRIEND_API(JSBool)
js_obj_defineSetter(JSContext *cx, uintN argc, Value *vp)
{
    if (argc <= 1 || !js_IsCallable(vp[3])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_setter_str);
        return JS_FALSE;
    }
    PropertyOp setter = CastAsPropertyOp(&vp[3].toObject());

    jsid id;
    if (!ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisFromVp(cx, vp);
    if (!obj || !CheckRedeclaration(cx, obj, id, JSPROP_SETTER, NULL, NULL))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    vp->setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), PropertyStub, setter,
                               JSPROP_ENUMERATE | JSPROP_SETTER | JSPROP_SHARED);
}

static JSBool
obj_lookupGetter(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisFromVp(cx, vp);
    JSObject *pobj;
    JSProperty *prop;
    if (!obj || !obj->lookupProperty(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasGetterValue())
                *vp = shape->getterValue();
            JS_UNLOCK_OBJ(cx, pobj);
        }
    }
    return JS_TRUE;
}

static JSBool
obj_lookupSetter(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisFromVp(cx, vp);
    JSObject *pobj;
    JSProperty *prop;
    if (!obj || !obj->lookupProperty(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasSetterValue())
                *vp = shape->setterValue();
            JS_UNLOCK_OBJ(cx, pobj);
        }
    }
    return JS_TRUE;
}
#endif 

JSBool
obj_getPrototypeOf(JSContext *cx, uintN argc, Value *vp)
{
    if (argc == 0) {
        js_ReportMissingArg(cx, *vp, 0);
        return JS_FALSE;
    }

    if (vp[2].isPrimitive()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, vp[2], NULL);
        if (!bytes)
            return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNEXPECTED_TYPE, bytes, "not an object");
        JS_free(cx, bytes);
        return JS_FALSE;
    }

    JSObject *obj = &vp[2].toObject();
    uintN attrs;
    return CheckAccess(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.protoAtom),
                       JSACC_PROTO, vp, &attrs);
}

extern JSBool
js_NewPropertyDescriptorObject(JSContext *cx, jsid id, uintN attrs,
                               const Value &getter, const Value &setter,
                               const Value &value, Value *vp)
{
    
    JSObject *desc = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!desc)
        return false;
    vp->setObject(*desc);    

    const JSAtomState &atomState = cx->runtime->atomState;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        if (!desc->defineProperty(cx, ATOM_TO_JSID(atomState.getAtom), getter,
                                  PropertyStub, PropertyStub, JSPROP_ENUMERATE) ||
            !desc->defineProperty(cx, ATOM_TO_JSID(atomState.setAtom), setter,
                                  PropertyStub, PropertyStub, JSPROP_ENUMERATE)) {
            return false;
        }
    } else {
        if (!desc->defineProperty(cx, ATOM_TO_JSID(atomState.valueAtom), value,
                                  PropertyStub, PropertyStub, JSPROP_ENUMERATE) ||
            !desc->defineProperty(cx, ATOM_TO_JSID(atomState.writableAtom),
                                  BooleanValue((attrs & JSPROP_READONLY) == 0),
                                  PropertyStub, PropertyStub, JSPROP_ENUMERATE)) {
            return false;
        }
    }

    return desc->defineProperty(cx, ATOM_TO_JSID(atomState.enumerableAtom),
                                BooleanValue((attrs & JSPROP_ENUMERATE) != 0),
                                PropertyStub, PropertyStub, JSPROP_ENUMERATE) &&
           desc->defineProperty(cx, ATOM_TO_JSID(atomState.configurableAtom),
                                BooleanValue((attrs & JSPROP_PERMANENT) == 0),
                                PropertyStub, PropertyStub, JSPROP_ENUMERATE);
}

JSBool
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (obj->isProxy()) {
        if (!JSProxy::getOwnPropertyDescriptor(cx, obj, id, vp))
            return false;
    }

    JSObject *pobj;
    JSProperty *prop;
    if (!js_HasOwnProperty(cx, obj->getOps()->lookupProperty, obj, id, &pobj, &prop))
        return false;
    if (!prop) {
        vp->setUndefined();
        return true;
    }

    Value roots[] = { UndefinedValue(), UndefinedValue(), UndefinedValue() };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(roots), roots);
    unsigned attrs;
    bool doGet = true;
    if (pobj->isNative()) {
        Shape *shape = (Shape *) prop;
        attrs = shape->attributes();
        if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
            doGet = false;
            if (attrs & JSPROP_GETTER)
                roots[0] = shape->getterValue();
            if (attrs & JSPROP_SETTER)
                roots[1] = shape->setterValue();
        }
        JS_UNLOCK_OBJ(cx, pobj);
    } else if (!pobj->getAttributes(cx, id, &attrs)) {
        return false;
    }

    if (doGet && !obj->getProperty(cx, id, &roots[2]))
        return false;

    return js_NewPropertyDescriptorObject(cx, id,
                                          attrs,
                                          roots[0], 
                                          roots[1], 
                                          roots[2], 
                                          vp);
}

static bool
GetFirstArgumentAsObject(JSContext *cx, uintN argc, Value *vp, const char *method, JSObject **objp)
{
    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             method, "0", "s");
        return false;
    }

    const Value &v = vp[2];
    if (v.isPrimitive()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NULL);
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object");
        JS_free(cx, bytes);
        return false;
    }

    *objp = &v.toObject();
    return true;
}

static JSBool
obj_getOwnPropertyDescriptor(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.getOwnPropertyDescriptor", &obj))
        return JS_FALSE;
    AutoIdRooter nameidr(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : UndefinedValue(), nameidr.addr()))
        return JS_FALSE;
    return js_GetOwnPropertyDescriptor(cx, obj, nameidr.id(), vp);
}

static JSBool
obj_keys(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.keys", &obj))
        return JS_FALSE;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, props))
        return JS_FALSE;

    AutoValueVector vals(cx);
    vals.resize(props.length());
    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];
        if (JSID_IS_STRING(id)) {
            vals[i].setString(JSID_TO_STRING(id));
        } else {
            JS_ASSERT(JSID_IS_INT(id));
            JSString *str = js_IntToString(cx, JSID_TO_INT(id));
            if (!str)
                return JS_FALSE;
            vals[i].setString(str);
        }
    }

    JS_ASSERT(props.length() <= UINT32_MAX);
    JSObject *aobj = js_NewArrayObject(cx, jsuint(vals.length()), vals.begin());
    if (!aobj)
        return JS_FALSE;
    vp->setObject(*aobj);

    return JS_TRUE;
}

static JSBool
HasProperty(JSContext* cx, JSObject* obj, jsid id, Value* vp, JSBool* answerp)
{
    if (!JS_HasPropertyById(cx, obj, id, answerp))
        return JS_FALSE;
    if (!*answerp) {
        vp->setUndefined();
        return JS_TRUE;
    }
    return JS_GetPropertyById(cx, obj, id, Jsvalify(vp));
}

PropDesc::PropDesc()
  : pd(UndefinedValue()),
    id(INT_TO_JSID(0)),
    value(UndefinedValue()),
    get(UndefinedValue()),
    set(UndefinedValue()),
    attrs(0),
    hasGet(false),
    hasSet(false),
    hasValue(false),
    hasWritable(false),
    hasEnumerable(false),
    hasConfigurable(false)
{
}

bool
PropDesc::initialize(JSContext* cx, jsid id, const Value &origval)
{
    Value v = origval;
    this->id = id;

    
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    JSObject* desc = &v.toObject();

    
    pd = v;

    
    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    JSBool hasProperty;

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.enumerableAtom), &v,
                     &hasProperty)) {
        return false;
    }
    if (hasProperty) {
        hasEnumerable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.configurableAtom), &v,
                     &hasProperty)) {
        return false;
    }
    if (hasProperty) {
        hasConfigurable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.valueAtom), &v, &hasProperty))
        return false;
    if (hasProperty) {
        hasValue = true;
        value = v;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.writableAtom), &v, &hasProperty))
        return false;
    if (hasProperty) {
        hasWritable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.getAtom), &v, &hasProperty))
        return false;
    if (hasProperty) {
        if ((v.isPrimitive() || !js_IsCallable(v)) && !v.isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_GET_SET_FIELD,
                                 js_getter_str);
            return false;
        }
        hasGet = true;
        get = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.setAtom), &v, &hasProperty))
        return false;
    if (hasProperty) {
        if ((v.isPrimitive() || !js_IsCallable(v)) && !v.isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_GET_SET_FIELD,
                                 js_setter_str);
            return false;
        }
        hasSet = true;
        set = v;
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
    }

    
    if ((hasGet || hasSet) && (hasValue || hasWritable)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INVALID_DESCRIPTOR);
        return false;
    }

    return true;
}

static JSBool
Reject(JSContext *cx, uintN errorNumber, bool throwError, jsid id, bool *rval)
{
    if (throwError) {
        jsid idstr;
        if (!js_ValueToStringId(cx, IdToValue(id), &idstr))
           return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber,
                             JS_GetStringBytes(JSID_TO_STRING(idstr)));
        return JS_FALSE;
    }

    *rval = false;
    return JS_TRUE;
}

static JSBool
Reject(JSContext *cx, uintN errorNumber, bool throwError, bool *rval)
{
    if (throwError) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber);
        return JS_FALSE;
    }

    *rval = false;
    return JS_TRUE;
}

static JSBool
Reject(JSContext *cx, JSObject *obj, JSProperty *prop, uintN errorNumber, bool throwError,
       jsid id, bool *rval)
{
    obj->dropProperty(cx, prop);
    return Reject(cx, errorNumber, throwError, id, rval);
}

static JSBool
DefinePropertyOnObject(JSContext *cx, JSObject *obj, const PropDesc &desc,
                       bool throwError, bool *rval)
{
    
    JSProperty *current;
    JSObject *obj2;
    JS_ASSERT(!obj->getOps()->lookupProperty);
    if (!js_HasOwnProperty(cx, NULL, obj, desc.id, &obj2, &current))
        return JS_FALSE;

    JS_ASSERT(!obj->getOps()->defineProperty);

    
    if (!current) {
        if (obj->sealed())
            return Reject(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            return js_DefineProperty(cx, obj, desc.id, &desc.value,
                                     PropertyStub, PropertyStub, desc.attrs);
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        uintN dummyAttrs;
        if (!CheckAccess(cx, obj, desc.id, JSACC_WATCH, &dummy, &dummyAttrs))
            return JS_FALSE;

        Value tmp = UndefinedValue();
        return js_DefineProperty(cx, obj, desc.id, &tmp,
                                 desc.getter(), desc.setter(), desc.attrs);
    }

    
    Value v = UndefinedValue();

    







    JS_ASSERT(obj->getClass() == obj2->getClass());

    const Shape *shape = reinterpret_cast<Shape *>(current);
    do {
        if (desc.isAccessorDescriptor()) {
            if (!shape->isAccessorDescriptor())
                break;

            if (desc.hasGet &&
                !SameValue(desc.getterValue(), shape->getterOrUndefined(), cx)) {
                break;
            }

            if (desc.hasSet &&
                !SameValue(desc.setterValue(), shape->setterOrUndefined(), cx)) {
                break;
            }
        } else {
            






            if (shape->isDataDescriptor()) {
                






















                if (!shape->configurable() &&
                    (!shape->hasDefaultGetter() || !shape->hasDefaultSetter())) {
                    return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                                  throwError, desc.id, rval);
                }

                if (!js_NativeGet(cx, obj, obj2, shape, JSGET_NO_METHOD_BARRIER, &v)) {
                    
                    return JS_FALSE;
                }
            }

            if (desc.isDataDescriptor()) {
                if (!shape->isDataDescriptor())
                    break;

                if (desc.hasValue && !SameValue(desc.value, v, cx))
                    break;
                if (desc.hasWritable && desc.writable() != shape->writable())
                    break;
            } else {
                
                JS_ASSERT(desc.isGenericDescriptor());
            }
        }

        if (desc.hasConfigurable && desc.configurable() != shape->configurable())
            break;
        if (desc.hasEnumerable && desc.enumerable() != shape->enumerable())
            break;

        
        obj2->dropProperty(cx, current);
        *rval = true;
        return JS_TRUE;
    } while (0);

    
    if (!shape->configurable()) {
        






        JS_ASSERT_IF(!desc.hasConfigurable, !desc.configurable());
        if (desc.configurable() ||
            (desc.hasEnumerable && desc.enumerable() != shape->enumerable())) {
            return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP, throwError,
                          desc.id, rval);
        }
    }

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shape->isDataDescriptor()) {
        
        if (!shape->configurable()) {
            return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                          throwError, desc.id, rval);
        }
    } else if (desc.isDataDescriptor()) {
        
        JS_ASSERT(shape->isDataDescriptor());
        if (!shape->configurable() && !shape->writable()) {
            if ((desc.hasWritable && desc.writable()) ||
                (desc.hasValue && !SameValue(desc.value, v, cx))) {
                return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                              throwError, desc.id, rval);
            }
        }
    } else {
        
        JS_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            if ((desc.hasSet &&
                 !SameValue(desc.setterValue(), shape->setterOrUndefined(), cx)) ||
                (desc.hasGet &&
                 !SameValue(desc.getterValue(), shape->getterOrUndefined(), cx))) {
                return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                              throwError, desc.id, rval);
            }
        }
    }

    
    uintN attrs;
    PropertyOp getter, setter;
    if (desc.isGenericDescriptor()) {
        uintN changed = 0;
        if (desc.hasConfigurable)
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable)
            changed |= JSPROP_ENUMERATE;

        attrs = (shape->attributes() & ~changed) | (desc.attrs & changed);
        if (shape->isMethod()) {
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            getter = setter = PropertyStub;
        } else {
            getter = shape->getter();
            setter = shape->setter();
        }
    } else if (desc.isDataDescriptor()) {
        uintN unchanged = 0;
        if (!desc.hasConfigurable)
            unchanged |= JSPROP_PERMANENT;
        if (!desc.hasEnumerable)
            unchanged |= JSPROP_ENUMERATE;
        if (!desc.hasWritable)
            unchanged |= JSPROP_READONLY;

        if (desc.hasValue)
            v = desc.value;
        attrs = (desc.attrs & ~unchanged) | (shape->attributes() & unchanged);
        getter = setter = PropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        if (!CheckAccess(cx, obj2, desc.id, JSACC_WATCH, &dummy, &attrs)) {
             obj2->dropProperty(cx, current);
             return JS_FALSE;
        }

        JS_ASSERT_IF(shape->isMethod(), !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

        
        uintN changed = 0;
        if (desc.hasConfigurable)
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable)
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGet)
            changed |= JSPROP_GETTER | JSPROP_SHARED;
        if (desc.hasSet)
            changed |= JSPROP_SETTER | JSPROP_SHARED;

        attrs = (desc.attrs & changed) | (shape->attributes() & ~changed);
        if (desc.hasGet) {
            getter = desc.getter();
        } else {
            getter = (shape->isMethod() || (shape->hasDefaultGetter() && !shape->hasGetterValue()))
                     ? PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet) {
            setter = desc.setter();
        } else {
            setter = (shape->hasDefaultSetter() && !shape->hasSetterValue())
                     ? PropertyStub
                     : shape->setter();
        }
    }

    *rval = true;
    obj2->dropProperty(cx, current);
    return js_DefineProperty(cx, obj, desc.id, &v, getter, setter, attrs);
}

static JSBool
DefinePropertyOnArray(JSContext *cx, JSObject *obj, const PropDesc &desc,
                      bool throwError, bool *rval)
{
    






    if (obj->isDenseArray() && !obj->makeDenseArraySlow(cx))
        return JS_FALSE;

    jsuint oldLen = obj->getArrayLength();

    if (JSID_IS_ATOM(desc.id, cx->runtime->atomState.lengthAtom)) {
        






        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEFINE_ARRAY_LENGTH_UNSUPPORTED);
        return JS_FALSE;
    }

    uint32 index;
    if (js_IdIsIndex(desc.id, &index)) {
        




        if (!DefinePropertyOnObject(cx, obj, desc, false, rval))
            return JS_FALSE;
        if (!*rval)
            return Reject(cx, JSMSG_CANT_DEFINE_ARRAY_INDEX, throwError, rval);

        if (index >= oldLen) {
            JS_ASSERT(index != UINT32_MAX);
            obj->setArrayLength(index + 1);
        }

        *rval = true;
        return JS_TRUE;
    }

    return DefinePropertyOnObject(cx, obj, desc, throwError, rval);
}

static JSBool
DefineProperty(JSContext *cx, JSObject *obj, const PropDesc &desc, bool throwError,
               bool *rval)
{
    if (obj->isArray())
        return DefinePropertyOnArray(cx, obj, desc, throwError, rval);

    if (obj->getOps()->lookupProperty) {
        if (obj->isProxy())
            return JSProxy::defineProperty(cx, obj, desc.id, desc.pd);
        return Reject(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, desc, throwError, rval);
}

JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const Value &descriptor, JSBool *bp)
{
    AutoPropDescArrayRooter descs(cx);
    PropDesc *desc = descs.append();
    if (!desc || !desc->initialize(cx, id, descriptor))
        return false;

    bool rval;
    if (!DefineProperty(cx, obj, *desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}


static JSBool
obj_defineProperty(JSContext* cx, uintN argc, Value* vp)
{
    
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.defineProperty", &obj))
        return JS_FALSE;
    vp->setObject(*obj);

    
    AutoIdRooter nameidr(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : UndefinedValue(), nameidr.addr()))
        return JS_FALSE;

    
    const Value &descval = argc >= 3 ? vp[4] : UndefinedValue();

    
    JSBool junk;
    return js_DefineOwnProperty(cx, obj, nameidr.id(), descval, &junk);
}

static bool
DefineProperties(JSContext *cx, JSObject *obj, JSObject *props)
{
    AutoIdArray ida(cx, JS_Enumerate(cx, props));
    if (!ida)
        return false;

     AutoPropDescArrayRooter descs(cx);
     size_t len = ida.length();
     for (size_t i = 0; i < len; i++) {
         jsid id = ida[i];
         PropDesc* desc = descs.append();
         AutoValueRooter tvr(cx);
         if (!desc ||
             !JS_GetPropertyById(cx, props, id, tvr.jsval_addr()) ||
             !desc->initialize(cx, id, tvr.value())) {
             return false;
         }
     }

     bool dummy;
     for (size_t i = 0; i < len; i++) {
         if (!DefineProperty(cx, obj, descs[i], true, &dummy))
             return false;
     }

     return true;
}

extern JSBool
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props)
{
    return DefineProperties(cx, newborn, props);
}


static JSBool
obj_defineProperties(JSContext* cx, uintN argc, Value* vp)
{
    
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.defineProperties", &obj))
        return false;
    vp->setObject(*obj);

    if (argc < 2) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Object.defineProperties", "0", "s");
        return false;
    }

    JSObject* props = js_ValueToNonNullObject(cx, vp[3]);
    if (!props)
        return false;
    vp[3].setObject(*props);

    return DefineProperties(cx, obj, props);
}


static JSBool
obj_create(JSContext *cx, uintN argc, Value *vp)
{
    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Object.create", "0", "s");
        return JS_FALSE;
    }

    const Value &v = vp[2];
    if (!v.isObjectOrNull()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NULL);
        if (!bytes)
            return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object or null");
        JS_free(cx, bytes);
        return JS_FALSE;
    }

    



    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &js_ObjectClass, v.toObjectOrNull(),
                                                        vp->toObject().getGlobal());
    if (!obj)
        return JS_FALSE;
    vp->setObject(*obj); 

    
    if (argc > 1 && !vp[3].isUndefined()) {
        if (vp[3].isPrimitive()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
            return JS_FALSE;
        }

        JSObject *props = &vp[3].toObject();
        AutoIdArray ida(cx, JS_Enumerate(cx, props));
        if (!ida)
            return JS_FALSE;

        AutoPropDescArrayRooter descs(cx);
        size_t len = ida.length();
        for (size_t i = 0; i < len; i++) {
            jsid id = ida[i];
            PropDesc *desc = descs.append();
            if (!desc || !JS_GetPropertyById(cx, props, id, Jsvalify(&vp[1])) ||
                !desc->initialize(cx, id, vp[1])) {
                return JS_FALSE;
            }
        }

        bool dummy;
        for (size_t i = 0; i < len; i++) {
            if (!DefineProperty(cx, obj, descs[i], true, &dummy))
                return JS_FALSE;
        }
    }

    
    return JS_TRUE;
}

static JSBool
obj_getOwnPropertyNames(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.getOwnPropertyNames", &obj))
        return false;

    AutoIdVector keys(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, keys))
        return false;

    AutoValueVector vals(cx);
    if (!vals.resize(keys.length()))
        return false;

    for (size_t i = 0, len = keys.length(); i < len; i++) {
         jsid id = keys[i];
         if (JSID_IS_INT(id)) {
             JSString *str = js_ValueToString(cx, Int32Value(JSID_TO_INT(id)));
             if (!str)
                 return false;
             vals[i].setString(str);
         } else if (JSID_IS_ATOM(id)) {
             vals[i].setString(JSID_TO_STRING(id));
         } else {
             vals[i].setObject(*JSID_TO_OBJECT(id));
         }
    }

    JSObject *aobj = js_NewArrayObject(cx, vals.length(), vals.begin());
    if (!aobj)
        return false;

    vp->setObject(*aobj);
    return true;
}


#if JS_HAS_OBJ_WATCHPOINT
const char js_watch_str[] = "watch";
const char js_unwatch_str[] = "unwatch";
#endif
const char js_hasOwnProperty_str[] = "hasOwnProperty";
const char js_isPrototypeOf_str[] = "isPrototypeOf";
const char js_propertyIsEnumerable_str[] = "propertyIsEnumerable";

static JSFunctionSpec object_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,             obj_toSource,                0,0),
#endif
    JS_FN(js_toString_str,             obj_toString,                0,0),
    JS_FN(js_toLocaleString_str,       obj_toLocaleString,          0,0),
    JS_FN(js_valueOf_str,              obj_valueOf,                 0,0),
#if JS_HAS_OBJ_WATCHPOINT
    JS_FN(js_watch_str,                obj_watch,                   2,0),
    JS_FN(js_unwatch_str,              obj_unwatch,                 1,0),
#endif
    JS_FN(js_hasOwnProperty_str,       obj_hasOwnProperty,          1,0),
    JS_FN(js_isPrototypeOf_str,        obj_isPrototypeOf,           1,0),
    JS_FN(js_propertyIsEnumerable_str, obj_propertyIsEnumerable,    1,0),
#if OLD_GETTER_SETTER_METHODS
    JS_FN(js_defineGetter_str,         js_obj_defineGetter,         2,0),
    JS_FN(js_defineSetter_str,         js_obj_defineSetter,         2,0),
    JS_FN(js_lookupGetter_str,         obj_lookupGetter,            1,0),
    JS_FN(js_lookupSetter_str,         obj_lookupSetter,            1,0),
#endif
    JS_FS_END
};

static JSFunctionSpec object_static_methods[] = {
    JS_FN("getPrototypeOf",            obj_getPrototypeOf,          1,0),
    JS_FN("getOwnPropertyDescriptor",  obj_getOwnPropertyDescriptor,2,0),
    JS_FN("keys",                      obj_keys,                    1,0),
    JS_FN("defineProperty",            obj_defineProperty,          3,0),
    JS_FN("defineProperties",          obj_defineProperties,        2,0),
    JS_FN("create",                    obj_create,                  2,0),
    JS_FN("getOwnPropertyNames",       obj_getOwnPropertyNames,     1,0),
    JS_FS_END
};

JSBool
js_Object(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (argc == 0) {
        
        obj = NULL;
    } else {
        
        if (!js_ValueToObjectOrNull(cx, vp[2], &obj))
            return JS_FALSE;
    }
    if (!obj) {
        
        JS_ASSERT(!argc || vp[2].isNull() || vp[2].isUndefined());
        obj = NewBuiltinClassInstance(cx, &js_ObjectClass);
        if (!obj)
            return JS_FALSE;
    }
    vp->setObject(*obj);
    return JS_TRUE;
}

JSObject*
js_NewInstance(JSContext *cx, JSObject *callee)
{
    Class *clasp = callee->getClass();

    Class *newclasp = &js_ObjectClass;
    if (clasp == &js_FunctionClass) {
        JSFunction *fun = callee->getFunctionPrivate();
        if (fun->isNative() && fun->u.n.clasp)
            newclasp = fun->u.n.clasp;
    }

    Value protov;
    if (!callee->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &protov))
        return NULL;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : NULL;
    JSObject *parent = callee->getParent();
    return NewObject<WithProto::Class>(cx, newclasp, proto, parent);
}

#ifdef JS_TRACER

static JS_ALWAYS_INLINE JSObject*
NewObjectWithClassProto(JSContext *cx, Class *clasp, JSObject *proto,
                        const Value &privateSlotValue)
{
    JS_ASSERT(clasp->isNative());

    JSObject* obj = js_NewGCObject(cx);
    if (!obj)
        return NULL;

    obj->initSharingEmptyShape(clasp, proto, proto->getParent(), privateSlotValue, cx);
    return obj;
}

JSObject* FASTCALL
js_Object_tn(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(js_ObjectClass.flags & JSCLASS_HAS_PRIVATE));

    return NewObjectWithClassProto(cx, &js_ObjectClass, proto, UndefinedValue());
}

JS_DEFINE_TRCINFO_1(js_Object,
    (2, (extern, CONSTRUCTOR_RETRY, js_Object_tn, CONTEXT, CALLEE_PROTOTYPE, 0,
         nanojit::ACCSET_STORE_ANY)))

JSObject* FASTCALL
js_NonEmptyObject(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(js_ObjectClass.flags & JSCLASS_HAS_PRIVATE));

    JSObject *obj = NewObjectWithClassProto(cx, &js_ObjectClass, proto, UndefinedValue());
    return (obj && obj->ensureClassReservedSlotsForEmptyObject(cx)) ? obj : NULL;
}

JS_DEFINE_CALLINFO_2(extern, CONSTRUCTOR_RETRY, js_NonEmptyObject, CONTEXT, CALLEE_PROTOTYPE, 0,
                     nanojit::ACCSET_STORE_ANY)

JSObject* FASTCALL
js_String_tn(JSContext* cx, JSObject* proto, JSString* str)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    return NewObjectWithClassProto(cx, &js_StringClass, proto, StringValue(str));
}
JS_DEFINE_CALLINFO_3(extern, OBJECT, js_String_tn, CONTEXT, CALLEE_PROTOTYPE, STRING, 0,
                     nanojit::ACCSET_STORE_ANY)

JSObject* FASTCALL
js_NewInstanceFromTrace(JSContext *cx, Class *clasp, JSObject *ctor)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    JS_ASSERT(ctor->isFunction());
#ifdef JS_THREADSAFE
    if (ctor->title.ownercx != cx)
        return NULL;
#endif

    if (!ctor->ensureClassReservedSlots(cx))
        return NULL;

    jsid classPrototypeId = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    const Shape *shape = ctor->nativeLookup(classPrototypeId);
    Value pval = shape ? ctor->getSlot(shape->slot) : MagicValue(JS_GENERIC_MAGIC);

    JSObject *parent = ctor->getParent();
    JSObject *proto;
    if (pval.isObject()) {
        
        proto = &pval.toObject();
    } else {
        
        if (!js_GetClassPrototype(cx, parent, JSProto_Object, &proto))
            return NULL;

        if (pval.isMagic(JS_GENERIC_MAGIC)) {
            



            proto = NewNativeClassInstance(cx, clasp, proto, parent);
            if (!proto)
                return NULL;
            if (!js_SetClassPrototype(cx, ctor, proto, JSPROP_ENUMERATE | JSPROP_PERMANENT))
                return NULL;
        } else {
            



        }
    }

    



    return NewNonFunction<WithProto::Given>(cx, clasp, proto, parent);
}

JS_DEFINE_CALLINFO_3(extern, CONSTRUCTOR_RETRY, js_NewInstanceFromTrace, CONTEXT, CLASS, OBJECT, 0,
                     nanojit::ACCSET_STORE_ANY)

#else  

# define js_Object_trcinfo NULL

#endif 






JS_REQUIRES_STACK JSBool
Detecting(JSContext *cx, jsbytecode *pc)
{
    JSScript *script;
    jsbytecode *endpc;
    JSOp op;
    JSAtom *atom;

    script = cx->fp()->script();
    endpc = script->code + script->length;
    for (;; pc += js_CodeSpec[op].length) {
        JS_ASSERT_IF(!cx->fp()->hasImacropc(), script->code <= pc && pc < endpc);

        
        op = js_GetOpcode(cx, script, pc);
        if (js_CodeSpec[op].format & JOF_DETECTING)
            return JS_TRUE;

        switch (op) {
          case JSOP_NULL:
            



            if (++pc < endpc) {
                op = js_GetOpcode(cx, script, pc);
                return *pc == JSOP_EQ || *pc == JSOP_NE;
            }
            return JS_FALSE;

          case JSOP_GETGNAME:
          case JSOP_NAME:
            




            GET_ATOM_FROM_BYTECODE(script, pc, 0, atom);
            if (atom == cx->runtime->atomState.typeAtoms[JSTYPE_VOID] &&
                (pc += js_CodeSpec[op].length) < endpc) {
                op = js_GetOpcode(cx, script, pc);
                return op == JSOP_EQ || op == JSOP_NE ||
                       op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
            }
            return JS_FALSE;

          default:
            



            if (!(js_CodeSpec[op].format & JOF_INDEXBASE))
                return JS_FALSE;
            break;
        }
    }
}







uintN
js_InferFlags(JSContext *cx, uintN defaultFlags)
{
#ifdef JS_TRACER
    if (JS_ON_TRACE(cx))
        return cx->bailExit->lookupFlags;
#endif

    JS_ASSERT_NOT_ON_TRACE(cx);

    jsbytecode *pc;
    const JSCodeSpec *cs;
    uint32 format;
    uintN flags = 0;

    JSStackFrame *const fp = js_GetTopStackFrame(cx);
    if (!fp || !(pc = cx->regs->pc))
        return defaultFlags;
    cs = &js_CodeSpec[js_GetOpcode(cx, fp->script(), pc)];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if ((format & (JOF_SET | JOF_FOR)) || fp->isAssigning()) {
        flags |= JSRESOLVE_ASSIGNING;
    } else if (cs->length >= 0) {
        pc += cs->length;
        JSScript *script = cx->fp()->script();
        if (pc < script->code + script->length && Detecting(cx, pc))
            flags |= JSRESOLVE_DETECTING;
    }
    if (format & JOF_DECLARING)
        flags |= JSRESOLVE_DECLARING;
    return flags;
}




static JSBool
with_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                    JSProperty **propp)
{
    
    uintN flags = cx->resolveFlags;
    if (flags == JSRESOLVE_INFER)
        flags = js_InferFlags(cx, flags);
    flags |= JSRESOLVE_WITH;
    JSAutoResolveFlags rf(cx, flags);
    return obj->getProto()->lookupProperty(cx, id, objp, propp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return obj->getProto()->getProperty(cx, id, vp);
}

static JSBool
with_SetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return obj->getProto()->setProperty(cx, id, vp, strict);
}

static JSBool
with_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj->getProto()->getAttributes(cx, id, attrsp);
}

static JSBool
with_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj->getProto()->setAttributes(cx, id, attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    return obj->getProto()->deleteProperty(cx, id, rval, strict);
}

static JSBool
with_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
               Value *statep, jsid *idp)
{
    return obj->getProto()->enumerate(cx, enum_op, statep, idp);
}

static JSType
with_TypeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_OBJECT;
}

static JSObject *
with_ThisObject(JSContext *cx, JSObject *obj)
{
    return obj->getWithThis();
}

Class js_WithClass = {
    "With",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_IS_ANONYMOUS,
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub,
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    JS_NULL_CLASS_EXT,
    {
        with_LookupProperty,
        NULL,       
        with_GetProperty,
        with_SetProperty,
        with_GetAttributes,
        with_SetAttributes,
        with_DeleteProperty,
        with_Enumerate,
        with_TypeOf,
        NULL,       
        with_ThisObject,
        NULL,       
    }
};

JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth)
{
    JSObject *obj;

    obj = js_NewGCObject(cx);
    if (!obj)
        return NULL;

    obj->init(&js_WithClass, proto, parent, js_FloatingFrameIfGenerator(cx, cx->fp()), cx);
    obj->setMap(cx->runtime->emptyWithShape);
    OBJ_SET_BLOCK_DEPTH(cx, obj, depth);

    AutoObjectRooter tvr(cx, obj);
    JSObject *thisp = proto->thisObject(cx);
    if (!thisp)
        return NULL;

    obj->setWithThis(thisp);
    return obj;
}

JSObject *
js_NewBlockObject(JSContext *cx)
{
    



    JSObject *blockObj = js_NewGCObject(cx);
    if (!blockObj)
        return NULL;

    blockObj->init(&js_BlockClass, NULL, NULL, NullValue(), cx);
    blockObj->setMap(cx->runtime->emptyBlockShape);
    return blockObj;
}

JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp)
{
    JS_ASSERT(proto->isStaticBlock());

    JSObject *clone = js_NewGCObject(cx);
    if (!clone)
        return NULL;

    JSStackFrame *priv = js_FloatingFrameIfGenerator(cx, fp);

    
    clone->init(&js_BlockClass, proto, NULL, priv, cx);
    clone->fslots[JSSLOT_BLOCK_DEPTH] = proto->fslots[JSSLOT_BLOCK_DEPTH];

    clone->setMap(proto->map);
    if (!clone->ensureInstanceReservedSlots(cx, OBJ_BLOCK_COUNT(cx, proto)))
        return NULL;

    JS_ASSERT(clone->isClonedBlock());
    return clone;
}

JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind)
{
    
    JS_STATIC_ASSERT(JS_INITIAL_NSLOTS == JSSLOT_BLOCK_DEPTH + 2);

    JSStackFrame *const fp = cx->fp();
    JSObject *obj = &fp->scopeChain();
    JS_ASSERT(obj->isClonedBlock());
    JS_ASSERT(obj->getPrivate() == js_FloatingFrameIfGenerator(cx, cx->fp()));

    
    uintN count = OBJ_BLOCK_COUNT(cx, obj);
    JS_ASSERT(obj->numSlots() == JSSLOT_BLOCK_DEPTH + 1 + count);

    
    uintN depth = OBJ_BLOCK_DEPTH(cx, obj);
    JS_ASSERT(depth <= size_t(cx->regs->sp - fp->base()));
    JS_ASSERT(count <= size_t(cx->regs->sp - fp->base() - depth));

    
    JS_ASSERT(count >= 1);

    if (normalUnwind) {
        uintN slot = JSSLOT_BLOCK_DEPTH + 1;
        uintN flen = JS_MIN(count, JS_INITIAL_NSLOTS - slot);
        uintN stop = slot + flen;

        depth += fp->numFixed();
        while (slot < stop)
            obj->fslots[slot++] = fp->slots()[depth++];
        count -= flen;
        if (count != 0)
            memcpy(obj->dslots, fp->slots() + depth, count * sizeof(Value));
    }

    
    obj->setPrivate(NULL);
    fp->setScopeChainNoCallObj(*obj->getParent());
    return normalUnwind;
}

static JSBool
block_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    




    JS_ASSERT(obj->isClonedBlock());
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->numFixed() + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->numSlots());
        *vp = fp->slots()[index];
        return true;
    }

    
    JS_ASSERT(obj->getSlot(JSSLOT_FREE(&js_BlockClass) + index) == *vp);
    return true;
}

static JSBool
block_setProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(obj->isClonedBlock());
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->numFixed() + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->numSlots());
        fp->slots()[index] = *vp;
        return true;
    }

    



    return true;
}

const Shape *
JSObject::defineBlockVariable(JSContext *cx, jsid id, intN index)
{
    JS_ASSERT(isStaticBlock());

    
    uint32 slot = JSSLOT_FREE(&js_BlockClass) + index;
    const Shape *shape = addProperty(cx, id,
                                     block_getProperty, block_setProperty,
                                     slot, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                     Shape::HAS_SHORTID, index);
    if (!shape)
        return NULL;
    if (slot >= numSlots() && !growSlots(cx, slot + 1))
        return NULL;
    return shape;
}

static size_t
GetObjectSize(JSObject *obj)
{
    return (obj->isFunction() && !obj->getPrivate())
           ? sizeof(JSFunction)
           : sizeof(JSObject);
}







void
JSObject::swap(JSObject *other)
{
    size_t size = GetObjectSize(this);
    JS_ASSERT(size == GetObjectSize(other));

    
    char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject)>::result];
    memcpy(tmp, this, size);
    memcpy(this, other, size);
    memcpy(other, tmp, size);
}

#if JS_HAS_XDR

#define NO_PARENT_INDEX ((uint32)-1)

uint32
FindObjectIndex(JSObjectArray *array, JSObject *obj)
{
    size_t i;

    if (array) {
        i = array->length;
        do {

            if (array->vector[--i] == obj)
                return i;
        } while (i != 0);
    }

    return NO_PARENT_INDEX;
}

JSBool
js_XDRBlockObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    uint32 parentId;
    JSObject *obj, *parent;
    uintN depth, count;
    uint32 depthAndCount;
    const Shape *shape;

    cx = xdr->cx;
#ifdef __GNUC__
    obj = NULL;         
#endif

    if (xdr->mode == JSXDR_ENCODE) {
        obj = *objp;
        parent = obj->getParent();
        parentId = (xdr->script->objectsOffset == 0)
                   ? NO_PARENT_INDEX
                   : FindObjectIndex(xdr->script->objects(), parent);
        depth = (uint16)OBJ_BLOCK_DEPTH(cx, obj);
        count = (uint16)OBJ_BLOCK_COUNT(cx, obj);
        depthAndCount = (uint32)(depth << 16) | count;
    }
#ifdef __GNUC__ 
    else count = 0;
#endif

    
    if (!JS_XDRUint32(xdr, &parentId))
        return JS_FALSE;

    if (xdr->mode == JSXDR_DECODE) {
        obj = js_NewBlockObject(cx);
        if (!obj)
            return JS_FALSE;
        *objp = obj;

        




        if (parentId == NO_PARENT_INDEX)
            parent = NULL;
        else
            parent = xdr->script->getObject(parentId);
        obj->setParent(parent);
    }

    AutoObjectRooter tvr(cx, obj);

    if (!JS_XDRUint32(xdr, &depthAndCount))
        return false;

    if (xdr->mode == JSXDR_DECODE) {
        depth = (uint16)(depthAndCount >> 16);
        count = (uint16)depthAndCount;
        obj->setSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Value(depth)));

        



        for (uintN i = 0; i < count; i++) {
            JSAtom *atom;
            uint16 shortid;

            
            if (!js_XDRAtom(xdr, &atom) || !JS_XDRUint16(xdr, &shortid))
                return false;

            if (!obj->defineBlockVariable(cx, ATOM_TO_JSID(atom), shortid))
                return false;
        }
    } else {
        Vector<const Shape *, 8> shapes(cx);
        shapes.growByUninitialized(count);

        for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
            shape = &r.front();
            shapes[shape->shortid] = shape;
        }

        



        for (uintN i = 0; i < count; i++) {
            shape = shapes[i];
            JS_ASSERT(shape->getter() == block_getProperty);

            jsid propid = shape->id;
            JS_ASSERT(JSID_IS_ATOM(propid));
            JSAtom *atom = JSID_TO_ATOM(propid);

            uint16 shortid = uint16(shape->shortid);
            JS_ASSERT(shortid == i);

            
            if (!js_XDRAtom(xdr, &atom) || !JS_XDRUint16(xdr, &shortid))
                return false;
        }
    }
    return true;
}

#endif

Class js_BlockClass = {
    "Block",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_IS_ANONYMOUS,
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    PropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub
};

JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = js_InitClass(cx, obj, NULL, &js_ObjectClass, js_Object, 1,
                                   object_props, object_methods, NULL, object_static_methods);
    if (!proto)
        return NULL;

    
    if (!js_DefineFunction(cx, obj, cx->runtime->atomState.evalAtom, obj_eval, 1,
                           JSFUN_STUB_GSOPS)) {
        return NULL;
    }

    return proto;
}

static bool
DefineStandardSlot(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                   const Value &v, uint32 attrs, bool &named)
{
    jsid id = ATOM_TO_JSID(atom);

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->getClass()->flags & JSCLASS_IS_GLOBAL);
        JS_ASSERT(obj->isNative());

        JS_LOCK_OBJ(cx, obj);
        if (!obj->ensureClassReservedSlots(cx)) {
            JS_UNLOCK_OBJ(cx, obj);
            return false;
        }

        const Shape *shape = obj->nativeLookup(id);
        if (!shape) {
            uint32 index = 2 * JSProto_LIMIT + key;
            if (!js_SetReservedSlot(cx, obj, index, v)) {
                JS_UNLOCK_OBJ(cx, obj);
                return false;
            }

            uint32 slot = JSSLOT_START(obj->getClass()) + index;
            shape = obj->addProperty(cx, id, PropertyStub, PropertyStub, slot, attrs, 0, 0);

            JS_UNLOCK_OBJ(cx, obj);
            if (!shape)
                return false;

            named = true;
            return true;
        }
        JS_UNLOCK_OBJ(cx, obj);
    }

    named = obj->defineProperty(cx, id, v, PropertyStub, PropertyStub, attrs);
    return named;
}

JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             Class *clasp, Native constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
    JSAtom *atom;
    JSProtoKey key;
    JSFunction *fun;
    bool named = false;

    atom = js_Atomize(cx, clasp->name, strlen(clasp->name), 0);
    if (!atom)
        return NULL;

    













    key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !parent_proto &&
        !js_GetClassPrototype(cx, obj, JSProto_Object, &parent_proto)) {
        return NULL;
    }

    




















    JSObject *proto = NewObject<WithProto::Class>(cx, clasp, parent_proto, obj);
    if (!proto)
        return NULL;

    
    AutoObjectRooter tvr(cx, proto);

    JSObject *ctor;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) ||
            !(obj->getClass()->flags & JSCLASS_IS_GLOBAL) ||
            key == JSProto_Null)
        {
            uint32 attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            if (!DefineStandardSlot(cx, obj, key, atom, ObjectValue(*proto), attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        fun = js_NewFunction(cx, NULL, constructor, nargs, JSFUN_CONSTRUCTOR, obj, atom);
        if (!fun)
            goto bad;

        AutoValueRooter tvr2(cx, ObjectValue(*fun));
        if (!DefineStandardSlot(cx, obj, key, atom, tvr2.value(), 0, named))
            goto bad;

        




        FUN_CLASP(fun) = clasp;

        





        ctor = FUN_OBJECT(fun);
        if (clasp->flags & JSCLASS_CONSTRUCT_PROTOTYPE) {
            Value rval;
            if (!InvokeConstructorWithGivenThis(cx, proto, ObjectOrNullValue(ctor),
                                                0, NULL, &rval)) {
                goto bad;
            }
            if (rval.isObject() && &rval.toObject() != proto)
                proto = &rval.toObject();
        }

        
        if (!js_SetClassPrototype(cx, ctor, proto,
                                  JSPROP_READONLY | JSPROP_PERMANENT)) {
            goto bad;
        }

        
        if (ctor->getClass() == clasp)
            ctor->setProto(proto);
    }

    
    if ((ps && !JS_DefineProperties(cx, proto, ps)) ||
        (fs && !JS_DefineFunctions(cx, proto, fs)) ||
        (static_ps && !JS_DefineProperties(cx, ctor, static_ps)) ||
        (static_fs && !JS_DefineFunctions(cx, ctor, static_fs))) {
        goto bad;
    }

    










    JS_ASSERT_IF(proto->clasp != clasp,
                 clasp == &js_ArrayClass && proto->clasp == &js_SlowArrayClass);
    if (!proto->getEmptyShape(cx, proto->clasp))
        goto bad;

    
    if (key != JSProto_Null && !js_SetClassObject(cx, obj, key, ctor, proto))
        goto bad;

    return proto;

bad:
    if (named) {
        Value rval;
        obj->deleteProperty(cx, ATOM_TO_JSID(atom), &rval, false);
    }
    return NULL;
}

bool
JSObject::allocSlots(JSContext *cx, size_t nslots)
{
    JS_ASSERT(!dslots);
    JS_ASSERT(nslots > JS_INITIAL_NSLOTS);

    size_t nwords = slotsToDynamicWords(nslots);
    dslots = (Value*) cx->malloc(nwords * sizeof(Value));
    if (!dslots)
        return false;

    dslots++;
    dslots[-1].setPrivateUint32(nslots);
    SetValueRangeToUndefined(dslots, nslots - JS_INITIAL_NSLOTS);
    return true;
}

bool
JSObject::growSlots(JSContext *cx, size_t nslots)
{
    


    const size_t MIN_DYNAMIC_WORDS = 4;

    



    const size_t LINEAR_GROWTH_STEP = JS_BIT(16);

    
    if (nslots <= JS_INITIAL_NSLOTS)
        return true;

    
    if (nslots >= NSLOTS_LIMIT) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    size_t nwords = slotsToDynamicWords(nslots);

    



    uintN log;
    if (nwords <= MIN_DYNAMIC_WORDS) {
        nwords = MIN_DYNAMIC_WORDS;
    } else if (nwords < LINEAR_GROWTH_STEP) {
        JS_CEILING_LOG2(log, nwords);
        nwords = JS_BIT(log);
    } else {
        nwords = JS_ROUNDUP(nwords, LINEAR_GROWTH_STEP);
    }
    nslots = dynamicWordsToSlots(nwords);

    



    if (!dslots)
        return allocSlots(cx, nslots);

    size_t oldnslots = dslots[-1].toPrivateUint32();

    Value *tmpdslots = (Value*) cx->realloc(dslots - 1, nwords * sizeof(Value));
    if (!tmpdslots)
        return false;   

    dslots = tmpdslots;
    dslots++;
    dslots[-1].setPrivateUint32(nslots);

    
    JS_ASSERT(nslots > oldnslots);
    Value *beg = dslots + (oldnslots - JS_INITIAL_NSLOTS);
    Value *end = dslots + (nslots - JS_INITIAL_NSLOTS);
    SetValueRangeToUndefined(beg, end);

    return true;
}

void
JSObject::shrinkSlots(JSContext *cx, size_t nslots)
{
    
    if (!dslots)
        return;

    JS_ASSERT(dslots[-1].toPrivateUint32() > JS_INITIAL_NSLOTS);
    JS_ASSERT(nslots <= dslots[-1].toPrivateUint32());

    if (nslots <= JS_INITIAL_NSLOTS) {
        freeSlotsArray(cx);
        dslots = NULL;
    } else {
        size_t nwords = slotsToDynamicWords(nslots);
        Value *tmpdslots = (Value*) cx->realloc(dslots - 1, nwords * sizeof(Value));
        if (!tmpdslots)
            return;     

        dslots = tmpdslots;
        dslots++;
        dslots[-1].setPrivateUint32(nslots);
    }
}

bool
JSObject::ensureInstanceReservedSlots(JSContext *cx, size_t nreserved)
{
    JS_ASSERT_IF(isNative(),
                 isBlock() || isCall() || (isFunction() && getFunctionPrivate()->isBound()));

    uintN nslots = JSSLOT_FREE(clasp) + nreserved;
    return nslots <= numSlots() || allocSlots(cx, nslots);
}

static JSObject *
js_InitNullClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(0);
    return NULL;
}

#define JS_PROTO(name,code,init) extern JSObject *init(JSContext *, JSObject *);
#include "jsproto.tbl"
#undef JS_PROTO

static JSObjectOp lazy_prototype_init[JSProto_LIMIT] = {
#define JS_PROTO(name,code,init) init,
#include "jsproto.tbl"
#undef JS_PROTO
};

namespace js {

bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles)
{
    JS_ASSERT_IF(!checkForCycles, obj != proto);

    if (obj->isNative()) {
        JS_LOCK_OBJ(cx, obj);
        bool ok = obj->ensureClassReservedSlots(cx);
        JS_UNLOCK_OBJ(cx, obj);
        if (!ok)
            return false;
    }

    




    JSObject *oldproto = obj;
    while (oldproto && oldproto->isNative()) {
        JS_LOCK_OBJ(cx, oldproto);
        oldproto->protoShapeChange(cx);
        JSObject *tmp = oldproto->getProto();
        JS_UNLOCK_OBJ(cx, oldproto);
        oldproto = tmp;
    }

    if (!proto || !checkForCycles) {
        obj->setProto(proto);
    } else if (!SetProtoCheckingForCycles(cx, obj, proto)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE, js_proto_str);
        return false;
    }
    return true;
}

}

JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp)
{
    JSObject *tmp, *cobj;
    JSResolvingKey rkey;
    JSResolvingEntry *rentry;
    uint32 generation;
    JSObjectOp init;
    Value v;

    while ((tmp = obj->getParent()) != NULL)
        obj = tmp;
    if (!(obj->getClass()->flags & JSCLASS_IS_GLOBAL)) {
        *objp = NULL;
        return JS_TRUE;
    }

    v = obj->getReservedSlot(key);
    if (v.isObject()) {
        *objp = &v.toObject();
        return JS_TRUE;
    }

    rkey.obj = obj;
    rkey.id = ATOM_TO_JSID(cx->runtime->atomState.classAtoms[key]);
    if (!js_StartResolving(cx, &rkey, JSRESFLAG_LOOKUP, &rentry))
        return JS_FALSE;
    if (!rentry) {
        
        *objp = NULL;
        return JS_TRUE;
    }
    generation = cx->resolvingTable->generation;

    JSBool ok = true;
    cobj = NULL;
    init = lazy_prototype_init[key];
    if (init) {
        if (!init(cx, obj)) {
            ok = JS_FALSE;
        } else {
            v = obj->getReservedSlot(key);
            if (v.isObject())
                cobj = &v.toObject();
        }
    }

    js_StopResolving(cx, &rkey, JSRESFLAG_LOOKUP, rentry, generation);
    *objp = cobj;
    return ok;
}

JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!(obj->getClass()->flags & JSCLASS_IS_GLOBAL))
        return JS_TRUE;

    return js_SetReservedSlot(cx, obj, key, ObjectOrNullValue(cobj)) &&
           js_SetReservedSlot(cx, obj, JSProto_LIMIT + key, ObjectOrNullValue(proto));
}

JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey protoKey,
                   Value *vp, Class *clasp)
{
    JSStackFrame *fp;
    JSObject *obj, *cobj, *pobj;
    jsid id;
    JSProperty *prop;
    const Shape *shape;

    




    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (!start && (fp = cx->maybefp()) != NULL)
        start = &fp->scopeChain();

    if (start) {
        
        do {
            obj = start;
            start = obj->getParent();
        } while (start);
    } else {
        obj = cx->globalObject;
        if (!obj) {
            vp->setUndefined();
            return JS_TRUE;
        }
    }

    OBJ_TO_INNER_OBJECT(cx, obj);
    if (!obj)
        return JS_FALSE;

    if (protoKey != JSProto_Null) {
        JS_ASSERT(JSProto_Null < protoKey);
        JS_ASSERT(protoKey < JSProto_LIMIT);
        if (!js_GetClassObject(cx, obj, protoKey, &cobj))
            return JS_FALSE;
        if (cobj) {
            vp->setObject(*cobj);
            return JS_TRUE;
        }
        id = ATOM_TO_JSID(cx->runtime->atomState.classAtoms[protoKey]);
    } else {
        JSAtom *atom = js_Atomize(cx, clasp->name, strlen(clasp->name), 0);
        if (!atom)
            return false;
        id = ATOM_TO_JSID(atom);
    }

    JS_ASSERT(obj->isNative());
    if (js_LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_CLASSNAME,
                                   &pobj, &prop) < 0) {
        return JS_FALSE;
    }
    Value v = UndefinedValue();
    if (prop && pobj->isNative()) {
        shape = (Shape *) prop;
        if (pobj->containsSlot(shape->slot)) {
            v = pobj->lockedGetSlot(shape->slot);
            if (v.isPrimitive())
                v.setUndefined();
        }
        JS_UNLOCK_OBJ(cx, pobj);
    }
    *vp = v;
    return JS_TRUE;
}

JSObject *
js_ConstructObject(JSContext *cx, Class *clasp, JSObject *proto, JSObject *parent,
                   uintN argc, Value *argv)
{
    AutoArrayRooter argtvr(cx, argc, argv);

    JSProtoKey protoKey = GetClassProtoKey(clasp);

    
    AutoValueRooter tvr(cx);
    if (!js_FindClassObject(cx, parent, protoKey, tvr.addr(), clasp))
        return NULL;

    const Value &cval = tvr.value();
    if (tvr.value().isPrimitive()) {
        js_ReportIsNotFunction(cx, tvr.addr(), JSV2F_CONSTRUCT | JSV2F_SEARCH_STACK);
        return NULL;
    }

    



    JSObject *ctor = &cval.toObject();
    if (!parent)
        parent = ctor->getParent();
    if (!proto) {
        Value rval;
        if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                               &rval)) {
            return NULL;
        }
        if (rval.isObjectOrNull())
            proto = rval.toObjectOrNull();
    }

    JSObject *obj = NewObject<WithProto::Class>(cx, clasp, proto, parent);
    if (!obj)
        return NULL;

    Value rval;
    if (!InvokeConstructorWithGivenThis(cx, obj, cval, argc, argv, &rval))
        return NULL;

    if (rval.isPrimitive())
        return obj;

    






    obj = &rval.toObject();
    if (obj->getClass() != clasp ||
        (!(~clasp->flags & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_CONSTRUCT_PROTOTYPE)) &&
         !obj->getPrivate())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_WRONG_CONSTRUCTOR, clasp->name);
        return NULL;
    }
    return obj;
}

bool
JSObject::allocSlot(JSContext *cx, uint32 *slotp)
{
    uint32 slot = slotSpan();
    JS_ASSERT(slot >= JSSLOT_FREE(clasp));

    



    if (inDictionaryMode() && lastProp->table) {
        uint32 &last = lastProp->table->freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            JS_ASSERT(last < slot);
            uint32 next = getSlot(last).toPrivateUint32();
            JS_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            Value &vref = getSlotRef(last);
            last = vref.toPrivateUint32();
            vref.setUndefined();
            return true;
        }
    }

    if (slot >= numSlots() && !growSlots(cx, slot + 1))
        return false;

    
    JS_ASSERT(getSlot(slot).isUndefined());
    *slotp = slot;
    return true;
}

void
JSObject::freeSlot(JSContext *cx, uint32 slot)
{
    uint32 limit = slotSpan();
    JS_ASSERT(slot < limit);

    Value &vref = getSlotRef(slot);
    if (inDictionaryMode() && lastProp->table) {
        uint32 &last = lastProp->table->freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < limit && last != slot);

        





        if (slot + 1 < limit) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            vref.setPrivateUint32(last);
            last = slot;
            return;
        }
    }
    vref.setUndefined();
}


#define JSBOXEDWORD_INT_MAX_STRING "1073741823"







jsid
js_CheckForStringIndex(jsid id)
{
    if (!JSID_IS_ATOM(id))
        return id;

    JSAtom *atom = JSID_TO_ATOM(id);
    JSString *str = ATOM_TO_STRING(atom);
    const jschar *s = str->flatChars();
    jschar ch = *s;

    JSBool negative = (ch == '-');
    if (negative)
        ch = *++s;

    if (!JS7_ISDEC(ch))
        return id;

    size_t n = str->flatLength() - negative;
    if (n > sizeof(JSBOXEDWORD_INT_MAX_STRING) - 1)
        return id;

    const jschar *cp = s;
    const jschar *end = s + n;

    jsuint index = JS7_UNDEC(*cp++);
    jsuint oldIndex = 0;
    jsuint c = 0;

    if (index != 0) {
        while (JS7_ISDEC(*cp)) {
            oldIndex = index;
            c = JS7_UNDEC(*cp);
            index = 10 * index + c;
            cp++;
        }
    }

    



    if (cp != end || (negative && index == 0))
        return id;

    if (oldIndex < JSID_INT_MAX / 10 ||
        (oldIndex == JSID_INT_MAX / 10 && c <= (JSID_INT_MAX % 10))) {
        if (negative)
            index = 0 - index;
        id = INT_TO_JSID((jsint)index);
    }

    return id;
}

static JSBool
PurgeProtoChain(JSContext *cx, JSObject *obj, jsid id)
{
    const Shape *shape;

    while (obj) {
        if (!obj->isNative()) {
            obj = obj->getProto();
            continue;
        }
        JS_LOCK_OBJ(cx, obj);
        shape = obj->nativeLookup(id);
        if (shape) {
            PCMETER(JS_PROPERTY_CACHE(cx).pcpurges++);
            obj->shadowingShapeChange(cx, *shape);
            JS_UNLOCK_OBJ(cx, obj);

            if (!obj->getParent()) {
                




                LeaveTrace(cx);
            }
            return JS_TRUE;
        }
#ifdef JS_THREADSAFE
        JSObject *pobj = obj;
#endif
        obj = obj->getProto();
        JS_UNLOCK_OBJ(cx, pobj);
    }
    return JS_FALSE;
}

void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id)
{
    JS_ASSERT(obj->isDelegate());
    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->isCall()) {
        while ((obj = obj->getParent()) != NULL) {
            if (PurgeProtoChain(cx, obj, id))
                break;
        }
    }
}

const Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     PropertyOp getter, PropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid)
{
    const Shape *shape;

    JS_ASSERT(!(flags & Shape::METHOD));

    




    js_PurgeScopeChain(cx, obj, id);

    JS_LOCK_OBJ(cx, obj);
    if (!obj->ensureClassReservedSlots(cx)) {
        shape = NULL;
    } else {
        
        id = js_CheckForStringIndex(id);
        shape = obj->putProperty(cx, id, getter, setter, slot, attrs, flags, shortid);
    }
    JS_UNLOCK_OBJ(cx, obj);
    return shape;
}

const Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const Shape *shape, uintN attrs, uintN mask,
                             PropertyOp getter, PropertyOp setter)
{
    JS_LOCK_OBJ(cx, obj);
    shape = obj->ensureClassReservedSlots(cx)
            ? obj->changeProperty(cx, shape, attrs, mask, getter, setter)
            : NULL;
    JS_UNLOCK_OBJ(cx, obj);
    return shape;
}

JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                  PropertyOp getter, PropertyOp setter, uintN attrs)
{
    return js_DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs,
                                   0, 0, NULL);
}







static inline bool
CallAddPropertyHook(JSContext *cx, Class *clasp, JSObject *obj, const Shape *shape, Value *vp)
{
    if (clasp->addProperty != PropertyStub) {
        Value nominal = *vp;

        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, SHAPE_USERID(shape), vp))
            return false;
        if (*vp != nominal) {
            if (obj->containsSlot(shape->slot))
                obj->lockedSetSlot(shape->slot, *vp);
        }
    }
    return true;
}

JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const Value &value,
                        PropertyOp getter, PropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow )
{
    Class *clasp;
    const Shape *shape;
    JSBool added;
    Value valueCopy;

    JS_ASSERT((defineHow & ~(JSDNP_CACHE_RESULT | JSDNP_DONT_PURGE | JSDNP_SET_METHOD)) == 0);
    LeaveTraceIfGlobalObject(cx, obj);

    
    id = js_CheckForStringIndex(id);

    




    shape = NULL;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        







        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return JS_FALSE;
        shape = (Shape *) prop;
        if (shape && pobj == obj && shape->isAccessorDescriptor()) {
            shape = obj->changeProperty(cx, shape, attrs,
                                        JSPROP_GETTER | JSPROP_SETTER,
                                        (attrs & JSPROP_GETTER)
                                        ? getter
                                        : shape->getter(),
                                        (attrs & JSPROP_SETTER)
                                        ? setter
                                        : shape->setter());

            
            if (!shape)
                goto error;
        } else if (prop) {
            pobj->dropProperty(cx, prop);
            prop = NULL;
            shape = NULL;
        }
    }

    




    if (!(defineHow & JSDNP_DONT_PURGE))
        js_PurgeScopeChain(cx, obj, id);

    




    if (obj->isDelegate() && (attrs & (JSPROP_READONLY | JSPROP_SETTER)))
        cx->runtime->protoHazardShape = js_GenerateShape(cx, false);

    
    JS_LOCK_OBJ(cx, obj);

    
    clasp = obj->getClass();
    if (!(defineHow & JSDNP_SET_METHOD)) {
        if (!getter && !(attrs & JSPROP_GETTER))
            getter = clasp->getProperty;
        if (!setter && !(attrs & JSPROP_SETTER))
            setter = clasp->setProperty;
    }

    
    if (!obj->ensureClassReservedSlots(cx))
        goto error;

    added = false;
    if (!shape) {
        
        if (defineHow & JSDNP_SET_METHOD) {
            JS_ASSERT(clasp == &js_ObjectClass);
            JS_ASSERT(IsFunctionObject(value));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            JS_ASSERT(!getter && !setter);

            JSObject *funobj = &value.toObject();
            if (FUN_OBJECT(GET_FUNCTION_PRIVATE(cx, funobj)) == funobj) {
                flags |= Shape::METHOD;
                getter = CastAsPropertyOp(funobj);
            }
        }

        added = !obj->nativeContains(id);
        uint32 oldShape = obj->shape();
        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            goto error;

        





        if (obj->shape() == oldShape && obj->branded() && shape->slot != SHAPE_INVALID_SLOT)
            obj->methodWriteBarrier(cx, shape->slot, value);
    }

    
    if (obj->containsSlot(shape->slot))
        obj->lockedSetSlot(shape->slot, value);

    
    valueCopy = value;
    if (!CallAddPropertyHook(cx, clasp, obj, shape, &valueCopy)) {
        obj->removeProperty(cx, id);
        goto error;
    }

    if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
        JS_ASSERT_NOT_ON_TRACE(cx);
        PropertyCacheEntry *entry =
#endif
            JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, 0, obj, shape, added);
        TRACE_2(SetPropHit, entry, shape);
    }
    if (propp)
        *propp = (JSProperty *) shape;
    else
        JS_UNLOCK_OBJ(cx, obj);
    return JS_TRUE;

error: 
    JS_UNLOCK_OBJ(cx, obj);
    return JS_FALSE;
}

JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp)
{
    return js_LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags,
                                      objp, propp) >= 0;
}

#define SCOPE_DEPTH_ACCUM(bs,val)                                             \
    JS_SCOPE_DEPTH_METERING(JS_BASIC_STATS_ACCUM(bs, val))























static JSBool
CallResolveOp(JSContext *cx, JSObject *start, JSObject *obj, jsid id, uintN flags,
              JSObject **objp, JSProperty **propp, bool *recursedp)
{
    Class *clasp = obj->getClass();
    JSResolveOp resolve = clasp->resolve;

    







    JSResolvingKey key = {obj, id};
    JSResolvingEntry *entry;
    if (!js_StartResolving(cx, &key, JSRESFLAG_LOOKUP, &entry)) {
        JS_UNLOCK_OBJ(cx, obj);
        return false;
    }
    if (!entry) {
        
        JS_UNLOCK_OBJ(cx, obj);
        *recursedp = true;
        return true;
    }
    uint32 generation = cx->resolvingTable->generation;
    *recursedp = false;

    *propp = NULL;

    JSBool ok;
    const Shape *shape = NULL;
    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = (JSNewResolveOp)resolve;
        if (flags == JSRESOLVE_INFER)
            flags = js_InferFlags(cx, 0);
        JSObject *obj2 = (clasp->flags & JSCLASS_NEW_RESOLVE_GETS_START) ? start : NULL;
        JS_UNLOCK_OBJ(cx, obj);

        {
            
            AutoKeepAtoms keep(cx->runtime);
            ok = newresolve(cx, obj, id, flags, &obj2);
        }
        if (!ok)
            goto cleanup;

        JS_LOCK_OBJ(cx, obj);
        if (obj2) {
            
            if (obj2 != obj) {
                JS_UNLOCK_OBJ(cx, obj);
                if (obj2->isNative())
                    JS_LOCK_OBJ(cx, obj2);
            }
            if (!obj2->isNative()) {
                
                JS_ASSERT(obj2 != obj);
                ok = obj2->lookupProperty(cx, id, objp, propp);
                if (!ok || *propp)
                    goto cleanup;
                JS_LOCK_OBJ(cx, obj2);
            } else {
                






                if (!obj2->nativeEmpty())
                    shape = obj2->nativeLookup(id);
            }
            if (shape) {
                JS_ASSERT(!obj2->nativeEmpty());
                obj = obj2;
            } else if (obj2 != obj) {
                if (obj2->isNative())
                    JS_UNLOCK_OBJ(cx, obj2);
                JS_LOCK_OBJ(cx, obj);
            }
        }
    } else {
        



        JS_UNLOCK_OBJ(cx, obj);
        ok = resolve(cx, obj, id);
        if (!ok)
            goto cleanup;
        JS_LOCK_OBJ(cx, obj);
        JS_ASSERT(obj->isNative());
        if (!obj->nativeEmpty())
            shape = obj->nativeLookup(id);
    }

cleanup:
    if (ok && shape) {
        *objp = obj;
        *propp = (JSProperty *) shape;
    }
    js_StopResolving(cx, &key, JSRESFLAG_LOOKUP, entry, generation);
    return ok;
}

int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp)
{
    
    id = js_CheckForStringIndex(id);

    
    JSObject *start = obj;
    int protoIndex;
    for (protoIndex = 0; ; protoIndex++) {
        JS_LOCK_OBJ(cx, obj);
        const Shape *shape = obj->nativeLookup(id);
        if (shape) {
            SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
            *objp = obj;
            *propp = (JSProperty *) shape;
            return protoIndex;
        }

        
        if (!shape && obj->getClass()->resolve != JS_ResolveStub) {
            bool recursed;
            if (!CallResolveOp(cx, start, obj, id, flags, objp, propp, &recursed))
                return -1;
            if (recursed)
                break;
            if (*propp) {
                
                protoIndex = 0;
                for (JSObject *proto = start; proto && proto != *objp; proto = proto->getProto())
                    protoIndex++;
                SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
                return protoIndex;
            }
        }

        JSObject *proto = obj->getProto();
        JS_UNLOCK_OBJ(cx, obj);
        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!proto->lookupProperty(cx, id, objp, propp))
                return -1;
            return protoIndex + 1;
        }

        obj = proto;
    }

    *objp = NULL;
    *propp = NULL;
    return protoIndex;
}

PropertyCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    JSObject *scopeChain, *obj, *parent, *pobj;
    PropertyCacheEntry *entry;
    int scopeIndex, protoIndex;
    JSProperty *prop;

    JS_ASSERT_IF(cacheResult, !JS_ON_TRACE(cx));
    scopeChain = &js_GetTopStackFrame(cx)->scopeChain();

    
    entry = JS_NO_PROP_CACHE_FILL;
    obj = scopeChain;
    parent = obj->getParent();
    for (scopeIndex = 0;
         parent
         ? js_IsCacheableNonGlobalScope(obj)
         : !obj->getOps()->lookupProperty;
         ++scopeIndex) {
        protoIndex =
            js_LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags,
                                       &pobj, &prop);
        if (protoIndex < 0)
            return NULL;

        if (prop) {
#ifdef DEBUG
            if (parent) {
                Class *clasp = obj->getClass();
                JS_ASSERT(pobj->isNative());
                JS_ASSERT(pobj->getClass() == clasp);
                if (clasp == &js_BlockClass) {
                    



                    JS_ASSERT(pobj == obj);
                    JS_ASSERT(pobj->isClonedBlock());
                    JS_ASSERT(protoIndex == 0);
                } else {
                    
                    JS_ASSERT(!obj->getProto());
                    JS_ASSERT(protoIndex == 0);
                }
            } else {
                JS_ASSERT(obj->isNative());
            }
#endif
            



            if (cacheResult && pobj->isNative()) {
                entry = JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex,
                                                   protoIndex, pobj,
                                                   (Shape *) prop);
            }
            SCOPE_DEPTH_ACCUM(&cx->runtime->scopeSearchDepthStats, scopeIndex);
            goto out;
        }

        if (!parent) {
            pobj = NULL;
            goto out;
        }
        obj = parent;
        parent = obj->getParent();
    }

    for (;;) {
        if (!obj->lookupProperty(cx, id, &pobj, &prop))
            return NULL;
        if (prop) {
            PCMETER(JS_PROPERTY_CACHE(cx).nofills++);
            goto out;
        }

        



        parent = obj->getParent();
        if (!parent) {
            pobj = NULL;
            break;
        }
        obj = parent;
    }

  out:
    JS_ASSERT(!!pobj == !!prop);
    *objp = obj;
    *pobjp = pobj;
    *propp = prop;
    return entry;
}

JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, JSObject **objp, JSObject **pobjp,
                JSProperty **propp)
{
    return !!js_FindPropertyHelper(cx, id, false, objp, pobjp, propp);
}

JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id)
{
    



    JS_ASSERT(scopeChain->getParent());
    JS_ASSERT(!JS_ON_TRACE(cx));

    JSObject *obj = scopeChain;

    








    for (int scopeIndex = 0;
         !obj->getParent() || js_IsCacheableNonGlobalScope(obj);
         scopeIndex++) {
        JSObject *pobj;
        JSProperty *prop;
        int protoIndex = js_LookupPropertyWithFlags(cx, obj, id,
                                                    cx->resolveFlags,
                                                    &pobj, &prop);
        if (protoIndex < 0)
            return NULL;
        if (prop) {
            if (!pobj->isNative()) {
                JS_ASSERT(!obj->getParent());
                return obj;
            }
            JS_ASSERT_IF(obj->getParent(), pobj->getClass() == obj->getClass());
#ifdef DEBUG
            PropertyCacheEntry *entry =
#endif
                JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, protoIndex, pobj,
                                           (Shape *) prop);
            JS_ASSERT(entry);
            JS_UNLOCK_OBJ(cx, pobj);
            return obj;
        }

        JSObject *parent = obj->getParent();
        if (!parent)
            return obj;
        obj = parent;
    }

    
    do {
        JSObject *pobj;
        JSProperty *prop;
        if (!obj->lookupProperty(cx, id, &pobj, &prop))
            return NULL;
        if (prop) {
            pobj->dropProperty(cx, prop);
            break;
        }

        




        JSObject *parent = obj->getParent();
        if (!parent)
            break;
        obj = parent;
    } while (obj->getParent());
    return obj;
}

JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const Shape *shape, uintN getHow,
             Value *vp)
{
    LeaveTraceIfGlobalObject(cx, pobj);

    uint32 slot;
    int32 sample;

    JS_ASSERT(pobj->isNative());
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, pobj));

    slot = shape->slot;
    if (slot != SHAPE_INVALID_SLOT)
        *vp = pobj->lockedGetSlot(slot);
    else
        vp->setUndefined();
    if (shape->hasDefaultGetter())
        return true;

    if (JS_UNLIKELY(shape->isMethod()) && (getHow & JSGET_NO_METHOD_BARRIER)) {
        JS_ASSERT(&shape->methodObject() == &vp->toObject());
        return true;
    }

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_OBJ(cx, pobj);
    {
        AutoShapeRooter tvr(cx, shape);
        AutoObjectRooter tvr2(cx, pobj);
        if (!shape->get(cx, obj, pobj, vp))
            return false;
    }
    JS_LOCK_OBJ(cx, pobj);

    if (pobj->containsSlot(slot) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         pobj->nativeContains(*shape))) {
        if (!pobj->methodWriteBarrier(cx, *shape, *vp)) {
            JS_UNLOCK_OBJ(cx, pobj);
            return false;
        }
        pobj->lockedSetSlot(slot, *vp);
    }

    return true;
}

JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const Shape *shape, bool added, Value *vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    uint32 slot;
    int32 sample;

    JS_ASSERT(obj->isNative());
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, obj));

    slot = shape->slot;
    if (slot != SHAPE_INVALID_SLOT) {
        OBJ_CHECK_SLOT(obj, slot);

        
        if (shape->hasDefaultSetter()) {
            if (!added && !obj->methodWriteBarrier(cx, *shape, *vp)) {
                JS_UNLOCK_OBJ(cx, obj);
                return false;
            }
            obj->lockedSetSlot(slot, *vp);
            return true;
        }
    } else {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_OBJ(cx, obj);
    {
        AutoShapeRooter tvr(cx, shape);
        if (!shape->set(cx, obj, vp))
            return false;
    }

    JS_LOCK_OBJ(cx, obj);
    if (obj->containsSlot(slot) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         obj->nativeContains(*shape))) {
        if (!added && !obj->methodWriteBarrier(cx, *shape, *vp)) {
            JS_UNLOCK_OBJ(cx, obj);
            return false;
        }
        obj->lockedSetSlot(slot, *vp);
    }

    return true;
}

JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN getHow,
                     Value *vp)
{
    JSObject *aobj, *obj2;
    int protoIndex;
    JSProperty *prop;
    const Shape *shape;

    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, !JS_ON_TRACE(cx));

    
    id = js_CheckForStringIndex(id);

    aobj = js_GetProtoIfDenseArray(obj);
    protoIndex = js_LookupPropertyWithFlags(cx, aobj, id, cx->resolveFlags,
                                            &obj2, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (!prop) {
        vp->setUndefined();

        if (!CallJSPropertyOp(cx, obj->getClass()->getProperty, obj, id, vp))
            return JS_FALSE;

        PCMETER(getHow & JSGET_CACHE_RESULT && JS_PROPERTY_CACHE(cx).nofills++);

        



        jsbytecode *pc;
        if (vp->isUndefined() && ((pc = js_GetCurrentBytecodePC(cx)) != NULL)) {
            JSOp op;
            uintN flags;

            op = (JSOp) *pc;
            if (op == JSOP_TRAP) {
                JS_ASSERT_NOT_ON_TRACE(cx);
                op = JS_GetTrapOpcode(cx, cx->fp()->script(), pc);
            }
            if (op == JSOP_GETXPROP) {
                flags = JSREPORT_ERROR;
            } else {
                if (!JS_HAS_STRICT_OPTION(cx) ||
                    (op != JSOP_GETPROP && op != JSOP_GETELEM) ||
                    js_CurrentPCIsInImacro(cx)) {
                    return JS_TRUE;
                }

                



                if (JSID_IS_ATOM(id, cx->runtime->atomState.iteratorAtom))
                    return JS_TRUE;

                
                if (cx->resolveFlags == JSRESOLVE_INFER) {
                    LeaveTrace(cx);
                    pc += js_CodeSpec[op].length;
                    if (Detecting(cx, pc))
                        return JS_TRUE;
                } else if (cx->resolveFlags & JSRESOLVE_DETECTING) {
                    return JS_TRUE;
                }

                flags = JSREPORT_WARNING | JSREPORT_STRICT;
            }

            
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, IdToValue(id),
                                          NULL, NULL, NULL)) {
                return JS_FALSE;
            }
        }
        return JS_TRUE;
    }

    if (!obj2->isNative())
        return obj2->getProperty(cx, id, vp);

    shape = (Shape *) prop;

    if (getHow & JSGET_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        JS_PROPERTY_CACHE(cx).fill(cx, aobj, 0, protoIndex, obj2, shape);
    }

    if (!js_NativeGet(cx, obj, obj2, shape, getHow, vp))
        return JS_FALSE;

    JS_UNLOCK_OBJ(cx, obj2);
    return JS_TRUE;
}

JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return js_GetPropertyHelper(cx, obj, id, JSGET_METHOD_BARRIER, vp);
}

JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, Value *vp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED);

    PropertyIdOp op = obj->getOps()->getProperty;
    if (!op) {
#if JS_HAS_XML_SUPPORT
        JS_ASSERT(!obj->isXML());
#endif
        return js_GetPropertyHelper(cx, obj, id, getHow, vp);
    }
    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, obj->isDenseArray());
#if JS_HAS_XML_SUPPORT
    if (obj->isXML())
        return js_GetXMLMethod(cx, obj, id, vp);
#endif
    return op(cx, obj, id, vp);
}

JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    JSStackFrame *const fp = js_GetTopStackFrame(cx);
    if (!fp)
        return true;

    
    if (!(fp->isScriptFrame() && fp->script()->strictModeCode) &&
        !JS_HAS_STRICT_OPTION(cx)) {
        return true;
    }

    const char *bytes = js_GetStringBytes(cx, propname);
    return bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes);
}

namespace js {

JSBool
ReportReadOnly(JSContext* cx, jsid id, uintN flags)
{
    return js_ReportValueErrorFlags(cx, flags, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

JSBool
ReportNotConfigurable(JSContext* cx, jsid id, uintN flags)
{
    return js_ReportValueErrorFlags(cx, flags, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

}






JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     Value *vp, JSBool strict)
{
    int protoIndex;
    JSObject *pobj;
    JSProperty *prop;
    const Shape *shape;
    uintN attrs, flags;
    intN shortid;
    Class *clasp;
    PropertyOp getter, setter;
    bool added;

    JS_ASSERT((defineHow &
               ~(JSDNP_CACHE_RESULT | JSDNP_SET_METHOD | JSDNP_UNQUALIFIED)) == 0);
    if (defineHow & JSDNP_CACHE_RESULT)
        JS_ASSERT_NOT_ON_TRACE(cx);

    
    id = js_CheckForStringIndex(id);

    
    if (obj->sealed())
        return ReportReadOnly(cx, id, JSREPORT_ERROR);

    protoIndex = js_LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags,
                                            &pobj, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (prop) {
        if (!pobj->isNative())
            prop = NULL;
    } else {
        
        JS_ASSERT(!obj->isBlock());

        if (!obj->getParent() &&
            (defineHow & JSDNP_UNQUALIFIED) &&
            !js_CheckUndeclaredVarAssignment(cx, JSID_TO_STRING(id))) {
            return JS_FALSE;
        }
    }
    shape = (Shape *) prop;

    







    attrs = JSPROP_ENUMERATE;
    flags = 0;
    shortid = 0;
    clasp = obj->getClass();
    getter = clasp->getProperty;
    setter = clasp->setProperty;

    if (shape) {
        
        if (shape->isAccessorDescriptor()) {
            if (shape->hasDefaultSetter()) {
                JS_UNLOCK_OBJ(cx, pobj);
                if (defineHow & JSDNP_CACHE_RESULT)
                    TRACE_2(SetPropHit, JS_NO_PROP_CACHE_FILL, shape);
                return js_ReportGetterOnlyAssignment(cx);
            }
        } else {
            JS_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                JS_UNLOCK_OBJ(cx, pobj);

                PCMETER((defineHow & JSDNP_CACHE_RESULT) && JS_PROPERTY_CACHE(cx).rofills++);
                if (defineHow & JSDNP_CACHE_RESULT) {
                    JS_ASSERT_NOT_ON_TRACE(cx);
                    TRACE_2(SetPropHit, JS_NO_PROP_CACHE_FILL, shape);
                }

                
                if (strict)
                    return ReportReadOnly(cx, id, 0);
                if (JS_HAS_STRICT_OPTION(cx))
                    return ReportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return JS_TRUE;

#ifdef JS_TRACER
              error: 
                return JS_FALSE;
#endif
            }
        }
        if (pobj->sealed() && !shape->hasSlot()) {
            JS_UNLOCK_OBJ(cx, pobj);
            return ReportReadOnly(cx, id, JSREPORT_ERROR);
        }

        attrs = shape->attributes();
        if (pobj != obj) {
            






            JS_UNLOCK_OBJ(cx, pobj);

            
            if (!shape->hasSlot()) {
                if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
                    JS_ASSERT_NOT_ON_TRACE(cx);
                    PropertyCacheEntry *entry =
#endif
                        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, protoIndex, pobj, shape);
                    TRACE_2(SetPropHit, entry, shape);
                }

                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return JS_TRUE;

                return shape->set(cx, obj, vp);
            }

            
            attrs = JSPROP_ENUMERATE;

            






            if (shape->hasShortID()) {
                flags = Shape::HAS_SHORTID;
                shortid = shape->shortid;
                getter = shape->getter();
                setter = shape->setter();
            }

            



            shape = NULL;
        }

        if (shape) {
            if (shape->isMethod()) {
                JS_ASSERT(pobj->hasMethodBarrier());
            } else if ((defineHow & JSDNP_SET_METHOD) && obj->canHaveMethodBarrier()) {
                JS_ASSERT(IsFunctionObject(*vp));
                JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

                JSObject *funobj = &vp->toObject();
                JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
                if (fun == funobj) {
                    funobj = CloneFunctionObject(cx, fun, fun->parent);
                    if (!funobj)
                        return JS_FALSE;
                    vp->setObject(*funobj);
                }
            }
        }
    }

    added = false;
    if (!shape) {
        



        js_PurgeScopeChain(cx, obj, id);

        
        JS_LOCK_OBJ(cx, obj);
        if (!obj->ensureClassReservedSlots(cx)) {
            JS_UNLOCK_OBJ(cx, obj);
            return JS_FALSE;
        }

        



        if ((defineHow & JSDNP_SET_METHOD) && obj->canHaveMethodBarrier()) {
            JS_ASSERT(IsFunctionObject(*vp));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

            JSObject *funobj = &vp->toObject();
            JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
            if (fun == funobj) {
                flags |= Shape::METHOD;
                getter = CastAsPropertyOp(funobj);
            }
        }

        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape) {
            JS_UNLOCK_OBJ(cx, obj);
            return JS_FALSE;
        }

        




        if (obj->containsSlot(shape->slot))
            obj->lockedSetSlot(shape->slot, UndefinedValue());

        
        if (!CallAddPropertyHook(cx, clasp, obj, shape, vp)) {
            obj->removeProperty(cx, id);
            JS_UNLOCK_OBJ(cx, obj);
            return JS_FALSE;
        }
        added = true;
    }

    if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
        JS_ASSERT_NOT_ON_TRACE(cx);
        PropertyCacheEntry *entry =
#endif
            JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, 0, obj, shape, added);
        TRACE_2(SetPropHit, entry, shape);
    }

    if (!js_NativeSet(cx, obj, shape, added, vp))
        return NULL;

    JS_UNLOCK_OBJ(cx, obj);
    return JS_TRUE;
}

JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return js_SetPropertyHelper(cx, obj, id, 0, vp, strict);
}

JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JSProperty *prop;
    if (!js_LookupProperty(cx, obj, id, &obj, &prop))
        return false;
    if (!prop) {
        *attrsp = 0;
        return true;
    }
    if (!obj->isNative())
        return obj->getAttributes(cx, id, attrsp);

    const Shape *shape = (Shape *)prop;
    *attrsp = shape->attributes();
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, Shape *shape, uintN attrs)
{
    JS_ASSERT(obj->isNative());
    bool ok = !!js_ChangeNativePropertyAttrs(cx, obj, shape, attrs, 0,
                                             shape->getter(), shape->setter());
    JS_UNLOCK_OBJ(cx, obj);
    return ok;
}

JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JSProperty *prop;
    if (!js_LookupProperty(cx, obj, id, &obj, &prop))
        return false;
    if (!prop)
        return true;
    return obj->isNative()
           ? js_SetNativeAttributes(cx, obj, (Shape *) prop, *attrsp)
           : obj->setAttributes(cx, id, attrsp);
}

JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    JSObject *proto;
    JSProperty *prop;
    const Shape *shape;
    JSBool ok;

    rval->setBoolean(true);

    
    id = js_CheckForStringIndex(id);

    if (!js_LookupProperty(cx, obj, id, &proto, &prop))
        return false;
    if (!prop || proto != obj) {
        





        if (prop && proto->isNative()) {
            shape = (Shape *)prop;
            if (shape->isSharedPermanent()) {
                JS_UNLOCK_OBJ(cx, proto);
                if (strict)
                    return ReportNotConfigurable(cx, id, 0);
                rval->setBoolean(false);
                return true;
            }
            JS_UNLOCK_OBJ(cx, proto);
        }

        




        return CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval);
    }

    shape = (Shape *)prop;
    if (!shape->configurable()) {
        JS_UNLOCK_OBJ(cx, obj);
        if (strict)
            return ReportNotConfigurable(cx, id, 0);
        rval->setBoolean(false);
        return true;
    }

    
    if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, SHAPE_USERID(shape), rval)) {
        JS_UNLOCK_OBJ(cx, obj);
        return false;
    }

    if (obj->containsSlot(shape->slot)) {
        const Value &v = obj->lockedGetSlot(shape->slot);
        GC_POKE(cx, v);

        










        if (obj->hasMethodBarrier()) {
            JSObject *funobj;

            if (IsFunctionObject(v, &funobj)) {
                JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

                if (fun != funobj) {
                    for (JSStackFrame *fp = cx->maybefp(); fp; fp = fp->prev()) {
                        if (fp->isFunctionFrame() &&
                            &fp->callee() == &fun->compiledFunObj() &&
                            fp->thisValue().isObject() &&
                            &fp->thisValue().toObject() == obj) {
                            fp->calleeValue().setObject(*funobj);
                        }
                    }
                }
            }
        }
    }

    ok = obj->removeProperty(cx, id);
    JS_UNLOCK_OBJ(cx, obj);

    return ok && js_SuppressDeletedProperty(cx, obj, id);
}

namespace js {

JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint != JSTYPE_OBJECT && hint != JSTYPE_FUNCTION);

    Value v = ObjectValue(*obj);
    if (hint == JSTYPE_STRING) {
        




        if (obj->getClass() == &js_StringClass) {
            jsid toStringId = ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);

            JS_LOCK_OBJ(cx, obj);
            JSObject *lockedobj = obj;
            const Shape *shape = obj->nativeLookup(toStringId);
            JSObject *pobj = obj;

            if (!shape) {
                pobj = obj->getProto();

                if (pobj && pobj->getClass() == &js_StringClass) {
                    JS_UNLOCK_OBJ(cx, obj);
                    JS_LOCK_OBJ(cx, pobj);
                    lockedobj = pobj;
                    shape = pobj->nativeLookup(toStringId);
                }
            }

            if (shape && shape->hasDefaultGetter() && pobj->containsSlot(shape->slot)) {
                const Value &fval = pobj->lockedGetSlot(shape->slot);

                JSObject *funobj;
                if (IsFunctionObject(fval, &funobj)) {
                    JSFunction *fun = funobj->getFunctionPrivate();
                    if (fun->maybeNative() == js_str_toString) {
                        JS_UNLOCK_OBJ(cx, lockedobj);
                        *vp = obj->getPrimitiveThis();
                        return JS_TRUE;
                    }
                }
            }
            JS_UNLOCK_OBJ(cx, lockedobj);
        }

        



        if (!js_TryMethod(cx, obj, cx->runtime->atomState.toStringAtom,
                          0, NULL, &v)) {
            return JS_FALSE;
        }

        if (!v.isPrimitive()) {
            if (!obj->getClass()->convert(cx, obj, hint, &v))
                return JS_FALSE;
        }
    } else {
        if (!obj->getClass()->convert(cx, obj, hint, &v))
            return JS_FALSE;
        if (v.isObject()) {
            JS_ASSERT(hint != TypeOfValue(cx, v));
            if (!js_TryMethod(cx, obj, cx->runtime->atomState.toStringAtom, 0, NULL, &v))
                return JS_FALSE;
        }
    }
    if (v.isObject()) {
        
        JSString *str;
        if (hint == JSTYPE_STRING) {
            str = JS_InternString(cx, obj->getClass()->name);
            if (!str)
                return JS_FALSE;
        } else {
            str = NULL;
        }
        vp->setObject(*obj);
        js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO,
                             JSDVG_SEARCH_STACK, *vp, str,
                             (hint == JSTYPE_VOID)
                             ? "primitive type"
                             : JS_TYPE_STR(hint));
        return JS_FALSE;
    }
    *vp = v;
    return JS_TRUE;
}

} 

JS_FRIEND_API(JSBool)
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op, Value *statep, jsid *idp)
{
    
    Class *clasp = obj->getClass();
    JSEnumerateOp enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((NewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
    }

    if (!enumerate(cx, obj))
        return false;

    
    JS_ASSERT(enum_op == JSENUMERATE_INIT || enum_op == JSENUMERATE_INIT_ALL);
    statep->setMagic(JS_NATIVE_ENUMERATE);
    return true;
}

namespace js {

JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            Value *vp, uintN *attrsp)
{
    JSBool writing;
    JSObject *pobj;
    JSProperty *prop;
    Class *clasp;
    const Shape *shape;
    JSSecurityCallbacks *callbacks;
    CheckAccessOp check;

    while (JS_UNLIKELY(obj->getClass() == &js_WithClass))
        obj = obj->getProto();

    writing = (mode & JSACC_WRITE) != 0;
    switch (mode & JSACC_TYPEMASK) {
      case JSACC_PROTO:
        pobj = obj;
        if (!writing)
            vp->setObjectOrNull(obj->getProto());
        *attrsp = JSPROP_PERMANENT;
        break;

      case JSACC_PARENT:
        JS_ASSERT(!writing);
        pobj = obj;
        vp->setObject(*obj->getParent());
        *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
        break;

      default:
        if (!obj->lookupProperty(cx, id, &pobj, &prop))
            return JS_FALSE;
        if (!prop) {
            if (!writing)
                vp->setUndefined();
            *attrsp = 0;
            pobj = obj;
            break;
        }

        if (!pobj->isNative()) {
            if (!writing) {
                    vp->setUndefined();
                *attrsp = 0;
            }
            break;
        }

        shape = (Shape *)prop;
        *attrsp = shape->attributes();
        if (!writing) {
            if (pobj->containsSlot(shape->slot))
                *vp = pobj->lockedGetSlot(shape->slot);
            else
                vp->setUndefined();
        }
        JS_UNLOCK_OBJ(cx, pobj);
    }

    











    clasp = pobj->getClass();
    check = clasp->checkAccess;
    if (!check) {
        callbacks = JS_GetSecurityCallbacks(cx);
        check = callbacks ? Valueify(callbacks->checkObjectAccess) : NULL;
    }
    return !check || check(cx, pobj, id, mode, vp);
}

}

JSType
js_TypeOf(JSContext *cx, JSObject *obj)
{
    




    obj = obj->wrappedObject(cx);

    




    if (obj->isCallable()) {
        return (obj->getClass() != &js_RegExpClass)
               ? JSTYPE_FUNCTION
               : JSTYPE_OBJECT;
    }

    return JSTYPE_OBJECT;
}

#ifdef JS_THREADSAFE
void
js_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop)
{
    JS_UNLOCK_OBJ(cx, obj);
}
#endif

bool
js_IsDelegate(JSContext *cx, JSObject *obj, const Value &v)
{
    if (v.isPrimitive())
        return false;
    JSObject *obj2 = v.toObject().wrappedObject(cx);
    while ((obj2 = obj2->getProto()) != NULL) {
        if (obj2 == obj)
            return true;
    }
    return false;
}

bool
js::FindClassPrototype(JSContext *cx, JSObject *scopeobj, JSProtoKey protoKey,
                       JSObject **protop, Class *clasp)
{
    Value v;
    if (!js_FindClassObject(cx, scopeobj, protoKey, &v, clasp))
        return false;

    if (IsFunctionObject(v)) {
        JSObject *ctor = &v.toObject();
        if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &v))
            return false;
    }

    *protop = v.isObject() ? &v.toObject() : NULL;
    return true;
}





JSBool
js_GetClassPrototype(JSContext *cx, JSObject *scopeobj, JSProtoKey protoKey,
                     JSObject **protop, Class *clasp)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();
    JS_ASSERT(JSProto_Null <= protoKey);
    JS_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        if (!scopeobj) {
            if (cx->hasfp())
                scopeobj = &cx->fp()->scopeChain();
            if (!scopeobj) {
                scopeobj = cx->globalObject;
                if (!scopeobj) {
                    *protop = NULL;
                    return true;
                }
            }
        }
        scopeobj = scopeobj->getGlobal();
        if (scopeobj->getClass()->flags & JSCLASS_IS_GLOBAL) {
            const Value &v = scopeobj->getReservedSlot(JSProto_LIMIT + protoKey);
            if (v.isObject()) {
                *protop = &v.toObject();
                return true;
            }
        }
    }

    return FindClassPrototype(cx, scopeobj, protoKey, protop, clasp);
}
















static JSBool
CheckCtorGetAccess(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSAtom *atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    uintN attrs;
    return CheckAccess(cx, obj, ATOM_TO_JSID(atom), JSACC_READ, vp, &attrs);
}

static JSBool
CheckCtorSetAccess(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSAtom *atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    uintN attrs;
    return CheckAccess(cx, obj, ATOM_TO_JSID(atom), JSACC_WRITE, vp, &attrs);
}

JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto, uintN attrs)
{
    





    if (!ctor->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                              ObjectOrNullValue(proto), PropertyStub, PropertyStub, attrs)) {
        return JS_FALSE;
    }

    



    return proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                                 ObjectOrNullValue(ctor), CheckCtorGetAccess, CheckCtorSetAccess, 0);
}

JSBool
js_PrimitiveToObject(JSContext *cx, Value *vp)
{
    Value v = *vp;
    JS_ASSERT(v.isPrimitive());

    Class *clasp;
    if (v.isNumber())
        clasp = &js_NumberClass;
    else if (v.isString())
        clasp = &js_StringClass;
    else
        clasp = &js_BooleanClass;

    JSObject *obj = NewBuiltinClassInstance(cx, clasp);
    if (!obj)
        return JS_FALSE;

    obj->setPrimitiveThis(v);
    vp->setObject(*obj);
    return JS_TRUE;
}

JSBool
js_ValueToObjectOrNull(JSContext *cx, const Value &v, JSObject **objp)
{
    JSObject *obj;

    if (v.isObjectOrNull()) {
        obj = v.toObjectOrNull();
    } else if (v.isUndefined()) {
        obj = NULL;
    } else {
        Value tmp = v;
        if (!js_PrimitiveToObject(cx, &tmp))
            return JS_FALSE;
        obj = &tmp.toObject();
    }
    *objp = obj;
    return JS_TRUE;
}

JSObject *
js_ValueToNonNullObject(JSContext *cx, const Value &v)
{
    JSObject *obj;

    if (!js_ValueToObjectOrNull(cx, v, &obj))
        return NULL;
    if (!obj)
        js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, v, NULL);
    return obj;
}

JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, Value *rval)
{
    Value argv[1];

    argv[0].setString(ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[type]));
    return js_TryMethod(cx, obj, cx->runtime->atomState.valueOfAtom,
                        1, argv, rval);
}

JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, Value *argv, Value *rval)
{
    JS_CHECK_RECURSION(cx, return JS_FALSE);

    




    JSErrorReporter older = JS_SetErrorReporter(cx, NULL);
    jsid id = ATOM_TO_JSID(atom);
    Value fval;
    JSBool ok = js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &fval);
    JS_SetErrorReporter(cx, older);
    if (!ok)
        return false;

    if (fval.isPrimitive())
        return JS_TRUE;
    return ExternalInvoke(cx, obj, fval, argc, argv, rval);
}

#if JS_HAS_XDR

JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSAtom *atom;
    Class *clasp;
    uint32 classId, classDef;
    JSProtoKey protoKey;
    JSObject *proto;

    cx = xdr->cx;
    atom = NULL;
    if (xdr->mode == JSXDR_ENCODE) {
        clasp = (*objp)->getClass();
        classId = JS_XDRFindClassIdByName(xdr, clasp->name);
        classDef = !classId;
        if (classDef) {
            if (!JS_XDRRegisterClass(xdr, Jsvalify(clasp), &classId))
                return JS_FALSE;
            protoKey = JSCLASS_CACHED_PROTO_KEY(clasp);
            if (protoKey != JSProto_Null) {
                classDef |= (protoKey << 1);
            } else {
                atom = js_Atomize(cx, clasp->name, strlen(clasp->name), 0);
                if (!atom)
                    return JS_FALSE;
            }
        }
    } else {
        clasp = NULL;           
        classDef = 0;
    }

    







    if (!JS_XDRUint32(xdr, &classDef))
        return JS_FALSE;
    if (classDef == 1 && !js_XDRAtom(xdr, &atom))
        return JS_FALSE;

    if (!JS_XDRUint32(xdr, &classId))
        return JS_FALSE;

    if (xdr->mode == JSXDR_DECODE) {
        if (classDef) {
            
            protoKey = (JSProtoKey) (classDef >> 1);
            if (!js_GetClassPrototype(cx, NULL, protoKey, &proto, clasp))
                return JS_FALSE;
            clasp = proto->getClass();
            if (!JS_XDRRegisterClass(xdr, Jsvalify(clasp), &classId))
                return JS_FALSE;
        } else {
            clasp = Valueify(JS_XDRFindClassById(xdr, classId));
            if (!clasp) {
                char numBuf[12];
                JS_snprintf(numBuf, sizeof numBuf, "%ld", (long)classId);
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_CANT_FIND_CLASS, numBuf);
                return JS_FALSE;
            }
        }
    }

    if (!clasp->xdrObject) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_XDR_CLASS, clasp->name);
        return JS_FALSE;
    }
    return clasp->xdrObject(xdr, objp);
}

#endif 

#ifdef JS_DUMP_SCOPE_METERS

#include <stdio.h>

JSBasicStats js_entry_count_bs = JS_INIT_STATIC_BASIC_STATS;

static void
MeterEntryCount(uintN count)
{
    JS_BASIC_STATS_ACCUM(&js_entry_count_bs, count);
}

void
js_DumpScopeMeters(JSRuntime *rt)
{
    static FILE *logfp;
    if (!logfp)
        logfp = fopen("/tmp/scope.stats", "a");

    {
        double mean, sigma;

        mean = JS_MeanAndStdDevBS(&js_entry_count_bs, &sigma);

        fprintf(logfp, "scopes %u entries %g mean %g sigma %g max %u",
                js_entry_count_bs.num, js_entry_count_bs.sum, mean, sigma,
                js_entry_count_bs.max);
    }

    JS_DumpHistogram(&js_entry_count_bs, logfp);
    JS_BASIC_STATS_INIT(&js_entry_count_bs);
    fflush(logfp);
}
#endif

#ifdef DEBUG
void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize)
{
    JS_ASSERT(trc->debugPrinter == js_PrintObjectSlotName);

    JSObject *obj = (JSObject *)trc->debugPrintArg;
    uint32 slot = (uint32)trc->debugPrintIndex;
    JS_ASSERT(slot >= JSSLOT_START(obj->getClass()));

    const Shape *shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape->previous() && shape->slot != slot)
            shape = shape->previous();
    } else {
        shape = NULL;
    }

    if (!shape) {
        const char *slotname = NULL;
        Class *clasp = obj->getClass();
        if (clasp->flags & JSCLASS_IS_GLOBAL) {
            uint32 key = slot - JSSLOT_START(clasp);
#define JS_PROTO(name,code,init)                                              \
    if ((code) == key) { slotname = js_##name##_str; goto found; }
#include "jsproto.tbl"
#undef JS_PROTO
        }
      found:
        if (slotname)
            JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
        else
            JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
    } else {
        jsid id = shape->id;
        if (JSID_IS_INT(id)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(id));
        } else if (JSID_IS_ATOM(id)) {
            js_PutEscapedString(buf, bufsize, JSID_TO_STRING(id), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}
#endif

void
js_TraceObject(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    JSContext *cx = trc->context;
    if (!obj->nativeEmpty() && IS_GC_MARKING_TRACER(trc)) {
        





        size_t slots = obj->slotSpan();
        if (obj->numSlots() != slots)
            obj->shrinkSlots(cx, slots);
    }

#ifdef JS_DUMP_SCOPE_METERS
    MeterEntryCount(obj->propertyCount);
#endif

    obj->trace(trc);

    if (!JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList))
        js_TraceWatchPoints(trc, obj);

    
    Class *clasp = obj->getClass();
    if (clasp->mark) {
        if (clasp->flags & JSCLASS_MARK_IS_TRACE)
            ((JSTraceOp) clasp->mark)(trc, obj);
        else if (IS_GC_MARKING_TRACER(trc))
            (void) clasp->mark(cx, obj, trc);
    }
    if (clasp->flags & JSCLASS_IS_GLOBAL) {
        JSCompartment *compartment = obj->getCompartment(cx);
        compartment->marked = true;
    }

    









    uint32 nslots = obj->numSlots();
    if (!obj->nativeEmpty() && obj->slotSpan() < nslots)
        nslots = obj->slotSpan();
    JS_ASSERT(nslots >= JSSLOT_START(clasp));

    for (uint32 i = JSSLOT_START(clasp); i != nslots; ++i) {
        const Value &v = obj->getSlot(i);
        JS_SET_TRACING_DETAILS(trc, js_PrintObjectSlotName, obj, i);
        MarkValueRaw(trc, v);
    }
}

void
js_ClearNative(JSContext *cx, JSObject *obj)
{
    



    JS_LOCK_OBJ(cx, obj);
    if (!obj->nativeEmpty()) {
        
        obj->clear(cx);

        
        uint32 freeslot = JSSLOT_FREE(obj->getClass());
        uint32 n = obj->numSlots();
        for (uint32 i = freeslot; i < n; ++i)
            obj->setSlot(i, UndefinedValue());
    }
    JS_UNLOCK_OBJ(cx, obj);
}

bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, Value *vp)
{
    if (!obj->isNative()) {
        vp->setUndefined();
        return true;
    }

    uint32 slot = JSSLOT_START(obj->getClass()) + index;
    JS_LOCK_OBJ(cx, obj);
    if (slot < obj->numSlots())
        *vp = obj->getSlot(slot);
    else
        vp->setUndefined();
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, const Value &v)
{
    if (!obj->isNative())
        return true;

    Class *clasp = obj->getClass();
    uint32 slot = JSSLOT_START(clasp) + index;

    JS_LOCK_OBJ(cx, obj);
    if (slot >= obj->numSlots()) {
        uint32 nslots = JSSLOT_FREE(clasp);
        JS_ASSERT(slot < nslots);
        if (!obj->allocSlots(cx, nslots)) {
            JS_UNLOCK_OBJ(cx, obj);
            return false;
        }
    }

    obj->setSlot(slot, v);
    GC_POKE(cx, JS_NULL);
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

JSObject *
JSObject::wrappedObject(JSContext *cx) const
{
    if (JSObjectOp op = getClass()->ext.wrappedObject) {
        if (JSObject *obj = op(cx, const_cast<JSObject *>(this)))
            return obj;
    }
    return const_cast<JSObject *>(this);
}

JSObject *
JSObject::getGlobal() const
{
    JSObject *obj = const_cast<JSObject *>(this);
    while (JSObject *parent = obj->getParent())
        obj = parent;
    return obj;
}

JSBool
js_ReportGetterOnlyAssignment(JSContext *cx)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_GETTER_ONLY);
}

JSCompartment *
JSObject::getCompartment(JSContext *cx)
{
    JSObject *obj = getGlobal();

    Class *clasp = obj->getClass();
    if (!(clasp->flags & JSCLASS_IS_GLOBAL)) {
#if JS_HAS_XML_SUPPORT
        
        if (clasp == &js_AnyNameClass)
            return cx->runtime->defaultCompartment;

        
        if (clasp == &js_NamespaceClass &&
            obj->getNameURI() == ATOM_TO_JSVAL(cx->runtime->
                                               atomState.functionNamespaceURIAtom)) {
            return cx->runtime->defaultCompartment;
        }
#endif

        



        if (clasp == &js_FunctionClass || clasp == &js_BlockClass || clasp == &js_RegExpClass ||
            clasp == &js_ScriptClass) {
            
            return cx->runtime->defaultCompartment;
        }
        JS_NOT_REACHED("non-global object at end of scope chain");
    }
    const Value &v = obj->getReservedSlot(JSRESERVED_GLOBAL_COMPARTMENT);
    return (JSCompartment *)v.toPrivate();
}

JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_GETTER_ONLY);
    return JS_FALSE;
}

#ifdef DEBUG







void
dumpChars(const jschar *s, size_t n)
{
    size_t i;

    if (n == (size_t) -1) {
        while (s[++n]) ;
    }

    fputc('"', stderr);
    for (i = 0; i < n; i++) {
        if (s[i] == '\n')
            fprintf(stderr, "\\n");
        else if (s[i] == '\t')
            fprintf(stderr, "\\t");
        else if (s[i] >= 32 && s[i] < 127)
            fputc(s[i], stderr);
        else if (s[i] <= 255)
            fprintf(stderr, "\\x%02x", (unsigned int) s[i]);
        else
            fprintf(stderr, "\\u%04x", (unsigned int) s[i]);
    }
    fputc('"', stderr);
}

JS_FRIEND_API(void)
js_DumpChars(const jschar *s, size_t n)
{
    fprintf(stderr, "jschar * (%p) = ", (void *) s);
    dumpChars(s, n);
    fputc('\n', stderr);
}

void
dumpString(JSString *str)
{
    dumpChars(str->chars(), str->length());
}

JS_FRIEND_API(void)
js_DumpString(JSString *str)
{
    fprintf(stderr, "JSString* (%p) = jschar * (%p) = ",
            (void *) str, (void *) str->chars());
    dumpString(str);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpAtom(JSAtom *atom)
{
    fprintf(stderr, "JSAtom* (%p) = ", (void *) atom);
    js_DumpString(ATOM_TO_STRING(atom));
}

void
dumpValue(const Value &v)
{
    if (v.isNull())
        fprintf(stderr, "null");
    else if (v.isUndefined())
        fprintf(stderr, "undefined");
    else if (v.isInt32())
        fprintf(stderr, "%d", v.toInt32());
    else if (v.isDouble())
        fprintf(stderr, "%g", v.toDouble());
    else if (v.isString())
        dumpString(v.toString());
    else if (v.isObject() && v.toObject().isFunction()) {
        JSObject *funobj = &v.toObject();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
        fprintf(stderr, "<%s %s at %p (JSFunction at %p)>",
                fun->atom ? "function" : "unnamed",
                fun->atom ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom)) : "function",
                (void *) funobj,
                (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.toObject();
        Class *clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &js_ObjectClass) ? "" : " object",
                (void *) obj);
    } else if (v.isBoolean()) {
        if (v.toBoolean())
            fprintf(stderr, "true");
        else
            fprintf(stderr, "false");
    } else if (v.isMagic()) {
        fprintf(stderr, "<invalid");
#ifdef DEBUG
        switch (v.whyMagic()) {
          case JS_ARRAY_HOLE:        fprintf(stderr, " array hole");         break;
          case JS_ARGS_HOLE:         fprintf(stderr, " args hole");          break;
          case JS_NATIVE_ENUMERATE:  fprintf(stderr, " native enumeration"); break;
          case JS_NO_ITER_VALUE:     fprintf(stderr, " no iter value");      break;
          case JS_GENERATOR_CLOSING: fprintf(stderr, " generator closing");  break;
          default:                   fprintf(stderr, " ?!");                 break;
        }
#endif
        fprintf(stderr, ">");
    } else {
        fprintf(stderr, "unexpected value");
    }
}

JS_FRIEND_API(void)
js_DumpValue(const Value &val)
{
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void *) JSID_BITS(id));
    dumpValue(IdToValue(id));
    fputc('\n', stderr);
}

static void
DumpShape(const Shape &shape)
{
    jsid id = shape.id;
    uint8 attrs = shape.attributes();

    fprintf(stderr, "    ");
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_GETTER) fprintf(stderr, "getter ");
    if (attrs & JSPROP_SETTER) fprintf(stderr, "setter ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");
    if (shape.isAlias()) fprintf(stderr, "alias ");
    if (JSID_IS_ATOM(id))
        dumpString(JSID_TO_STRING(id));
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));
    fprintf(stderr, ": slot %d", shape.slot);
    fprintf(stderr, "\n");
}

JS_FRIEND_API(void)
js_DumpObject(JSObject *obj)
{
    uint32 i, slots;
    Class *clasp;
    jsuint reservedEnd;

    fprintf(stderr, "object %p\n", (void *) obj);
    clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (void *)clasp, clasp->name);

    fprintf(stderr, "flags:");
    uint32 flags = obj->flags;
    if (flags & JSObject::DELEGATE) fprintf(stderr, " delegate");
    if (flags & JSObject::SYSTEM) fprintf(stderr, " system");
    if (flags & JSObject::SEALED) fprintf(stderr, " sealed");
    if (flags & JSObject::BRANDED) fprintf(stderr, " branded");
    if (flags & JSObject::GENERIC) fprintf(stderr, " generic");
    if (flags & JSObject::METHOD_BARRIER) fprintf(stderr, " method_barrier");
    if (flags & JSObject::INDEXED) fprintf(stderr, " indexed");
    if (flags & JSObject::OWN_SHAPE) fprintf(stderr, " own_shape");
    bool anyFlags = flags != 0;
    if (obj->isNative()) {
        if (obj->inDictionaryMode()) {
            fprintf(stderr, " inDictionaryMode");
            anyFlags = true;
        }
        if (obj->hasPropertyTable()) {
            fprintf(stderr, " hasPropertyTable");
            anyFlags = true;
        }
    }
    if (!anyFlags)
        fprintf(stderr, " none");
    fprintf(stderr, "\n");

    if (obj->isDenseArray()) {
        slots = JS_MIN(obj->getArrayLength(), obj->getDenseArrayCapacity());
        fprintf(stderr, "elements\n");
        for (i = 0; i < slots; i++) {
            fprintf(stderr, " %3d: ", i);
            dumpValue(obj->getDenseArrayElement(i));
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        return;
    }

    if (obj->isNative()) {
        if (obj->sealed())
            fprintf(stderr, "sealed\n");

        fprintf(stderr, "properties:\n");
        for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront())
            DumpShape(r.front());
    } else {
        if (!obj->isNative())
            fprintf(stderr, "not native\n");
    }

    fprintf(stderr, "proto ");
    dumpValue(ObjectOrNullValue(obj->getProto()));
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(ObjectOrNullValue(obj->getParent()));
    fputc('\n', stderr);

    i = JSSLOT_PRIVATE;
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        i = JSSLOT_PRIVATE + 1;
        fprintf(stderr, "private %p\n", obj->getPrivate());
    }

    fprintf(stderr, "slots:\n");
    reservedEnd = i + JSCLASS_RESERVED_SLOTS(clasp);
    slots = obj->slotSpan();
    for (; i < slots; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->getSlot(i));
        fputc('\n', stderr);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char *name, JSObject *obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(ObjectValue(*obj));
        fputc('\n', stderr);
    }
}

static void
MaybeDumpValue(const char *name, const Value &v)
{
    if (!v.isNull()) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(v);
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js_DumpStackFrame(JSContext *cx, JSStackFrame *start)
{
    
    VOUCH_DOES_NOT_REQUIRE_STACK();

    if (!start)
        start = cx->maybefp();
    FrameRegsIter i(cx);
    while (!i.done() && i.fp() != start)
        ++i;

    if (i.done()) {
        fprintf(stderr, "fp = %p not found in cx = %p\n", (void *)start, (void *)cx);
        return;
    }

    for (; !i.done(); ++i) {
        JSStackFrame *const fp = i.fp();

        fprintf(stderr, "JSStackFrame at %p\n", (void *) fp);
        if (fp->isFunctionFrame()) {
            fprintf(stderr, "callee fun: ");
            dumpValue(ObjectValue(fp->callee()));
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        if (fp->isScriptFrame()) {
            fprintf(stderr, "file %s line %u\n",
                    fp->script()->filename, (unsigned) fp->script()->lineno);
        }

        if (jsbytecode *pc = i.pc()) {
            if (!fp->isScriptFrame()) {
                fprintf(stderr, "*** pc && !script, skipping frame\n\n");
                continue;
            }
            if (fp->hasImacropc()) {
                fprintf(stderr, "  pc in imacro at %p\n  called from ", pc);
                pc = fp->imacropc();
            } else {
                fprintf(stderr, "  ");
            }
            fprintf(stderr, "pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
        }
        Value *sp = i.sp();
        fprintf(stderr, "  slots: %p\n", (void *) fp->slots());
        fprintf(stderr, "  sp:    %p = slots + %u\n", (void *) sp, (unsigned) (sp - fp->slots()));
        if (sp - fp->slots() < 10000) { 
            for (Value *p = fp->slots(); p < sp; p++) {
                fprintf(stderr, "    %p: ", (void *) p);
                dumpValue(*p);
                fputc('\n', stderr);
            }
        }
        if (fp->isFunctionFrame() && !fp->isEvalFrame()) {
            fprintf(stderr, "  actuals: %p (%u) ", (void *) fp->actualArgs(), (unsigned) fp->numActualArgs());
            fprintf(stderr, "  formals: %p (%u)\n", (void *) fp->formalArgs(), (unsigned) fp->numFormalArgs());
        }
        MaybeDumpObject("callobj", fp->maybeCallObj());
        MaybeDumpObject("argsobj", fp->maybeArgsObj());
        MaybeDumpValue("this", fp->thisValue());
        fprintf(stderr, "  rval: ");
        dumpValue(fp->returnValue());
        fputc('\n', stderr);

        fprintf(stderr, "  flags:");
        if (fp->isConstructing())
            fprintf(stderr, " constructing");
        if (fp->hasOverriddenArgs())
            fprintf(stderr, " overridden_args");
        if (fp->isAssigning())
            fprintf(stderr, " assigning");
        if (fp->isDebuggerFrame())
            fprintf(stderr, " debugger");
        if (fp->isEvalFrame())
            fprintf(stderr, " eval");
        if (fp->isYielding())
            fprintf(stderr, " yielding");
        if (fp->isGeneratorFrame())
            fprintf(stderr, " generator");
        fputc('\n', stderr);

        fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) &fp->scopeChain());
        if (fp->hasBlockChain())
            fprintf(stderr, "  blockChain: (JSObject *) %p\n", (void *) fp->blockChain());

        fputc('\n', stderr);
    }
}

#ifdef DEBUG
bool
IsSaneThisObject(JSObject &obj)
{
    Class *clasp = obj.getClass();
    return clasp != &js_CallClass &&
           clasp != &js_BlockClass &&
           clasp != &js_DeclEnvClass &&
           clasp != &js_WithClass;
}
#endif

#endif 


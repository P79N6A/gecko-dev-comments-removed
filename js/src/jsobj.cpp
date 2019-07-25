










































#define __STDC_LIMIT_MACROS

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

#include "jsdtracef.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jsautooplen.h"

using namespace js;

JS_FRIEND_DATA(JSObjectOps) js_ObjectOps = {
    NULL,
    js_LookupProperty,
    js_DefineProperty,
    js_GetProperty,
    js_SetProperty,
    js_GetAttributes,
    js_SetAttributes,
    js_DeleteProperty,
    js_DefaultValue,
    js_Enumerate,
    js_CheckAccess,
    js_TypeOf,
    js_TraceObject,
    NULL,   
    js_Call,
    js_Construct,
    js_HasInstance,
    js_Clear
};

Class js_ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,     PropertyStub,     PropertyStub,     PropertyStub,
    EnumerateStub,    ResolveStub,      ConvertStub,      NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#if JS_HAS_OBJ_PROTO_PROP

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp);

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, Value *vp);

static JSPropertySpec object_props[] = {
    {js_proto_str, JSSLOT_PROTO, JSPROP_PERMANENT|JSPROP_SHARED,
     Jsvalify(obj_getProto), Jsvalify(obj_setProto) },
    {0,0,0,0,0}
};

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(id == INT_TO_JSID(JSSLOT_PROTO));

    
    uintN attrs;
    id = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    return obj->checkAccess(cx, id, JSACC_PROTO, vp, &attrs);
}

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(id == INT_TO_JSID(JSSLOT_PROTO));

    if (!vp->isObjectOrNull())
        return JS_TRUE;

    JSObject *pobj = vp->asObjectOrNull();
    if (pobj) {
        




        Innerize(cx, &pobj);
        if (!pobj)
            return JS_FALSE;
    }

    uintN attrs;
    id = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    if (!obj->checkAccess(cx, id, JSAccessMode(JSACC_PROTO|JSACC_WRITE), vp, &attrs))
        return JS_FALSE;

    return js_SetProtoOrParent(cx, obj, JSSLOT_PROTO, pobj, JS_TRUE);
}

#else  

#define object_props NULL

#endif 

JSBool
js_SetProtoOrParent(JSContext *cx, JSObject *obj, uint32 slot, JSObject *pobj,
                    JSBool checkForCycles)
{
    JS_ASSERT(slot == JSSLOT_PARENT || slot == JSSLOT_PROTO);
    JS_ASSERT_IF(!checkForCycles, obj != pobj);

    if (slot == JSSLOT_PROTO) {
        if (obj->isNative()) {
            JS_LOCK_OBJ(cx, obj);
            bool ok = !!js_GetMutableScope(cx, obj);
            JS_UNLOCK_OBJ(cx, obj);
            if (!ok)
                return false;
        }

        




        JSObject *oldproto = obj;
        while (oldproto && oldproto->isNative()) {
            JS_LOCK_OBJ(cx, oldproto);
            JSScope *scope = oldproto->scope();
            scope->protoShapeChange(cx);
            JSObject *tmp = oldproto->getProto();
            JS_UNLOCK_OBJ(cx, oldproto);
            oldproto = tmp;
        }
    }

    if (!pobj || !checkForCycles) {
        if (slot == JSSLOT_PROTO)
            obj->setProto(ObjectOrNullTag(pobj));
        else
            obj->setParent(ObjectOrNullTag(pobj));
    } else if (!js_SetProtoOrParentCheckingForCycles(cx, obj, slot, pobj)) {
        const char *name = (slot == JSSLOT_PARENT) ? "parent" : js_proto_str;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE,
                             name);
        return false;
    }
    return true;
}

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
            js::Value val;
            ok = obj->lookupProperty(cx, id, &obj2, &prop);
            if (!ok)
                break;
            if (!prop)
                continue;
            bool hasGetter, hasSetter;
            AutoValueRooter v(cx);
            AutoValueRooter setter(cx);
            if (obj2->isNative()) {
                JSScopeProperty *sprop = (JSScopeProperty *) prop;
                hasGetter = sprop->hasGetterValue();
                hasSetter = sprop->hasSetterValue();
                if (hasGetter)
                    v.set(sprop->getterValue());
                if (hasSetter)
                    setter.set(sprop->setterValue());
                JS_UNLOCK_OBJ(cx, obj2);
            } else {
                hasGetter = hasSetter = false;
            }
            if (hasSetter) {
                
                if (hasGetter && v.value().isObject()) {
                    ok = !!MarkSharpObjects(cx, &v.value().asObject(), NULL);
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
                !MarkSharpObjects(cx, &v.value().asObject(), NULL)) {
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
    obj = ComputeThisObjectFromVp(cx, vp);
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
        
        chars = (jschar *) js_malloc(((outermost ? 4 : 2) + 1) * sizeof(jschar));
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
                JSScopeProperty *sprop = (JSScopeProperty *) prop;
                unsigned attrs = sprop->attributes();
                if (attrs & JSPROP_GETTER) {
                    doGet = false;
                    val[valcnt] = sprop->getterValue();
                    gsop[valcnt] = ATOM_TO_STRING(cx->runtime->atomState.getAtom);
                    valcnt++;
                }
                if (attrs & JSPROP_SETTER) {
                    doGet = false;
                    val[valcnt] = sprop->setterValue();
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
                he = js_EnterSharpObject(cx, &val[j].asObject(), NULL, &vsharp);
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
                JSFunction *fun = js_ValueToFunction(cx, &val[j], JSV2F_SEARCH_STACK);
                const jschar *start = vchars;
                const jschar *end = vchars + vlength;

                uint8 parenChomp = 0;
                if (vchars[0] == '(') {
                    vchars++;
                    parenChomp = 1;
                }

                




                if (JSFUN_GETTER_TEST(fun->flags) || JSFUN_SETTER_TEST(fun->flags)) {
                    
                    const jschar *tmp = js_strchr_limit(vchars, ' ', end);
                    if (tmp)
                        vchars = tmp + 1;
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

static JSBool
obj_toString(JSContext *cx, uintN argc, Value *vp)
{
    jschar *chars;
    size_t nchars;
    const char *clazz, *prefix;
    JSString *str;

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    if (obj->isProxy()) {
        if (!GetProxyObjectClass(cx, obj, &clazz))
            return false;
    } else {
        obj = obj->wrappedObject(cx);
        clazz = obj->getClass()->name;
    }
    nchars = 9 + strlen(clazz);         
    chars = (jschar *) cx->malloc((nchars + 1) * sizeof(jschar));
    if (!chars)
        return JS_FALSE;

    prefix = "[object ";
    nchars = 0;
    while ((chars[nchars] = (jschar)*prefix) != 0)
        nchars++, prefix++;
    while ((chars[nchars] = (jschar)*clazz) != 0)
        nchars++, clazz++;
    chars[nchars++] = ']';
    chars[nchars] = 0;

    str = js_NewString(cx, chars, nchars);
    if (!str) {
        cx->free(chars);
        return JS_FALSE;
    }
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
obj_toLocaleString(JSContext *cx, uintN argc, Value *vp)
{
    if (!ComputeThisFromVpInPlace(cx, vp))
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
    if (!ComputeThisFromVpInPlace(cx, vp))
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

JSObject *
js_CheckScopeChainValidity(JSContext *cx, JSObject *scopeobj, const char *caller)
{
    Class *clasp;
    JSExtendedClass *xclasp;
    JSObject *inner;

    if (!scopeobj)
        goto bad;

    Innerize(cx, &scopeobj);
    if (!scopeobj)
        return NULL;

    inner = scopeobj;

    
    while (scopeobj) {
        clasp = scopeobj->getClass();
        if (clasp->flags & JSCLASS_IS_EXTENDED) {
            xclasp = (JSExtendedClass*)clasp;
            if (xclasp->innerObject &&
                xclasp->innerObject(cx, scopeobj) != scopeobj) {
                goto bad;
            }
        }

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
    flags = JS_GetScriptFilenameFlags(caller->script);
    if ((flags & JSFILENAME_PROTECTED) &&
        principals &&
        strcmp(principals->codebase, "[System Principal]")) {
        *linenop = 0;
        return principals->codebase;
    }

    jsbytecode *pc = caller->pc(cx);
    if (pc && js_GetOpcode(cx, caller->script, pc) == JSOP_EVAL) {
        JS_ASSERT(js_GetOpcode(cx, caller->script, pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        *linenop = GET_UINT16(pc + JSOP_EVAL_LENGTH);
    } else {
        *linenop = js_FramePCToLineNumber(cx, caller);
    }
    return caller->script->filename;
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
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    obj = obj->wrappedObject(cx);

    




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

    
    JSObject *scopeobj = NULL;
    if (argc >= 2) {
        if (!js_ValueToObjectOrNull(cx, argv[1], &argv[1]))
            return JS_FALSE;
        scopeobj = argv[1].asObjectOrNull();
        JSObject *obj = scopeobj;
        while (obj) {
            if (obj->isDenseArray() && !obj->makeDenseArraySlow(cx))
                return false;
            JSObject *parent = obj->getParent();
            if (!obj->isNative() ||
                (!parent && !(obj->getClass()->flags & JSCLASS_IS_GLOBAL))) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_INVALID_EVAL_SCOPE_ARG);
                return false;
            }
            obj = parent;
        }
    }

    
    struct WithGuard {
        JSObject *obj;
        WithGuard() : obj(NULL) {}
        ~WithGuard() { if (obj) obj->setPrivate(NULL); }
    } withGuard;

    
    MUST_FLOW_THROUGH("out");
    uintN staticLevel = caller->script->staticLevel + 1;
    if (!scopeobj) {
        



        JSObject *callerScopeChain = js_GetScopeChain(cx, caller);
        if (!callerScopeChain)
            return JS_FALSE;

#if JS_HAS_EVAL_THIS_SCOPE
        




        if (indirectCall) {
            
            staticLevel = 0;

            Innerize(cx, &obj);
            if (!obj)
                return JS_FALSE;

            if (!js_CheckPrincipalsAccess(cx, obj,
                                          JS_StackFramePrincipals(cx, caller),
                                          cx->runtime->atomState.evalAtom)) {
                return JS_FALSE;
            }

            
            JS_ASSERT(!obj->getParent());
            scopeobj = obj;
        } else {
            





            JS_ASSERT_IF(caller->argv, caller->callobj);
            scopeobj = callerScopeChain;
        }
#endif
    } else {
        scopeobj = scopeobj->wrappedObject(cx);
        Innerize(cx, &scopeobj);
        if (!scopeobj)
            return JS_FALSE;

        if (!js_CheckPrincipalsAccess(cx, scopeobj,
                                      JS_StackFramePrincipals(cx, caller),
                                      cx->runtime->atomState.evalAtom))
            return JS_FALSE;

        



        if (scopeobj->getParent()) {
            JSObject *global = scopeobj->getGlobal();
            withGuard.obj = js_NewWithObject(cx, scopeobj, global, 0);
            if (!withGuard.obj)
                return JS_FALSE;

            scopeobj = withGuard.obj;
            JS_ASSERT(argc >= 2);
            argv[1].setObject(*withGuard.obj);
        }

        
        staticLevel = 0;
    }

    
    JSObject *result = js_CheckScopeChainValidity(cx, scopeobj, js_eval_str);
    JS_ASSERT_IF(result, result == scopeobj);
    if (!result)
        return JS_FALSE;

    
    
    if (!js_CheckContentSecurityPolicy(cx)) {
        JS_ReportError(cx, "call to eval() blocked by CSP");
        return  JS_FALSE;
    }

    JSObject *callee = &vp[0].asObject();
    JSPrincipals *principals = js_EvalFramePrincipals(cx, callee, caller);
    uintN line;
    const char *file = js_ComputeFilename(cx, caller, principals, &line);

    JSString *str = argv[0].asString();
    JSScript *script = NULL;

    








    JSScript **bucket = EvalCacheHash(cx, str);
    if (!indirectCall && caller->fun) {
        uintN count = 0;
        JSScript **scriptp = bucket;

        EVAL_CACHE_METER(probe);
        while ((script = *scriptp) != NULL) {
            if (script->savedCallerFun &&
                script->staticLevel == staticLevel &&
                script->version == cx->version &&
                (script->principals == principals ||
                 (principals->subsume(principals, script->principals) &&
                  script->principals->subsume(script->principals, principals)))) {
                



                JSFunction *fun = script->getFunction(0);

                if (fun == caller->fun) {
                    



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
                                         str->chars(), str->length(),
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
            subject = JS_StackFramePrincipals(cx, caller);

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
    ok = InternalCall(cx, obj, ObjectOrNullTag(callable), 3, argv, Valueify(nvp));
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

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;

    Value value;
    uintN attrs;
    if (!obj->checkAccess(cx, propid, JSACC_WATCH, &value, &attrs))
        return JS_FALSE;

    
    vp[0].setUndefined();

    if (attrs & JSPROP_READONLY)
        return JS_TRUE;
    if (obj->isDenseArray() && !obj->makeDenseArraySlow(cx))
        return JS_FALSE;
    return JS_SetWatchPoint(cx, obj, propid, obj_watch_handler, callable);
}

static JSBool
obj_unwatch(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    vp->setUndefined();
    if (argc == 0)
        return JS_TRUE;
    jsid id;
    if (!ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    return JS_ClearWatchPoint(cx, obj, id, NULL, NULL);
}

#endif 







static JSBool
obj_hasOwnProperty(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    return obj &&
           js_HasOwnPropertyHelper(cx, obj->map->ops->lookupProperty, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : Value(UndefinedTag()), &id))
        return JS_FALSE;

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
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
    if (!lookup(cx, obj, id, objp, propp))
        return false;
    if (!*propp)
        return true;

    if (*objp == obj)
        return true;

    JSExtendedClass *xclasp;
    JSObject *outer;
    Class *clasp = (*objp)->getClass();
    if (!(clasp->flags & JSCLASS_IS_EXTENDED) ||
        !(xclasp = (JSExtendedClass *) clasp)->outerObject) {
        outer = NULL;
    } else {
        outer = xclasp->outerObject(cx, *objp);
        if (!outer)
            return false;
    }

    if (outer != *objp) {
        if ((*objp)->isNative() && obj->getClass() == clasp) {
            














            JSScopeProperty *sprop = reinterpret_cast<JSScopeProperty *>(*propp);
            if (sprop->isSharedPermanent())
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
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj)
        return JS_FALSE;
    const Value &v = argc != 0 ? vp[2] : Value(UndefinedTag());
    vp->setBoolean(js_IsDelegate(cx, obj, v));
    return JS_TRUE;
}


static JSBool
obj_propertyIsEnumerable(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : Value(UndefinedTag()), &id))
        return JS_FALSE;

    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
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
        JSScopeProperty *sprop = (JSScopeProperty *) prop;
        shared = sprop->isSharedPermanent();
        attrs = sprop->attributes();
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
    PropertyOp getter = CastAsPropertyOp(&vp[3].asObject());

    jsid id;
    if (!ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj || !CheckRedeclaration(cx, obj, id, JSPROP_GETTER, NULL, NULL))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!obj->checkAccess(cx, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    vp->setUndefined();
    return obj->defineProperty(cx, id, Value(UndefinedTag()), getter, PropertyStub,
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
    PropertyOp setter = CastAsPropertyOp(&vp[3].asObject());

    jsid id;
    if (!ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    if (!obj || !CheckRedeclaration(cx, obj, id, JSPROP_SETTER, NULL, NULL))
        return JS_FALSE;
    



    uintN attrs;
    Value junk;
    if (!obj->checkAccess(cx, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    vp->setUndefined();
    return obj->defineProperty(cx, id, Value(UndefinedTag()), PropertyStub, setter,
                               JSPROP_ENUMERATE | JSPROP_SETTER | JSPROP_SHARED);
}

static JSBool
obj_lookupGetter(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : Value(UndefinedTag()), &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    JSObject *pobj;
    JSProperty *prop;
    if (!obj || !obj->lookupProperty(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            JSScopeProperty *sprop = (JSScopeProperty *) prop;
            if (sprop->hasGetterValue())
                *vp = sprop->getterValue();
            JS_UNLOCK_OBJ(cx, pobj);
        }
    }
    return JS_TRUE;
}

static JSBool
obj_lookupSetter(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : Value(UndefinedTag()), &id))
        return JS_FALSE;
    JSObject *obj = ComputeThisObjectFromVp(cx, vp);
    JSObject *pobj;
    JSProperty *prop;
    if (!obj || !obj->lookupProperty(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            JSScopeProperty *sprop = (JSScopeProperty *) prop;
            if (sprop->hasSetterValue())
                *vp = sprop->setterValue();
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
        char *bytes = DecompileValueGenerator(cx, 0 - argc, vp[2], NULL);
        if (!bytes)
            return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNEXPECTED_TYPE, bytes, "not an object");
        JS_free(cx, bytes);
        return JS_FALSE;
    }

    JSObject &obj = vp[2].asObject();
    uintN attrs;
    return obj.checkAccess(cx, ATOM_TO_JSID(cx->runtime->atomState.protoAtom),
                           JSACC_PROTO, vp, &attrs);
}

extern JSBool
js_NewPropertyDescriptorObject(JSContext *cx, jsid id, uintN attrs,
                               const Value &getter, const Value &setter,
                               const Value &value, Value *vp)
{
    
    JSObject *desc = NewObject(cx, &js_ObjectClass, NULL, NULL);
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
                                  BooleanTag((attrs & JSPROP_READONLY) == 0),
                                  PropertyStub, PropertyStub, JSPROP_ENUMERATE)) {
            return false;
        }
    }

    return desc->defineProperty(cx, ATOM_TO_JSID(atomState.enumerableAtom),
                                BooleanTag((attrs & JSPROP_ENUMERATE) != 0),
                                PropertyStub, PropertyStub, JSPROP_ENUMERATE) &&
           desc->defineProperty(cx, ATOM_TO_JSID(atomState.configurableAtom),
                                BooleanTag((attrs & JSPROP_PERMANENT) == 0),
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
    if (!js_HasOwnProperty(cx, obj->map->ops->lookupProperty, obj, id, &pobj, &prop))
        return false;
    if (!prop) {
        vp->setUndefined();
        return true;
    }

    Value roots[] = { UndefinedTag(), UndefinedTag(), UndefinedTag() };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(roots), roots);
    unsigned attrs;
    bool doGet = true;
    if (pobj->isNative()) {
        JSScopeProperty *sprop = (JSScopeProperty *) prop;
        attrs = sprop->attributes();
        if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
            doGet = false;
            if (attrs & JSPROP_GETTER)
                roots[0] = sprop->getterValue();
            if (attrs & JSPROP_SETTER)
                roots[1] = sprop->setterValue();
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

static JSBool
obj_getOwnPropertyDescriptor(JSContext *cx, uintN argc, Value *vp)
{
    if (argc == 0 || !vp[2].isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    JSObject *obj = &vp[2].asObject();
    AutoIdRooter nameidr(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : Value(UndefinedTag()), nameidr.addr()))
        return JS_FALSE;
    return js_GetOwnPropertyDescriptor(cx, obj, nameidr.id(), vp);
}

static JSBool
obj_keys(JSContext *cx, uintN argc, Value *vp)
{
    const Value &v = argc == 0 ? Value(UndefinedTag()) : vp[2];
    if (!v.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return JS_FALSE;
    }

    AutoIdArray ida(cx, JS_Enumerate(cx, &v.asObject()));

    if (!ida)
        return JS_FALSE;

    JSObject *proto;
    if (!js_GetClassPrototype(cx, NULL, JSProto_Array, &proto))
        return JS_FALSE;
    vp[1].setObject(*proto);

    JS_ASSERT(ida.length() <= UINT32_MAX);
    JSObject *aobj = js_NewArrayWithSlots(cx, proto, uint32(ida.length()));
    if (!aobj)
        return JS_FALSE;
    vp->setObject(*aobj);

    size_t len = ida.length();
    JS_ASSERT(aobj->getDenseArrayCapacity() >= len);
    for (size_t i = 0; i < len; i++) {
        jsid id = ida[i];
        if (JSID_IS_INT(id)) {
            Value idval(Int32Tag(JSID_TO_INT(id)));
            jsid id;
            if (!js_ValueToStringId(cx, idval, &id))
                return JS_FALSE;
            aobj->setDenseArrayElement(i, StringTag(JSID_TO_STRING(id)));
        } else {
            






            aobj->setDenseArrayElement(i, IdToValue(id));
        }
    }

    JS_ASSERT(len <= UINT32_MAX);
    aobj->setDenseArrayCount(len);

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
  : pd(UndefinedTag()),
    id(INT_TO_JSID(0)),
    value(UndefinedTag()),
    get(UndefinedTag()),
    set(UndefinedTag()),
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
    JSObject* desc = &v.asObject();

    
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
    JS_ASSERT(obj->map->ops->lookupProperty == js_LookupProperty);
    if (!js_HasOwnProperty(cx, js_LookupProperty, obj, desc.id, &obj2, &current))
        return JS_FALSE;

    JS_ASSERT(obj->map->ops->defineProperty == js_DefineProperty);

    
    JSScope *scope = obj->scope();
    if (!current) {
        if (scope->sealed())
            return Reject(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(obj->map->ops->defineProperty == js_DefineProperty);
            return js_DefineProperty(cx, obj, desc.id, &desc.value,
                                     PropertyStub, PropertyStub, desc.attrs);
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        uintN dummyAttrs;
        JS_ASSERT(obj->map->ops->checkAccess == js_CheckAccess);
        if (!js_CheckAccess(cx, obj, desc.id, JSACC_WATCH, &dummy, &dummyAttrs))
            return JS_FALSE;

        Value tmp = UndefinedTag();
        return js_DefineProperty(cx, obj, desc.id, &tmp,
                                 desc.getter(), desc.setter(), desc.attrs);
    }

    
    Value v = UndefinedTag();

    







    JS_ASSERT(obj->getClass() == obj2->getClass());

    JSScopeProperty *sprop = reinterpret_cast<JSScopeProperty *>(current);
    do {
        if (desc.isAccessorDescriptor()) {
            if (!sprop->isAccessorDescriptor())
                break;

            if (desc.hasGet &&
                !SameValue(desc.getterValue(), sprop->getterOrUndefined(), cx)) {
                break;
            }

            if (desc.hasSet &&
                !SameValue(desc.setterValue(), sprop->setterOrUndefined(), cx)) {
                break;
            }
        } else {
            






            if (sprop->isDataDescriptor()) {
                






















                if (!sprop->configurable() &&
                    (!sprop->hasDefaultGetter() || !sprop->hasDefaultSetter())) {
                    return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                                  throwError, desc.id, rval);
                }

                if (!js_NativeGet(cx, obj, obj2, sprop, JSGET_NO_METHOD_BARRIER, &v)) {
                    
                    return JS_FALSE;
                }
            }

            if (desc.isDataDescriptor()) {
                if (!sprop->isDataDescriptor())
                    break;

                if (desc.hasValue && !SameValue(desc.value, v, cx))
                    break;
                if (desc.hasWritable && desc.writable() != sprop->writable())
                    break;
            } else {
                
                JS_ASSERT(desc.isGenericDescriptor());
            }
        }

        if (desc.hasConfigurable && desc.configurable() != sprop->configurable())
            break;
        if (desc.hasEnumerable && desc.enumerable() != sprop->enumerable())
            break;

        
        obj2->dropProperty(cx, current);
        *rval = true;
        return JS_TRUE;
    } while (0);

    
    if (!sprop->configurable()) {
        






        JS_ASSERT_IF(!desc.hasConfigurable, !desc.configurable());
        if (desc.configurable() ||
            (desc.hasEnumerable && desc.enumerable() != sprop->enumerable())) {
            return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP, throwError,
                          desc.id, rval);
        }
    }

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != sprop->isDataDescriptor()) {
        
        if (!sprop->configurable()) {
            return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                          throwError, desc.id, rval);
        }
    } else if (desc.isDataDescriptor()) {
        
        JS_ASSERT(sprop->isDataDescriptor());
        if (!sprop->configurable() && !sprop->writable()) {
            if ((desc.hasWritable && desc.writable()) ||
                (desc.hasValue && !SameValue(desc.value, v, cx))) {
                return Reject(cx, obj2, current, JSMSG_CANT_REDEFINE_UNCONFIGURABLE_PROP,
                              throwError, desc.id, rval);
            }
        }
    } else {
        
        JS_ASSERT(desc.isAccessorDescriptor() && sprop->isAccessorDescriptor());
        if (!sprop->configurable()) {
            if ((desc.hasSet &&
                 !SameValue(desc.setterValue(), sprop->setterOrUndefined(), cx)) ||
                (desc.hasGet &&
                 !SameValue(desc.getterValue(), sprop->getterOrUndefined(), cx))) {
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

        attrs = (sprop->attributes() & ~changed) | (desc.attrs & changed);
        if (sprop->isMethod()) {
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            getter = setter = PropertyStub;
        } else {
            getter = sprop->getter();
            setter = sprop->setter();
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
        attrs = (desc.attrs & ~unchanged) | (sprop->attributes() & unchanged);
        getter = setter = PropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        JS_ASSERT(obj2->map->ops->checkAccess == js_CheckAccess);
        if (!js_CheckAccess(cx, obj2, desc.id, JSACC_WATCH, &dummy, &attrs)) {
             obj2->dropProperty(cx, current);
             return JS_FALSE;
        }

        
        uintN changed = 0;
        if (desc.hasConfigurable)
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable)
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGet)
            changed |= JSPROP_GETTER | JSPROP_SHARED;
        if (desc.hasSet)
            changed |= JSPROP_SETTER | JSPROP_SHARED;

        attrs = (desc.attrs & changed) | (sprop->attributes() & ~changed);
        JS_ASSERT_IF(sprop->isMethod(), !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        if (desc.hasGet) {
            getter = desc.getter();
        } else {
            getter = (sprop->isMethod() || (sprop->hasDefaultGetter() && !sprop->hasGetterValue()))
                     ? PropertyStub
                     : sprop->getter();
        }
        if (desc.hasSet) {
            setter = desc.setter();
        } else {
            setter = (sprop->hasDefaultSetter() && !sprop->hasSetterValue())
                     ? PropertyStub
                     : sprop->setter();
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
            obj->setSlowArrayLength(index + 1);
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

    if (obj->map->ops->lookupProperty != js_LookupProperty) {
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
    
    const Value &v = (argc == 0) ? Value(UndefinedTag()) : vp[2];
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return JS_FALSE;
    }
    *vp = vp[2];
    JSObject* obj = &vp->asObject();

    
    AutoIdRooter nameidr(cx);
    if (!ValueToId(cx, argc >= 2 ? vp[3] : Value(UndefinedTag()), nameidr.addr()))
        return JS_FALSE;

    
    const Value &descval = argc >= 3 ? vp[4] : Value(UndefinedTag());

    
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
    
    if (argc < 2) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Object.defineProperties", "0", "s");
        return false;
    }

    *vp = vp[2];
    if (!vp->isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }

    if (!js_ValueToNonNullObject(cx, vp[3], &vp[3]))
        return false;
    JSObject *props = &vp[3].asObject();

    JSObject *obj = &vp->asObject();

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

    



    JSObject *obj =
        NewObjectWithGivenProto(cx, &js_ObjectClass, v.asObjectOrNull(), JS_GetScopeChain(cx));
    if (!obj)
        return JS_FALSE;
    vp->setObject(*obj); 

    
    if (argc > 1 && !vp[3].isUndefined()) {
        if (vp[3].isPrimitive()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
            return JS_FALSE;
        }

        JSObject *props = &vp[3].asObject();
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
    JS_FS_END
};

JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{
    if (argc > 0) {
        
        Value objv;
        if (!js_ValueToObjectOrNull(cx, argv[0], &objv))
            return JS_FALSE;
        if (!objv.isNull()) {
            *rval = objv;
            return JS_TRUE;
        }
    }

    JS_ASSERT(!argc || argv[0].isNull() || argv[0].isUndefined());
    if (cx->isConstructing())
        return JS_TRUE;

    JSObject *nullobj = NewObject(cx, &js_ObjectClass, NULL, NULL);
    if (!nullobj)
        return JS_FALSE;

    rval->setObject(*nullobj);
    return JS_TRUE;
}

#ifdef JS_TRACER

JSObject*
js_NewObjectWithClassProto(JSContext *cx, Class *clasp, JSObject *proto,
                           const Value &privateSlotValue)
{
    JS_ASSERT(!clasp->getObjectOps);
    JS_ASSERT(proto->map->ops == &js_ObjectOps);

    JSObject* obj = js_NewGCObject(cx);
    if (!obj)
        return NULL;

    obj->initSharingEmptyScope(clasp, ObjectTag(*proto), ObjectTag(*proto->getParent()), privateSlotValue);
    return obj;
}

JSObject* FASTCALL
js_Object_tn(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(js_ObjectClass.flags & JSCLASS_HAS_PRIVATE));
    return js_NewObjectWithClassProto(cx, &js_ObjectClass, proto, UndefinedTag());
}

JS_DEFINE_TRCINFO_1(js_Object,
    (2, (extern, CONSTRUCTOR_RETRY, js_Object_tn, CONTEXT, CALLEE_PROTOTYPE, 0,
         nanojit::ACC_STORE_ANY)))

JSObject* FASTCALL
js_NonEmptyObject(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(js_ObjectClass.flags & JSCLASS_HAS_PRIVATE));
    JSObject *obj = js_NewObjectWithClassProto(cx, &js_ObjectClass, proto, UndefinedTag());
    if (!obj)
        return NULL;
    JS_LOCK_OBJ(cx, obj);
    JSScope *scope = js_GetMutableScope(cx, obj);
    if (!scope) {
        JS_UNLOCK_OBJ(cx, obj);
        return NULL;
    }

    



    JS_UNLOCK_SCOPE(cx, scope);
    return obj;
}

JS_DEFINE_CALLINFO_2(extern, CONSTRUCTOR_RETRY, js_NonEmptyObject, CONTEXT, CALLEE_PROTOTYPE, 0,
                     nanojit::ACC_STORE_ANY)


static inline JSObject*
NewNativeObject(JSContext* cx, Class* clasp, JSObject* proto,
                JSObject *parent, const Value &privateSlotValue)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    JSObject* obj = js_NewGCObject(cx);
    if (!obj)
        return NULL;

    obj->init(clasp, ObjectTag(*proto), ObjectTag(*parent), privateSlotValue);
    return InitScopeForObject(cx, obj, clasp, proto, &js_ObjectOps) ? obj : NULL;
}

JSObject* FASTCALL
js_NewInstance(JSContext *cx, Class *clasp, JSObject *ctor)
{
    JS_ASSERT(ctor->isFunction());

    JSAtom *atom = cx->runtime->atomState.classPrototypeAtom;

    JSScope *scope = ctor->scope();
#ifdef JS_THREADSAFE
    if (scope->title.ownercx != cx)
        return NULL;
#endif
    if (scope->isSharedEmpty()) {
        scope = js_GetMutableScope(cx, ctor);
        if (!scope)
            return NULL;
    }

    JSScopeProperty *sprop = scope->lookup(ATOM_TO_JSID(atom));
    Value pval = sprop ? ctor->getSlot(sprop->slot) : JSWhyMagic(JS_NO_CONSTANT);

    JSObject *proto;
    if (!pval.isPrimitive()) {
        
        proto = &pval.asObject();
    } else if (pval.isMagic(JS_NO_CONSTANT)) {
        
        proto = NewObject(cx, clasp, NULL, ctor->getParent());
        if (!proto)
            return NULL;
        if (!js_SetClassPrototype(cx, ctor, proto, JSPROP_ENUMERATE | JSPROP_PERMANENT))
            return NULL;
    } else {
        
        if (!js_GetClassPrototype(cx, &ctor->fslots[JSSLOT_PARENT].asObject(),
                                  JSProto_Object, &proto)) {
            return NULL;
        }
    }

    return NewNativeObject(cx, clasp, proto, ctor->getParent(),
                           JSObject::defaultPrivate(clasp));
}

JS_DEFINE_CALLINFO_3(extern, CONSTRUCTOR_RETRY, js_NewInstance, CONTEXT, CLASS, OBJECT, 0,
                     nanojit::ACC_STORE_ANY)

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

    script = cx->fp->script;
    endpc = script->code + script->length;
    for (;; pc += js_CodeSpec[op].length) {
        JS_ASSERT_IF(!cx->fp->imacpc, script->code <= pc && pc < endpc);

        
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
    cs = &js_CodeSpec[js_GetOpcode(cx, fp->script, pc)];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if ((format & (JOF_SET | JOF_FOR)) ||
        (fp->flags & JSFRAME_ASSIGNING)) {
        flags |= JSRESOLVE_ASSIGNING;
    } else if (cs->length >= 0) {
        pc += cs->length;
        if (pc < cx->fp->script->code + cx->fp->script->length && Detecting(cx, pc))
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
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_LookupProperty(cx, obj, id, objp, propp);
    return proto->lookupProperty(cx, id, objp, propp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_GetProperty(cx, obj, id, vp);
    return proto->getProperty(cx, id, vp);
}

static JSBool
with_SetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_SetProperty(cx, obj, id, vp);
    return proto->setProperty(cx, id, vp);
}

static JSBool
with_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_GetAttributes(cx, obj, id, attrsp);
    return proto->getAttributes(cx, id, attrsp);
}

static JSBool
with_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_SetAttributes(cx, obj, id, attrsp);
    return proto->setAttributes(cx, id, attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_DeleteProperty(cx, obj, id, rval);
    return proto->deleteProperty(cx, id, rval);
}

static JSBool
with_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_DefaultValue(cx, obj, hint, vp);
    return proto->defaultValue(cx, hint, vp);
}

static JSBool
with_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
               Value *statep, jsid *idp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_Enumerate(cx, obj, enum_op, statep, idp);
    return proto->enumerate(cx, enum_op, statep, idp);
}

static JSBool
with_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                 Value *vp, uintN *attrsp)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return js_CheckAccess(cx, obj, id, mode, vp, attrsp);
    return proto->checkAccess(cx, id, mode, vp, attrsp);
}

static JSType
with_TypeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_OBJECT;
}

static JSObject *
with_ThisObject(JSContext *cx, JSObject *obj)
{
    JSObject *proto = obj->getProto();
    if (!proto)
        return obj;
    return proto->thisObject(cx);
}

JS_FRIEND_DATA(JSObjectOps) js_WithObjectOps = {
    NULL,
    with_LookupProperty,
    js_DefineProperty,
    with_GetProperty,
    with_SetProperty,
    with_GetAttributes,
    with_SetAttributes,
    with_DeleteProperty,
    with_DefaultValue,
    with_Enumerate,
    with_CheckAccess,
    with_TypeOf,
    js_TraceObject,
    with_ThisObject,
    NULL,   
    NULL,   
    NULL,   
    js_Clear
};

static JSObjectOps *
with_getObjectOps(JSContext *cx, Class *clasp)
{
    return &js_WithObjectOps;
}

Class js_WithClass = {
    "With",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_IS_ANONYMOUS,
    PropertyStub,     PropertyStub,     PropertyStub,     PropertyStub,
    EnumerateStub,    ResolveStub,      ConvertStub,      NULL,
    with_getObjectOps,
    0,0,0,0,0,0,0
};

JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth)
{
    JSObject *obj;

    obj = NewObject(cx, &js_WithClass, proto, parent);
    if (!obj)
        return NULL;
    obj->setPrivate(js_FloatingFrameIfGenerator(cx, cx->fp));
    OBJ_SET_BLOCK_DEPTH(cx, obj, depth);
    return obj;
}

JSObject *
js_NewBlockObject(JSContext *cx)
{
    



    JSObject *blockObj = NewObjectWithGivenProto(cx, &js_BlockClass, NULL, NULL);
    JS_ASSERT_IF(blockObj, !OBJ_IS_CLONED_BLOCK(blockObj));
    return blockObj;
}

JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp)
{
    JS_ASSERT(!OBJ_IS_CLONED_BLOCK(proto));
    JS_ASSERT(proto->getClass() == &js_BlockClass);

    JSObject *clone = js_NewGCObject(cx);
    if (!clone)
        return NULL;

    Value privateValue = PrivateTag(js_FloatingFrameIfGenerator(cx, fp));

    
    clone->init(&js_BlockClass, ObjectTag(*proto), NullTag(), privateValue);
    clone->fslots[JSSLOT_BLOCK_DEPTH] = proto->fslots[JSSLOT_BLOCK_DEPTH];

    JS_ASSERT(cx->runtime->emptyBlockScope->freeslot == JSSLOT_BLOCK_DEPTH + 1);
    clone->map = cx->runtime->emptyBlockScope->hold();
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(clone));
    return clone;
}

JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind)
{
    
    JS_STATIC_ASSERT(JS_INITIAL_NSLOTS == JSSLOT_BLOCK_DEPTH + 2);

    JSStackFrame *const fp = cx->fp;
    JSObject *obj = fp->scopeChainObj();
    JS_ASSERT(obj->getClass() == &js_BlockClass);
    JS_ASSERT(obj->getPrivate() == js_FloatingFrameIfGenerator(cx, cx->fp));
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(obj));

    





    JS_ASSERT(obj->scope()->object != obj);

    
    JS_ASSERT(obj->numSlots() == JS_INITIAL_NSLOTS);

    
    uintN depth = OBJ_BLOCK_DEPTH(cx, obj);
    uintN count = OBJ_BLOCK_COUNT(cx, obj);
    JS_ASSERT(depth <= (size_t) (cx->regs->sp - fp->base()));
    JS_ASSERT(count <= (size_t) (cx->regs->sp - fp->base() - depth));

    
    JS_ASSERT(count >= 1);

    depth += fp->script->nfixed;
    obj->fslots[JSSLOT_BLOCK_DEPTH + 1] = fp->slots()[depth];
    if (normalUnwind && count > 1) {
        --count;
        JS_LOCK_OBJ(cx, obj);
        if (!obj->allocSlots(cx, JS_INITIAL_NSLOTS + count))
            normalUnwind = JS_FALSE;
        else
            memcpy(obj->dslots, fp->slots() + depth + 1, count * sizeof(Value));
        JS_UNLOCK_OBJ(cx, obj);
    }

    
    obj->setPrivate(NULL);
    fp->setScopeChainObj(obj->getParent());
    return normalUnwind;
}

static JSBool
block_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    




    JS_ASSERT(obj->getClass() == &js_BlockClass);
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(obj));
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->script->nfixed + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->script->nslots);
        *vp = fp->slots()[index];
        return true;
    }

    
    uint32 slot = JSSLOT_BLOCK_DEPTH + 1 + index;
    JS_LOCK_OBJ(cx, obj);
    JS_ASSERT(slot < obj->numSlots());
    *vp = obj->getSlot(slot);
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

static JSBool
block_setProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(obj->getClass() == &js_BlockClass);
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(obj));
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->script->nfixed + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->script->nslots);
        fp->slots()[index] = *vp;
        return true;
    }

    
    uint32 slot = JSSLOT_BLOCK_DEPTH + 1 + index;
    JS_LOCK_OBJ(cx, obj);
    JS_ASSERT(slot < obj->numSlots());
    obj->setSlot(slot, *vp);
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

JSBool
js_DefineBlockVariable(JSContext *cx, JSObject *obj, jsid id, intN index)
{
    JS_ASSERT(obj->getClass() == &js_BlockClass);
    JS_ASSERT(!OBJ_IS_CLONED_BLOCK(obj));

    
    return js_DefineNativeProperty(cx, obj, id, Value(UndefinedTag()),
                                   block_getProperty,
                                   block_setProperty,
                                   JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED,
                                   JSScopeProperty::HAS_SHORTID, index, NULL);
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
    
    bool thisOwns = this->isNative() && scope()->object == this;
    bool otherOwns = other->isNative() && other->scope()->object == other;

    size_t size = GetObjectSize(this);
    JS_ASSERT(size == GetObjectSize(other));

    
    char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject)>::result];
    memcpy(tmp, this, size);
    memcpy(this, other, size);
    memcpy(other, tmp, size);

    
    if (otherOwns)
        scope()->object = this;
    if (thisOwns)
        other->scope()->object = other;
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
    uint16 depth, count, i;
    uint32 tmp;
    JSScopeProperty *sprop;
    jsid propid;
    JSAtom *atom;
    int16 shortid;
    JSBool ok;

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
        tmp = (uint32)(depth << 16) | count;
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
        obj->setParent(ObjectOrNullTag(parent));
    }

    AutoObjectRooter tvr(cx, obj);

    if (!JS_XDRUint32(xdr, &tmp))
        return false;

    if (xdr->mode == JSXDR_DECODE) {
        depth = (uint16)(tmp >> 16);
        count = (uint16)tmp;
        obj->setSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Tag(depth)));
    }

    




    sprop = NULL;
    ok = JS_TRUE;
    for (i = 0; i < count; i++) {
        if (xdr->mode == JSXDR_ENCODE) {
            
            do {
                
                sprop = sprop ? sprop->parent : obj->scope()->lastProperty();
            } while (!sprop->hasShortID());

            JS_ASSERT(sprop->getter() == block_getProperty);
            propid = sprop->id;
            JS_ASSERT(JSID_IS_ATOM(propid));
            atom = JSID_TO_ATOM(propid);
            shortid = sprop->shortid;
            JS_ASSERT(shortid >= 0);
        }

        
        if (!js_XDRStringAtom(xdr, &atom) ||
            !JS_XDRUint16(xdr, (uint16 *)&shortid)) {
            return false;
        }

        if (xdr->mode == JSXDR_DECODE) {
            if (!js_DefineBlockVariable(cx, obj, ATOM_TO_JSID(atom), shortid))
                return false;
        }
    }
    return true;
}

#endif

static uint32
block_reserveSlots(JSContext *cx, JSObject *obj)
{
    return OBJ_IS_CLONED_BLOCK(obj) ? OBJ_BLOCK_COUNT(cx, obj) : 0;
}

Class js_BlockClass = {
    "Block",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_IS_ANONYMOUS,
    PropertyStub,     PropertyStub,     PropertyStub,      PropertyStub,
    EnumerateStub,    ResolveStub,      ConvertStub,       NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, block_reserveSlots
};

JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = js_InitClass(cx, obj, NULL, &js_ObjectClass, js_Object, 1,
                                   object_props, object_methods, NULL, object_static_methods);
    if (!proto)
        return NULL;

    
    if (!js_DefineFunction(cx, obj, cx->runtime->atomState.evalAtom,
                           (Native)obj_eval, 1,
                           JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS)) {
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

        JSScope *scope = js_GetMutableScope(cx, obj);
        if (!scope) {
            JS_UNLOCK_OBJ(cx, obj);
            return false;
        }

        JSScopeProperty *sprop = scope->lookup(id);
        if (!sprop) {
            uint32 index = 2 * JSProto_LIMIT + key;
            if (!js_SetReservedSlot(cx, obj, index, v)) {
                JS_UNLOCK_SCOPE(cx, scope);
                return false;
            }

            uint32 slot = JSSLOT_START(obj->getClass()) + index;
            sprop = scope->addProperty(cx, id, PropertyStub, PropertyStub,
                                       slot, attrs, 0, 0);

            JS_UNLOCK_SCOPE(cx, scope);
            if (!sprop)
                return false;

            named = true;
            return true;
        }
        JS_UNLOCK_SCOPE(cx, scope);
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

    
    JSObject *proto = NewObject(cx, clasp, parent_proto, obj);
    if (!proto)
        return NULL;

    
    AutoValueRooter tvr(cx, ObjectTag(*proto));

    JSObject *ctor;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) ||
            !(obj->getClass()->flags & JSCLASS_IS_GLOBAL) ||
            key == JSProto_Null)
        {
            uint32 attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            if (!DefineStandardSlot(cx, obj, key, atom, tvr.value(), attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        fun = js_NewFunction(cx, NULL, constructor, nargs, 0, obj, atom);
        if (!fun)
            goto bad;

        AutoValueRooter tvr2(cx, ObjectTag(*fun));
        if (!DefineStandardSlot(cx, obj, key, atom, tvr2.value(), 0, named))
            goto bad;

        




        FUN_CLASP(fun) = clasp;

        





        ctor = FUN_OBJECT(fun);
        if (clasp->flags & JSCLASS_CONSTRUCT_PROTOTYPE) {
            Value rval;
            if (!InternalConstruct(cx, proto, ObjectOrNullTag(ctor), 0, NULL, &rval))
                goto bad;
            if (rval.isObject() && &rval.asObject() != proto)
                proto = &rval.asObject();
        }

        
        if (!js_SetClassPrototype(cx, ctor, proto,
                                  JSPROP_READONLY | JSPROP_PERMANENT)) {
            goto bad;
        }

        
        if (ctor->getClass() == clasp)
            ctor->setProto(ObjectOrNullTag(proto));
    }

    
    if ((ps && !JS_DefineProperties(cx, proto, ps)) ||
        (fs && !JS_DefineFunctions(cx, proto, fs)) ||
        (static_ps && !JS_DefineProperties(cx, ctor, static_ps)) ||
        (static_fs && !JS_DefineFunctions(cx, ctor, static_fs))) {
        goto bad;
    }

    







    if (!proto->scope()->ensureEmptyScope(cx, clasp))
        goto bad;

    
    if (key != JSProto_Null && !js_SetClassObject(cx, obj, key, ctor, proto))
        goto bad;

    return proto;

bad:
    if (named) {
        Value rval;
        obj->deleteProperty(cx, ATOM_TO_JSID(atom), &rval);
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
        return JS_TRUE;

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

    size_t oldnslots = dslots[-1].asPrivateUint32();

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

    JS_ASSERT(dslots[-1].asPrivateUint32() > JS_INITIAL_NSLOTS);
    JS_ASSERT(nslots <= dslots[-1].asPrivateUint32());

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
js_EnsureReservedSlots(JSContext *cx, JSObject *obj, size_t nreserved)
{
    JS_ASSERT(obj->isNative());

    uintN nslots = JSSLOT_FREE(obj->getClass()) + nreserved;
    if (nslots > obj->numSlots() && !obj->allocSlots(cx, nslots))
        return false;

    JSScope *scope = obj->scope();
    if (!scope->isSharedEmpty()) {
#ifdef JS_THREADSAFE
        JS_ASSERT(scope->title.ownercx->thread == cx->thread);
#endif
        JS_ASSERT(scope->freeslot == JSSLOT_FREE(obj->getClass()));
        if (scope->freeslot < nslots)
            scope->freeslot = nslots;
    }
    return true;
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
        *objp = &v.asObject();
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
                cobj = &v.asObject();
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

    return js_SetReservedSlot(cx, obj, key, ObjectOrNullTag(cobj)) &&
           js_SetReservedSlot(cx, obj, JSProto_LIMIT + key, ObjectOrNullTag(proto));
}

JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey protoKey,
                   Value *vp, Class *clasp)
{
    JSStackFrame *fp;
    JSObject *obj, *cobj, *pobj;
    jsid id;
    JSProperty *prop;
    JSScopeProperty *sprop;

    




    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (!start && (fp = cx->fp) != NULL)
        start = fp->scopeChainObj();

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

    Innerize(cx, &obj);
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
    Value v = UndefinedTag();
    if (prop && pobj->isNative()) {
        sprop = (JSScopeProperty *) prop;
        if (SPROP_HAS_VALID_SLOT(sprop, pobj->scope())) {
            v = pobj->lockedGetSlot(sprop->slot);
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

    



    JSObject *ctor = &cval.asObject();
    if (!parent)
        parent = ctor->getParent();
    if (!proto) {
        Value rval;
        if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                               &rval)) {
            return NULL;
        }
        if (rval.isObjectOrNull())
            proto = rval.asObjectOrNull();
    }

    JSObject *obj = NewObject(cx, clasp, proto, parent);
    if (!obj)
        return NULL;

    Value rval;
    if (!InternalConstruct(cx, obj, cval, argc, argv, &rval))
        return NULL;

    if (rval.isPrimitive())
        return obj;

    






    obj = &rval.asObject();
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





JSBool
js_AllocSlot(JSContext *cx, JSObject *obj, uint32 *slotp)
{
    JSScope *scope = obj->scope();
    JS_ASSERT(scope->object == obj);

    Class *clasp = obj->getClass();
    if (scope->freeslot == JSSLOT_FREE(clasp) && clasp->reserveSlots) {
        
        scope->freeslot += clasp->reserveSlots(cx, obj);
    }

    if (scope->freeslot >= obj->numSlots() &&
        !obj->growSlots(cx, scope->freeslot + 1)) {
        return JS_FALSE;
    }

    
    JS_ASSERT(obj->getSlot(scope->freeslot).isUndefined());
    *slotp = scope->freeslot++;
    return JS_TRUE;
}

void
js_FreeSlot(JSContext *cx, JSObject *obj, uint32 slot)
{
    JSScope *scope = obj->scope();
    JS_ASSERT(scope->object == obj);
    obj->lockedSetSlot(slot, Value(UndefinedTag()));
    if (scope->freeslot == slot + 1)
        scope->freeslot = slot;
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
    JSScope *scope;
    JSScopeProperty *sprop;

    while (obj) {
        if (!obj->isNative()) {
            obj = obj->getProto();
            continue;
        }
        JS_LOCK_OBJ(cx, obj);
        scope = obj->scope();
        sprop = scope->lookup(id);
        if (sprop) {
            PCMETER(JS_PROPERTY_CACHE(cx).pcpurges++);
            scope->shadowingShapeChange(cx, sprop);
            JS_UNLOCK_SCOPE(cx, scope);

            if (!obj->getParent()) {
                




                LeaveTrace(cx);
            }
            return JS_TRUE;
        }
        obj = obj->getProto();
        JS_UNLOCK_SCOPE(cx, scope);
    }
    return JS_FALSE;
}

void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id)
{
    JS_ASSERT(obj->isDelegate());
    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->getClass() == &js_CallClass) {
        while ((obj = obj->getParent()) != NULL) {
            if (PurgeProtoChain(cx, obj, id))
                break;
        }
    }
}

JSScopeProperty *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     PropertyOp getter, PropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid)
{
    JSScope *scope;
    JSScopeProperty *sprop;

    JS_ASSERT(!(flags & JSScopeProperty::METHOD));

    




    js_PurgeScopeChain(cx, obj, id);

    JS_LOCK_OBJ(cx, obj);
    scope = js_GetMutableScope(cx, obj);
    if (!scope) {
        sprop = NULL;
    } else {
        
        id = js_CheckForStringIndex(id);
        sprop = scope->putProperty(cx, id, getter, setter, slot, attrs, flags, shortid);
    }
    JS_UNLOCK_OBJ(cx, obj);
    return sprop;
}

JSScopeProperty *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             JSScopeProperty *sprop, uintN attrs, uintN mask,
                             PropertyOp getter, PropertyOp setter)
{
    JSScope *scope;

    JS_LOCK_OBJ(cx, obj);
    scope = js_GetMutableScope(cx, obj);
    if (!scope) {
        sprop = NULL;
    } else {
        sprop = scope->changeProperty(cx, sprop, attrs, mask, getter, setter);
    }
    JS_UNLOCK_OBJ(cx, obj);
    return sprop;
}

JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                  PropertyOp getter, PropertyOp setter, uintN attrs)
{
    return js_DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs,
                                   0, 0, NULL);
}







static inline bool
AddPropertyHelper(JSContext *cx, Class *clasp, JSObject *obj, JSScope *scope,
                  JSScopeProperty *sprop, Value *vp)
{
    if (clasp->addProperty != PropertyStub) {
        Value nominal = *vp;

        if (!clasp->addProperty(cx, obj, SPROP_USERID(sprop), vp))
            return false;
        if (*vp != nominal) {
            if (SPROP_HAS_VALID_SLOT(sprop, scope))
                obj->lockedSetSlot(sprop->slot, *vp);
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
    JSScope *scope;
    JSScopeProperty *sprop;
    JSBool added;
    Value valueCopy;

    JS_ASSERT((defineHow & ~(JSDNP_CACHE_RESULT | JSDNP_DONT_PURGE | JSDNP_SET_METHOD)) == 0);
    LeaveTraceIfGlobalObject(cx, obj);

    
    id = js_CheckForStringIndex(id);

    




    sprop = NULL;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        







        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return JS_FALSE;
        sprop = (JSScopeProperty *) prop;
        if (sprop && pobj == obj && sprop->isAccessorDescriptor()) {
            sprop = obj->scope()->changeProperty(cx, sprop, attrs,
                                                 JSPROP_GETTER | JSPROP_SETTER,
                                                 (attrs & JSPROP_GETTER)
                                                 ? getter
                                                 : sprop->getter(),
                                                 (attrs & JSPROP_SETTER)
                                                 ? setter
                                                 : sprop->setter());

            
            if (!sprop)
                goto error;
        } else if (prop) {
            pobj->dropProperty(cx, prop);
            prop = NULL;
            sprop = NULL;
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

    
    scope = js_GetMutableScope(cx, obj);
    if (!scope)
        goto error;

    added = false;
    if (!sprop) {
        
        if (defineHow & JSDNP_SET_METHOD) {
            JS_ASSERT(clasp == &js_ObjectClass);
            JS_ASSERT(IsFunctionObject(value));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            JS_ASSERT(!getter && !setter);

            JSObject *funobj = &value.asObject();
            if (FUN_OBJECT(GET_FUNCTION_PRIVATE(cx, funobj)) == funobj) {
                flags |= JSScopeProperty::METHOD;
                getter = CastAsPropertyOp(funobj);
            }
        }

        added = !scope->hasProperty(id);
        uint32 oldShape = scope->shape;
        sprop = scope->putProperty(cx, id, getter, setter, SPROP_INVALID_SLOT,
                                   attrs, flags, shortid);
        if (!sprop)
            goto error;

        





        if (scope->shape == oldShape && scope->branded() && sprop->slot != SPROP_INVALID_SLOT)
            scope->methodWriteBarrier(cx, sprop->slot, value);
    }

    
    if (SPROP_HAS_VALID_SLOT(sprop, scope))
        obj->lockedSetSlot(sprop->slot, value);

    
    valueCopy = value;
    if (!AddPropertyHelper(cx, clasp, obj, scope, sprop, &valueCopy)) {
        scope->removeProperty(cx, id);
        goto error;
    }

    if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
        JS_ASSERT_NOT_ON_TRACE(cx);
        PropertyCacheEntry *entry =
#endif
            JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, 0, obj, sprop, added);
        TRACE_2(SetPropHit, entry, sprop);
    }
    if (propp)
        *propp = (JSProperty *) sprop;
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
    JSScope *scope = obj->scope();

    







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
    JSScopeProperty *sprop = NULL;
    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = (JSNewResolveOp)resolve;
        if (flags == JSRESOLVE_INFER)
            flags = js_InferFlags(cx, flags);
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
                







                scope = obj2->scope();
                if (!scope->isSharedEmpty())
                    sprop = scope->lookup(id);
            }
            if (sprop) {
                JS_ASSERT(scope == obj2->scope());
                JS_ASSERT(!scope->isSharedEmpty());
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
        scope = obj->scope();
        if (!scope->isSharedEmpty())
            sprop = scope->lookup(id);
    }

cleanup:
    if (ok && sprop) {
        JS_ASSERT(obj->scope() == scope);
        *objp = obj;
        *propp = (JSProperty *) sprop;
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
        JSScopeProperty *sprop = obj->scope()->lookup(id);
        if (sprop) {
            SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
            *objp = obj;
            *propp = (JSProperty *) sprop;
            return protoIndex;
        }

        
        if (!sprop && obj->getClass()->resolve != JS_ResolveStub) {
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
    scopeChain = js_GetTopStackFrame(cx)->scopeChainObj();

    
    entry = JS_NO_PROP_CACHE_FILL;
    obj = scopeChain;
    parent = obj->getParent();
    for (scopeIndex = 0;
         parent
         ? js_IsCacheableNonGlobalScope(obj)
         : obj->map->ops->lookupProperty == js_LookupProperty;
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
                    



                    JS_ASSERT(pobj == obj->getProto());
                    JS_ASSERT(protoIndex == 1);
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
                                                   (JSScopeProperty *) prop);
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
                                           (JSScopeProperty *) prop);
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
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj,
             JSScopeProperty *sprop, uintN getHow, Value *vp)
{
    LeaveTraceIfGlobalObject(cx, pobj);

    JSScope *scope;
    uint32 slot;
    int32 sample;

    JS_ASSERT(pobj->isNative());
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, pobj));
    scope = pobj->scope();

    slot = sprop->slot;
    if (slot != SPROP_INVALID_SLOT)
        *vp = pobj->lockedGetSlot(slot);
    else
        vp->setUndefined();
    if (sprop->hasDefaultGetter())
        return true;

    if (JS_UNLIKELY(sprop->isMethod()) && (getHow & JSGET_NO_METHOD_BARRIER)) {
        JS_ASSERT(&sprop->methodObject() == &vp->asObject());
        return true;
    }

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_SCOPE(cx, scope);
    {
        AutoScopePropertyRooter tvr(cx, sprop);
        AutoObjectRooter tvr2(cx, pobj);
        if (!sprop->get(cx, obj, pobj, vp))
            return false;
    }
    JS_LOCK_SCOPE(cx, scope);

    if (SLOT_IN_SCOPE(slot, scope) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         scope->hasProperty(sprop))) {
        if (!scope->methodWriteBarrier(cx, sprop, *vp)) {
            JS_UNLOCK_SCOPE(cx, scope);
            return false;
        }
        pobj->lockedSetSlot(slot, *vp);
    }

    return true;
}

JSBool
js_NativeSet(JSContext *cx, JSObject *obj, JSScopeProperty *sprop, bool added,
             Value *vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    JSScope *scope;
    uint32 slot;
    int32 sample;

    JS_ASSERT(obj->isNative());
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, obj));
    scope = obj->scope();

    slot = sprop->slot;
    if (slot != SPROP_INVALID_SLOT) {
        OBJ_CHECK_SLOT(obj, slot);

        
        if (sprop->hasDefaultSetter()) {
            if (!added && !scope->methodWriteBarrier(cx, sprop, *vp)) {
                JS_UNLOCK_SCOPE(cx, scope);
                return false;
            }
            obj->lockedSetSlot(slot, *vp);
            return true;
        }
    } else {
        





        if (!sprop->hasGetterValue() && sprop->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_SCOPE(cx, scope);
    {
        AutoScopePropertyRooter tvr(cx, sprop);
        if (!sprop->set(cx, obj, vp))
            return false;
    }

    JS_LOCK_SCOPE(cx, scope);
    if (SLOT_IN_SCOPE(slot, scope) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         scope->hasProperty(sprop))) {
        if (!added && !scope->methodWriteBarrier(cx, sprop, *vp)) {
            JS_UNLOCK_SCOPE(cx, scope);
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
    JSScopeProperty *sprop;

    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, !JS_ON_TRACE(cx));

    
    id = js_CheckForStringIndex(id);

    aobj = js_GetProtoIfDenseArray(obj);
    protoIndex = js_LookupPropertyWithFlags(cx, aobj, id, cx->resolveFlags,
                                            &obj2, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (!prop) {
        vp->setUndefined();

        if (!obj->getClass()->getProperty(cx, obj, id, vp))
            return JS_FALSE;

        PCMETER(getHow & JSGET_CACHE_RESULT && JS_PROPERTY_CACHE(cx).nofills++);

        



        jsbytecode *pc;
        if (vp->isUndefined() && ((pc = js_GetCurrentBytecodePC(cx)) != NULL)) {
            JSOp op;
            uintN flags;

            op = (JSOp) *pc;
            if (op == JSOP_TRAP) {
                JS_ASSERT_NOT_ON_TRACE(cx);
                op = JS_GetTrapOpcode(cx, cx->fp->script, pc);
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

    sprop = (JSScopeProperty *) prop;

    if (getHow & JSGET_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        JS_PROPERTY_CACHE(cx).fill(cx, aobj, 0, protoIndex, obj2, sprop);
    }

    if (!js_NativeGet(cx, obj, obj2, sprop, getHow, vp))
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

    if (obj->map->ops == &js_ObjectOps ||
        obj->map->ops->getProperty == js_GetProperty) {
        return js_GetPropertyHelper(cx, obj, id, getHow, vp);
    }
    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, obj->isDenseArray());
#if JS_HAS_XML_SUPPORT
    if (obj->isXML())
        return js_GetXMLMethod(cx, obj, id, vp);
#endif
    return obj->getProperty(cx, id, vp);
}

JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    JSStackFrame *const fp = js_GetTopStackFrame(cx);
    if (!fp)
        return true;

    
    if (!(fp->script && fp->script->strictModeCode) &&
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

}






JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     Value *vp)
{
    int protoIndex;
    JSObject *pobj;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSScope *scope;
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

    



    if (obj->scope()->sealed() && obj->scope()->object == obj)
        return ReportReadOnly(cx, id, JSREPORT_ERROR);

    protoIndex = js_LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags,
                                            &pobj, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (prop) {
        if (!pobj->isNative())
            prop = NULL;
    } else {
        
        JS_ASSERT(obj->getClass() != &js_BlockClass);

        if (!obj->getParent() &&
            (defineHow & JSDNP_UNQUALIFIED) &&
            !js_CheckUndeclaredVarAssignment(cx, JSID_TO_STRING(id))) {
            return JS_FALSE;
        }
    }
    sprop = (JSScopeProperty *) prop;

    







    attrs = JSPROP_ENUMERATE;
    flags = 0;
    shortid = 0;
    clasp = obj->getClass();
    getter = clasp->getProperty;
    setter = clasp->setProperty;

    if (sprop) {
        




        scope = pobj->scope();

        
        if (sprop->isAccessorDescriptor()) {
            if (sprop->hasDefaultSetter()) {
                JS_UNLOCK_SCOPE(cx, scope);
                if (defineHow & JSDNP_CACHE_RESULT)
                    TRACE_2(SetPropHit, JS_NO_PROP_CACHE_FILL, sprop);
                return js_ReportGetterOnlyAssignment(cx);
            }
        } else {
            JS_ASSERT(sprop->isDataDescriptor());

            if (!sprop->writable()) {
                JS_UNLOCK_SCOPE(cx, scope);

                PCMETER((defineHow & JSDNP_CACHE_RESULT) && JS_PROPERTY_CACHE(cx).rofills++);
                if (defineHow & JSDNP_CACHE_RESULT) {
                    JS_ASSERT_NOT_ON_TRACE(cx);
                    TRACE_2(SetPropHit, JS_NO_PROP_CACHE_FILL, sprop);
                }

                
                if (JS_HAS_STRICT_OPTION(cx))
                    return ReportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return JS_TRUE;

#ifdef JS_TRACER
              error: 
                return JS_FALSE;
#endif
            }
        }
        if (scope->sealed() && !sprop->hasSlot()) {
            JS_UNLOCK_SCOPE(cx, scope);
            return ReportReadOnly(cx, id, JSREPORT_ERROR);
        }

        attrs = sprop->attributes();
        if (pobj != obj) {
            






            JS_UNLOCK_SCOPE(cx, scope);

            
            if (!sprop->hasSlot()) {
                if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
                    JS_ASSERT_NOT_ON_TRACE(cx);
                    PropertyCacheEntry *entry =
#endif
                        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, protoIndex, pobj, sprop);
                    TRACE_2(SetPropHit, entry, sprop);
                }

                if (sprop->hasDefaultSetter() && !sprop->hasGetterValue())
                    return JS_TRUE;

                return sprop->set(cx, obj, vp);
            }

            
            attrs = JSPROP_ENUMERATE;

            






            if (sprop->hasShortID()) {
                flags = JSScopeProperty::HAS_SHORTID;
                shortid = sprop->shortid;
                getter = sprop->getter();
                setter = sprop->setter();
            }

            



            sprop = NULL;
        }
#ifdef __GNUC__ 
    } else {
        scope = NULL;
#endif
    }

    added = false;
    if (!sprop) {
        



        js_PurgeScopeChain(cx, obj, id);

        
        JS_LOCK_OBJ(cx, obj);
        scope = js_GetMutableScope(cx, obj);
        if (!scope) {
            JS_UNLOCK_OBJ(cx, obj);
            return JS_FALSE;
        }

        



        if ((defineHow & JSDNP_SET_METHOD) &&
            obj->getClass() == &js_ObjectClass) {
            JS_ASSERT(IsFunctionObject(*vp));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

            JSObject *funobj = &vp->asObject();
            if (FUN_OBJECT(GET_FUNCTION_PRIVATE(cx, funobj)) == funobj) {
                flags |= JSScopeProperty::METHOD;
                getter = CastAsPropertyOp(funobj);
            }
        }

        sprop = scope->putProperty(cx, id, getter, setter, SPROP_INVALID_SLOT,
                                   attrs, flags, shortid);
        if (!sprop) {
            JS_UNLOCK_SCOPE(cx, scope);
            return JS_FALSE;
        }

        




        if (SPROP_HAS_VALID_SLOT(sprop, scope))
            obj->lockedSetSlot(sprop->slot, Value(UndefinedTag()));

        
        if (!AddPropertyHelper(cx, clasp, obj, scope, sprop, vp)) {
            scope->removeProperty(cx, id);
            JS_UNLOCK_SCOPE(cx, scope);
            return JS_FALSE;
        }
        added = true;
    }

    if (defineHow & JSDNP_CACHE_RESULT) {
#ifdef JS_TRACER
        JS_ASSERT_NOT_ON_TRACE(cx);
        PropertyCacheEntry *entry =
#endif
            JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, 0, obj, sprop, added);
        TRACE_2(SetPropHit, entry, sprop);
    }

    if (!js_NativeSet(cx, obj, sprop, added, vp))
        return NULL;

    JS_UNLOCK_SCOPE(cx, scope);
    return JS_TRUE;
}

JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return js_SetPropertyHelper(cx, obj, id, 0, vp);
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

    JSScopeProperty *sprop = (JSScopeProperty *)prop;
    *attrsp = sprop->attributes();
    JS_UNLOCK_OBJ(cx, obj);
    return true;
}

JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, JSScopeProperty *sprop,
                       uintN attrs)
{
    JS_ASSERT(obj->isNative());
    sprop = js_ChangeNativePropertyAttrs(cx, obj, sprop, attrs, 0,
                                         sprop->getter(), sprop->setter());
    JS_UNLOCK_OBJ(cx, obj);
    return (sprop != NULL);
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
           ? js_SetNativeAttributes(cx, obj, (JSScopeProperty *) prop, *attrsp)
           : obj->setAttributes(cx, id, attrsp);
}

JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval)
{
    JSObject *proto;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSScope *scope;
    JSBool ok;

    rval->setBoolean(true);

    
    id = js_CheckForStringIndex(id);

    if (!js_LookupProperty(cx, obj, id, &proto, &prop))
        return JS_FALSE;
    if (!prop || proto != obj) {
        





        if (prop) {
            if (proto->isNative()) {
                sprop = (JSScopeProperty *)prop;
                if (sprop->isSharedPermanent())
                    rval->setBoolean(false);
                JS_UNLOCK_OBJ(cx, proto);
            }
            if (rval->isBoolean() && rval->asBoolean() == false)
                return JS_TRUE;
        }

        




        return obj->getClass()->delProperty(cx, obj, id, rval);
    }

    sprop = (JSScopeProperty *)prop;
    if (!sprop->configurable()) {
        JS_UNLOCK_OBJ(cx, obj);
        rval->setBoolean(false);
        return JS_TRUE;
    }

    
    if (!obj->getClass()->delProperty(cx, obj, SPROP_USERID(sprop), rval)) {
        JS_UNLOCK_OBJ(cx, obj);
        return JS_FALSE;
    }

    scope = obj->scope();
    if (SPROP_HAS_VALID_SLOT(sprop, scope))
        GC_POKE(cx, obj->lockedGetSlot(sprop->slot));

    ok = scope->removeProperty(cx, id);
    JS_UNLOCK_OBJ(cx, obj);

    return ok && js_SuppressDeletedProperty(cx, obj, id);
}

JSBool
js_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint != JSTYPE_OBJECT && hint != JSTYPE_FUNCTION);

    Value v = ObjectTag(*obj);
    if (hint == JSTYPE_STRING) {
        




        if (obj->getClass() == &js_StringClass) {
            jsid toStringId = ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);

            JS_LOCK_OBJ(cx, obj);
            JSScope *scope = obj->scope();
            JSScopeProperty *sprop = scope->lookup(toStringId);
            JSObject *pobj = obj;

            if (!sprop) {
                pobj = obj->getProto();

                if (pobj && pobj->getClass() == &js_StringClass) {
                    JS_UNLOCK_SCOPE(cx, scope);
                    JS_LOCK_OBJ(cx, pobj);
                    scope = pobj->scope();
                    sprop = scope->lookup(toStringId);
                }
            }

            if (sprop && sprop->hasDefaultGetter() && SPROP_HAS_VALID_SLOT(sprop, scope)) {
                const Value &fval = pobj->lockedGetSlot(sprop->slot);

                JSObject *funobj;
                if (IsFunctionObject(fval, &funobj)) {
                    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

                    if (FUN_FAST_NATIVE(fun) == js_str_toString) {
                        JS_UNLOCK_SCOPE(cx, scope);
                        *vp = obj->getPrimitiveThis();
                        return JS_TRUE;
                    }
                }
            }
            JS_UNLOCK_SCOPE(cx, scope);
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
    if (!v.isPrimitive()) {
        
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

JSBool
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

    
    JS_ASSERT(enum_op == JSENUMERATE_INIT);
    statep->setMagic(JS_NATIVE_ENUMERATE);
    return true;
}

JSBool
js_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               Value *vp, uintN *attrsp)
{
    JSBool writing;
    JSObject *pobj;
    JSProperty *prop;
    Class *clasp;
    JSScopeProperty *sprop;
    JSSecurityCallbacks *callbacks;
    CheckAccessOp check;

    writing = (mode & JSACC_WRITE) != 0;
    switch (mode & JSACC_TYPEMASK) {
      case JSACC_PROTO:
        pobj = obj;
        if (!writing)
            *vp = obj->getProtoValue();
        *attrsp = JSPROP_PERMANENT;
        break;

      case JSACC_PARENT:
        JS_ASSERT(!writing);
        pobj = obj;
        *vp = obj->getParentValue();
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
            
            if (pobj->map->ops->checkAccess == js_CheckAccess) {
                if (!writing) {
                    vp->setUndefined();
                    *attrsp = 0;
                }
                break;
            }
            return pobj->checkAccess(cx, id, mode, vp, attrsp);
        }

        sprop = (JSScopeProperty *)prop;
        *attrsp = sprop->attributes();
        if (!writing) {
            if (SPROP_HAS_VALID_SLOT(sprop, pobj->scope()))
                *vp = pobj->lockedGetSlot(sprop->slot);
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

JSType
js_TypeOf(JSContext *cx, JSObject *obj)
{
    




    obj = obj->wrappedObject(cx);

    




    if (obj->isCallable()) {
        return (obj->getClass() != &js_RegExpClass)
               ? JSTYPE_FUNCTION
               : JSTYPE_OBJECT;
    }

#ifdef NARCISSUS
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED);
    Value v;

    if (!obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.__call__Atom), &v)) {
        JS_ClearPendingException(cx);
    } else {
        if (IsFunctionObject(v))
            return JSTYPE_FUNCTION;
    }
#endif

    return JSTYPE_OBJECT;
}

#ifdef JS_THREADSAFE
void
js_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop)
{
    JS_UNLOCK_OBJ(cx, obj);
}
#endif

#ifdef NARCISSUS
static JSBool
GetCurrentExecutionContext(JSContext *cx, JSObject *obj, Value *rval)
{
    JSObject *tmp;
    Value xcval;

    while ((tmp = obj->getParent()) != NULL)
        obj = tmp;
    if (!obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.ExecutionContextAtom), &xcval))
        return JS_FALSE;
    if (xcval.isPrimitive()) {
        JS_ReportError(cx, "invalid ExecutionContext in global object");
        return JS_FALSE;
    }
    if (!xcval.asObject().getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.currentAtom),
                                      rval)) {
        return JS_FALSE;
    }
    return JS_TRUE;
}
#endif

JSBool
js_Call(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{
    Class *clasp = argv[-2].asObject().getClass();
    if (!clasp->call) {
#ifdef NARCISSUS
        JSObject *callee, *args;
        Value fval, nargv[3];
        JSBool ok;

        callee = &argv[-2].asObject();
        if (!callee->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.__call__Atom), &fval))
            return JS_FALSE;
        if (IsFunctionObject(fval)) {
            if (!GetCurrentExecutionContext(cx, obj, &nargv[2]))
                return JS_FALSE;
            args = js_GetArgsObject(cx, js_GetTopStackFrame(cx));
            if (!args)
                return JS_FALSE;
            nargv[0].setObject(*obj);
            nargv[1].setObject(*args);
            return InternalCall(cx, callee, fval, 3, nargv, rval);
        }
        if (fval.isObjectOrNull() && fval.asObjectOrNull() != callee) {
            argv[-2] = fval;
            ok = js_Call(cx, obj, argc, argv, rval);
            argv[-2].setObject(*callee);
            return ok;
        }
#endif
        js_ReportIsNotFunction(cx, &argv[-2], 0);
        return JS_FALSE;
    }
    return clasp->call(cx, obj, argc, argv, rval);
}

JSBool
js_Construct(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{

    Class *clasp = argv[-2].asObject().getClass();
    if (!clasp->construct) {
#ifdef NARCISSUS
        JSObject *callee, *args;
        Value cval, nargv[2];
        JSBool ok;

        callee = &argv[-2].asObject();
        if (!callee->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.__construct__Atom),
                                 &cval)) {
            return JS_FALSE;
        }
        if (IsFunctionObject(cval)) {
            if (!GetCurrentExecutionContext(cx, obj, &nargv[1]))
                return JS_FALSE;
            args = js_GetArgsObject(cx, js_GetTopStackFrame(cx));
            if (!args)
                return JS_FALSE;
            nargv[0].setObject(*args);
            return InternalCall(cx, callee, cval, 2, nargv, rval);
        }
        if (cval.isObjectOrNull() && cval.asObjectOrNull() != callee) {
            argv[-2] = cval;
            ok = js_Call(cx, obj, argc, argv, rval);
            argv[-2].setObject(*callee);
            return ok;
        }
#endif
        js_ReportIsNotFunction(cx, &argv[-2], JSV2F_CONSTRUCT);
        return JS_FALSE;
    }
    return clasp->construct(cx, obj, argc, argv, rval);
}

JSBool
js_HasInstance(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    Class *clasp = obj->getClass();
    if (clasp->hasInstance)
        return clasp->hasInstance(cx, obj, v, bp);
#ifdef NARCISSUS
    {
        Value fval, rval;

        if (!obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.__hasInstance__Atom), &fval))
            return JS_FALSE;
        if (IsFunctionObject(fval)) {
            if (!InternalCall(cx, obj, fval, 1, v, &rval))
                return JS_FALSE;
            *bp = js_ValueToBoolean(rval);
            return JS_TRUE;
        }
    }
#endif
    js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                        JSDVG_SEARCH_STACK, ObjectTag(*obj), NULL);
    return JS_FALSE;
}

bool
js_IsDelegate(JSContext *cx, JSObject *obj, const Value &v)
{
    if (v.isPrimitive())
        return false;
    JSObject *obj2 = v.asObject().wrappedObject(cx);
    while ((obj2 = obj2->getProto()) != NULL) {
        if (obj2 == obj)
            return true;
    }
    return false;
}

JSBool
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, Class *clasp)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();
    JS_ASSERT(JSProto_Null <= protoKey);
    JS_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        if (!scope) {
            if (cx->fp)
                scope = cx->fp->scopeChainObj();
            if (!scope) {
                scope = cx->globalObject;
                if (!scope) {
                    *protop = NULL;
                    return true;
                }
            }
        }
        scope = scope->getGlobal();
        if (scope->getClass()->flags & JSCLASS_IS_GLOBAL) {
            const Value &v = scope->getReservedSlot(JSProto_LIMIT + protoKey);
            if (v.isObject()) {
                *protop = &v.asObject();
                return true;
            }
        }
    }

    Value v;
    if (!js_FindClassObject(cx, scope, protoKey, &v, clasp))
        return JS_FALSE;
    JSObject *ctor;
    if (IsFunctionObject(v, &ctor)) {
        if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &v))
            return JS_FALSE;
        if (v.isObject()) {
            








            cx->weakRoots.finalizableNewborns[FINALIZE_OBJECT] =
                &v.asObject();
        }
    }
    *protop = v.isObject() ? &v.asObject() : NULL;
    return JS_TRUE;
}
















static JSBool
CheckCtorGetAccess(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSAtom *atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    uintN attrs;
    return obj->checkAccess(cx, ATOM_TO_JSID(atom), JSACC_READ, vp, &attrs);
}

static JSBool
CheckCtorSetAccess(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSAtom *atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    uintN attrs;
    return obj->checkAccess(cx, ATOM_TO_JSID(atom), JSACC_WRITE, vp, &attrs);
}

JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto, uintN attrs)
{
    





    if (!ctor->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                              ObjectOrNullTag(proto), PropertyStub, PropertyStub, attrs)) {
        return JS_FALSE;
    }

    



    return proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                                 ObjectOrNullTag(ctor), CheckCtorGetAccess, CheckCtorSetAccess, 0);
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

    JSObject *obj = NewObject(cx, clasp, NULL, NULL);
    if (!obj)
        return JS_FALSE;

    obj->setPrimitiveThis(v);
    vp->setObject(*obj);
    return JS_TRUE;
}

JSBool
js_ValueToObjectOrNull(JSContext *cx, const Value &v, Value *vp)
{
    if (v.isObjectOrNull()) {
        *vp = v;
        return JS_TRUE;
    }
    if (v.isUndefined()) {
        vp->setNull();
        return JS_TRUE;
    }
    *vp = v;
    return js_PrimitiveToObject(cx, vp);
}

JSBool
js_ValueToNonNullObject(JSContext *cx, const Value &v, js::Value *vp)
{
    if (!js_ValueToObjectOrNull(cx, v, vp))
        return JS_FALSE;
    if (vp->isNull()) {
        js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, v, NULL);
        return JS_FALSE;
    }
    return JS_TRUE;
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
    return InternalCall(cx, obj, fval, argc, argv, rval);
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
    if (classDef == 1 && !js_XDRStringAtom(xdr, &atom))
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

    JSScopeProperty *sprop;
    if (obj->isNative()) {
        JSScope *scope = obj->scope();
        sprop = scope->lastProperty();
        while (sprop && sprop->slot != slot)
            sprop = sprop->parent;
    } else {
        sprop = NULL;
    }

    if (!sprop) {
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
        jsid id = sprop->id;
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
    JSScope *scope = obj->scope();
    if (!scope->isSharedEmpty() && IS_GC_MARKING_TRACER(trc)) {
        




        size_t slots = scope->freeslot;
        if (obj->numSlots() != slots)
            obj->shrinkSlots(cx, slots);
    }

#ifdef JS_DUMP_SCOPE_METERS
    MeterEntryCount(scope->entryCount);
#endif

    scope->trace(trc);

    if (!JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList))
        js_TraceWatchPoints(trc, obj);

    
    Class *clasp = obj->getClass();
    if (clasp->mark) {
        if (clasp->flags & JSCLASS_MARK_IS_TRACE)
            ((JSTraceOp) clasp->mark)(trc, obj);
        else if (IS_GC_MARKING_TRACER(trc))
            (void) clasp->mark(cx, obj, trc);
    }
    if (clasp->flags & JSCLASS_IS_GLOBAL)
        obj->getCompartment(cx)->marked = true;

    obj->traceProtoAndParent(trc);

    








    uint32 nslots = obj->numSlots();
    if (!scope->isSharedEmpty() && scope->freeslot < nslots)
        nslots = scope->freeslot;
    JS_ASSERT(nslots >= JSSLOT_START(clasp));

    for (uint32 i = JSSLOT_START(clasp); i != nslots; ++i) {
        const Value &v = obj->getSlot(i);
        JS_SET_TRACING_DETAILS(trc, js_PrintObjectSlotName, obj, i);
        MarkValueRaw(trc, v);
    }
}

void
js_Clear(JSContext *cx, JSObject *obj)
{
    JSScope *scope;
    uint32 i, n;

    




    JS_LOCK_OBJ(cx, obj);
    scope = obj->scope();
    if (!scope->isSharedEmpty()) {
        
        scope->clear(cx);

        
        i = obj->numSlots();
        n = JSSLOT_FREE(obj->getClass());
        while (--i >= n)
            obj->setSlot(i, Value(UndefinedTag()));
        scope->freeslot = n;
    }
    JS_UNLOCK_OBJ(cx, obj);
}


static bool
ReservedSlotIndexOK(JSContext *cx, JSObject *obj, Class *clasp,
                    uint32 index, uint32 limit)
{
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, obj));

    
    if (clasp->reserveSlots)
        limit += clasp->reserveSlots(cx, obj);
    if (index >= limit) {
        JS_UNLOCK_OBJ(cx, obj);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_RESERVED_SLOT_RANGE);
        return false;
    }
    return true;
}

bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, Value *vp)
{
    if (!obj->isNative()) {
        vp->setUndefined();
        return true;
    }

    Class *clasp = obj->getClass();
    uint32 limit = JSCLASS_RESERVED_SLOTS(clasp);

    JS_LOCK_OBJ(cx, obj);
    if (index >= limit && !ReservedSlotIndexOK(cx, obj, clasp, index, limit))
        return false;

    uint32 slot = JSSLOT_START(clasp) + index;
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

    JS_LOCK_OBJ(cx, obj);
#ifdef DEBUG
    uint32 limit = JSCLASS_RESERVED_SLOTS(clasp);
    JS_ASSERT(index < limit || ReservedSlotIndexOK(cx, obj, clasp, index, limit));
#endif

    uint32 slot = JSSLOT_START(clasp) + index;
    if (slot >= obj->numSlots()) {
        




        uint32 nslots = JSSLOT_FREE(clasp);
        if (clasp->reserveSlots)
            nslots += clasp->reserveSlots(cx, obj);
        JS_ASSERT(slot < nslots);
        if (!obj->allocSlots(cx, nslots)) {
            JS_UNLOCK_OBJ(cx, obj);
            return false;
        }
    }

    







    JSScope *scope = obj->scope();
    if (!scope->isSharedEmpty() && slot >= scope->freeslot)
        scope->freeslot = slot + 1;

    obj->setSlot(slot, v);
    GC_POKE(cx, JS_NULL);
    JS_UNLOCK_SCOPE(cx, scope);
    return true;
}

JSObject *
JSObject::wrappedObject(JSContext *cx) const
{
    Class *clasp = getClass();
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        if (JSObjectOp wrappedObject = reinterpret_cast<JSExtendedClass *>(clasp)->wrappedObject) {
            if (JSObject *obj = wrappedObject(cx, const_cast<JSObject *>(this)))
                return obj;
        }
    }
    return const_cast<JSObject *>(this);
}

JSObject *
JSObject::getGlobal()
{
    JSObject *obj = this;
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
JSObject::getCompartment(JSContext *cx) {
    JSObject *obj = getGlobal();
    JS_ASSERT(obj->getClass()->flags & JSCLASS_IS_GLOBAL);
    const Value &v = obj->getReservedSlot(JSRESERVED_GLOBAL_COMPARTMENT);
    return (JSCompartment *) v.asPrivate();
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
        fprintf(stderr, "%d", v.asInt32());
    else if (v.isDouble())
        fprintf(stderr, "%g", v.asDouble());
    else if (v.isString())
        dumpString(v.asString());
    else if (v.isObject() && v.asObject().isFunction()) {
        JSObject *funobj = &v.asObject();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
        fprintf(stderr, "<%s %s at %p (JSFunction at %p)>",
                fun->atom ? "function" : "unnamed",
                fun->atom ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom)) : "function",
                (void *) funobj,
                (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.asObject();
        Class *cls = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                cls->name,
                cls == &js_ObjectClass ? "" : " object",
                (void *) obj);
    } else if (v.isBoolean()) {
        if (v.asBoolean())
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
dumpScopeProp(JSScopeProperty *sprop)
{
    jsid id = sprop->id;
    uint8 attrs = sprop->attributes();

    fprintf(stderr, "    ");
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_GETTER) fprintf(stderr, "getter ");
    if (attrs & JSPROP_SETTER) fprintf(stderr, "setter ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");
    if (sprop->isAlias()) fprintf(stderr, "alias ");
    if (JSID_IS_ATOM(id))
        dumpString(JSID_TO_STRING(id));
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));
    fprintf(stderr, ": slot %d", sprop->slot);
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
        JSScope *scope = obj->scope();
        if (scope->sealed())
            fprintf(stderr, "sealed\n");

        fprintf(stderr, "properties:\n");
        for (JSScopeProperty *sprop = scope->lastProperty(); sprop;
             sprop = sprop->parent) {
            dumpScopeProp(sprop);
        }
    } else {
        if (!obj->isNative())
            fprintf(stderr, "not native\n");
    }

    fprintf(stderr, "proto ");
    dumpValue(obj->getProtoValue());
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(obj->getParentValue());
    fputc('\n', stderr);

    i = JSSLOT_PRIVATE;
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        i = JSSLOT_PRIVATE + 1;
        fprintf(stderr, "private %p\n", obj->getPrivate());
    }

    fprintf(stderr, "slots:\n");
    reservedEnd = i + JSCLASS_RESERVED_SLOTS(clasp);
    slots = (obj->isNative() && !obj->scope()->isSharedEmpty())
            ? obj->scope()->freeslot
            : obj->numSlots();
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
        dumpValue(ObjectTag(*obj));
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
        start = cx->fp;
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
        if (fp->argv) {
            fprintf(stderr, "callee: ");
            dumpValue(fp->argv[-2]);
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        if (fp->script)
            fprintf(stderr, "file %s line %u\n", fp->script->filename, (unsigned) fp->script->lineno);

        if (jsbytecode *pc = i.pc()) {
            if (!fp->script) {
                fprintf(stderr, "*** pc && !script, skipping frame\n\n");
                continue;
            }
            if (fp->imacpc) {
                fprintf(stderr, "  pc in imacro at %p\n  called from ", pc);
                pc = fp->imacpc;
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
        fprintf(stderr, "  argv:  %p (argc: %u)\n", (void *) fp->argv, (unsigned) fp->argc);
        MaybeDumpObject("callobj", fp->callobj);
        MaybeDumpObject("argsobj", fp->argsObj());
        MaybeDumpValue("this", fp->thisv);
        fprintf(stderr, "  rval: ");
        dumpValue(fp->rval);
        fputc('\n', stderr);

        fprintf(stderr, "  flags:");
        if (fp->flags == 0)
            fprintf(stderr, " none");
        if (fp->flags & JSFRAME_CONSTRUCTING)
            fprintf(stderr, " constructing");
        if (fp->flags & JSFRAME_ASSIGNING)
            fprintf(stderr, " assigning");
        if (fp->flags & JSFRAME_DEBUGGER)
            fprintf(stderr, " debugger");
        if (fp->flags & JSFRAME_EVAL)
            fprintf(stderr, " eval");
        if (fp->flags & JSFRAME_YIELDING)
            fprintf(stderr, " yielding");
        if (fp->flags & JSFRAME_GENERATOR)
            fprintf(stderr, " generator");
        if (fp->flags & JSFRAME_OVERRIDE_ARGS)
            fprintf(stderr, " overridden_args");
        fputc('\n', stderr);

        if (fp->scopeChainObj())
            fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) fp->scopeChainObj());
        if (fp->blockChain)
            fprintf(stderr, "  blockChain: (JSObject *) %p\n", (void *) fp->blockChain);

        if (fp->displaySave)
            fprintf(stderr, "  displaySave: (JSStackFrame *) %p\n", (void *) fp->displaySave);

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


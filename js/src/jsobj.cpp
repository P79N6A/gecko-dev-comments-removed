










































#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsstdint.h"
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
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
#include "jsopcode.h"
#include "jsprobes.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstdint.h"
#include "jsstr.h"
#include "jstracer.h"
#include "jsdbgapi.h"
#include "json.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeGenerator.h"
#include "frontend/Parser.h"

#include "jsarrayinlines.h"
#include "jsinterpinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"

#include "vm/NumberObject-inl.h"
#include "vm/StringObject-inl.h"

#if JS_HAS_GENERATORS
#include "jsiter.h"
#endif

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jsautooplen.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;
using namespace js::types;

Class js::ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *obj)
{
    if (!obj) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INACTIVE);
        return NULL;
    }
    OBJ_TO_INNER_OBJECT(cx, obj);
    return obj;
}

JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj)
{
    OBJ_TO_OUTER_OBJECT(cx, obj);
    return obj;
}

#if JS_HAS_OBJ_PROTO_PROP

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp);

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);

JSPropertySpec object_props[] = {
    {js_proto_str, 0, JSPROP_PERMANENT|JSPROP_SHARED, obj_getProto, obj_setProto},
    {0,0,0,0,0}
};

static JSBool
obj_getProto(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    
    uintN attrs;
    id = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    return CheckAccess(cx, obj, id, JSACC_PROTO, vp, &attrs);
}

size_t sSetProtoCalled = 0;

static JSBool
obj_setProto(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    if (!cx->runningWithTrustedPrincipals())
        ++sSetProtoCalled;

    
    if (!obj->isExtensible()) {
        obj->reportNotExtensible(cx);
        return false;
    }

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
            ok = obj->lookupGeneric(cx, id, &obj2, &prop);
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
                ok = obj->getGeneric(cx, id, v.addr());
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
        *sp = InflateString(cx, buf, &len);
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
                    cx->free_(*sp);
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
    MarkObject((JSTracer *)arg, *(JSObject *)he->key, "sharp table entry");
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
    JSBool ok;
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
    JSString *valstr, *str;
    JSLinearString *idstr;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    Value localroot[4];
    PodArrayZero(localroot);
    AutoArrayRooter tvr(cx, ArrayLength(localroot), localroot);

    
    JSBool outermost = (cx->sharpObjectMap.depth == 0);

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    if (!(he = js_EnterSharpObject(cx, obj, &ida, &chars))) {
        ok = JS_FALSE;
        goto out;
    }
    if (!ida) {
        




        JS_ASSERT(IS_SHARP(he));
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
    JS_ASSERT(!IS_SHARP(he));
    ok = JS_TRUE;

    if (!chars) {
        
        chars = (jschar *) cx->malloc_(((outermost ? 4 : 2) + 1) * sizeof(jschar));
        nchars = 0;
        if (!chars)
            goto error;
        if (outermost)
            chars[nchars++] = '(';
    } else {
        
        MAKE_SHARP(he);
        nchars = js_strlen(chars);
        chars = (jschar *)
            cx->realloc_((ochars = chars), (nchars + 2 + 1) * sizeof(jschar));
        if (!chars) {
            Foreground::free_(ochars);
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

        ok = obj->lookupGeneric(cx, id, &obj2, &prop);
        if (!ok)
            goto error;

        



        JSString *s = js_ValueToString(cx, IdToValue(id));
        if (!s || !(idstr = s->ensureLinear(cx))) {
            ok = JS_FALSE;
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
                    gsop[valcnt] = cx->runtime->atomState.getAtom;
                    valcnt++;
                }
                if (attrs & JSPROP_SETTER) {
                    doGet = false;
                    val[valcnt] = shape->setterValue();
                    gsop[valcnt] = cx->runtime->atomState.setAtom;
                    valcnt++;
                }
            }
            if (doGet) {
                valcnt = 1;
                gsop[0] = NULL;
                ok = obj->getGeneric(cx, id, &val[0]);
                if (!ok)
                    goto error;
            }
        }

        



        bool idIsLexicalIdentifier = js_IsIdentifier(idstr);
        if (JSID_IS_ATOM(id)
            ? !idIsLexicalIdentifier
            : (!JSID_IS_INT(id) || JSID_TO_INT(id) < 0)) {
            s = js_QuoteString(cx, idstr, jschar('\''));
            if (!s || !(idstr = s->ensureLinear(cx))) {
                ok = JS_FALSE;
                goto error;
            }
            vp->setString(idstr);                       
        }
        idstrlength = idstr->length();
        idstrchars = idstr->getChars(cx);
        if (!idstrchars) {
            ok = JS_FALSE;
            goto error;
        }

        for (jsint j = 0; j < valcnt; j++) {
            



            if (gsop[j] && val[j].isUndefined())
                continue;

            
            valstr = js_ValueToSource(cx, val[j]);
            if (!valstr) {
                ok = JS_FALSE;
                goto error;
            }
            localroot[j].setString(valstr);             
            vchars = valstr->getChars(cx);
            if (!vchars) {
                ok = JS_FALSE;
                goto error;
            }
            vlength = valstr->length();

            




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

            
            chars = (jschar *) cx->realloc_((ochars = chars), curlen * sizeof(jschar));
            if (!chars) {
                chars = ochars;
                goto overflow;
            }

            if (comma) {
                chars[nchars++] = comma[0];
                chars[nchars++] = comma[1];
            }
            comma = ", ";

            if (gsop[j]) {
                gsoplength = gsop[j]->length();
                const jschar *gsopchars = gsop[j]->getChars(cx);
                if (!gsopchars)
                    goto overflow;
                js_strncpy(&chars[nchars], gsopchars, gsoplength);
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
                cx->free_(vsharp);
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
            Foreground::free_(chars);
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
        cx->free_(chars);
        ok = JS_FALSE;
        goto out;
    }
    vp->setString(str);
    ok = JS_TRUE;
  out:
    return ok;

  overflow:
    cx->free_(vsharp);
    cx->free_(chars);
    chars = NULL;
    goto error;
}
#endif 

namespace js {

JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj)
{
    if (obj->isProxy())
        return Proxy::obj_toString(cx, obj);

    const char *clazz = obj->getClass()->name;
    size_t nchars = 9 + strlen(clazz); 
    jschar *chars = (jschar *) cx->malloc_((nchars + 1) * sizeof(jschar));
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
        cx->free_(chars);
    return str;
}

JSObject *
NonNullObject(JSContext *cx, const Value &v)
{
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return NULL;
    }
    return &v.toObject();
}

const char *
InformalValueTypeName(const Value &v)
{
    if (v.isObject())
        return v.toObject().getClass()->name;
    if (v.isString())
        return "string";
    if (v.isNumber())
        return "number";
    if (v.isBoolean())
        return "boolean";
    if (v.isNull())
        return "null";
    if (v.isUndefined())
        return "undefined";
    return "value";
}

} 


static JSBool
obj_toString(JSContext *cx, uintN argc, Value *vp)
{
    Value &thisv = vp[1];

    
    if (thisv.isUndefined()) {
        vp->setString(cx->runtime->atomState.objectUndefinedAtom);
        return true;
    }

    
    if (thisv.isNull()) {
        vp->setString(cx->runtime->atomState.objectNullAtom);
        return true;
    }

    
    JSObject *obj = ToObject(cx, &thisv);
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
    JS_CHECK_RECURSION(cx, return false);

    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    return obj->callMethod(cx, ATOM_TO_JSID(cx->runtime->atomState.toStringAtom), 0, NULL, vp);
}

static JSBool
obj_valueOf(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    vp->setObject(*obj);
    return true;
}


static void
AssertInnerizedScopeChain(JSContext *cx, JSObject &scopeobj)
{
#ifdef DEBUG
    for (JSObject *o = &scopeobj; o; o = o->getParentOrScopeChain()) {
        if (JSObjectOp op = o->getClass()->ext.innerObject)
            JS_ASSERT(op(cx, o) == o);
    }
#endif
}

#ifndef EVAL_CACHE_CHAIN_LIMIT
# define EVAL_CACHE_CHAIN_LIMIT 4
#endif

static inline JSScript **
EvalCacheHash(JSContext *cx, JSLinearString *str)
{
    const jschar *s = str->chars();
    size_t n = str->length();

    if (n > 100)
        n = 100;
    uint32 h;
    for (h = 0; n; s++, n--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;

    h *= JS_GOLDEN_RATIO;
    h >>= 32 - JS_EVAL_CACHE_SHIFT;
    return &cx->compartment->evalCache[h];
}

static JS_ALWAYS_INLINE JSScript *
EvalCacheLookup(JSContext *cx, JSLinearString *str, StackFrame *caller, uintN staticLevel,
                JSPrincipals *principals, JSObject &scopeobj, JSScript **bucket)
{
    















    uintN count = 0;
    JSScript **scriptp = bucket;

    JSVersion version = cx->findVersion();
    JSScript *script;
    while ((script = *scriptp) != NULL) {
        if (script->savedCallerFun &&
            script->staticLevel == staticLevel &&
            script->getVersion() == version &&
            !script->hasSingletons &&
            (script->principals == principals ||
             (principals && script->principals &&
              principals->subsume(principals, script->principals) &&
              script->principals->subsume(script->principals, principals)))) {
            



            JSFunction *fun = script->getCallerFunction();

            if (fun == caller->fun()) {
                



                JSAtom *src = script->atoms[0];

                if (src == str || EqualStrings(src, str)) {
                    





                    JS_ASSERT(script->objects()->length >= 1);
                    if (script->objects()->length == 1 &&
                        !JSScript::isValidOffset(script->regexpsOffset)) {
                        JS_ASSERT(staticLevel == script->staticLevel);
                        *scriptp = script->u.evalHashLink;
                        script->u.evalHashLink = NULL;
                        return script;
                    }
                }
            }
        }

        if (++count == EVAL_CACHE_CHAIN_LIMIT)
            return NULL;
        scriptp = &script->u.evalHashLink;
    }
    return NULL;
}












class EvalScriptGuard
{
    JSContext *cx_;
    JSLinearString *str_;
    JSScript **bucket_;
    JSScript *script_;

  public:
    EvalScriptGuard(JSContext *cx, JSLinearString *str)
      : cx_(cx),
        str_(str),
        script_(NULL) {
        bucket_ = EvalCacheHash(cx, str);
    }

    ~EvalScriptGuard() {
        if (script_) {
            js_CallDestroyScriptHook(cx_, script_);
            script_->isActiveEval = false;
            script_->isCachedEval = true;
            script_->u.evalHashLink = *bucket_;
            *bucket_ = script_;
        }
    }

    void lookupInEvalCache(StackFrame *caller, uintN staticLevel,
                           JSPrincipals *principals, JSObject &scopeobj) {
        if (JSScript *found = EvalCacheLookup(cx_, str_, caller, staticLevel,
                                              principals, scopeobj, bucket_)) {
            js_CallNewScriptHook(cx_, found, NULL);
            script_ = found;
            script_->isCachedEval = false;
            script_->isActiveEval = true;
        }
    }

    void setNewScript(JSScript *script) {
        
        JS_ASSERT(!script_ && script);
        script->setOwnerObject(JS_CACHED_SCRIPT);
        script_ = script;
        script_->isActiveEval = true;
    }

    bool foundScript() {
        return !!script_;
    }

    JSScript *script() const {
        JS_ASSERT(script_);
        return script_;
    }
};


enum EvalType { DIRECT_EVAL = EXECUTE_DIRECT_EVAL, INDIRECT_EVAL = EXECUTE_INDIRECT_EVAL };











static bool
EvalKernel(JSContext *cx, const CallArgs &args, EvalType evalType, StackFrame *caller,
           JSObject &scopeobj)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == (caller == NULL));
    AssertInnerizedScopeChain(cx, scopeobj);

    if (!scopeobj.getGlobal()->isRuntimeCodeGenEnabled(cx)) {
        JS_ReportError(cx, "call to eval() blocked by CSP");
        return false;
    }

    
    if (args.length() < 1) {
        args.rval().setUndefined();
        return true;
    }
    if (!args[0].isString()) {
        args.rval() = args[0];
        return true;
    }
    JSString *str = args[0].toString();

    

    




    uintN staticLevel;
    Value thisv;
    if (evalType == DIRECT_EVAL) {
        staticLevel = caller->script()->staticLevel + 1;

        




        if (!ComputeThis(cx, caller))
            return false;
        thisv = caller->thisValue();

#ifdef DEBUG
        jsbytecode *callerPC = caller->pcQuadratic(cx);
        JS_ASSERT(callerPC && js_GetOpcode(cx, caller->script(), callerPC) == JSOP_EVAL);
#endif
    } else {
        JS_ASSERT(args.callee().getGlobal() == &scopeobj);
        staticLevel = 0;

        
        JSObject *thisobj = scopeobj.thisObject(cx);
        if (!thisobj)
            return false;
        thisv = ObjectValue(*thisobj);
    }

    JSLinearString *linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;
    const jschar *chars = linearStr->chars();
    size_t length = linearStr->length();

    










    if (length > 2 &&
        chars[0] == '(' && chars[length - 1] == ')' &&
        (!caller || !caller->script()->strictModeCode))
    {
        







        for (const jschar *cp = &chars[1], *end = &chars[length - 2]; ; cp++) {
            if (*cp == 0x2028 || *cp == 0x2029)
                break;

            if (cp == end) {
                JSONParser parser(cx, chars + 1, length - 2,
                                  JSONParser::StrictJSON, JSONParser::NoError);
                Value tmp;
                if (!parser.parse(&tmp))
                    return false;
                if (tmp.isUndefined())
                    break;
                args.rval() = tmp;
                return true;
            }
        }
    }

    EvalScriptGuard esg(cx, linearStr);

    JSPrincipals *principals = PrincipalsForCompiledCode(args, cx);

    if (evalType == DIRECT_EVAL && caller->isNonEvalFunctionFrame())
        esg.lookupInEvalCache(caller, staticLevel, principals, scopeobj);

    if (!esg.foundScript()) {
        uintN lineno;
        const char *filename = CurrentScriptFileAndLine(cx, &lineno,
                                                        evalType == DIRECT_EVAL
                                                        ? CALLED_FROM_JSOP_EVAL
                                                        : NOT_CALLED_FROM_JSOP_EVAL);
        uint32 tcflags = TCF_COMPILE_N_GO | TCF_NEED_MUTABLE_SCRIPT | TCF_COMPILE_FOR_EVAL;
        JSScript *compiled = Compiler::compileScript(cx, &scopeobj, caller, principals, tcflags,
                                                     chars, length, filename, lineno,
                                                     cx->findVersion(), linearStr, staticLevel);
        if (!compiled)
            return false;

        esg.setNewScript(compiled);
    }

    return ExecuteKernel(cx, esg.script(), scopeobj, thisv, ExecuteType(evalType),
                         NULL , &args.rval());
}






static inline bool
WarnOnTooManyArgs(JSContext *cx, const CallArgs &args)
{
    if (args.length() > 1) {
        if (JSScript *script = cx->stack.currentScript()) {
            if (!script->warnedAboutTwoArgumentEval) {
                static const char TWO_ARGUMENT_WARNING[] =
                    "Support for eval(code, scopeObject) has been removed. "
                    "Use |with (scopeObject) eval(code);| instead.";
                if (!JS_ReportWarning(cx, TWO_ARGUMENT_WARNING))
                    return false;
                script->warnedAboutTwoArgumentEval = true;
            }
        } else {
            



        }
    }

    return true;
}

namespace js {






JSBool
eval(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return WarnOnTooManyArgs(cx, args) &&
           EvalKernel(cx, args, INDIRECT_EVAL, NULL, *args.callee().getGlobal());
}

bool
DirectEval(JSContext *cx, const CallArgs &args)
{
    
    StackFrame *caller = cx->fp();
    JS_ASSERT(caller->isScriptFrame());
    JS_ASSERT(IsBuiltinEvalForScope(&caller->scopeChain(), args.calleev()));
    JS_ASSERT(js_GetOpcode(cx, cx->fp()->script(), cx->regs().pc) == JSOP_EVAL);

    AutoFunctionCallProbe callProbe(cx, args.callee().toFunction(), caller->script());

    JSObject *scopeChain =
        GetScopeChainFast(cx, caller, JSOP_EVAL, JSOP_EVAL_LENGTH + JSOP_LINENO_LENGTH);

    return scopeChain &&
           WarnOnTooManyArgs(cx, args) &&
           EvalKernel(cx, args, DIRECT_EVAL, caller, *scopeChain);
}

bool
IsBuiltinEvalForScope(JSObject *scopeChain, const Value &v)
{
    return scopeChain->getGlobal()->getOriginalEval() == v;
}

bool
IsAnyBuiltinEval(JSFunction *fun)
{
    return fun->maybeNative() == eval;
}

JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx)
{
    JS_ASSERT(IsAnyBuiltinEval(call.callee().toFunction()) ||
              IsBuiltinFunctionConstructor(call.callee().toFunction()));

    















    return call.callee().principals(cx);
}

}  

#if JS_HAS_OBJ_WATCHPOINT

static JSBool
obj_watch_handler(JSContext *cx, JSObject *obj, jsid id, jsval old,
                  jsval *nvp, void *closure)
{
    JSObject *callable = (JSObject *) closure;
    if (JSPrincipals *watcher = callable->principals(cx)) {
        if (JSObject *scopeChain = cx->stack.currentScriptedScopeChain()) {
            if (JSPrincipals *subject = scopeChain->principals(cx)) {
                if (!watcher->subsume(watcher, subject)) {
                    
                    return JS_TRUE;
                }
            }
        }
    }

    
    AutoResolving resolving(cx, obj, id, AutoResolving::WATCH);
    if (resolving.alreadyStarted())
        return true;

    Value argv[] = { IdToValue(id), old, *nvp };
    return Invoke(cx, ObjectValue(*obj), ObjectOrNullValue(callable), ArrayLength(argv), argv, nvp);
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

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    Value tmp;
    uintN attrs;
    if (!CheckAccess(cx, obj, propid, JSACC_WATCH, &tmp, &attrs))
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
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
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
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    return js_HasOwnPropertyHelper(cx, obj->getOps()->lookupGeneric, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, LookupGenericOp lookup, uintN argc,
                        Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    JSObject *obj2;
    JSProperty *prop;
    if (obj->isProxy()) {
        bool has;
        if (!Proxy::hasOwn(cx, obj, id, &has))
            return false;
        vp->setBoolean(has);
        return true;
    }
    if (!js_HasOwnProperty(cx, lookup, obj, id, &obj2, &prop))
        return JS_FALSE;
    vp->setBoolean(!!prop);
    return JS_TRUE;
}

JSBool
js_HasOwnProperty(JSContext *cx, LookupGenericOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED | JSRESOLVE_DETECTING);
    if (!(lookup ? lookup : js_LookupProperty)(cx, obj, id, objp, propp))
        return false;
    if (!*propp)
        return true;

    if (*objp == obj)
        return true;

    JSObject *outer = NULL;
    if (JSObjectOp op = (*objp)->getClass()->ext.outerObject) {
        outer = op(cx, *objp);
        if (!outer)
            return false;
    }

    if (outer != *objp)
        *propp = NULL;
    return true;
}


static JSBool
obj_isPrototypeOf(JSContext *cx, uintN argc, Value *vp)
{
    
    if (argc < 1 || !vp[2].isObject()) {
        vp->setBoolean(false);
        return true;
    }

    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    vp->setBoolean(js_IsDelegate(cx, obj, vp[2]));
    return true;
}


static JSBool
obj_propertyIsEnumerable(JSContext *cx, uintN argc, Value *vp)
{
    
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return false;

    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    return js_PropertyIsEnumerable(cx, obj, id, vp);
}

JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return false;

    if (!prop) {
        vp->setBoolean(false);
        return true;
    }

    



    if (pobj != obj) {
        vp->setBoolean(false);
        return true;
    }

    uintN attrs;
    if (!pobj->getAttributes(cx, id, &attrs))
        return false;

    vp->setBoolean((attrs & JSPROP_ENUMERATE) != 0);
    return true;
}

#if OLD_GETTER_SETTER_METHODS

const char js_defineGetter_str[] = "__defineGetter__";
const char js_defineSetter_str[] = "__defineSetter__";
const char js_lookupGetter_str[] = "__lookupGetter__";
const char js_lookupSetter_str[] = "__lookupSetter__";

JS_FRIEND_API(JSBool)
js::obj_defineGetter(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!BoxNonStrictThis(cx, args))
        return false;
    JSObject *obj = &args.thisv().toObject();

    if (args.length() <= 1 || !js_IsCallable(args[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_getter_str);
        return JS_FALSE;
    }
    PropertyOp getter = CastAsPropertyOp(&args[1].toObject());

    jsid id;
    if (!ValueToId(cx, args[0], &id))
        return JS_FALSE;
    if (!CheckRedeclaration(cx, obj, id, JSPROP_GETTER))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    args.rval().setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), getter, JS_StrictPropertyStub,
                               JSPROP_ENUMERATE | JSPROP_GETTER | JSPROP_SHARED);
}

JS_FRIEND_API(JSBool)
js::obj_defineSetter(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!BoxNonStrictThis(cx, args))
        return false;
    JSObject *obj = &args.thisv().toObject();

    if (args.length() <= 1 || !js_IsCallable(args[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_setter_str);
        return JS_FALSE;
    }
    StrictPropertyOp setter = CastAsStrictPropertyOp(&args[1].toObject());

    jsid id;
    if (!ValueToId(cx, args[0], &id))
        return JS_FALSE;
    if (!CheckRedeclaration(cx, obj, id, JSPROP_SETTER))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    args.rval().setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), JS_PropertyStub, setter,
                               JSPROP_ENUMERATE | JSPROP_SETTER | JSPROP_SHARED);
}

static JSBool
obj_lookupGetter(JSContext *cx, uintN argc, Value *vp)
{
    jsid id;
    if (!ValueToId(cx, argc != 0 ? vp[2] : UndefinedValue(), &id))
        return JS_FALSE;
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasGetterValue())
                *vp = shape->getterValue();
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
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    JSObject *pobj;
    JSProperty *prop;
    if (!obj->lookupGeneric(cx, id, &pobj, &prop))
        return JS_FALSE;
    vp->setUndefined();
    if (prop) {
        if (pobj->isNative()) {
            Shape *shape = (Shape *) prop;
            if (shape->hasSetterValue())
                *vp = shape->setterValue();
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

namespace js {

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp)
{
    if (!desc->obj) {
        vp->setUndefined();
        return true;
    }

    
    PropDesc d;
    d.initFromPropertyDescriptor(*desc);
    if (!d.makeObject(cx))
        return false;
    *vp = d.pd;
    return true;
}

void
PropDesc::initFromPropertyDescriptor(const PropertyDescriptor &desc)
{
    pd.setUndefined();
    attrs = uint8(desc.attrs);
    if (desc.attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        hasGet = true;
        get = ((desc.attrs & JSPROP_GETTER) && desc.getter)
              ? CastAsObjectJsval(desc.getter)
              : UndefinedValue();
        hasSet = true;
        set = ((desc.attrs & JSPROP_SETTER) && desc.setter)
              ? CastAsObjectJsval(desc.setter)
              : UndefinedValue();
        hasValue = false;
        value.setUndefined();
        hasWritable = false;
    } else {
        hasGet = false;
        get.setUndefined();
        hasSet = false;
        set.setUndefined();
        hasValue = true;
        value = desc.value;
        hasWritable = true;
    }
    hasEnumerable = true;
    hasConfigurable = true;
}

bool
PropDesc::makeObject(JSContext *cx)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &ObjectClass);
    if (!obj)
        return false;

    const JSAtomState &atomState = cx->runtime->atomState;
    if ((hasConfigurable &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.configurableAtom),
                              BooleanValue((attrs & JSPROP_PERMANENT) == 0),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)) ||
        (hasEnumerable &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.enumerableAtom),
                              BooleanValue((attrs & JSPROP_ENUMERATE) != 0),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)) ||
        (hasGet &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.getAtom), get,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)) ||
        (hasSet &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.setAtom), set,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)) ||
        (hasValue &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.valueAtom), value,
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)) ||
        (hasWritable &&
         !obj->defineProperty(cx, ATOM_TO_JSID(atomState.writableAtom),
                              BooleanValue((attrs & JSPROP_READONLY) == 0),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE)))
    {
        return false;
    }

    pd.setObject(*obj);
    return true;
}

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, PropertyDescriptor *desc)
{
    if (obj->isProxy())
        return Proxy::getOwnPropertyDescriptor(cx, obj, id, false, desc);

    JSObject *pobj;
    JSProperty *prop;
    if (!js_HasOwnProperty(cx, obj->getOps()->lookupGeneric, obj, id, &pobj, &prop))
        return false;
    if (!prop) {
        desc->obj = NULL;
        return true;
    }

    bool doGet = true;
    if (pobj->isNative()) {
        Shape *shape = (Shape *) prop;
        desc->attrs = shape->attributes();
        if (desc->attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
            doGet = false;
            if (desc->attrs & JSPROP_GETTER)
                desc->getter = CastAsPropertyOp(shape->getterObject());
            if (desc->attrs & JSPROP_SETTER)
                desc->setter = CastAsStrictPropertyOp(shape->setterObject());
        }
    } else {
        if (!pobj->getAttributes(cx, id, &desc->attrs))
            return false;
    }

    if (doGet && !obj->getGeneric(cx, id, &desc->value))
        return false;

    desc->obj = obj;
    return true;
}

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    AutoPropertyDescriptorRooter desc(cx);
    return GetOwnPropertyDescriptor(cx, obj, id, &desc) &&
           NewPropertyDescriptorObject(cx, &desc, vp);
}

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
    if (!v.isObject()) {
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
    return GetOwnPropertyDescriptor(cx, obj, nameidr.id(), vp);
}

static JSBool
obj_keys(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.keys", &obj))
        return false;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &props))
        return false;

    AutoValueVector vals(cx);
    if (!vals.reserve(props.length()))
        return false;
    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];
        if (JSID_IS_STRING(id)) {
            JS_ALWAYS_TRUE(vals.append(StringValue(JSID_TO_STRING(id))));
        } else if (JSID_IS_INT(id)) {
            JSString *str = js_IntToString(cx, JSID_TO_INT(id));
            if (!str)
                return false;
            vals.infallibleAppend(StringValue(str));
        } else {
            JS_ASSERT(JSID_IS_OBJECT(id));
        }
    }

    JS_ASSERT(props.length() <= UINT32_MAX);
    JSObject *aobj = NewDenseCopiedArray(cx, jsuint(vals.length()), vals.begin());
    if (!aobj)
        return false;
    vp->setObject(*aobj);

    return true;
}

static bool
HasProperty(JSContext* cx, JSObject* obj, jsid id, Value* vp, bool *foundp)
{
    if (!obj->hasProperty(cx, id, foundp, JSRESOLVE_QUALIFIED | JSRESOLVE_DETECTING))
        return false;
    if (!*foundp) {
        vp->setUndefined();
        return true;
    }

    





    return !!obj->getGeneric(cx, id, vp);
}

PropDesc::PropDesc()
  : pd(UndefinedValue()),
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
PropDesc::initialize(JSContext* cx, const Value &origval, bool checkAccessors)
{
    Value v = origval;

    
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    JSObject* desc = &v.toObject();

    
    pd = v;

    
    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    bool found;

    
#ifdef __GNUC__ 
    found = false;
#endif
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.enumerableAtom), &v, &found))
        return false;
    if (found) {
        hasEnumerable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.configurableAtom), &v, &found))
        return false;
    if (found) {
        hasConfigurable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.valueAtom), &v, &found))
        return false;
    if (found) {
        hasValue = true;
        value = v;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.writableAtom), &v, &found))
        return false;
    if (found) {
        hasWritable = JS_TRUE;
        if (js_ValueToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.getAtom), &v, &found))
        return false;
    if (found) {
        hasGet = true;
        get = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
        if (checkAccessors && !checkGetter(cx))
            return false;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.setAtom), &v, &found))
        return false;
    if (found) {
        hasSet = true;
        set = v;
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
        if (checkAccessors && !checkSetter(cx))
            return false;
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
        JSAutoByteString bytes(cx, JSID_TO_STRING(idstr));
        if (!bytes)
            return JS_FALSE;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber, bytes.ptr());
        return JS_FALSE;
    }

    *rval = false;
    return JS_TRUE;
}

static JSBool
Reject(JSContext *cx, JSObject *obj, uintN errorNumber, bool throwError, bool *rval)
{
    if (throwError) {
        if (js_ErrorFormatString[errorNumber].argCount == 1) {
            js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,
                                     JSDVG_IGNORE_STACK, ObjectValue(*obj),
                                     NULL, NULL, NULL);
        } else {
            JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, errorNumber);
        }
        return JS_FALSE;
    }

    *rval = false;
    return JS_TRUE;
}

static JSBool
DefinePropertyOnObject(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc,
                       bool throwError, bool *rval)
{
    
    JSProperty *current;
    JSObject *obj2;
    JS_ASSERT(!obj->getOps()->lookupGeneric);
    if (!js_HasOwnProperty(cx, NULL, obj, id, &obj2, &current))
        return JS_FALSE;

    JS_ASSERT(!obj->getOps()->defineProperty);

    
    if (!current) {
        if (!obj->isExtensible())
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            return js_DefineProperty(cx, obj, id, &desc.value,
                                     JS_PropertyStub, JS_StrictPropertyStub, desc.attrs);
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        uintN dummyAttrs;
        if (!CheckAccess(cx, obj, id, JSACC_WATCH, &dummy, &dummyAttrs))
            return JS_FALSE;

        Value tmp = UndefinedValue();
        return js_DefineProperty(cx, obj, id, &tmp,
                                 desc.getter(), desc.setter(), desc.attrs);
    }

    
    Value v = UndefinedValue();

    JS_ASSERT(obj == obj2);

    const Shape *shape = reinterpret_cast<Shape *>(current);
    do {
        if (desc.isAccessorDescriptor()) {
            if (!shape->isAccessorDescriptor())
                break;

            if (desc.hasGet) {
                JSBool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return JS_FALSE;
                if (!same)
                    break;
            }

            if (desc.hasSet) {
                JSBool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return JS_FALSE;
                if (!same)
                    break;
            }
        } else {
            






            if (shape->isDataDescriptor()) {
                









                if (!shape->configurable() &&
                    (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()) &&
                    desc.isDataDescriptor() &&
                    (desc.hasWritable ? desc.writable() : shape->writable()))
                {
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                }

                if (!js_NativeGet(cx, obj, obj2, shape, JSGET_NO_METHOD_BARRIER, &v))
                    return JS_FALSE;
            }

            if (desc.isDataDescriptor()) {
                if (!shape->isDataDescriptor())
                    break;

                JSBool same;
                if (desc.hasValue) {
                    if (!SameValue(cx, desc.value, v, &same))
                        return JS_FALSE;
                    if (!same) {
                        















                        if (!shape->configurable() &&
                            (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()))
                        {
                            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
                        }
                        break;
                    }
                }
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

        
        *rval = true;
        return JS_TRUE;
    } while (0);

    
    if (!shape->configurable()) {
        






        JS_ASSERT_IF(!desc.hasConfigurable, !desc.configurable());
        if (desc.configurable() ||
            (desc.hasEnumerable && desc.enumerable() != shape->enumerable())) {
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
        }
    }

    bool callDelProperty = false;

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shape->isDataDescriptor()) {
        
        if (!shape->configurable())
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
    } else if (desc.isDataDescriptor()) {
        
        JS_ASSERT(shape->isDataDescriptor());
        if (!shape->configurable() && !shape->writable()) {
            if (desc.hasWritable && desc.writable())
                return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            if (desc.hasValue) {
                JSBool same;
                if (!SameValue(cx, desc.value, v, &same))
                    return JS_FALSE;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }

        callDelProperty = !shape->hasDefaultGetter() || !shape->hasDefaultSetter();
    } else {
        
        JS_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            if (desc.hasSet) {
                JSBool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return JS_FALSE;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }

            if (desc.hasGet) {
                JSBool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return JS_FALSE;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }
    }

    
    uintN attrs;
    PropertyOp getter;
    StrictPropertyOp setter;
    if (desc.isGenericDescriptor()) {
        uintN changed = 0;
        if (desc.hasConfigurable)
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable)
            changed |= JSPROP_ENUMERATE;

        attrs = (shape->attributes() & ~changed) | (desc.attrs & changed);
        if (shape->isMethod()) {
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            getter = JS_PropertyStub;
            setter = JS_StrictPropertyStub;
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
        getter = JS_PropertyStub;
        setter = JS_StrictPropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        



        Value dummy;
        if (!CheckAccess(cx, obj2, id, JSACC_WATCH, &dummy, &attrs))
             return JS_FALSE;

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
                     ? JS_PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet) {
            setter = desc.setter();
        } else {
            setter = (shape->hasDefaultSetter() && !shape->hasSetterValue())
                     ? JS_StrictPropertyStub
                     : shape->setter();
        }
    }

    *rval = true;

    








    if (callDelProperty) {
        Value dummy = UndefinedValue();
        if (!CallJSPropertyOp(cx, obj2->getClass()->delProperty, obj2, id, &dummy))
            return false;
    }

    return js_DefineProperty(cx, obj, id, &v, getter, setter, attrs);
}

static JSBool
DefinePropertyOnArray(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc,
                      bool throwError, bool *rval)
{
    






    if (obj->isDenseArray() && !obj->makeDenseArraySlow(cx))
        return JS_FALSE;

    jsuint oldLen = obj->getArrayLength();

    if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        






        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_DEFINE_ARRAY_LENGTH);
        return JS_FALSE;
    }

    uint32 index;
    if (js_IdIsIndex(id, &index)) {
        




        if (!DefinePropertyOnObject(cx, obj, id, desc, false, rval))
            return JS_FALSE;
        if (!*rval)
            return Reject(cx, obj, JSMSG_CANT_DEFINE_ARRAY_INDEX, throwError, rval);

        if (index >= oldLen) {
            JS_ASSERT(index != UINT32_MAX);
            obj->setArrayLength(cx, index + 1);
        }

        *rval = true;
        return JS_TRUE;
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

namespace js {

bool
DefineProperty(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc, bool throwError,
               bool *rval)
{
    if (obj->isArray())
        return DefinePropertyOnArray(cx, obj, id, desc, throwError, rval);

    if (obj->getOps()->lookupGeneric) {
        if (obj->isProxy())
            return Proxy::defineProperty(cx, obj, id, desc.pd);
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

} 

JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id, const Value &descriptor, JSBool *bp)
{
    AutoPropDescArrayRooter descs(cx);
    PropDesc *desc = descs.append();
    if (!desc || !desc->initialize(cx, descriptor))
        return false;

    bool rval;
    if (!DefineProperty(cx, obj, id, *desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}


static JSBool
obj_defineProperty(JSContext* cx, uintN argc, Value* vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.defineProperty", &obj))
        return false;

    jsid id;
    if (!ValueToId(cx, argc >= 2 ? vp[3] : UndefinedValue(), &id))
        return JS_FALSE;

    const Value descval = argc >= 3 ? vp[4] : UndefinedValue();

    JSBool junk;
    if (!js_DefineOwnProperty(cx, obj, id, descval, &junk))
        return false;

    vp->setObject(*obj);
    return true;
}

namespace js {

bool
ReadPropertyDescriptors(JSContext *cx, JSObject *props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs)
{
    if (!GetPropertyNames(cx, props, JSITER_OWNONLY, ids))
        return false;

    for (size_t i = 0, len = ids->length(); i < len; i++) {
        jsid id = (*ids)[i];
        PropDesc* desc = descs->append();
        Value v;
        if (!desc || !props->getGeneric(cx, id, &v) || !desc->initialize(cx, v, checkAccessors))
            return false;
    }
    return true;
}

} 

static bool
DefineProperties(JSContext *cx, JSObject *obj, JSObject *props)
{
    AutoIdVector ids(cx);
    AutoPropDescArrayRooter descs(cx);
    if (!ReadPropertyDescriptors(cx, props, true, &ids, &descs))
        return false;

    bool dummy;
    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!DefineProperty(cx, obj, ids[i], descs[i], true, &dummy))
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
    JSObject* props = ToObject(cx, &vp[3]);
    if (!props)
        return false;

    
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

    JSObject *proto = v.toObjectOrNull();
    if (proto && proto->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_XML_PROTO_FORBIDDEN);
        return false;
    }

    



    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &ObjectClass, proto,
                                                     vp->toObject().getGlobal());
    if (!obj)
        return JS_FALSE;
    vp->setObject(*obj); 

    
    MarkTypeObjectUnknownProperties(cx, obj->type());

    
    if (argc > 1 && !vp[3].isUndefined()) {
        if (vp[3].isPrimitive()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
            return JS_FALSE;
        }

        if (!DefineProperties(cx, obj, &vp[3].toObject()))
            return JS_FALSE;
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
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, &keys))
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

    JSObject *aobj = NewDenseCopiedArray(cx, vals.length(), vals.begin());
    if (!aobj)
        return false;

    vp->setObject(*aobj);
    return true;
}

static JSBool
obj_isExtensible(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.isExtensible", &obj))
        return false;

    vp->setBoolean(obj->isExtensible());
    return true;
}

static JSBool
obj_preventExtensions(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.preventExtensions", &obj))
        return false;

    vp->setObject(*obj);
    if (!obj->isExtensible())
        return true;

    AutoIdVector props(cx);
    return obj->preventExtensions(cx, &props);
}

bool
JSObject::sealOrFreeze(JSContext *cx, ImmutabilityType it)
{
    assertSameCompartment(cx, this);
    JS_ASSERT(it == SEAL || it == FREEZE);

    AutoIdVector props(cx);
    if (isExtensible()) {
        if (!preventExtensions(cx, &props))
            return false;
    } else {
        if (!GetPropertyNames(cx, this, JSITER_HIDDEN | JSITER_OWNONLY, &props))
            return false;
    }

    
    JS_ASSERT(!isDenseArray());

    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];

        uintN attrs;
        if (!getAttributes(cx, id, &attrs))
            return false;

        
        uintN new_attrs;
        if (it == FREEZE && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
            new_attrs = JSPROP_PERMANENT | JSPROP_READONLY;
        else
            new_attrs = JSPROP_PERMANENT;

        
        if ((attrs | new_attrs) == attrs)
            continue;

        attrs |= new_attrs;
        if (!setAttributes(cx, id, &attrs))
            return false;
    }

    return true;
}

bool
JSObject::isSealedOrFrozen(JSContext *cx, ImmutabilityType it, bool *resultp)
{
    if (isExtensible()) {
        *resultp = false;
        return true;
    }

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, this, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];

        uintN attrs;
        if (!getAttributes(cx, id, &attrs))
            return false;

        




        if (!(attrs & JSPROP_PERMANENT) ||
            (it == FREEZE && !(attrs & (JSPROP_READONLY | JSPROP_GETTER | JSPROP_SETTER))))
        {
            *resultp = false;
            return true;
        }
    }

    
    *resultp = true;
    return true;
}

static JSBool
obj_freeze(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.freeze", &obj))
        return false;

    vp->setObject(*obj);

    return obj->freeze(cx);
}

static JSBool
obj_isFrozen(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.preventExtensions", &obj))
        return false;

    bool frozen;
    if (!obj->isFrozen(cx, &frozen))
        return false;
    vp->setBoolean(frozen);
    return true;
}

static JSBool
obj_seal(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.seal", &obj))
        return false;

    vp->setObject(*obj);

    return obj->seal(cx);
}

static JSBool
obj_isSealed(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj;
    if (!GetFirstArgumentAsObject(cx, argc, vp, "Object.isSealed", &obj))
        return false;

    bool sealed;
    if (!obj->isSealed(cx, &sealed))
        return false;
    vp->setBoolean(sealed);
    return true;
}

#if JS_HAS_OBJ_WATCHPOINT
const char js_watch_str[] = "watch";
const char js_unwatch_str[] = "unwatch";
#endif
const char js_hasOwnProperty_str[] = "hasOwnProperty";
const char js_isPrototypeOf_str[] = "isPrototypeOf";
const char js_propertyIsEnumerable_str[] = "propertyIsEnumerable";

JSFunctionSpec object_methods[] = {
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
    JS_FN(js_defineGetter_str,         js::obj_defineGetter,        2,0),
    JS_FN(js_defineSetter_str,         js::obj_defineSetter,        2,0),
    JS_FN(js_lookupGetter_str,         obj_lookupGetter,            1,0),
    JS_FN(js_lookupSetter_str,         obj_lookupSetter,            1,0),
#endif
    JS_FS_END
};

JSFunctionSpec object_static_methods[] = {
    JS_FN("getPrototypeOf",            obj_getPrototypeOf,          1,0),
    JS_FN("getOwnPropertyDescriptor",  obj_getOwnPropertyDescriptor,2,0),
    JS_FN("keys",                      obj_keys,                    1,0),
    JS_FN("defineProperty",            obj_defineProperty,          3,0),
    JS_FN("defineProperties",          obj_defineProperties,        2,0),
    JS_FN("create",                    obj_create,                  2,0),
    JS_FN("getOwnPropertyNames",       obj_getOwnPropertyNames,     1,0),
    JS_FN("isExtensible",              obj_isExtensible,            1,0),
    JS_FN("preventExtensions",         obj_preventExtensions,       1,0),
    JS_FN("freeze",                    obj_freeze,                  1,0),
    JS_FN("isFrozen",                  obj_isFrozen,                1,0),
    JS_FN("seal",                      obj_seal,                    1,0),
    JS_FN("isSealed",                  obj_isSealed,                1,0),
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
        gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
        obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
        if (!obj)
            return JS_FALSE;
        TypeObject *type = GetTypeCallerInitObject(cx, JSProto_Object);
        if (!type)
            return JS_FALSE;
        obj->setType(type);
    }
    vp->setObject(*obj);
    return JS_TRUE;
}

JSObject *
js::NewReshapedObject(JSContext *cx, TypeObject *type, JSObject *parent,
                      gc::AllocKind kind, const Shape *shape)
{
    JSObject *res = NewObjectWithType(cx, type, parent, kind);
    if (!res)
        return NULL;

    if (shape->isEmptyShape())
        return res;

    
    js::AutoIdVector ids(cx);
    for (unsigned i = 0; i <= shape->slot(); i++) {
        if (!ids.append(JSID_VOID))
            return NULL;
    }
    const js::Shape *nshape = shape;
    while (!nshape->isEmptyShape()) {
        ids[nshape->slot()] = nshape->propid();
        nshape = nshape->previous();
    }

    
    for (unsigned i = 0; i < ids.length(); i++) {
        if (!DefineNativeProperty(cx, res, ids[i], js::UndefinedValue(), NULL, NULL,
                                  JSPROP_ENUMERATE, 0, 0, DNP_SKIP_TYPE)) {
            return NULL;
        }
    }
    JS_ASSERT(!res->inDictionaryMode());

    return res;
}

JSObject*
js_CreateThis(JSContext *cx, JSObject *callee)
{
    Class *clasp = callee->getClass();

    Class *newclasp = &ObjectClass;
    if (clasp == &FunctionClass) {
        JSFunction *fun = callee->toFunction();
        if (fun->isNative() && fun->u.n.clasp)
            newclasp = fun->u.n.clasp;
    }

    Value protov;
    if (!callee->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &protov))
        return NULL;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : NULL;
    JSObject *parent = callee->getParent();
    gc::AllocKind kind = NewObjectGCKind(cx, newclasp);
    return NewObject<WithProto::Class>(cx, newclasp, proto, parent, kind);
}

static inline JSObject *
CreateThisForFunctionWithType(JSContext *cx, types::TypeObject *type, JSObject *parent)
{
    if (type->newScript) {
        




        gc::AllocKind kind = type->newScript->allocKind;
        JSObject *res = NewObjectWithType(cx, type, parent, kind);
        if (res)
            JS_ALWAYS_TRUE(res->setLastProperty(cx, (Shape *) type->newScript->shape));
        return res;
    }

    gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
    return NewObjectWithType(cx, type, parent, kind);
}

JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto)
{
    JSObject *res;

    if (proto) {
        types::TypeObject *type = proto->getNewType(cx, callee->toFunction());
        if (!type)
            return NULL;
        res = CreateThisForFunctionWithType(cx, type, callee->getParent());
    } else {
        gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
        res = NewNonFunction<WithProto::Class>(cx, &ObjectClass, proto, callee->getParent(), kind);
    }

    if (res && cx->typeInferenceEnabled())
        TypeScript::SetThis(cx, callee->toFunction()->script(), types::Type::ObjectType(res));

    return res;
}

JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee, bool newType)
{
    Value protov;
    if (!callee->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &protov))
        return NULL;
    JSObject *proto;
    if (protov.isObject())
        proto = &protov.toObject();
    else
        proto = NULL;
    JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);

    if (obj && newType) {
        



        obj->clear(cx);
        if (!obj->setSingletonType(cx))
            return NULL;

        JSScript *calleeScript = callee->toFunction()->script();
        TypeScript::SetThis(cx, calleeScript, types::Type::ObjectType(obj));
    }

    return obj;
}

#ifdef JS_TRACER

JSObject* FASTCALL
js_Object_tn(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(ObjectClass.flags & JSCLASS_HAS_PRIVATE));
    return NewObjectWithClassProto(cx, &ObjectClass, proto, FINALIZE_OBJECT8);
}

JS_DEFINE_TRCINFO_1(js_Object,
    (2, (extern, CONSTRUCTOR_RETRY, js_Object_tn, CONTEXT, CALLEE_PROTOTYPE, 0,
         nanojit::ACCSET_STORE_ANY)))

JSObject* FASTCALL
js_InitializerObject(JSContext* cx, JSObject *proto, JSObject *baseobj)
{
    if (!baseobj) {
        gc::AllocKind kind = GuessObjectGCKind(0);
        return NewObjectWithClassProto(cx, &ObjectClass, proto, kind);
    }

    
    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    return CopyInitializerObject(cx, baseobj, type);
}

JS_DEFINE_CALLINFO_3(extern, OBJECT, js_InitializerObject, CONTEXT, OBJECT, OBJECT,
                     0, nanojit::ACCSET_STORE_ANY)

JSObject* FASTCALL
js_String_tn(JSContext* cx, JSObject* proto, JSString* str)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    JS_ASSERT(proto);
    return StringObject::createWithProto(cx, str, *proto);
}
JS_DEFINE_CALLINFO_3(extern, OBJECT, js_String_tn, CONTEXT, CALLEE_PROTOTYPE, STRING, 0,
                     nanojit::ACCSET_STORE_ANY)

JSObject * FASTCALL
js_CreateThisFromTrace(JSContext *cx, JSObject *ctor, uintN protoSlot)
{
    JS_NOT_REACHED("FIXME");
    return NULL;
#if 0
#ifdef DEBUG
    JS_ASSERT(ctor->isFunction());
    JS_ASSERT(ctor->toFunction()->isInterpreted());
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    const Shape *shape = ctor->nativeLookup(cx, id);
    JS_ASSERT(shape->slot() == protoSlot);
    JS_ASSERT(!shape->configurable());
    JS_ASSERT(!shape->isMethod());
#endif

    JSObject *parent = ctor->getParent();
    JSObject *proto;
    const Value &protov = ctor->getSlot(protoSlot);
    if (protov.isObject()) {
        proto = &protov.toObject();
    } else {
        



        if (!js_GetClassPrototype(cx, parent, JSProto_Object, &proto))
            return NULL;
    }

    gc::AllocKind kind = NewObjectGCKind(cx, &ObjectClass);
    return NewNativeClassInstance(cx, &ObjectClass, proto, parent, kind);
#endif
}
JS_DEFINE_CALLINFO_3(extern, CONSTRUCTOR_RETRY, js_CreateThisFromTrace, CONTEXT, OBJECT, UINTN, 0,
                     nanojit::ACCSET_STORE_ANY)

#else  

# define js_Object_trcinfo NULL

#endif 






JS_REQUIRES_STACK JSBool
Detecting(JSContext *cx, jsbytecode *pc)
{
    jsbytecode *endpc;
    JSOp op;
    JSAtom *atom;

    JSScript *script = cx->stack.currentScript();
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
        return JS_TRACE_MONITOR_ON_TRACE(cx)->bailExit->lookupFlags;
#endif

    JS_ASSERT_NOT_ON_TRACE(cx);

    const JSCodeSpec *cs;
    uint32 format;
    uintN flags = 0;

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    if (!script || !pc)
        return defaultFlags;

    cs = &js_CodeSpec[js_GetOpcode(cx, script, pc)];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if (format & JOF_SET) {
        flags |= JSRESOLVE_ASSIGNING;
    } else if (cs->length >= 0) {
        pc += cs->length;
        if (pc < script->code + script->length && Detecting(cx, pc))
            flags |= JSRESOLVE_DETECTING;
    }
    if (format & JOF_DECLARING)
        flags |= JSRESOLVE_DECLARING;
    return flags;
}




static JSBool
with_LookupGeneric(JSContext *cx, JSObject *obj, jsid id, JSObject **objp, JSProperty **propp)
{
    
    uintN flags = cx->resolveFlags;
    if (flags == RESOLVE_INFER)
        flags = js_InferFlags(cx, flags);
    flags |= JSRESOLVE_WITH;
    JSAutoResolveFlags rf(cx, flags);
    return obj->getProto()->lookupGeneric(cx, id, objp, propp);
}

static JSBool
with_LookupProperty(JSContext *cx, JSObject *obj, PropertyName *name, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, ATOM_TO_JSID(name), objp, propp);
}

static JSBool
with_LookupElement(JSContext *cx, JSObject *obj, uint32 index, JSObject **objp,
                   JSProperty **propp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_LookupGeneric(cx, obj, id, objp, propp);
}

static JSBool
with_LookupSpecial(JSContext *cx, JSObject *obj, SpecialId sid, JSObject **objp, JSProperty **propp)
{
    return with_LookupGeneric(cx, obj, SPECIALID_TO_JSID(sid), objp, propp);
}

static JSBool
with_GetGeneric(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    return obj->getProto()->getGeneric(cx, id, vp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name, Value *vp)
{
    return with_GetGeneric(cx, obj, receiver, ATOM_TO_JSID(name), vp);
}

static JSBool
with_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_GetGeneric(cx, obj, receiver, id, vp);
}

static JSBool
with_GetSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, SpecialId sid, Value *vp)
{
    return with_GetGeneric(cx, obj, receiver, SPECIALID_TO_JSID(sid), vp);
}

static JSBool
with_SetProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return obj->getProto()->setProperty(cx, id, vp, strict);
}

static JSBool
with_SetElement(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_SetProperty(cx, obj, id, vp, strict);
}

static JSBool
with_SetSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict)
{
    return with_SetProperty(cx, obj, SPECIALID_TO_JSID(sid), vp, strict);
}

static JSBool
with_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj->getProto()->getAttributes(cx, id, attrsp);
}

static JSBool
with_GetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_GetAttributes(cx, obj, id, attrsp);
}

static JSBool
with_GetSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, uintN *attrsp)
{
    return with_GetAttributes(cx, obj, SPECIALID_TO_JSID(sid), attrsp);
}

static JSBool
with_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj->getProto()->setAttributes(cx, id, attrsp);
}

static JSBool
with_SetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_SetAttributes(cx, obj, id, attrsp);
}

static JSBool
with_SetSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, uintN *attrsp)
{
    return with_SetAttributes(cx, obj, SPECIALID_TO_JSID(sid), attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    return obj->getProto()->deleteProperty(cx, id, rval, strict);
}

static JSBool
with_DeleteElement(JSContext *cx, JSObject *obj, uint32 index, Value *rval, JSBool strict)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return with_DeleteProperty(cx, obj, id, rval, strict);
}

static JSBool
with_DeleteSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *rval, JSBool strict)
{
    return with_DeleteProperty(cx, obj, SPECIALID_TO_JSID(sid), rval, strict);
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

Class js::WithClass = {
    "With",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(3) | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
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
        with_LookupGeneric,
        with_LookupProperty,
        with_LookupElement,
        with_LookupSpecial,
        NULL,             
        NULL,             
        NULL,             
        NULL,             
        with_GetGeneric,
        with_GetProperty,
        with_GetElement,
        with_GetSpecial,
        with_SetProperty,
        with_SetProperty,
        with_SetElement,
        with_SetSpecial,
        with_GetAttributes,
        with_GetAttributes,
        with_GetElementAttributes,
        with_GetSpecialAttributes,
        with_SetAttributes,
        with_SetAttributes,
        with_SetElementAttributes,
        with_SetSpecialAttributes,
        with_DeleteProperty,
        with_DeleteProperty,
        with_DeleteElement,
        with_DeleteSpecial,
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

    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    obj = js_NewGCObject(cx, FINALIZE_OBJECT4);
    if (!obj)
        return NULL;

    StackFrame *priv = js_FloatingFrameIfGenerator(cx, cx->fp());

    obj->init(cx, type);

    Shape *emptyWithShape = BaseShape::lookupInitialShape(cx, &WithClass,
                                                          parent->getGlobal(),
                                                          FINALIZE_OBJECT4);
    if (!emptyWithShape)
        return NULL;

    obj->setInitialPropertyInfallible(emptyWithShape);
    OBJ_SET_BLOCK_DEPTH(cx, obj, depth);

    obj->setScopeChain(parent);
    obj->setPrivate(priv);

    AutoObjectRooter tvr(cx, obj);
    JSObject *thisp = proto->thisObject(cx);
    if (!thisp)
        return NULL;

    assertSameCompartment(cx, obj, thisp);

    obj->setWithThis(thisp);
    return obj;
}

JSObject *
js_NewBlockObject(JSContext *cx)
{
    



    JSObject *blockObj = js_NewGCObject(cx, FINALIZE_OBJECT4);
    if (!blockObj)
        return NULL;

    types::TypeObject *type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    Shape *emptyBlockShape = BaseShape::lookupInitialShape(cx, &BlockClass, NULL,
                                                           FINALIZE_OBJECT4);
    if (!emptyBlockShape)
        return NULL;

    blockObj->init(cx, type);
    blockObj->setInitialPropertyInfallible(emptyBlockShape);

    return blockObj;
}

static const uint32 BLOCK_RESERVED_SLOTS = 2;

JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, StackFrame *fp)
{
    JS_ASSERT(proto->isStaticBlock());

    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;

    JSObject *clone = js_NewGCObject(cx, FINALIZE_OBJECT4);
    if (!clone)
        return NULL;

    StackFrame *priv = js_FloatingFrameIfGenerator(cx, fp);

    
    if (!clone->initClonedBlock(cx, type, priv))
        return NULL;

    JS_ASSERT(clone->slotSpan() >= OBJ_BLOCK_COUNT(cx, proto) + BLOCK_RESERVED_SLOTS);

    clone->setSlot(JSSLOT_BLOCK_DEPTH, proto->getSlot(JSSLOT_BLOCK_DEPTH));

    JS_ASSERT(clone->isClonedBlock());
    return clone;
}

JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind)
{
    StackFrame *const fp = cx->fp();
    JSObject *obj = &fp->scopeChain();
    JS_ASSERT(obj->isClonedBlock());
    JS_ASSERT(obj->getPrivate() == js_FloatingFrameIfGenerator(cx, cx->fp()));

    
    uintN count = OBJ_BLOCK_COUNT(cx, obj);
    JS_ASSERT(obj->slotSpan() >= JSSLOT_BLOCK_DEPTH + 1 + count);

    
    uintN depth = OBJ_BLOCK_DEPTH(cx, obj);
    JS_ASSERT(depth <= size_t(cx->regs().sp - fp->base()));
    JS_ASSERT(count <= size_t(cx->regs().sp - fp->base() - depth));

    
    JS_ASSERT(count >= 1);

    if (normalUnwind) {
        uintN slot = JSSLOT_BLOCK_FIRST_FREE_SLOT;
        depth += fp->numFixed();
        obj->copySlotRange(slot, fp->slots() + depth, count);
    }

    
    obj->setPrivate(NULL);
    fp->setScopeChainNoCallObj(*obj->scopeChain());
    return normalUnwind;
}

static JSBool
block_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    




    JS_ASSERT(obj->isClonedBlock());
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    StackFrame *fp = (StackFrame *) obj->getPrivate();
    if (fp) {
        fp = js_LiveFrameIfGenerator(fp);
        index += fp->numFixed() + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->numSlots());
        *vp = fp->slots()[index];
        return true;
    }

    
    JS_ASSERT(obj->getSlot(JSSLOT_FREE(&BlockClass) + index) == *vp);
    return true;
}

static JSBool
block_setProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    JS_ASSERT(obj->isClonedBlock());
    uintN index = (uintN) JSID_TO_INT(id);
    JS_ASSERT(index < OBJ_BLOCK_COUNT(cx, obj));

    StackFrame *fp = (StackFrame *) obj->getPrivate();
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

    



    uint32 slot = JSSLOT_FREE(&BlockClass) + index;
    const Shape *shape = addProperty(cx, id,
                                     block_getProperty, block_setProperty,
                                     slot, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                     Shape::HAS_SHORTID, index,
                                      false);
    if (!shape)
        return NULL;
    return shape;
}

JSBool
JSObject::nonNativeSetProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict)
{
    if (JS_UNLIKELY(watched())) {
        id = js_CheckForStringIndex(id);
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, this, id, vp))
            return false;
    }
    return getOps()->setProperty(cx, this, id, vp, strict);
}

JSBool
JSObject::nonNativeSetElement(JSContext *cx, uint32 index, js::Value *vp, JSBool strict)
{
    if (JS_UNLIKELY(watched())) {
        jsid id;
        if (!IndexToId(cx, index, &id))
            return false;
        JS_ASSERT(id == js_CheckForStringIndex(id));
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, this, id, vp))
            return false;
    }
    return getOps()->setElement(cx, this, index, vp, strict);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *target, JSObject *obj)
{
    
    JS_ASSERT(target->isNative() == obj->isNative());
    if (!target->isNative())
        return true;

    AutoShapeVector shapes(cx);
    for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
        if (!shapes.append(&r.front()))
            return false;
    }

    size_t n = shapes.length();
    while (n > 0) {
        const Shape *shape = shapes[--n];
        uintN attrs = shape->attributes();
        PropertyOp getter = shape->getter();
        if ((attrs & JSPROP_GETTER) && !cx->compartment->wrap(cx, &getter))
            return false;
        StrictPropertyOp setter = shape->setter();
        if ((attrs & JSPROP_SETTER) && !cx->compartment->wrap(cx, &setter))
            return false;
        Value v = shape->hasSlot() ? obj->getSlot(shape->slot()) : UndefinedValue();
        if (!cx->compartment->wrap(cx, &v))
            return false;
        if (!target->defineProperty(cx, shape->propid(), v, getter, setter, attrs))
            return false;
    }
    return true;
}

static bool
CopySlots(JSContext *cx, JSObject *from, JSObject *to)
{
    JS_ASSERT(!from->isNative() && !to->isNative());
    JS_ASSERT(from->getClass() == to->getClass());

    size_t n = 0;
    if (from->isWrapper() &&
        (Wrapper::wrapperHandler(from)->flags() & Wrapper::CROSS_COMPARTMENT)) {
        to->setSlot(0, from->getSlot(0));
        to->setSlot(1, from->getSlot(1));
        n = 2;
    }

    size_t span = JSCLASS_RESERVED_SLOTS(from->getClass());
    for (; n < span; ++n) {
        Value v = from->getSlot(n);
        if (!cx->compartment->wrap(cx, &v))
            return false;
        to->setSlot(n, v);
    }
    return true;
}

JS_FRIEND_API(JSObject *)
JS_CloneObject(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent)
{
    



    if (!obj->isNative()) {
        if (obj->isDenseArray()) {
            if (!obj->makeDenseArraySlow(cx))
                return NULL;
        } else if (!obj->isProxy()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }
    }
    JSObject *clone = NewObject<WithProto::Given>(cx, obj->getClass(), proto, parent, obj->getAllocKind());
    if (!clone)
        return NULL;
    if (obj->isNative()) {
        if (clone->isFunction() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }

        if (obj->hasPrivate())
            clone->setPrivate(obj->getPrivate());
    } else {
        JS_ASSERT(obj->isProxy());
        if (!CopySlots(cx, obj, clone))
            return NULL;
    }

    return clone;
}

struct JSObject::TradeGutsReserved {
    JSContext *cx;
    Vector<Value> avals;
    Vector<Value> bvals;
    int newafixed;
    int newbfixed;
    Shape *newashape;
    Shape *newbshape;
    Value *newaslots;
    Value *newbslots;

    TradeGutsReserved(JSContext *cx)
        : cx(cx), avals(cx), bvals(cx),
          newafixed(0), newbfixed(0),
          newashape(NULL), newbshape(NULL),
          newaslots(NULL), newbslots(NULL)
    {}

    ~TradeGutsReserved()
    {
        if (newaslots)
            cx->free_(newaslots);
        if (newbslots)
            cx->free_(newbslots);
    }
};

bool
JSObject::ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                              TradeGutsReserved &reserved)
{
    





    if (a->structSize() == b->structSize())
        return true;

    





    if (a->isNative()) {
        if (!a->generateOwnShape(cx))
            return false;
    } else {
        reserved.newbshape = BaseShape::lookupInitialShape(cx, a->getClass(), a->getParent(),
                                                           b->getAllocKind());
        if (!reserved.newbshape)
            return false;
    }
    if (b->isNative()) {
        if (!b->generateOwnShape(cx))
            return false;
    } else {
        reserved.newashape = BaseShape::lookupInitialShape(cx, b->getClass(), b->getParent(),
                                                           a->getAllocKind());
        if (!reserved.newashape)
            return false;
    }

    

    if (!reserved.avals.reserve(a->slotSpan()))
        return false;
    if (!reserved.bvals.reserve(b->slotSpan()))
        return false;

    JS_ASSERT(a->elements == emptyObjectElements);
    JS_ASSERT(b->elements == emptyObjectElements);

    





    reserved.newafixed = a->numFixedSlots();
    reserved.newbfixed = b->numFixedSlots();

    if (a->hasPrivate()) {
        reserved.newafixed++;
        reserved.newbfixed--;
    }
    if (b->hasPrivate()) {
        reserved.newbfixed++;
        reserved.newafixed--;
    }

    JS_ASSERT(reserved.newafixed >= 0);
    JS_ASSERT(reserved.newbfixed >= 0);

    





    unsigned adynamic = dynamicSlotsCount(reserved.newafixed, b->slotSpan());
    unsigned bdynamic = dynamicSlotsCount(reserved.newbfixed, a->slotSpan());

    if (adynamic) {
        reserved.newaslots = (Value *) cx->malloc_(sizeof(Value) * adynamic);
        if (!reserved.newaslots)
            return false;
        Debug_SetValueRangeToCrashOnTouch(reserved.newaslots, adynamic);
    }
    if (bdynamic) {
        reserved.newbslots = (Value *) cx->malloc_(sizeof(Value) * bdynamic);
        if (!reserved.newbslots)
            return false;
        Debug_SetValueRangeToCrashOnTouch(reserved.newbslots, bdynamic);
    }

    return true;
}

void
JSObject::TradeGuts(JSContext *cx, JSObject *a, JSObject *b, TradeGutsReserved &reserved)
{
    JS_ASSERT(a->compartment() == b->compartment());
    JS_ASSERT(a->isFunction() == b->isFunction());

    
    JS_ASSERT_IF(a->isFunction(), a->structSize() == b->structSize());

    




    JS_ASSERT(!a->isRegExp() && !b->isRegExp());

    



    JS_ASSERT(!a->isDenseArray() && !b->isDenseArray());
    JS_ASSERT(!a->isArrayBuffer() && !b->isArrayBuffer());

    
    const size_t size = a->structSize();
    if (size == b->structSize()) {
        




        char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::result];
        JS_ASSERT(size <= sizeof(tmp));

        memcpy(tmp, a, size);
        memcpy(a, b, size);
        memcpy(b, tmp, size);
    } else {
        





        unsigned acap = a->slotSpan();
        unsigned bcap = b->slotSpan();

        for (size_t i = 0; i < acap; i++)
            reserved.avals.infallibleAppend(a->getSlot(i));

        for (size_t i = 0; i < bcap; i++)
            reserved.bvals.infallibleAppend(b->getSlot(i));

        
        if (a->hasDynamicSlots())
            cx->free_(a->slots);
        if (b->hasDynamicSlots())
            cx->free_(b->slots);

        void *apriv = a->hasPrivate() ? a->getPrivate() : NULL;
        void *bpriv = b->hasPrivate() ? b->getPrivate() : NULL;

        JSObject tmp;
        memcpy(&tmp, a, sizeof tmp);
        memcpy(a, b, sizeof tmp);
        memcpy(b, &tmp, sizeof tmp);

        if (a->isNative())
            a->shape_->setNumFixedSlots(reserved.newafixed);
        else
            a->shape_ = reserved.newashape;

        a->slots = reserved.newaslots;
        a->copySlotRange(0, reserved.bvals.begin(), bcap);
        if (a->hasPrivate())
            a->setPrivate(bpriv);

        if (b->isNative())
            b->shape_->setNumFixedSlots(reserved.newbfixed);
        else
            b->shape_ = reserved.newbshape;

        b->slots = reserved.newbslots;
        b->copySlotRange(0, reserved.avals.begin(), acap);
        if (b->hasPrivate())
            b->setPrivate(apriv);

        
        reserved.newaslots = NULL;
        reserved.newbslots = NULL;
    }
}







bool
JSObject::swap(JSContext *cx, JSObject *other)
{
    if (this->compartment() == other->compartment()) {
        TradeGutsReserved reserved(cx);
        if (!ReserveForTradeGuts(cx, this, other, reserved))
            return false;
        TradeGuts(cx, this, other, reserved);
        return true;
    }

    JSObject *thisClone;
    JSObject *otherClone;
    {
        AutoCompartment ac(cx, other);
        if (!ac.enter())
            return false;
        thisClone = JS_CloneObject(cx, this, other->getProto(), other->getParent());
        if (!thisClone || !JS_CopyPropertiesFrom(cx, thisClone, this))
            return false;
    }
    {
        AutoCompartment ac(cx, this);
        if (!ac.enter())
            return false;
        otherClone = JS_CloneObject(cx, other, other->getProto(), other->getParent());
        if (!otherClone || !JS_CopyPropertiesFrom(cx, otherClone, other))
            return false;
    }

    TradeGutsReserved reservedThis(cx);
    TradeGutsReserved reservedOther(cx);

    if (!ReserveForTradeGuts(cx, this, otherClone, reservedThis) ||
        !ReserveForTradeGuts(cx, other, thisClone, reservedOther)) {
        return false;
    }

    TradeGuts(cx, this, otherClone, reservedThis);
    TradeGuts(cx, other, thisClone, reservedOther);

    return true;
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
        parent = obj->getStaticBlockScopeChain();
        parentId = JSScript::isValidOffset(xdr->script->objectsOffset)
                   ? FindObjectIndex(xdr->script->objects(), parent)
                   : NO_PARENT_INDEX;
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

        




        if (parentId != NO_PARENT_INDEX) {
            parent = xdr->script->getObject(parentId);
            obj->setScopeChain(parent);
        }
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

            
            if (!js_XDRAtom(xdr, &atom))
                return false;

            if (!obj->defineBlockVariable(cx, ATOM_TO_JSID(atom), i))
                return false;
        }
    } else {
        AutoShapeVector shapes(cx);
        shapes.growBy(count);

        for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
            shape = &r.front();
            shapes[shape->shortid()] = shape;
        }

        



        for (uintN i = 0; i < count; i++) {
            shape = shapes[i];
            JS_ASSERT(shape->getter() == block_getProperty);

            jsid propid = shape->propid();
            JS_ASSERT(JSID_IS_ATOM(propid));
            JSAtom *atom = JSID_TO_ATOM(propid);

#ifdef DEBUG
            uint16 shortid = uint16(shape->shortid());
            JS_ASSERT(shortid == i);
#endif

            
            if (!js_XDRAtom(xdr, &atom))
                return false;
        }
    }
    return true;
}

#endif

Class js::BlockClass = {
    "Block",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(BLOCK_RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static bool
DefineStandardSlot(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                   const Value &v, uint32 attrs, bool &named)
{
    jsid id = ATOM_TO_JSID(atom);

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->isGlobal());
        JS_ASSERT(obj->isNative());

        const Shape *shape = obj->nativeLookup(cx, id);
        if (!shape) {
            uint32 slot = 2 * JSProto_LIMIT + key;
            if (!js_SetReservedSlot(cx, obj, slot, v))
                return false;
            if (!obj->addProperty(cx, id, JS_PropertyStub, JS_StrictPropertyStub, slot, attrs, 0, 0))
                return false;
            AddTypePropertyId(cx, obj, id, v);

            named = true;
            return true;
        }
    }

    named = obj->defineProperty(cx, id, v, JS_PropertyStub, JS_StrictPropertyStub, attrs);
    return named;
}

namespace js {

static bool
SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return true;

    return js_SetReservedSlot(cx, obj, key, ObjectOrNullValue(cobj)) &&
           js_SetReservedSlot(cx, obj, JSProto_LIMIT + key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSContext *cx, JSObject *obj, JSProtoKey key)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return;

    obj->setSlot(key, UndefinedValue());
    obj->setSlot(JSProto_LIMIT + key, UndefinedValue());
}

JSObject *
DefineConstructorAndPrototype(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, uintN nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
                              JSObject **ctorp, AllocKind ctorKind)
{
    





















    






    JSObject *proto = NewObject<WithProto::Class>(cx, clasp, protoProto, obj);
    if (!proto)
        return NULL;

    if (!proto->setSingletonType(cx))
        return NULL;

    if (clasp == &ArrayClass && !proto->makeDenseArraySlow(cx))
        return NULL;

    TypeObject *type = proto->getNewType(cx);
    if (!type || !type->getEmptyShape(cx, proto->getClass(), FINALIZE_OBJECT0))
        return NULL;

    
    JSObject *ctor;
    bool named = false;
    bool cached = false;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) || !obj->isGlobal() || key == JSProto_Null) {
            uint32 attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            if (!DefineStandardSlot(cx, obj, key, atom, ObjectValue(*proto), attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        





        JSFunction *fun =
            js_NewFunction(cx, NULL, constructor, nargs, JSFUN_CONSTRUCTOR, obj, atom,
                           ctorKind);
        if (!fun)
            goto bad;
        fun->setConstructorClass(clasp);

        




        if (key != JSProto_Null && !(clasp->flags & JSCLASS_CONSTRUCT_PROTOTYPE)) {
            if (!SetClassObject(cx, obj, key, fun, proto))
                goto bad;
            cached = true;
        }

        AutoValueRooter tvr2(cx, ObjectValue(*fun));
        if (!DefineStandardSlot(cx, obj, key, atom, tvr2.value(), 0, named))
            goto bad;

        





        ctor = fun;
        if (clasp->flags & JSCLASS_CONSTRUCT_PROTOTYPE) {
            Value rval;
            if (!InvokeConstructorWithGivenThis(cx, proto, ObjectOrNullValue(ctor),
                                                0, NULL, &rval)) {
                goto bad;
            }
            if (rval.isObject() && &rval.toObject() != proto)
                proto = &rval.toObject();
        }

        if (!LinkConstructorAndPrototype(cx, ctor, proto))
            goto bad;

        
        if (ctor->getClass() == clasp && !ctor->splicePrototype(cx, proto))
            goto bad;
    }

    if (!DefinePropertiesAndBrand(cx, proto, ps, fs) ||
        (ctor != proto && !DefinePropertiesAndBrand(cx, ctor, static_ps, static_fs)))
    {
        goto bad;
    }

    if (clasp->flags & (JSCLASS_FREEZE_PROTO|JSCLASS_FREEZE_CTOR)) {
        JS_ASSERT_IF(ctor == proto, !(clasp->flags & JSCLASS_FREEZE_CTOR));
        if (proto && (clasp->flags & JSCLASS_FREEZE_PROTO) && !proto->freeze(cx))
            goto bad;
        if (ctor && (clasp->flags & JSCLASS_FREEZE_CTOR) && !ctor->freeze(cx))
            goto bad;
    }

    
    if (!cached && key != JSProto_Null && !SetClassObject(cx, obj, key, ctor, proto))
        goto bad;

    if (ctorp)
        *ctorp = ctor;
    return proto;

bad:
    if (named) {
        Value rval;
        obj->deleteProperty(cx, ATOM_TO_JSID(atom), &rval, false);
    }
    if (cached)
        ClearClassObject(cx, obj, key);
    return NULL;
}










bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    
    return (obj->getReservedSlot(key) != UndefinedValue());
}

void
MarkStandardClassInitializedNoProto(JSObject* obj, js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);

    



    if (obj->getReservedSlot(key) == UndefinedValue())
        obj->setSlot(key, BooleanValue(true));
}

}

JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *protoProto,
             Class *clasp, Native constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs,
             JSObject **ctorp, AllocKind ctorKind)
{
    JSAtom *atom = js_Atomize(cx, clasp->name, strlen(clasp->name));
    if (!atom)
        return NULL;

    











    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !protoProto &&
        !js_GetClassPrototype(cx, obj, JSProto_Object, &protoProto)) {
        return NULL;
    }

    return DefineConstructorAndPrototype(cx, obj, key, atom, protoProto, clasp, constructor, nargs,
                                         ps, fs, static_ps, static_fs, ctorp, ctorKind);
}

void
JSObject::copySlotRange(size_t start, const Value *vector, size_t length)
{
    JS_ASSERT(!isDenseArray());
    JS_ASSERT(slotInRange(start + length,  true));
    size_t fixed = numFixedSlots();
    if (start < fixed) {
        if (start + length < fixed) {
            memcpy(fixedSlots() + start, vector, length * sizeof(Value));
        } else {
            size_t localCopy = fixed - start;
            memcpy(fixedSlots() + start, vector, localCopy * sizeof(Value));
            memcpy(slots, vector + localCopy, (length - localCopy) * sizeof(Value));
        }
    } else {
        memcpy(slots + start - fixed, vector, length * sizeof(Value));
    }
}

inline void
JSObject::clearSlotRange(size_t start, size_t length)
{
    JS_ASSERT(!isDenseArray());
    JS_ASSERT(slotInRange(start + length,  true));
    size_t fixed = numFixedSlots();
    if (start < fixed) {
        if (start + length < fixed) {
            ClearValueRange(fixedSlots() + start, length);
        } else {
            size_t localClear = fixed - start;
            ClearValueRange(fixedSlots() + start, localClear);
            ClearValueRange(slots, length - localClear);
        }
    } else {
        ClearValueRange(slots + start - fixed, length);
    }
}

inline void
JSObject::invalidateSlotRange(size_t start, size_t length)
{
#ifdef DEBUG
    JS_ASSERT(!isDenseArray());

    size_t fixed = numFixedSlots();
    size_t numSlots = fixed + numDynamicSlots();

    
    JS_ASSERT(start <= numSlots);
    if (start + length > numSlots)
        length = numSlots - start;

    JS_ASSERT(slotInRange(start + length,  true));
    if (start < fixed) {
        if (start + length < fixed) {
            Debug_SetValueRangeToCrashOnTouch(fixedSlots() + start, length);
        } else {
            size_t localClear = fixed - start;
            Debug_SetValueRangeToCrashOnTouch(fixedSlots() + start, localClear);
            Debug_SetValueRangeToCrashOnTouch(slots, length - localClear);
        }
    } else {
        Debug_SetValueRangeToCrashOnTouch(slots + start - fixed, length);
    }
#endif 
}

inline void
JSObject::updateSlotsForSpan(size_t oldSpan, size_t newSpan)
{
    JS_ASSERT(oldSpan != newSpan);

    if (newSpan == oldSpan + 1) {
        setSlot(oldSpan, UndefinedValue());
        return;
    }

    if (oldSpan < newSpan)
        clearSlotRange(oldSpan, newSpan - oldSpan);
    else
        invalidateSlotRange(newSpan, oldSpan - newSpan);
}

bool
JSObject::setInitialProperty(JSContext *cx, const js::Shape *shape)
{
    JS_ASSERT(isNewborn());
    JS_ASSERT(shape->compartment() == compartment());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(gc::GetGCKindSlots(getAllocKind(), shape->getObjectClass()) == shape->numFixedSlots());

    size_t span = shape->slotSpan();

    if (span) {
        size_t count = dynamicSlotsCount(shape->numFixedSlots(), span);
        if (count && !growSlots(cx, 0, count))
            return false;
    }

    shape_ = const_cast<js::Shape *>(shape);
    if (hasPrivate())
        setPrivate(NULL);

    if (span)
        updateSlotsForSpan(0, span);

    return true;
}

void
JSObject::setInitialPropertyInfallible(const js::Shape *shape)
{
    JS_ASSERT(isNewborn());
    JS_ASSERT(shape->compartment() == compartment());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(gc::GetGCKindSlots(getAllocKind(), shape->getObjectClass()) == shape->numFixedSlots());
    JS_ASSERT_IF(shape->getObjectClass()->ext.equality && !hasSingletonType(),
                 type()->hasAnyFlags(js::types::OBJECT_FLAG_SPECIAL_EQUALITY));

    JS_ASSERT(dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan()) == 0);

    shape_ = const_cast<js::Shape *>(shape);
    if (hasPrivate())
        setPrivate(NULL);

    size_t span = shape->slotSpan();
    if (span)
        updateSlotsForSpan(0, span);
}

bool
JSObject::setLastProperty(JSContext *cx, const js::Shape *shape)
{
    JS_ASSERT(!isNewborn());
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(shape->compartment() == compartment());
    JS_ASSERT(shape->numFixedSlots() == numFixedSlots());

    size_t oldSpan = lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        shape_ = const_cast<js::Shape *>(shape);
        return true;
    }

    size_t oldCount = dynamicSlotsCount(numFixedSlots(), oldSpan);
    size_t newCount = dynamicSlotsCount(numFixedSlots(), newSpan);
    if (!changeSlots(cx, oldCount, newCount))
        return false;

    shape_ = const_cast<js::Shape *>(shape);
    updateSlotsForSpan(oldSpan, newSpan);

    return true;
}

bool
JSObject::setSlotSpan(JSContext *cx, uint32 span)
{
    JS_ASSERT(inDictionaryMode());
    js::BaseShape *base = lastProperty()->base();

    size_t oldSpan = base->slotSpan();

    if (oldSpan == span)
        return true;

    size_t oldCount = dynamicSlotsCount(numFixedSlots(), oldSpan);
    size_t newCount = dynamicSlotsCount(numFixedSlots(), span);
    if (!changeSlots(cx, oldCount, newCount))
        return false;

    base->setSlotSpan(span);
    updateSlotsForSpan(oldSpan, span);

    return true;
}

bool
JSObject::growSlots(JSContext *cx, uint32 oldCount, uint32 newCount)
{
    JS_ASSERT(newCount > oldCount);
    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);
    JS_ASSERT_IF(!isNewborn(), !isDenseArray());

    





    JS_ASSERT_IF(!isNewborn() && isCall(), asCall().maybeStackFrame() != NULL);

    




    JS_ASSERT(newCount < NELEMENTS_LIMIT);

    size_t oldSize = Probes::objectResizeActive() ? slotsAndStructSize() : 0;
    size_t newSize = oldSize + (newCount - oldCount) * sizeof(Value);

    if (!oldCount) {
        slots = (Value *) cx->malloc_(newCount * sizeof(Value));
        if (!slots)
            return false;
        Debug_SetValueRangeToCrashOnTouch(slots, newCount);
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, this, oldSize, newSize);
        return true;
    }

    Value *tmpslots = (Value*) cx->realloc_(slots, oldCount * sizeof(Value),
                                            newCount * sizeof(Value));
    if (!tmpslots)
        return false;  

    bool changed = slots != tmpslots;
    slots = tmpslots;

    Debug_SetValueRangeToCrashOnTouch(slots + oldCount, newCount - oldCount);

    
    if (changed && isGlobal())
        types::MarkObjectStateChange(cx, this);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, newSize);

    return true;
}

void
JSObject::shrinkSlots(JSContext *cx, uint32 oldCount, uint32 newCount)
{
    JS_ASSERT(newCount < oldCount);
    JS_ASSERT(!isDenseArray());

    





    if (isCall())
        return;

    size_t oldSize = Probes::objectResizeActive() ? slotsAndStructSize() : 0;
    size_t newSize = oldSize - (oldCount - newCount) * sizeof(Value);

    if (newCount == 0) {
        cx->free_(slots);
        slots = NULL;
        if (Probes::objectResizeActive())
            Probes::resizeObject(cx, this, oldSize, newSize);
        return;
    }

    JS_ASSERT(newCount >= SLOT_CAPACITY_MIN);

    Value *tmpslots = (Value*) cx->realloc_(slots, newCount * sizeof(Value));
    if (!tmpslots)
        return;  

    bool changed = slots != tmpslots;
    slots = tmpslots;

    
    if (changed && isGlobal())
        types::MarkObjectStateChange(cx, this);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, newSize);
}

bool
JSObject::growElements(JSContext *cx, uintN newcap)
{
    JS_ASSERT(isDenseArray());

    






    static const size_t CAPACITY_DOUBLING_MAX = 1024 * 1024;
    static const size_t CAPACITY_CHUNK = CAPACITY_DOUBLING_MAX / sizeof(Value);

    uint32 oldcap = getDenseArrayCapacity();
    JS_ASSERT(oldcap <= newcap);

    size_t oldSize = Probes::objectResizeActive() ? slotsAndStructSize() : 0;

    uint32 nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                    ? oldcap * 2
                    : oldcap + (oldcap >> 3);

    uint32 actualCapacity = JS_MAX(newcap, nextsize);
    if (actualCapacity >= CAPACITY_CHUNK)
        actualCapacity = JS_ROUNDUP(actualCapacity, CAPACITY_CHUNK);
    else if (actualCapacity < SLOT_CAPACITY_MIN)
        actualCapacity = SLOT_CAPACITY_MIN;

    
    if (actualCapacity >= NELEMENTS_LIMIT || actualCapacity < oldcap || actualCapacity < newcap) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    JS_STATIC_ASSERT(sizeof(ObjectElements) == 2 * sizeof(js::Value));

    uint32 initlen = getDenseArrayInitializedLength();

    ObjectElements *tmpheader;
    if (hasDynamicElements()) {
        tmpheader = (ObjectElements *)
            cx->realloc_(getElementsHeader(), (oldcap + 2) * sizeof(Value),
                         (actualCapacity + 2) * sizeof(Value));
        if (!tmpheader)
            return false;  
    } else {
        tmpheader = (ObjectElements *) cx->malloc_((actualCapacity + 2) * sizeof(Value));
        if (!tmpheader)
            return false;  
        memcpy(tmpheader, getElementsHeader(), (initlen + 2) * sizeof(Value));
    }

    tmpheader->capacity = actualCapacity;
    elements = tmpheader->elements();

    Debug_SetValueRangeToCrashOnTouch(elements + initlen, actualCapacity - initlen);

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, slotsAndStructSize());

    return true;
}

void
JSObject::shrinkElements(JSContext *cx, uintN newcap)
{
    JS_ASSERT(isDenseArray());

    uint32 oldcap = getDenseArrayCapacity();
    JS_ASSERT(newcap <= oldcap);

    size_t oldSize = Probes::objectResizeActive() ? slotsAndStructSize() : 0;

    
    if (oldcap <= SLOT_CAPACITY_MIN || !hasDynamicElements())
        return;

    newcap = Max(newcap, SLOT_CAPACITY_MIN);

    JS_STATIC_ASSERT(sizeof(ObjectElements) == 2 * sizeof(js::Value));

    ObjectElements *tmpheader = (ObjectElements *)
        cx->realloc_(getElementsHeader(), (newcap + 2) * sizeof(Value));
    if (!tmpheader)
        return;  

    tmpheader->capacity = newcap;
    elements = tmpheader->elements();

    if (Probes::objectResizeActive())
        Probes::resizeObject(cx, this, oldSize, slotsAndStructSize());
}

#ifdef DEBUG
bool
JSObject::slotInRange(uintN slot, bool sentinelAllowed) const
{
    size_t capacity = numFixedSlots() + numDynamicSlots();
    if (sentinelAllowed)
        return slot <= capacity;
    return slot < capacity;
}
#endif 

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
    JS_ASSERT(obj->isExtensible());

    if (proto && proto->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_XML_PROTO_FORBIDDEN);
        return false;
    }

    



    JSObject *oldproto = obj;
    while (oldproto && oldproto->isNative()) {
        if (!oldproto->protoShapeChange(cx))
            return false;
        oldproto = oldproto->getProto();
    }

    if (checkForCycles) {
        for (JSObject *obj2 = proto; obj2; obj2 = obj2->getProto()) {
            if (obj2 == obj) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE,
                                     js_proto_str);
                return false;
            }
        }
    }

    if (obj->hasSingletonType()) {
        



        if (!obj->splicePrototype(cx, proto))
            return false;
        MarkTypeObjectUnknownProperties(cx, obj->type());
        return true;
    }

    TypeObject *type = proto
        ? proto->getNewType(cx, NULL,  true)
        : cx->compartment->getEmptyType(cx);
    if (!type)
        return false;

    







    MarkTypeObjectUnknownProperties(cx, obj->type(), true);
    MarkTypeObjectUnknownProperties(cx, type, true);

    obj->setType(type);
    return true;
}

}

JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp)
{
    obj = obj->getGlobal();
    if (!obj->isGlobal()) {
        *objp = NULL;
        return true;
    }

    Value v = obj->getReservedSlot(key);
    if (v.isObject()) {
        *objp = &v.toObject();
        return true;
    }

    AutoResolving resolving(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.classAtoms[key]));
    if (resolving.alreadyStarted()) {
        
        *objp = NULL;
        return true;
    }

    JSObject *cobj = NULL;
    if (JSObjectOp init = lazy_prototype_init[key]) {
        if (!init(cx, obj))
            return false;
        v = obj->getReservedSlot(key);
        if (v.isObject())
            cobj = &v.toObject();
    }

    *objp = cobj;
    return true;
}

JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey protoKey,
                   Value *vp, Class *clasp)
{
    StackFrame *fp;
    JSObject *obj, *cobj, *pobj;
    jsid id;
    JSProperty *prop;
    const Shape *shape;

    




    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (!start && (fp = cx->maybefp()) != NULL)
        start = &fp->scopeChain();

    if (start) {
        obj = start->getGlobal();
    } else {
        obj = cx->globalObject;
        if (!obj) {
            vp->setUndefined();
            return true;
        }
    }

    OBJ_TO_INNER_OBJECT(cx, obj);
    if (!obj)
        return false;

    if (protoKey != JSProto_Null) {
        JS_ASSERT(JSProto_Null < protoKey);
        JS_ASSERT(protoKey < JSProto_LIMIT);
        if (!js_GetClassObject(cx, obj, protoKey, &cobj))
            return false;
        if (cobj) {
            vp->setObject(*cobj);
            return JS_TRUE;
        }
        id = ATOM_TO_JSID(cx->runtime->atomState.classAtoms[protoKey]);
    } else {
        JSAtom *atom = js_Atomize(cx, clasp->name, strlen(clasp->name));
        if (!atom)
            return false;
        id = ATOM_TO_JSID(atom);
    }

    JS_ASSERT(obj->isNative());
    if (!LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_CLASSNAME, &pobj, &prop))
        return false;
    Value v = UndefinedValue();
    if (prop && pobj->isNative()) {
        shape = (Shape *) prop;
        if (pobj->containsSlot(shape->slot())) {
            v = pobj->nativeGetSlot(shape->slot());
            if (v.isPrimitive())
                v.setUndefined();
        }
    }
    *vp = v;
    return true;
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
        if (!ctor->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &rval))
            return NULL;
        if (rval.isObjectOrNull())
            proto = rval.toObjectOrNull();
    }

    JSObject *obj = NewObject<WithProto::Class>(cx, clasp, proto, parent);
    if (!obj)
        return NULL;

    MarkTypeObjectUnknownProperties(cx, obj->type());

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
    JS_ASSERT(slot >= JSSLOT_FREE(getClass()));

    



    if (inDictionaryMode()) {
        PropertyTable &table = lastProperty()->table();
        uint32 last = table.freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            JS_ASSERT(last < slot);
            uint32 next = getSlot(last).toPrivateUint32();
            JS_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value &vref = getSlot(last);
            table.freelist = vref.toPrivateUint32();
            setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (inDictionaryMode() && !setSlotSpan(cx, slot + 1))
        return false;

    return true;
}

void
JSObject::freeSlot(JSContext *cx, uint32 slot)
{
    JS_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        uint32 &last = lastProperty()->table().freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            last = slot;
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

static bool
PurgeProtoChain(JSContext *cx, JSObject *obj, jsid id)
{
    const Shape *shape;

    while (obj) {
        if (!obj->isNative()) {
            obj = obj->getProto();
            continue;
        }
        shape = obj->nativeLookup(cx, id);
        if (shape) {
            PCMETER(JS_PROPERTY_CACHE(cx).pcpurges++);
            if (!obj->shadowingShapeChange(cx, *shape))
                return false;

            if (obj->isGlobal()) {
                




                LeaveTrace(cx);
            }
            return true;
        }
        obj = obj->getProto();
    }

    return true;
}

bool
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id)
{
    JS_ASSERT(obj->isDelegate());
    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->isCall()) {
        while ((obj = obj->getParentOrScopeChain()) != NULL) {
            if (!PurgeProtoChain(cx, obj, id))
                return false;
        }
    }

    return true;
}

const Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     PropertyOp getter, StrictPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid)
{
    JS_ASSERT(!(flags & Shape::METHOD));

    
    id = js_CheckForStringIndex(id);

    




    if (!js_PurgeScopeChain(cx, obj, id))
        return NULL;

    return obj->putProperty(cx, id, getter, setter, slot, attrs, flags, shortid);
}

const Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const Shape *shape, uintN attrs, uintN mask,
                             PropertyOp getter, StrictPropertyOp setter)
{
    



    if ((attrs & JSPROP_READONLY) && shape->isMethod()) {
        Value v = ObjectValue(*obj->nativeGetMethod(shape));

        shape = obj->methodReadBarrier(cx, *shape, &v);
        if (!shape)
            return NULL;
    }

    return obj->changeProperty(cx, shape, attrs, mask, getter, setter);
}

JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                  PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
}

JSBool
js_DefineElement(JSContext *cx, JSObject *obj, uint32 index, const Value *value,
                 PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
}







static inline bool
CallAddPropertyHook(JSContext *cx, Class *clasp, JSObject *obj, const Shape *shape, Value *vp)
{
    if (clasp->addProperty != JS_PropertyStub) {
        Value nominal = *vp;

        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, shape->propid(), vp))
            return false;
        if (*vp != nominal) {
            if (obj->containsSlot(shape->slot()))
                obj->nativeSetSlotWithType(cx, shape, *vp);
        }
    }
    return true;
}

namespace js {

const Shape *
DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow )
{
    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_DONT_PURGE |
                             DNP_SET_METHOD | DNP_SKIP_TYPE)) == 0);
    LeaveTraceIfGlobalObject(cx, obj);

    
    id = js_CheckForStringIndex(id);

    




    const Shape *shape = NULL;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        
        AddTypePropertyId(cx, obj, id, types::Type::UnknownType());
        MarkTypePropertyConfigured(cx, obj, id);

        




        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return NULL;
        if (prop && pobj == obj) {
            shape = (const Shape *) prop;
            if (shape->isAccessorDescriptor()) {
                shape = obj->changeProperty(cx, shape, attrs,
                                            JSPROP_GETTER | JSPROP_SETTER,
                                            (attrs & JSPROP_GETTER)
                                            ? getter
                                            : shape->getter(),
                                            (attrs & JSPROP_SETTER)
                                            ? setter
                                            : shape->setter());
                if (!shape)
                    return NULL;
            } else {
                shape = NULL;
            }
        }
    }

    




    if (!(defineHow & DNP_DONT_PURGE)) {
        if (!js_PurgeScopeChain(cx, obj, id))
            return NULL;
    }

    
    Class *clasp = obj->getClass();
    if (!(defineHow & DNP_SET_METHOD)) {
        if (!getter && !(attrs & JSPROP_GETTER))
            getter = clasp->getProperty;
        if (!setter && !(attrs & JSPROP_SETTER))
            setter = clasp->setProperty;
    }

    if (((defineHow & DNP_SET_METHOD) || getter == JS_PropertyStub) &&
        !(defineHow & DNP_SKIP_TYPE)) {
        



        AddTypePropertyId(cx, obj, id, value);
        if (attrs & JSPROP_READONLY)
            MarkTypePropertyConfigured(cx, obj, id);
    }

    



    Value valueCopy = value;
    bool adding = false;

    if (!shape) {
        
        if (defineHow & DNP_SET_METHOD) {
            JS_ASSERT(clasp == &ObjectClass);
            JS_ASSERT(IsFunctionObject(value));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
            JS_ASSERT(!getter && !setter);

            JSObject *funobj = &value.toObject();
            if (!funobj->toFunction()->isClonedMethod())
                flags |= Shape::METHOD;
        }

        if (const Shape *existingShape = obj->nativeLookup(cx, id)) {
            if (existingShape->hasSlot())
                AbortRecordingIfUnexpectedGlobalWrite(cx, obj, existingShape->slot());

            if (existingShape->isMethod() &&
                ObjectValue(*obj->nativeGetMethod(existingShape)) == valueCopy)
            {
                




                if (!obj->methodReadBarrier(cx, *existingShape, &valueCopy))
                    return NULL;
            }
        } else {
            adding = true;
        }

        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            return NULL;
    }

    
    if (shape->hasSlot() && obj->containsSlot(shape->slot()))
        obj->nativeSetSlot(shape->slot(), valueCopy);

    
    if (!CallAddPropertyHook(cx, clasp, obj, shape, &valueCopy)) {
        obj->removeProperty(cx, id);
        return NULL;
    }

    if (defineHow & DNP_CACHE_RESULT)
        JS_ASSERT_NOT_ON_TRACE(cx);
    return shape;
}

} 





















static JSBool
CallResolveOp(JSContext *cx, JSObject *start, JSObject *obj, jsid id, uintN flags,
              JSObject **objp, JSProperty **propp, bool *recursedp)
{
    Class *clasp = obj->getClass();
    JSResolveOp resolve = clasp->resolve;

    







    AutoResolving resolving(cx, obj, id);
    if (resolving.alreadyStarted()) {
        
        *recursedp = true;
        return true;
    }
    *recursedp = false;

    *propp = NULL;

    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = reinterpret_cast<JSNewResolveOp>(resolve);
        if (flags == RESOLVE_INFER)
            flags = js_InferFlags(cx, 0);
        JSObject *obj2 = (clasp->flags & JSCLASS_NEW_RESOLVE_GETS_START) ? start : NULL;
        if (!newresolve(cx, obj, id, flags, &obj2))
            return false;

        





        if (!obj2)
            return true;

        if (!obj2->isNative()) {
            
            JS_ASSERT(obj2 != obj);
            return obj2->lookupGeneric(cx, id, objp, propp);
        }
        obj = obj2;
    } else {
        if (!resolve(cx, obj, id))
            return false;
    }

    if (!obj->nativeEmpty()) {
        if (const Shape *shape = obj->nativeLookup(cx, id)) {
            *objp = obj;
            *propp = (JSProperty *) shape;
        }
    }

    return true;
}

static JS_ALWAYS_INLINE bool
LookupPropertyWithFlagsInline(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                              JSObject **objp, JSProperty **propp)
{
    
    JS_ASSERT(id == js_CheckForStringIndex(id));

    
    JSObject *start = obj;
    while (true) {
        const Shape *shape = obj->nativeLookup(cx, id);
        if (shape) {
            *objp = obj;
            *propp = (JSProperty *) shape;
            return true;
        }

        
        if (obj->getClass()->resolve != JS_ResolveStub) {
            bool recursed;
            if (!CallResolveOp(cx, start, obj, id, flags, objp, propp, &recursed))
                return false;
            if (recursed)
                break;
            if (*propp) {
                



                return true;
            }
        }

        JSObject *proto = obj->getProto();
        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!proto->lookupGeneric(cx, id, objp, propp))
                return false;
#ifdef DEBUG
            








            if (*propp && (*objp)->isNative()) {
                while ((proto = proto->getProto()) != *objp)
                    JS_ASSERT(proto);
            }
#endif
            return true;
        }

        obj = proto;
    }

    *objp = NULL;
    *propp = NULL;
    return true;
}

JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp)
{
    
    id = js_CheckForStringIndex(id);

    return LookupPropertyWithFlagsInline(cx, obj, id, cx->resolveFlags, objp, propp);
}

JS_FRIEND_API(JSBool)
js_LookupElement(JSContext *cx, JSObject *obj, uint32 index, JSObject **objp, JSProperty **propp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;

    return LookupPropertyWithFlagsInline(cx, obj, id, cx->resolveFlags, objp, propp);
}

namespace js {

bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp, JSProperty **propp)
{
    
    id = js_CheckForStringIndex(id);

    return LookupPropertyWithFlagsInline(cx, obj, id, flags, objp, propp);
}

} 

PropertyCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, bool cacheResult, bool global,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    JSObject *scopeChain, *obj, *parent, *pobj;
    PropertyCacheEntry *entry;
    int scopeIndex;
    JSProperty *prop;

    JS_ASSERT_IF(cacheResult, !JS_ON_TRACE(cx));
    scopeChain = cx->stack.currentScriptedScopeChain();

    if (global) {
        








        scopeChain = scopeChain->getGlobal();
    }

    
    entry = JS_NO_PROP_CACHE_FILL;
    obj = scopeChain;
    parent = obj->getParentOrScopeChain();
    for (scopeIndex = 0;
         parent
         ? IsCacheableNonGlobalScope(obj)
         : !obj->getOps()->lookupProperty;
         ++scopeIndex) {
        if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
            return NULL;

        if (prop) {
#ifdef DEBUG
            if (parent) {
                JS_ASSERT(pobj->isNative());
                JS_ASSERT(pobj->getClass() == obj->getClass());
                if (obj->isBlock()) {
                    




                    JS_ASSERT(pobj->isClonedBlock());
                } else {
                    
                    JS_ASSERT(!obj->getProto());
                }
                JS_ASSERT(pobj == obj);
            } else {
                JS_ASSERT(obj->isNative());
            }
#endif
            



            if (cacheResult && pobj->isNative()) {
                entry = JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, pobj,
                                                   (Shape *) prop);
            }
            goto out;
        }

        if (!parent) {
            pobj = NULL;
            goto out;
        }
        obj = parent;
        parent = obj->getParentOrScopeChain();
    }

    for (;;) {
        if (!obj->lookupGeneric(cx, id, &pobj, &prop))
            return NULL;
        if (prop) {
            PCMETER(JS_PROPERTY_CACHE(cx).nofills++);
            goto out;
        }

        



        parent = obj->getParentOrScopeChain();
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
js_FindProperty(JSContext *cx, jsid id, bool global,
                JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    return !!js_FindPropertyHelper(cx, id, false, global, objp, pobjp, propp);
}

JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id)
{
    



    JS_ASSERT(!JS_ON_TRACE(cx));

    JSObject *obj = scopeChain;

    








    for (int scopeIndex = 0;
         obj->isGlobal() || IsCacheableNonGlobalScope(obj);
         scopeIndex++) {
        JSObject *pobj;
        JSProperty *prop;
        if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
            return NULL;
        if (prop) {
            if (!pobj->isNative()) {
                JS_ASSERT(obj->isGlobal());
                return obj;
            }
            JS_ASSERT_IF(obj->isScope(), pobj->getClass() == obj->getClass());
            DebugOnly<PropertyCacheEntry*> entry =
                JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, pobj, (Shape *) prop);
            JS_ASSERT(entry);
            return obj;
        }

        JSObject *parent = obj->getParentOrScopeChain();
        if (!parent)
            return obj;
        obj = parent;
    }

    
    do {
        JSObject *pobj;
        JSProperty *prop;
        if (!obj->lookupGeneric(cx, id, &pobj, &prop))
            return NULL;
        if (prop)
            break;

        




        JSObject *parent = obj->getParentOrScopeChain();
        if (!parent)
            break;
        obj = parent;
    } while (!obj->isGlobal());
    return obj;
}

static JS_ALWAYS_INLINE JSBool
js_NativeGetInline(JSContext *cx, JSObject *receiver, JSObject *obj, JSObject *pobj,
                   const Shape *shape, uintN getHow, Value *vp)
{
    LeaveTraceIfGlobalObject(cx, pobj);

    int32 sample;

    JS_ASSERT(pobj->isNative());

    if (shape->hasSlot()) {
        *vp = pobj->nativeGetSlot(shape->slot());
        JS_ASSERT(!vp->isMagic());
        JS_ASSERT_IF(!pobj->hasSingletonType() && shape->hasDefaultGetterOrIsMethod(),
                     js::types::TypeHasProperty(cx, pobj->type(), shape->propid(), *vp));
    } else {
        vp->setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    if (JS_UNLIKELY(shape->isMethod()) && (getHow & JSGET_NO_METHOD_BARRIER))
        return true;

    jsbytecode *pc;
    JSScript *script = cx->stack.currentScript(&pc);
    if (script && script->hasAnalysis() && !cx->fp()->hasImacropc()) {
        analyze::Bytecode *code = script->analysis()->maybeCode(pc);
        if (code)
            code->accessGetter = true;
    }

    sample = cx->runtime->propertyRemovals;
    if (!shape->get(cx, receiver, obj, pobj, vp))
        return false;

    
    if (shape->hasSlot() && pobj->nativeContains(cx, *shape)) {
        
        JS_ASSERT(!shape->isMethod());
        pobj->nativeSetSlot(shape->slot(), *vp);
    }

    return true;
}

JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const Shape *shape, uintN getHow,
             Value *vp)
{
    return js_NativeGetInline(cx, obj, obj, pobj, shape, getHow, vp);
}

JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const Shape *shape, bool added, bool strict, Value *vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    AddTypePropertyId(cx, obj, shape->propid(), *vp);

    JS_ASSERT(obj->isNative());

    if (shape->hasSlot()) {
        uint32 slot = shape->slot();
        JS_ASSERT(obj->containsSlot(slot));

        
        if (shape->hasDefaultSetter()) {
            if (!added) {
                AbortRecordingIfUnexpectedGlobalWrite(cx, obj, slot);

                if (shape->isMethod() && !obj->methodShapeChange(cx, *shape))
                    return false;
            }
            obj->nativeSetSlot(slot, *vp);
            return true;
        }
    } else {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    int32 sample = cx->runtime->propertyRemovals;
    if (!shape->set(cx, obj, strict, vp))
        return false;

    



    if (shape->hasSlot() &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         obj->nativeContains(cx, *shape))) {
        if (!added)
            AbortRecordingIfUnexpectedGlobalWrite(cx, obj, shape->slot());
        obj->setSlot(shape->slot(), *vp);
    }

    return true;
}

static JS_ALWAYS_INLINE JSBool
js_GetPropertyHelperInline(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                           uint32 getHow, Value *vp)
{
    JSObject *aobj, *obj2;
    JSProperty *prop;
    const Shape *shape;

    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, !JS_ON_TRACE(cx));

    
    id = js_CheckForStringIndex(id);

    aobj = js_GetProtoIfDenseArray(obj);
    
    if (!LookupPropertyWithFlagsInline(cx, aobj, id, cx->resolveFlags, &obj2, &prop))
        return false;

    if (!prop) {
        vp->setUndefined();

        if (!CallJSPropertyOp(cx, obj->getClass()->getProperty, obj, id, vp))
            return JS_FALSE;

        PCMETER(getHow & JSGET_CACHE_RESULT && JS_PROPERTY_CACHE(cx).nofills++);

        
        if (!vp->isUndefined())
            AddTypePropertyId(cx, obj, id, *vp);

        



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
                if (!cx->hasStrictOption() ||
                    cx->stack.currentScript()->warnedAboutUndefinedProp ||
                    (op != JSOP_GETPROP && op != JSOP_GETELEM) ||
                    js_CurrentPCIsInImacro(cx)) {
                    return JS_TRUE;
                }

                



                if (JSID_IS_ATOM(id, cx->runtime->atomState.iteratorAtom))
                    return JS_TRUE;

                
                if (cx->resolveFlags == RESOLVE_INFER) {
                    LeaveTrace(cx);
                    pc += js_CodeSpec[op].length;
                    if (Detecting(cx, pc))
                        return JS_TRUE;
                } else if (cx->resolveFlags & JSRESOLVE_DETECTING) {
                    return JS_TRUE;
                }

                flags = JSREPORT_WARNING | JSREPORT_STRICT;
                cx->stack.currentScript()->warnedAboutUndefinedProp = true;
            }

            
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, IdToValue(id),
                                          NULL, NULL, NULL)) {
                return JS_FALSE;
            }
        }
        return JS_TRUE;
    }

    if (!obj2->isNative()) {
        return obj2->isProxy()
               ? Proxy::get(cx, obj2, receiver, id, vp)
               : obj2->getGeneric(cx, id, vp);
    }

    shape = (Shape *) prop;

    if (getHow & JSGET_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        JS_PROPERTY_CACHE(cx).fill(cx, aobj, 0, obj2, shape);
    }

    
    if (!js_NativeGetInline(cx, receiver, obj, obj2, shape, getHow, vp))
        return JS_FALSE;

    return JS_TRUE;
}

JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32 getHow, Value *vp)
{
    return js_GetPropertyHelperInline(cx, obj, obj, id, getHow, vp);
}

JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    
    return js_GetPropertyHelperInline(cx, obj, receiver, id, JSGET_METHOD_BARRIER, vp);
}

JSBool
js_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;

    
    return js_GetPropertyHelperInline(cx, obj, receiver, id, JSGET_METHOD_BARRIER, vp);
}

JSBool
js::GetPropertyDefault(JSContext *cx, JSObject *obj, jsid id, const Value &def, Value *vp)
{
    JSProperty *prop;
    JSObject *obj2;
    if (!LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_QUALIFIED, &obj2, &prop))
        return false;

    if (!prop) {
        *vp = def;
        return true;
    }

    return js_GetProperty(cx, obj2, id, vp);
}

JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, Value *vp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED);

    GenericIdOp op = obj->getOps()->getGeneric;
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
    return op(cx, obj, obj, id, vp);
}

JS_FRIEND_API(bool)
js::CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    StackFrame *const fp = js_GetTopStackFrame(cx, FRAME_EXPAND_ALL);
    if (!fp)
        return true;

    
    if (!(fp->isScriptFrame() && fp->script()->strictModeCode) &&
        !cx->hasStrictOption()) {
        return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
JSObject::reportReadOnly(JSContext* cx, jsid id, uintN report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

bool
JSObject::reportNotConfigurable(JSContext* cx, jsid id, uintN report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, IdToValue(id), NULL,
                                    NULL, NULL);
}

bool
JSObject::reportNotExtensible(JSContext *cx, uintN report)
{
    return js_ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                    JSDVG_IGNORE_STACK, ObjectValue(*this),
                                    NULL, NULL, NULL);
}

bool
JSObject::callMethod(JSContext *cx, jsid id, uintN argc, Value *argv, Value *vp)
{
    Value fval;
    return js_GetMethod(cx, this, id, JSGET_NO_METHOD_BARRIER, &fval) &&
           Invoke(cx, ObjectValue(*this), fval, argc, argv, vp);
}

static bool
CloneFunctionForSetMethod(JSContext *cx, Value *vp)
{
    JSFunction *fun = vp->toObject().toFunction();

    
    if (!fun->isClonedMethod()) {
        fun = CloneFunctionObject(cx, fun);
        if (!fun)
            return false;
        vp->setObject(*fun);
    }
    return true;
}

JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     Value *vp, JSBool strict)
{
    JSObject *pobj;
    JSProperty *prop;
    const Shape *shape;
    uintN attrs, flags;
    intN shortid;
    Class *clasp;
    PropertyOp getter;
    StrictPropertyOp setter;
    bool added;

    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_SET_METHOD | DNP_UNQUALIFIED)) == 0);
    if (defineHow & DNP_CACHE_RESULT)
        JS_ASSERT_NOT_ON_TRACE(cx);

    
    id = js_CheckForStringIndex(id);

    if (JS_UNLIKELY(obj->watched())) {
        
        WatchpointMap *wpmap = cx->compartment->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;

        
        defineHow &= ~DNP_SET_METHOD;
    }

    if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
        return false;
    if (prop) {
        if (!pobj->isNative()) {
            if (pobj->isProxy()) {
                AutoPropertyDescriptorRooter pd(cx);
                if (!Proxy::getPropertyDescriptor(cx, pobj, id, true, &pd))
                    return false;

                if ((pd.attrs & (JSPROP_SHARED | JSPROP_SHADOWABLE)) == JSPROP_SHARED) {
                    return !pd.setter ||
                           CallSetter(cx, obj, id, pd.setter, pd.attrs, pd.shortid, strict, vp);
                }

                if (pd.attrs & JSPROP_READONLY) {
                    if (strict)
                        return obj->reportReadOnly(cx, id);
                    if (cx->hasStrictOption())
                        return obj->reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                    return true;
                }
            }

            prop = NULL;
        }
    } else {
        
        JS_ASSERT(!obj->isBlock());

        if (obj->isGlobal() &&
            (defineHow & DNP_UNQUALIFIED) &&
            !js::CheckUndeclaredVarAssignment(cx, JSID_TO_STRING(id))) {
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
            if (shape->hasDefaultSetter())
                return js_ReportGetterOnlyAssignment(cx);
        } else {
            JS_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                PCMETER((defineHow & JSDNP_CACHE_RESULT) && JS_PROPERTY_CACHE(cx).rofills++);

                
                if (strict)
                    return obj->reportReadOnly(cx, id);
                if (cx->hasStrictOption())
                    return obj->reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return JS_TRUE;
            }
        }

        attrs = shape->attributes();
        if (pobj != obj) {
            


            if (!shape->shadowable()) {
                if (defineHow & DNP_SET_METHOD) {
                    JS_ASSERT(!shape->isMethod());
                    if (!CloneFunctionForSetMethod(cx, vp))
                        return false;
                }

                if (defineHow & DNP_CACHE_RESULT)
                    JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, pobj, shape);

                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return JS_TRUE;

                return shape->set(cx, obj, strict, vp);
            }

            














            if (!shape->hasSlot()) {
                defineHow &= ~DNP_SET_METHOD;
                if (shape->hasShortID()) {
                    flags = Shape::HAS_SHORTID;
                    shortid = shape->shortid();
                }
                attrs &= ~JSPROP_SHARED;
                getter = shape->getter();
                setter = shape->setter();
            } else {
                
                attrs = JSPROP_ENUMERATE;
            }

            



            shape = NULL;
        }

        if (shape && (defineHow & DNP_SET_METHOD)) {
            






            if (shape->isMethod()) {
                if (obj->nativeGetMethod(shape) == &vp->toObject())
                    return true;
                shape = obj->methodShapeChange(cx, *shape);
                if (!shape)
                    return false;
            }
            if (!CloneFunctionForSetMethod(cx, vp))
                return false;
            return js_NativeSet(cx, obj, shape, false, strict, vp);
        }
    }

    added = false;
    if (!shape) {
        if (!obj->isExtensible()) {
            
            if (strict)
                return obj->reportNotExtensible(cx);
            if (cx->hasStrictOption())
                return obj->reportNotExtensible(cx, JSREPORT_STRICT | JSREPORT_WARNING);
            return JS_TRUE;
        }

        



        if (!js_PurgeScopeChain(cx, obj, id))
            return JS_FALSE;

        



        if ((defineHow & DNP_SET_METHOD) && obj->canHaveMethodBarrier()) {
            JS_ASSERT(IsFunctionObject(*vp));
            JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

            JSObject *funobj = &vp->toObject();
            if (!funobj->toFunction()->isClonedMethod())
                flags |= Shape::METHOD;
        }

        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            return JS_FALSE;

        if (defineHow & DNP_CACHE_RESULT)
            TRACE_1(AddProperty, obj);

        




        if (obj->containsSlot(shape->slot()))
            obj->nativeSetSlot(shape->slot(), UndefinedValue());

        
        if (!CallAddPropertyHook(cx, clasp, obj, shape, vp)) {
            obj->removeProperty(cx, id);
            return JS_FALSE;
        }
        added = true;
    }

    if ((defineHow & DNP_CACHE_RESULT) && !added)
        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, obj, shape);

    return js_NativeSet(cx, obj, shape, added, strict, vp);

#ifdef JS_TRACER
  error: 
    return JS_FALSE;
#endif
}

JSBool
js_SetElementHelper(JSContext *cx, JSObject *obj, uint32 index, uintN defineHow,
                    Value *vp, JSBool strict)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return js_SetPropertyHelper(cx, obj, id, defineHow, vp, strict);
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
    return true;
}

JSBool
js_GetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    JSProperty *prop;
    if (!js_LookupElement(cx, obj, index, &obj, &prop))
        return false;
    if (!prop) {
        *attrsp = 0;
        return true;
    }
    if (!obj->isNative())
        return obj->getElementAttributes(cx, index, attrsp);

    const Shape *shape = (Shape *)prop;
    *attrsp = shape->attributes();
    return true;
}

JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, Shape *shape, uintN attrs)
{
    JS_ASSERT(obj->isNative());
    return !!js_ChangeNativePropertyAttrs(cx, obj, shape, attrs, 0,
                                          shape->getter(), shape->setter());
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
js_SetElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    JSProperty *prop;
    if (!js_LookupElement(cx, obj, index, &obj, &prop))
        return false;
    if (!prop)
        return true;
    return obj->isNative()
           ? js_SetNativeAttributes(cx, obj, (Shape *) prop, *attrsp)
           : obj->setElementAttributes(cx, index, attrsp);
}

JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    JSObject *proto;
    JSProperty *prop;
    const Shape *shape;

    rval->setBoolean(true);

    
    id = js_CheckForStringIndex(id);

    if (!js_LookupProperty(cx, obj, id, &proto, &prop))
        return false;
    if (!prop || proto != obj) {
        



        return CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval);
    }

    shape = (Shape *)prop;
    if (!shape->configurable()) {
        if (strict)
            return obj->reportNotConfigurable(cx, id);
        rval->setBoolean(false);
        return true;
    }

    if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, shape->getUserId(), rval))
        return false;
    if (rval->isFalse())
        return true;

    if (shape->hasSlot() && obj->containsSlot(shape->slot())) {
        const Value &v = obj->nativeGetSlot(shape->slot());
        GCPoke(cx, v);

        










        JSFunction *fun;
        if (IsFunctionObject(v, &fun) && fun->isClonedMethod()) {
            for (StackFrame *fp = cx->maybefp(); fp; fp = fp->prev()) {
                if (fp->isFunctionFrame() &&
                    fp->fun()->script() == fun->script() &&
                    fp->thisValue().isObject())
                {
                    JSObject *tmp = &fp->thisValue().toObject();
                    do {
                        if (tmp == obj) {
                            fp->overwriteCallee(*fun);
                            break;
                        }
                    } while ((tmp = tmp->getProto()) != NULL);
                }
            }
        }
    }

    return obj->removeProperty(cx, id) && js_SuppressDeletedProperty(cx, obj, id);
}

JSBool
js_DeleteElement(JSContext *cx, JSObject *obj, uint32 index, Value *rval, JSBool strict)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return js_DeleteProperty(cx, obj, id, rval, strict);
}

namespace js {

bool
HasDataProperty(JSContext *cx, JSObject *obj, jsid methodid, Value *vp)
{
    if (const Shape *shape = obj->nativeLookup(cx, methodid)) {
        if (shape->hasDefaultGetterOrIsMethod() && obj->containsSlot(shape->slot())) {
            *vp = obj->nativeGetSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, vp))
        return false;
    if (!js_IsCallable(*vp)) {
        *vp = ObjectValue(*obj);
        return true;
    }
    return Invoke(cx, ObjectValue(*obj), *vp, 0, NULL, vp);
}

JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
    JS_ASSERT(!obj->isXML());

    Class *clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        
        if (clasp == &StringClass &&
            ClassMethodIsNative(cx, obj,
                                 &StringClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.toStringAtom),
                                 js_str_toString)) {
            *vp = obj->getPrimitiveThis();
            return true;
        }

        if (!MaybeCallMethod(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.toStringAtom), vp))
            return false;
        if (vp->isPrimitive())
            return true;

        if (!MaybeCallMethod(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom), vp))
            return false;
        if (vp->isPrimitive())
            return true;
    } else {
        
        if ((clasp == &StringClass &&
             ClassMethodIsNative(cx, obj, &StringClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom),
                                 js_str_toString)) ||
            (clasp == &NumberClass &&
             ClassMethodIsNative(cx, obj, &NumberClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom),
                                 js_num_valueOf))) {
            *vp = obj->getPrimitiveThis();
            return true;
        }

        if (!MaybeCallMethod(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom), vp))
            return false;
        if (vp->isPrimitive())
            return true;

        if (!MaybeCallMethod(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.toStringAtom), vp))
            return false;
        if (vp->isPrimitive())
            return true;
    }

    
    JSString *str;
    if (hint == JSTYPE_STRING) {
        str = JS_InternString(cx, clasp->name);
        if (!str)
            return false;
    } else {
        str = NULL;
    }

    js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, JSDVG_SEARCH_STACK, ObjectValue(*obj), str,
                         (hint == JSTYPE_VOID) ? "primitive type" : JS_TYPE_STR(hint));
    return false;
}

} 

JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSObject *obj, JSIterateOp enum_op, Value *statep, jsid *idp)
{
    
    Class *clasp = obj->getClass();
    JSEnumerateOp enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((JSNewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
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
    JSCheckAccessOp check;

    while (JS_UNLIKELY(obj->isWith()))
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
        if (!obj->lookupGeneric(cx, id, &pobj, &prop))
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
            if (shape->hasSlot())
                *vp = pobj->nativeGetSlot(shape->slot());
            else
                vp->setUndefined();
        }
    }

    











    clasp = pobj->getClass();
    check = clasp->checkAccess;
    if (!check) {
        callbacks = JS_GetSecurityCallbacks(cx);
        check = callbacks ? callbacks->checkObjectAccess : NULL;
    }
    return !check || check(cx, pobj, id, mode, vp);
}

}

JSType
js_TypeOf(JSContext *cx, JSObject *obj)
{
    return obj->isCallable() ? JSTYPE_FUNCTION : JSTYPE_OBJECT;
}

bool
js_IsDelegate(JSContext *cx, JSObject *obj, const Value &v)
{
    if (v.isPrimitive())
        return false;
    JSObject *obj2 = &v.toObject();
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
        if (!ctor->getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &v))
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
        if (scopeobj->isGlobal()) {
            const Value &v = scopeobj->getReservedSlot(JSProto_LIMIT + protoKey);
            if (v.isObject()) {
                *protop = &v.toObject();
                return true;
            }
        }
    }

    return FindClassPrototype(cx, scopeobj, protoKey, protop, clasp);
}

JSObject *
PrimitiveToObject(JSContext *cx, const Value &v)
{
    if (v.isString())
        return StringObject::create(cx, v.toString());
    if (v.isNumber())
        return NumberObject::create(cx, v.toNumber());

    JS_ASSERT(v.isBoolean());
    JSObject *obj = NewBuiltinClassInstance(cx, &BooleanClass);
    if (!obj)
        return NULL;

    obj->setPrimitiveThis(v);
    return obj;
}

JSBool
js_PrimitiveToObject(JSContext *cx, Value *vp)
{
    JSObject *obj = PrimitiveToObject(cx, *vp);
    if (!obj)
        return false;

    vp->setObject(*obj);
    return true;
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
        obj = PrimitiveToObject(cx, v);
        if (!obj)
            return false;
    }
    *objp = obj;
    return true;
}

namespace js {


JSObject *
ToObjectSlow(JSContext *cx, Value *vp)
{
    JS_ASSERT(!vp->isMagic());
    JS_ASSERT(!vp->isObject());

    if (vp->isNullOrUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                            vp->isNull() ? "null" : "undefined", "object");
        return NULL;
    }

    JSObject *obj = PrimitiveToObject(cx, *vp);
    if (obj)
        vp->setObject(*obj);
    return obj;
}

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
                atom = js_Atomize(cx, clasp->name, strlen(clasp->name));
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

#ifdef DEBUG
void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize)
{
    JS_ASSERT(trc->debugPrinter == js_PrintObjectSlotName);

    JSObject *obj = (JSObject *)trc->debugPrintArg;
    uint32 slot = (uint32)trc->debugPrintIndex;

    const Shape *shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape->previous() && shape->slot() != slot)
            shape = shape->previous();
        if (shape->slot() != slot)
            shape = NULL;
    } else {
        shape = NULL;
    }

    if (!shape) {
        const char *slotname = NULL;
        if (obj->isGlobal()) {
#define JS_PROTO(name,code,init)                                              \
    if ((code) == slot) { slotname = js_##name##_str; goto found; }
#include "jsproto.tbl"
#undef JS_PROTO
        }
      found:
        if (slotname)
            JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
        else
            JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
    } else {
        jsid propid = shape->propid();
        if (JSID_IS_INT(propid)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(propid));
        } else if (JSID_IS_ATOM(propid)) {
            PutEscapedString(buf, bufsize, JSID_TO_ATOM(propid), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}
#endif

static const Shape *
LastConfigurableShape(JSObject *obj)
{
    for (Shape::Range r(obj->lastProperty()->all()); !r.empty(); r.popFront()) {
        const Shape *shape = &r.front();
        if (shape->configurable())
            return shape;
    }
    return NULL;
}

bool
js_ClearNative(JSContext *cx, JSObject *obj)
{
    
    while (const Shape *shape = LastConfigurableShape(obj)) {
        if (!obj->removeProperty(cx, shape->propid()))
            return false;
    }

    
    for (Shape::Range r(obj->lastProperty()->all()); !r.empty(); r.popFront()) {
        const Shape *shape = &r.front();
        if (shape->isDataDescriptor() &&
            shape->writable() &&
            shape->hasDefaultSetter() &&
            obj->containsSlot(shape->slot())) {
            obj->setSlot(shape->slot(), UndefinedValue());
        }
    }
    return true;
}

bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 slot, Value *vp)
{
    if (!obj->isNative()) {
        vp->setUndefined();
        return true;
    }

    JS_ASSERT(slot < JSSLOT_FREE(obj->getClass()));
    *vp = obj->getSlot(slot);
    return true;
}

bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 slot, const Value &v)
{
    if (!obj->isNative())
        return true;

    JS_ASSERT(slot < JSSLOT_FREE(obj->getClass()));
    obj->setSlot(slot, v);
    GCPoke(cx, NullValue());
    return true;
}

GlobalObject *
JSObject::getGlobal() const
{
    JSObject *obj = const_cast<JSObject *>(this);
    while (JSObject *parent = obj->getParentMaybeScope())
        obj = parent;
    return obj->asGlobal();
}

static ObjectElements emptyObjectHeader(0);
Value *js::emptyObjectElements = (Value *) (jsuword(&emptyObjectHeader) + sizeof(ObjectElements));

JSBool
js_ReportGetterOnlyAssignment(JSContext *cx)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_GETTER_ONLY);
}

JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_GETTER_ONLY);
    return JS_FALSE;
}

void
js::ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp)
{
    Value &thisv = call.thisv();

#ifdef DEBUG
    if (thisv.isObject()) {
        JS_ASSERT(thisv.toObject().getClass() != clasp);
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
    if (const jschar *chars = str->getChars(NULL))
        dumpChars(chars, str->length());
    else
        fprintf(stderr, "(oom in dumpString)");
}

JS_FRIEND_API(void)
js_DumpString(JSString *str)
{
    if (const jschar *chars = str->getChars(NULL)) {
        fprintf(stderr, "JSString* (%p) = jschar * (%p) = ",
                (void *) str, (void *) chars);
        dumpString(str);
    } else {
        fprintf(stderr, "(oom in JS_DumpString)");
    }
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpAtom(JSAtom *atom)
{
    fprintf(stderr, "JSAtom* (%p) = ", (void *) atom);
    js_DumpString(atom);
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
        JSFunction *fun = v.toObject().toFunction();
        if (fun->atom) {
            fputs("<function ", stderr);
            FileEscapedString(stderr, fun->atom, 0);
        } else {
            fputs("<unnamed function", stderr);
        }
        if (fun->isInterpreted()) {
            JSScript *script = fun->script();
            fprintf(stderr, " (%s:%u)",
                    script->filename ? script->filename : "", script->lineno);
        }
        fprintf(stderr, " at %p>", (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.toObject();
        Class *clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &ObjectClass) ? "" : " object",
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
DumpProperty(JSObject *obj, const Shape &shape)
{
    jsid id = shape.propid();
    uint8 attrs = shape.attributes();

    fprintf(stderr, "    ((Shape *) %p) ", (void *) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");
    if (shape.isMethod()) fprintf(stderr, "method ");

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void *) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void *) shape.setterObject());
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.setterOp()));

    if (JSID_IS_ATOM(id))
        dumpString(JSID_TO_STRING(id));
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));

    uint32 slot = shape.hasSlot() ? shape.maybeSlot() : SHAPE_INVALID_SLOT;
    fprintf(stderr, ": slot %d", slot);
    if (obj->containsSlot(slot)) {
        fprintf(stderr, " = ");
        dumpValue(obj->getSlot(slot));
    } else if (slot != SHAPE_INVALID_SLOT) {
        fprintf(stderr, " (INVALID!)");
    }
    fprintf(stderr, "\n");
}

JS_FRIEND_API(void)
js_DumpObject(JSObject *obj)
{
    fprintf(stderr, "object %p\n", (void *) obj);
    Class *clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (void *)clasp, clasp->name);

    fprintf(stderr, "flags:");
    if (obj->isDelegate()) fprintf(stderr, " delegate");
    if (obj->isSystem()) fprintf(stderr, " system");
    if (!obj->isExtensible()) fprintf(stderr, " not_extensible");
    if (obj->isIndexed()) fprintf(stderr, " indexed");

    if (obj->isNative()) {
        if (obj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (obj->hasPropertyTable())
            fprintf(stderr, " hasPropertyTable");
    }
    fprintf(stderr, "\n");

    if (obj->isDenseArray()) {
        unsigned slots = obj->getDenseArrayInitializedLength();
        fprintf(stderr, "elements\n");
        for (unsigned i = 0; i < slots; i++) {
            fprintf(stderr, " %3d: ", i);
            dumpValue(obj->getDenseArrayElement(i));
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        return;
    }

    fprintf(stderr, "proto ");
    dumpValue(ObjectOrNullValue(obj->getProto()));
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(ObjectOrNullValue(obj->getParentMaybeScope()));
    fputc('\n', stderr);

    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        fprintf(stderr, "private %p\n", obj->getPrivate());

    if (!obj->isNative())
        fprintf(stderr, "not native\n");

    unsigned reservedEnd = JSCLASS_RESERVED_SLOTS(clasp);
    unsigned slots = obj->slotSpan();
    unsigned stop = obj->isNative() ? reservedEnd : slots;
    if (stop > 0)
        fprintf(stderr, obj->isNative() ? "reserved slots:\n" : "slots:\n");
    for (unsigned i = 0; i < stop; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->getSlot(i));
        fputc('\n', stderr);
    }

    if (obj->isNative()) {
        fprintf(stderr, "properties:\n");
        Vector<const Shape *, 8, SystemAllocPolicy> props;
        for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(obj, *props[i]);
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
js_DumpStackFrame(JSContext *cx, StackFrame *start)
{
    
    VOUCH_DOES_NOT_REQUIRE_STACK();

    FrameRegsIter i(cx, StackIter::GO_THROUGH_SAVED);
    if (!start) {
        if (i.done()) {
            fprintf(stderr, "no stack for cx = %p\n", (void*) cx);
            return;
        }
        start = i.fp();
    } else {
        while (!i.done() && i.fp() != start)
            ++i;

        if (i.done()) {
            fprintf(stderr, "fp = %p not found in cx = %p\n",
                    (void *)start, (void *)cx);
            return;
        }
    }

    for (; !i.done(); ++i) {
        StackFrame *const fp = i.fp();

        fprintf(stderr, "StackFrame at %p\n", (void *) fp);
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
        if (fp->hasArgs()) {
            fprintf(stderr, "  actuals: %p (%u) ", (void *) fp->actualArgs(), (unsigned) fp->numActualArgs());
            fprintf(stderr, "  formals: %p (%u)\n", (void *) fp->formalArgs(), (unsigned) fp->numFormalArgs());
        }
        if (fp->hasCallObj()) {
            fprintf(stderr, "  has call obj: ");
            dumpValue(ObjectValue(fp->callObj()));
            fprintf(stderr, "\n");
        }
        MaybeDumpObject("argsobj", fp->maybeArgsObj());
        if (!fp->isDummyFrame()) {
            MaybeDumpValue("this", fp->thisValue());
            fprintf(stderr, "  rval: ");
            dumpValue(fp->returnValue());
        } else {
            fprintf(stderr, "dummy frame");
        }
        fputc('\n', stderr);

        fprintf(stderr, "  flags:");
        if (fp->isConstructing())
            fprintf(stderr, " constructing");
        if (fp->hasOverriddenArgs())
            fprintf(stderr, " overridden_args");
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

        fputc('\n', stderr);
    }
}

#endif 













































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
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
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
#include "jswrapper.h"

#include "jsinterpinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"

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

#include "jsprobes.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::gc;

JS_FRIEND_DATA(js::Shape) Shape::sharedNonNative(SHAPELESS);

Class js_ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    PropertyStub,         
    PropertyStub,         
    PropertyStub,         
    StrictPropertyStub,   
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
obj_setProto(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp);

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
obj_setProto(JSContext *cx, JSObject *obj, jsid id, JSBool strict, Value *vp)
{
    
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
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(localroot), localroot);

    
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

        ok = obj->lookupProperty(cx, id, &obj2, &prop);
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
                ok = obj->getProperty(cx, id, &val[0]);
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
        return JSProxy::obj_toString(cx, obj);

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





JSBool
js_CheckContentSecurityPolicy(JSContext *cx, JSObject *scopeobj)
{
    return scopeobj->getGlobal()->isEvalAllowed(cx);
}


static void
AssertInnerizedScopeChain(JSContext *cx, JSObject &scopeobj)
{
#ifdef DEBUG
    for (JSObject *o = &scopeobj; o; o = o->getParent()) {
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
    return &JS_SCRIPTS_TO_GC(cx)[h];
}

static JS_ALWAYS_INLINE JSScript *
EvalCacheLookup(JSContext *cx, JSLinearString *str, StackFrame *caller, uintN staticLevel,
                JSPrincipals *principals, JSObject &scopeobj, JSScript **bucket)
{
    















    uintN count = 0;
    JSScript **scriptp = bucket;

    EVAL_CACHE_METER(probe);
    JSVersion version = cx->findVersion();
    JSScript *script;
    while ((script = *scriptp) != NULL) {
        if (script->savedCallerFun &&
            script->staticLevel == staticLevel &&
            script->getVersion() == version &&
            !script->hasSingletons &&
            (script->principals == principals ||
             (principals->subsume(principals, script->principals) &&
              script->principals->subsume(script->principals, principals)))) {
            



            JSFunction *fun = script->getFunction(0);

            if (fun == caller->fun()) {
                



                JSAtom *src = script->atomMap.vector[0];

                if (src == str || EqualStrings(src, str)) {
                    






                    JSObjectArray *objarray = script->objects();
                    int i = 1;

                    if (objarray->length == 1) {
                        if (JSScript::isValidOffset(script->regexpsOffset)) {
                            objarray = script->regexps();
                            i = 0;
                        } else {
                            EVAL_CACHE_METER(noscope);
                            i = -1;
                        }
                    }
                    if (i < 0 ||
                        objarray->vector[i]->getParent() == &scopeobj) {
                        JS_ASSERT(staticLevel == script->staticLevel);
                        EVAL_CACHE_METER(hit);
                        *scriptp = script->u.nextToGC;
                        script->u.nextToGC = NULL;
                        return script;
                    }
                }
            }
        }

        if (++count == EVAL_CACHE_CHAIN_LIMIT)
            return NULL;
        EVAL_CACHE_METER(step);
        scriptp = &script->u.nextToGC;
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
            script_->u.nextToGC = *bucket_;
            *bucket_ = script_;
#ifdef CHECK_SCRIPT_OWNER
            script_->owner = NULL;
#endif
        }
    }

    void lookupInEvalCache(StackFrame *caller, uintN staticLevel,
                           JSPrincipals *principals, JSObject &scopeobj) {
        if (JSScript *found = EvalCacheLookup(cx_, str_, caller, staticLevel,
                                              principals, scopeobj, bucket_)) {
            js_CallNewScriptHook(cx_, found, NULL);
            script_ = found;
        }
    }

    void setNewScript(JSScript *script) {
        
        JS_ASSERT(!script_ && script);
        script_ = script;
    }

    bool foundScript() {
        return !!script_;
    }

    JSScript *script() const {
        JS_ASSERT(script_);
        return script_;
    }
};











enum EvalType { DIRECT_EVAL, INDIRECT_EVAL };

static bool
EvalKernel(JSContext *cx, const CallArgs &call, EvalType evalType, StackFrame *caller,
           JSObject &scopeobj)
{
    JS_ASSERT((evalType == INDIRECT_EVAL) == (caller == NULL));
    AssertInnerizedScopeChain(cx, scopeobj);

    



    if (!js_CheckContentSecurityPolicy(cx, &scopeobj)) {
        JS_ReportError(cx, "call to eval() blocked by CSP");
        return false;
    }

    
    if (call.argc() < 1) {
        call.rval().setUndefined();
        return true;
    }
    if (!call[0].isString()) {
        call.rval() = call[0];
        return true;
    }
    JSString *str = call[0].toString();

    

    




    uintN staticLevel;
    if (evalType == DIRECT_EVAL) {
        staticLevel = caller->script()->staticLevel + 1;

#ifdef DEBUG
        jsbytecode *callerPC = caller->pc(cx);
        JS_ASSERT_IF(caller->isFunctionFrame(), caller->fun()->isHeavyweight());
        JS_ASSERT(callerPC && js_GetOpcode(cx, caller->script(), callerPC) == JSOP_EVAL);
#endif
    } else {
        JS_ASSERT(call.callee().getGlobal() == &scopeobj);
        staticLevel = 0;
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
        JSONParser parser(cx, chars + 1, length - 2, JSONParser::StrictJSON, JSONParser::NoError);
        Value tmp;
        if (!parser.parse(&tmp))
            return false;
        if (!tmp.isUndefined()) {
            call.rval() = tmp;
            return true;
        }
    }

    




    if (evalType == DIRECT_EVAL && !ComputeThis(cx, caller))
        return false;

    EvalScriptGuard esg(cx, linearStr);

    JSPrincipals *principals = PrincipalsForCompiledCode(call, cx);

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

    return Execute(cx, scopeobj, esg.script(), caller, StackFrame::EVAL, &call.rval());
}






static inline bool
WarnOnTooManyArgs(JSContext *cx, const CallArgs &call)
{
    if (call.argc() > 1) {
        if (StackFrame *caller = js_GetScriptedCaller(cx, NULL)) {
            if (!caller->script()->warnedAboutTwoArgumentEval) {
                static const char TWO_ARGUMENT_WARNING[] =
                    "Support for eval(code, scopeObject) has been removed. "
                    "Use |with (scopeObject) eval(code);| instead.";
                if (!JS_ReportWarning(cx, TWO_ARGUMENT_WARNING))
                    return false;
                caller->script()->warnedAboutTwoArgumentEval = true;
            }
        } else {
            



        }
    }

    return true;
}






static JSBool
eval(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs call = CallArgsFromVp(argc, vp);
    return WarnOnTooManyArgs(cx, call) &&
           EvalKernel(cx, call, INDIRECT_EVAL, NULL, *call.callee().getGlobal());
}

namespace js {

bool
DirectEval(JSContext *cx, const CallArgs &call)
{
    
    StackFrame *caller = cx->fp();
    JS_ASSERT(caller->isScriptFrame());
    JS_ASSERT(IsBuiltinEvalForScope(&caller->scopeChain(), call.calleev()));
    JS_ASSERT(*cx->regs().pc == JSOP_EVAL);

    AutoFunctionCallProbe callProbe(cx, call.callee().getFunctionPrivate(), caller->script());

    JSObject *scopeChain =
        GetScopeChainFast(cx, caller, JSOP_EVAL, JSOP_EVAL_LENGTH + JSOP_LINENO_LENGTH);

    return scopeChain &&
           WarnOnTooManyArgs(cx, call) &&
           EvalKernel(cx, call, DIRECT_EVAL, caller, *scopeChain);
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
PrincipalsForCompiledCode(const CallArgs &call, JSContext *cx)
{
    JS_ASSERT(IsAnyBuiltinEval(call.callee().getFunctionPrivate()) ||
              IsBuiltinFunctionConstructor(call.callee().getFunctionPrivate()));

    















    JSPrincipals *calleePrincipals = call.callee().principals(cx);

#ifdef DEBUG
    if (calleePrincipals) {
        if (StackFrame *caller = js_GetScriptedCaller(cx, NULL)) {
            if (JSPrincipals *callerPrincipals = caller->scopeChain().principals(cx)) {
                JS_ASSERT(callerPrincipals->subsume(callerPrincipals, calleePrincipals));
            }
        }
    }
#endif

    return calleePrincipals;
}

}  

#if JS_HAS_OBJ_WATCHPOINT

static JSBool
obj_watch_handler(JSContext *cx, JSObject *obj, jsid id, jsval old,
                  jsval *nvp, void *closure)
{
    JSObject *callable = (JSObject *) closure;
    if (JSPrincipals *watcher = callable->principals(cx)) {
        if (StackFrame *caller = js_GetScriptedCaller(cx, NULL)) {
            if (JSPrincipals *subject = caller->scopeChain().principals(cx)) {
                if (!watcher->subsume(watcher, subject)) {
                    
                    return JS_TRUE;
                }
            }
        }
    }

    
    AutoResolving resolving(cx, obj, id, AutoResolving::WATCH);
    if (resolving.alreadyStarted())
        return true;

    Value argv[] = { IdToValue(id), Valueify(old), Valueify(*nvp) };
    return ExternalInvoke(cx, ObjectValue(*obj), ObjectOrNullValue(callable),
                          JS_ARRAY_LENGTH(argv), argv, Valueify(nvp));
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
    return js_HasOwnPropertyHelper(cx, obj->getOps()->lookupProperty, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, LookupPropOp lookup, uintN argc,
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
        if (!JSProxy::hasOwn(cx, obj, id, &has))
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
js_HasOwnProperty(JSContext *cx, LookupPropOp lookup, JSObject *obj, jsid id,
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

        *propp = NULL;
    }
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
    CallArgs call = CallArgsFromVp(argc, vp);
    if (!BoxNonStrictThis(cx, call))
        return false;
    JSObject *obj = &call.thisv().toObject();

    if (argc <= 1 || !js_IsCallable(call[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_getter_str);
        return JS_FALSE;
    }
    PropertyOp getter = CastAsPropertyOp(&call[1].toObject());

    jsid id;
    if (!ValueToId(cx, call[0], &id))
        return JS_FALSE;
    if (!CheckRedeclaration(cx, obj, id, JSPROP_GETTER))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    call.rval().setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), getter, StrictPropertyStub,
                               JSPROP_ENUMERATE | JSPROP_GETTER | JSPROP_SHARED);
}

JS_FRIEND_API(JSBool)
js_obj_defineSetter(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs call = CallArgsFromVp(argc, vp);
    if (!BoxNonStrictThis(cx, call))
        return false;
    JSObject *obj = &call.thisv().toObject();

    if (argc <= 1 || !js_IsCallable(call[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_setter_str);
        return JS_FALSE;
    }
    StrictPropertyOp setter = CastAsStrictPropertyOp(&call[1].toObject());

    jsid id;
    if (!ValueToId(cx, call[0], &id))
        return JS_FALSE;
    if (!CheckRedeclaration(cx, obj, id, JSPROP_SETTER))
        return JS_FALSE;
    



    Value junk;
    uintN attrs;
    if (!CheckAccess(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    call.rval().setUndefined();
    return obj->defineProperty(cx, id, UndefinedValue(), PropertyStub, setter,
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
    if (!obj->lookupProperty(cx, id, &pobj, &prop))
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
    if (!obj->lookupProperty(cx, id, &pobj, &prop))
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
                                  PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE) ||
            !desc->defineProperty(cx, ATOM_TO_JSID(atomState.setAtom), setter,
                                  PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE)) {
            return false;
        }
    } else {
        if (!desc->defineProperty(cx, ATOM_TO_JSID(atomState.valueAtom), value,
                                  PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE) ||
            !desc->defineProperty(cx, ATOM_TO_JSID(atomState.writableAtom),
                                  BooleanValue((attrs & JSPROP_READONLY) == 0),
                                  PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE)) {
            return false;
        }
    }

    return desc->defineProperty(cx, ATOM_TO_JSID(atomState.enumerableAtom),
                                BooleanValue((attrs & JSPROP_ENUMERATE) != 0),
                                PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE) &&
           desc->defineProperty(cx, ATOM_TO_JSID(atomState.configurableAtom),
                                BooleanValue((attrs & JSPROP_PERMANENT) == 0),
                                PropertyStub, StrictPropertyStub, JSPROP_ENUMERATE);
}

JSBool
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (obj->isProxy())
        return JSProxy::getOwnPropertyDescriptor(cx, obj, id, false, vp);

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
    } else {
        if (!pobj->getAttributes(cx, id, &attrs))
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
    return js_GetOwnPropertyDescriptor(cx, obj, nameidr.id(), vp);
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

    





    return !!obj->getProperty(cx, id, vp);
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
PropDesc::initialize(JSContext* cx, const Value &origval)
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
        if ((v.isPrimitive() || !js_IsCallable(v)) && !v.isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_GET_SET_FIELD,
                                 js_getter_str);
            return false;
        }
        hasGet = true;
        get = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
    }

    
    if (!HasProperty(cx, desc, ATOM_TO_JSID(cx->runtime->atomState.setAtom), &v, &found))
        return false;
    if (found) {
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
    JS_ASSERT(!obj->getOps()->lookupProperty);
    if (!js_HasOwnProperty(cx, NULL, obj, id, &obj2, &current))
        return JS_FALSE;

    JS_ASSERT(!obj->getOps()->defineProperty);

    







    if (current && obj2 != obj && obj2->isNative()) {
        
        JS_ASSERT(obj2->getClass() == obj->getClass());

        Shape *shape = (Shape *) current;
        if (shape->isSharedPermanent())
            current = NULL;
    }

    
    if (!current) {
        if (!obj->isExtensible())
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            return js_DefineProperty(cx, obj, id, &desc.value,
                                     PropertyStub, StrictPropertyStub, desc.attrs);
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

    







    JS_ASSERT(obj->getClass() == obj2->getClass());

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
            getter = PropertyStub;
            setter = StrictPropertyStub;
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
        getter = PropertyStub;
        setter = StrictPropertyStub;
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
                     ? PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet) {
            setter = desc.setter();
        } else {
            setter = (shape->hasDefaultSetter() && !shape->hasSetterValue())
                     ? StrictPropertyStub
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
            obj->setArrayLength(index + 1);
        }

        *rval = true;
        return JS_TRUE;
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

static JSBool
DefineProperty(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc, bool throwError,
               bool *rval)
{
    if (obj->isArray())
        return DefinePropertyOnArray(cx, obj, id, desc, throwError, rval);

    if (obj->getOps()->lookupProperty) {
        if (obj->isProxy())
            return JSProxy::defineProperty(cx, obj, id, desc.pd);
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
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
    AutoIdVector ids(cx);
    if (!GetPropertyNames(cx, props, JSITER_OWNONLY, &ids))
        return false;

     AutoPropDescArrayRooter descs(cx);
     size_t len = ids.length();
     for (size_t i = 0; i < len; i++) {
         jsid id = ids[i];
         PropDesc* desc = descs.append();
         Value v;
         if (!desc || !props->getProperty(cx, id, &v) || !desc->initialize(cx, v))
             return false;
     }

     bool dummy;
     for (size_t i = 0; i < len; i++) {
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

    vp->setBoolean(false);

    if (obj->isExtensible())
        return true; 

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];

        uintN attrs = 0;
        if (!obj->getAttributes(cx, id, &attrs))
            return false;

        
        if (!(attrs & JSPROP_PERMANENT) ||
            !(attrs & (JSPROP_READONLY | JSPROP_GETTER | JSPROP_SETTER)))
            return true; 
    }

    
    vp->setBoolean(true);
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

    
    vp->setBoolean(false);

    if (obj->isExtensible())
        return true; 

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    for (size_t i = 0, len = props.length(); i < len; i++) {
        jsid id = props[i];

        uintN attrs;
        if (!obj->getAttributes(cx, id, &attrs))
            return false;

        if (!(attrs & JSPROP_PERMANENT))
            return true; 
    }

    
    vp->setBoolean(true);
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
        gc::FinalizeKind kind = NewObjectGCKind(cx, &js_ObjectClass);
        obj = NewBuiltinClassInstance(cx, &js_ObjectClass, kind);
        if (!obj)
            return JS_FALSE;
    }
    vp->setObject(*obj);
    return JS_TRUE;
}

JSObject*
js_CreateThis(JSContext *cx, JSObject *callee)
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
    gc::FinalizeKind kind = NewObjectGCKind(cx, newclasp);
    JSObject *obj = NewObject<WithProto::Class>(cx, newclasp, proto, parent, kind);
    if (obj)
        obj->syncSpecialEquality();
    return obj;
}

JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto)
{
    gc::FinalizeKind kind = NewObjectGCKind(cx, &js_ObjectClass);
    return NewNonFunction<WithProto::Class>(cx, &js_ObjectClass, proto, callee->getParent(), kind);
}

JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee)
{
    Value protov;
    if (!callee->getProperty(cx,
                             ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                             &protov)) {
        return NULL;
    }
    JSObject *proto = protov.isObject() ? &protov.toObject() : NULL;
    return js_CreateThisForFunctionWithProto(cx, callee, proto);
}

#ifdef JS_TRACER

JSObject* FASTCALL
js_Object_tn(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(!(js_ObjectClass.flags & JSCLASS_HAS_PRIVATE));
    return NewObjectWithClassProto(cx, &js_ObjectClass, proto, FINALIZE_OBJECT8);
}

JS_DEFINE_TRCINFO_1(js_Object,
    (2, (extern, CONSTRUCTOR_RETRY, js_Object_tn, CONTEXT, CALLEE_PROTOTYPE, 0,
         nanojit::ACCSET_STORE_ANY)))

JSObject* FASTCALL
js_InitializerObject(JSContext* cx, JSObject *proto, JSObject *baseobj)
{
    if (!baseobj) {
        gc::FinalizeKind kind = GuessObjectGCKind(0, false);
        return NewObjectWithClassProto(cx, &js_ObjectClass, proto, kind);
    }

    return CopyInitializerObject(cx, baseobj);
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
#ifdef DEBUG
    JS_ASSERT(ctor->isFunction());
    JS_ASSERT(ctor->getFunctionPrivate()->isInterpreted());
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    const Shape *shape = ctor->nativeLookup(id);
    JS_ASSERT(shape->slot == protoSlot);
    JS_ASSERT(!shape->configurable());
    JS_ASSERT(!shape->isMethod());
#endif

    JSObject *parent = ctor->getParent();
    JSObject *proto;
    const Value &protov = ctor->getSlotRef(protoSlot);
    if (protov.isObject()) {
        proto = &protov.toObject();
    } else {
        



        if (!js_GetClassPrototype(cx, parent, JSProto_Object, &proto))
            return NULL;
    }

    gc::FinalizeKind kind = NewObjectGCKind(cx, &js_ObjectClass);
    return NewNativeClassInstance(cx, &js_ObjectClass, proto, parent, kind);
}
JS_DEFINE_CALLINFO_3(extern, CONSTRUCTOR_RETRY, js_CreateThisFromTrace, CONTEXT, OBJECT, UINTN, 0,
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
        return JS_TRACE_MONITOR_ON_TRACE(cx)->bailExit->lookupFlags;
#endif

    JS_ASSERT_NOT_ON_TRACE(cx);

    jsbytecode *pc;
    const JSCodeSpec *cs;
    uint32 format;
    uintN flags = 0;

    StackFrame *const fp = js_GetTopStackFrame(cx);
    if (!fp || !(pc = cx->regs().pc))
        return defaultFlags;
    cs = &js_CodeSpec[js_GetOpcode(cx, fp->script(), pc)];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if (format & (JOF_SET | JOF_FOR)) {
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
    if (flags == RESOLVE_INFER)
        flags = js_InferFlags(cx, flags);
    flags |= JSRESOLVE_WITH;
    JSAutoResolveFlags rf(cx, flags);
    return obj->getProto()->lookupProperty(cx, id, objp, propp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
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
    StrictPropertyStub,   
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

    obj = js_NewGCObject(cx, FINALIZE_OBJECT2);
    if (!obj)
        return NULL;

    StackFrame *priv = js_FloatingFrameIfGenerator(cx, cx->fp());

    EmptyShape *emptyWithShape = EmptyShape::getEmptyWithShape(cx);
    if (!emptyWithShape)
        return NULL;

    obj->init(cx, &js_WithClass, proto, parent, priv, false);
    obj->setMap(emptyWithShape);
    OBJ_SET_BLOCK_DEPTH(cx, obj, depth);

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
    



    JSObject *blockObj = js_NewGCObject(cx, FINALIZE_OBJECT2);
    if (!blockObj)
        return NULL;

    EmptyShape *emptyBlockShape = EmptyShape::getEmptyBlockShape(cx);
    if (!emptyBlockShape)
        return NULL;

    blockObj->init(cx, &js_BlockClass, NULL, NULL, NULL, false);
    blockObj->setMap(emptyBlockShape);
    return blockObj;
}

JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, StackFrame *fp)
{
    JS_ASSERT(proto->isStaticBlock());

    size_t count = OBJ_BLOCK_COUNT(cx, proto);
    gc::FinalizeKind kind = gc::GetGCObjectKind(count + 1);

    JSObject *clone = js_NewGCObject(cx, kind);
    if (!clone)
        return NULL;

    StackFrame *priv = js_FloatingFrameIfGenerator(cx, fp);

    
    clone->initClonedBlock(cx, proto, priv);

    if (!clone->ensureInstanceReservedSlots(cx, count + 1))
        return NULL;

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
    JS_ASSERT(obj->numSlots() >= JSSLOT_BLOCK_DEPTH + 1 + count);

    
    uintN depth = OBJ_BLOCK_DEPTH(cx, obj);
    JS_ASSERT(depth <= size_t(cx->regs().sp - fp->base()));
    JS_ASSERT(count <= size_t(cx->regs().sp - fp->base() - depth));

    
    JS_ASSERT(count >= 1);

    if (normalUnwind) {
        uintN slot = JSSLOT_BLOCK_FIRST_FREE_SLOT;
        depth += fp->numFixed();
        memcpy(obj->getSlots() + slot, fp->slots() + depth, count * sizeof(Value));
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

    StackFrame *fp = (StackFrame *) obj->getPrivate();
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
           : sizeof(JSObject) + sizeof(js::Value) * obj->numFixedSlots();
}

bool
JSObject::copyPropertiesFrom(JSContext *cx, JSObject *obj)
{
    
    JS_ASSERT(isNative() == obj->isNative());
    if (!isNative())
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
        Value v = shape->hasSlot() ? obj->getSlot(shape->slot) : UndefinedValue();
        if (!cx->compartment->wrap(cx, &v))
            return false;
        if (!defineProperty(cx, shape->propid, v, getter, setter, attrs))
            return false;
    }
    return true;
}

static bool
CopySlots(JSContext *cx, JSObject *from, JSObject *to)
{
    JS_ASSERT(!from->isNative() && !to->isNative());
    size_t nslots = from->numSlots();
    if (to->ensureSlots(cx, nslots))
        return false;

    size_t n = 0;
    if (to->isWrapper() &&
        (JSWrapper::wrapperHandler(to)->flags() & JSWrapper::CROSS_COMPARTMENT)) {
        to->slots[0] = from->slots[0];
        to->slots[1] = from->slots[1];
        n = 2;
    }

    for (; n < nslots; ++n) {
        Value v = from->slots[n];
        if (!cx->compartment->wrap(cx, &v))
            return false;
        to->slots[n] = v;
    }
    return true;
}

JSObject *
JSObject::clone(JSContext *cx, JSObject *proto, JSObject *parent)
{
    



    if (!isNative()) {
        if (isDenseArray()) {
            if (!makeDenseArraySlow(cx))
                return NULL;
        } else if (!isProxy()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }
    }
    JSObject *clone = NewObject<WithProto::Given>(cx, getClass(),
                                                  proto, parent,
                                                  gc::FinalizeKind(finalizeKind()));
    if (!clone)
        return NULL;
    if (isNative()) {
        if (clone->isFunction() && (compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CLONE_OBJECT);
            return NULL;
        }

        if (getClass()->flags & JSCLASS_HAS_PRIVATE)
            clone->setPrivate(getPrivate());
    } else {
        JS_ASSERT(isProxy());
        if (!CopySlots(cx, this, clone))
            return NULL;
    }
    return clone;
}

static void
TradeGuts(JSObject *a, JSObject *b)
{
    JS_ASSERT(a->compartment() == b->compartment());
    JS_ASSERT(a->isFunction() == b->isFunction());

    




    JS_ASSERT(!a->isRegExp() && !b->isRegExp());

    bool aInline = !a->hasSlotsArray();
    bool bInline = !b->hasSlotsArray();

    
    const size_t size = GetObjectSize(a);
    if (size == GetObjectSize(b)) {
        




        char tmp[tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::result];
        JS_ASSERT(size <= sizeof(tmp));

        memcpy(tmp, a, size);
        memcpy(a, b, size);
        memcpy(b, tmp, size);

        
        if (aInline)
            b->slots = b->fixedSlots();
        if (bInline)
            a->slots = a->fixedSlots();
    } else {
        




        if (a->isFunction()) {
            JSFunction tmp;
            memcpy(&tmp, a, sizeof tmp);
            memcpy(a, b, sizeof tmp);
            memcpy(b, &tmp, sizeof tmp);
        } else {
            JSObject tmp;
            memcpy(&tmp, a, sizeof tmp);
            memcpy(a, b, sizeof tmp);
            memcpy(b, &tmp, sizeof tmp);
        }

        JS_ASSERT(!aInline);
        JS_ASSERT(!bInline);
    }
}







bool
JSObject::swap(JSContext *cx, JSObject *other)
{
    



    if (GetObjectSize(this) != GetObjectSize(other)) {
        if (!hasSlotsArray()) {
            if (!allocSlots(cx, numSlots()))
                return false;
        }
        if (!other->hasSlotsArray()) {
            if (!other->allocSlots(cx, other->numSlots()))
                return false;
        }
    }

    if (this->compartment() == other->compartment()) {
        TradeGuts(this, other);
        return true;
    }

    JSObject *thisClone;
    JSObject *otherClone;
    {
        AutoCompartment ac(cx, other);
        if (!ac.enter())
            return false;
        thisClone = this->clone(cx, other->getProto(), other->getParent());
        if (!thisClone || !thisClone->copyPropertiesFrom(cx, this))
            return false;
    }
    {
        AutoCompartment ac(cx, this);
        if (!ac.enter())
            return false;
        otherClone = other->clone(cx, other->getProto(), other->getParent());
        if (!otherClone || !otherClone->copyPropertiesFrom(cx, other))
            return false;
    }
    TradeGuts(this, otherClone);
    TradeGuts(other, thisClone);

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
        parent = obj->getParent();
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
            shapes[shape->shortid] = shape;
        }

        



        for (uintN i = 0; i < count; i++) {
            shape = shapes[i];
            JS_ASSERT(shape->getter() == block_getProperty);

            jsid propid = shape->propid;
            JS_ASSERT(JSID_IS_ATOM(propid));
            JSAtom *atom = JSID_TO_ATOM(propid);

#ifdef DEBUG
            uint16 shortid = uint16(shape->shortid);
            JS_ASSERT(shortid == i);
#endif

            
            if (!js_XDRAtom(xdr, &atom))
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
    StrictPropertyStub,   
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

    
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.evalAtom);
    JSObject *evalobj = js_DefineFunction(cx, obj, id, eval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return NULL;
    if (obj->isGlobal())
        obj->asGlobal()->setOriginalEval(evalobj);

    return proto;
}

static bool
DefineStandardSlot(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                   const Value &v, uint32 attrs, bool &named)
{
    jsid id = ATOM_TO_JSID(atom);

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->isGlobal());
        JS_ASSERT(obj->isNative());

        if (!obj->ensureClassReservedSlots(cx))
            return false;

        const Shape *shape = obj->nativeLookup(id);
        if (!shape) {
            uint32 slot = 2 * JSProto_LIMIT + key;
            if (!js_SetReservedSlot(cx, obj, slot, v))
                return false;
            if (!obj->addProperty(cx, id, PropertyStub, StrictPropertyStub, slot, attrs, 0, 0))
                return false;

            named = true;
            return true;
        }
    }

    named = obj->defineProperty(cx, id, v, PropertyStub, StrictPropertyStub, attrs);
    return named;
}

namespace js {

JSObject *
DefineConstructorAndPrototype(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, uintN nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
    




















    JSObject *proto = NewObject<WithProto::Class>(cx, clasp, protoProto, obj);
    if (!proto)
        return NULL;

    proto->syncSpecialEquality();

    
    AutoObjectRooter tvr(cx, proto);

    JSObject *ctor;
    bool named = false;
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
        JSFunction *fun = js_NewFunction(cx, NULL, constructor, nargs, JSFUN_CONSTRUCTOR, obj, atom);
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

    



    if (fs)
        proto->brand(cx);
    if (ctor != proto && static_fs)
        ctor->brand(cx);

    










    JS_ASSERT_IF(proto->clasp != clasp,
                 clasp == &js_ArrayClass && proto->clasp == &js_SlowArrayClass);
    if (!proto->getEmptyShape(cx, proto->clasp, FINALIZE_OBJECT0))
        goto bad;

    if (clasp->flags & (JSCLASS_FREEZE_PROTO|JSCLASS_FREEZE_CTOR)) {
        JS_ASSERT_IF(ctor == proto, !(clasp->flags & JSCLASS_FREEZE_CTOR));
        if (proto && (clasp->flags & JSCLASS_FREEZE_PROTO) && !proto->freeze(cx))
            goto bad;
        if (ctor && (clasp->flags & JSCLASS_FREEZE_CTOR) && !ctor->freeze(cx))
            goto bad;
    }

    
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
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
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
                                         ps, fs, static_ps, static_fs);
}

bool
JSObject::allocSlots(JSContext *cx, size_t newcap)
{
    uint32 oldcap = numSlots();

    JS_ASSERT(newcap >= oldcap && !hasSlotsArray());

    if (newcap > NSLOTS_LIMIT) {
        if (!JS_ON_TRACE(cx))
            js_ReportAllocationOverflow(cx);
        return false;
    }

    Value *tmpslots = (Value*) cx->malloc_(newcap * sizeof(Value));
    if (!tmpslots)
        return false;  
    slots = tmpslots;
    capacity = newcap;

    
    memcpy(slots, fixedSlots(), oldcap * sizeof(Value));
    ClearValueRange(slots + oldcap, newcap - oldcap, isDenseArray());
    return true;
}

bool
JSObject::growSlots(JSContext *cx, size_t newcap)
{
    






    static const size_t CAPACITY_DOUBLING_MAX = 1024 * 1024;
    static const size_t CAPACITY_CHUNK = CAPACITY_DOUBLING_MAX / sizeof(Value);

    uint32 oldcap = numSlots();
    JS_ASSERT(oldcap < newcap);

    uint32 nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                    ? oldcap * 2
                    : oldcap + (oldcap >> 3);

    uint32 actualCapacity = JS_MAX(newcap, nextsize);
    if (actualCapacity >= CAPACITY_CHUNK)
        actualCapacity = JS_ROUNDUP(actualCapacity, CAPACITY_CHUNK);
    else if (actualCapacity < SLOT_CAPACITY_MIN)
        actualCapacity = SLOT_CAPACITY_MIN;

    
    if (actualCapacity >= NSLOTS_LIMIT) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    
    if (!hasSlotsArray())
        return allocSlots(cx, actualCapacity);

    Value *tmpslots = (Value*) cx->realloc_(slots, oldcap * sizeof(Value), actualCapacity * sizeof(Value));
    if (!tmpslots)
        return false;    
    slots = tmpslots;
    capacity = actualCapacity;

    
    ClearValueRange(slots + oldcap, actualCapacity - oldcap, isDenseArray());
    return true;
}

void
JSObject::shrinkSlots(JSContext *cx, size_t newcap)
{
    uint32 oldcap = numSlots();
    JS_ASSERT(newcap <= oldcap);
    JS_ASSERT(newcap >= slotSpan());

    if (oldcap <= SLOT_CAPACITY_MIN || !hasSlotsArray()) {
        
        ClearValueRange(slots + newcap, oldcap - newcap, isDenseArray());
        return;
    }

    uint32 fill = newcap;
    if (newcap < SLOT_CAPACITY_MIN)
        newcap = SLOT_CAPACITY_MIN;
    if (newcap < numFixedSlots())
        newcap = numFixedSlots();

    Value *tmpslots = (Value*) cx->realloc_(slots, newcap * sizeof(Value));
    if (!tmpslots)
        return;  
    slots = tmpslots;
    capacity = newcap;

    if (fill < newcap) {
        
        ClearValueRange(slots + fill, newcap - fill, isDenseArray());
    }
}

bool
JSObject::ensureInstanceReservedSlots(JSContext *cx, size_t nreserved)
{
    JS_ASSERT_IF(isNative(),
                 isBlock() || isCall() || (isFunction() && isBoundFunction()));

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
    JS_ASSERT(obj->isExtensible());

    if (obj->isNative()) {
        if (!obj->ensureClassReservedSlots(cx))
            return false;
    }

    




    JSObject *oldproto = obj;
    while (oldproto && oldproto->isNative()) {
        oldproto->protoShapeChange(cx);
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
    obj->setProto(proto);
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
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->isGlobal())
        return JS_TRUE;

    return js_SetReservedSlot(cx, obj, key, ObjectOrNullValue(cobj)) &&
           js_SetReservedSlot(cx, obj, JSProto_LIMIT + key, ObjectOrNullValue(proto));
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
        
        do {
            obj = start;
            start = obj->getParent();
        } while (start);
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
        if (pobj->containsSlot(shape->slot)) {
            v = pobj->nativeGetSlot(shape->slot);
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

    obj->syncSpecialEquality();

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

    



    if (inDictionaryMode() && lastProp->hasTable()) {
        uint32 &last = lastProp->getTable()->freelist;
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

bool
JSObject::freeSlot(JSContext *cx, uint32 slot)
{
    uint32 limit = slotSpan();
    JS_ASSERT(slot < limit);

    Value &vref = getSlotRef(slot);
    if (inDictionaryMode() && lastProp->hasTable()) {
        uint32 &last = lastProp->getTable()->freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < limit && last != slot);

        





        if (JSSLOT_FREE(clasp) <= slot && slot + 1 < limit) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            vref.setPrivateUint32(last);
            last = slot;
            return true;
        }
    }
    vref.setUndefined();
    return false;
}


#define JSBOXEDWORD_INT_MAX_STRING "1073741823"







jsid
js_CheckForStringIndex(jsid id)
{
    if (!JSID_IS_ATOM(id))
        return id;

    JSAtom *atom = JSID_TO_ATOM(id);
    const jschar *s = atom->chars();
    jschar ch = *s;

    JSBool negative = (ch == '-');
    if (negative)
        ch = *++s;

    if (!JS7_ISDEC(ch))
        return id;

    size_t n = atom->length() - negative;
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

    if (negative) {
        if (oldIndex < -(JSID_INT_MIN / 10) ||
            (oldIndex == -(JSID_INT_MIN / 10) && c <= (-JSID_INT_MIN % 10)))
        {
            id = INT_TO_JSID(-jsint(index));
        }
    } else {
        if (oldIndex < JSID_INT_MAX / 10 ||
            (oldIndex == JSID_INT_MAX / 10 && c <= (JSID_INT_MAX % 10)))
        {
            id = INT_TO_JSID(jsint(index));
        }
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
        shape = obj->nativeLookup(id);
        if (shape) {
            PCMETER(JS_PROPERTY_CACHE(cx).pcpurges++);
            obj->shadowingShapeChange(cx, *shape);

            if (!obj->getParent()) {
                




                LeaveTrace(cx);
            }
            return JS_TRUE;
        }
        obj = obj->getProto();
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
                     PropertyOp getter, StrictPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid)
{
    JS_ASSERT(!(flags & Shape::METHOD));

    




    js_PurgeScopeChain(cx, obj, id);

    if (!obj->ensureClassReservedSlots(cx))
        return NULL;

    
    id = js_CheckForStringIndex(id);
    return obj->putProperty(cx, id, getter, setter, slot, attrs, flags, shortid);
}

const Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const Shape *shape, uintN attrs, uintN mask,
                             PropertyOp getter, StrictPropertyOp setter)
{
    if (!obj->ensureClassReservedSlots(cx))
        return NULL;

    





    if ((attrs & JSPROP_READONLY) && shape->isMethod()) {
        JSObject *funobj = &shape->methodObject();
        Value v = ObjectValue(*funobj);

        shape = obj->methodReadBarrier(cx, *shape, &v);
        if (!shape)
            return NULL;

        if (CastAsObject(getter) == funobj) {
            JS_ASSERT(!(attrs & JSPROP_GETTER));
            getter = NULL;
        }
    }

    return obj->changeProperty(cx, shape, attrs, mask, getter, setter);
}

JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                  PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    return !!DefineNativeProperty(cx, obj, id, *value, getter, setter, attrs, 0, 0);
}







static inline bool
CallAddPropertyHook(JSContext *cx, Class *clasp, JSObject *obj, const Shape *shape, Value *vp)
{
    if (clasp->addProperty != PropertyStub) {
        Value nominal = *vp;

        if (!CallJSPropertyOp(cx, clasp->addProperty, obj, shape->propid, vp))
            return false;
        if (*vp != nominal) {
            if (obj->containsSlot(shape->slot))
                obj->nativeSetSlot(shape->slot, *vp);
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
    JS_ASSERT((defineHow & ~(DNP_CACHE_RESULT | DNP_DONT_PURGE | DNP_SET_METHOD)) == 0);
    LeaveTraceIfGlobalObject(cx, obj);

    
    id = js_CheckForStringIndex(id);

    




    const Shape *shape = NULL;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        




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

    




    if (!(defineHow & DNP_DONT_PURGE))
        js_PurgeScopeChain(cx, obj, id);

    




    if (obj->isDelegate() && (attrs & (JSPROP_READONLY | JSPROP_SETTER)))
        cx->runtime->protoHazardShape = js_GenerateShape(cx);

    
    Class *clasp = obj->getClass();
    if (!(defineHow & DNP_SET_METHOD)) {
        if (!getter && !(attrs & JSPROP_GETTER))
            getter = clasp->getProperty;
        if (!setter && !(attrs & JSPROP_SETTER))
            setter = clasp->setProperty;
    }

    
    if (!obj->ensureClassReservedSlots(cx))
        return NULL;

    



    Value valueCopy = value;
    bool adding = false;

    if (!shape) {
        
        if (defineHow & DNP_SET_METHOD) {
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

        if (const Shape *existingShape = obj->nativeLookup(id)) {
            if (existingShape->hasSlot())
                AbortRecordingIfUnexpectedGlobalWrite(cx, obj, existingShape->slot);

            if (existingShape->isMethod() &&
                ObjectValue(existingShape->methodObject()) == valueCopy)
            {
                










                JS_ASSERT(existingShape->getter() != getter);

                if (!obj->methodReadBarrier(cx, *existingShape, &valueCopy))
                    return NULL;
            }
        } else {
            adding = true;
        }

        uint32 oldShape = obj->shape();
        shape = obj->putProperty(cx, id, getter, setter, SHAPE_INVALID_SLOT,
                                 attrs, flags, shortid);
        if (!shape)
            return NULL;

        









        if (obj->shape() == oldShape && obj->branded() && shape->slot != SHAPE_INVALID_SLOT) {
            DebugOnly<const Shape*> newshape =
                obj->methodWriteBarrier(cx, *shape, valueCopy);
            JS_ASSERT(newshape == shape);
        }
    }

    
    if (obj->containsSlot(shape->slot))
        obj->nativeSetSlot(shape->slot, valueCopy);

    
    if (!CallAddPropertyHook(cx, clasp, obj, shape, &valueCopy)) {
        obj->removeProperty(cx, id);
        return NULL;
    }

    if (defineHow & DNP_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        if (adding) {
            JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, obj, shape, true);
            TRACE_1(AddProperty, obj);
        }
    }
    return shape;

#ifdef JS_TRACER
  error:
    
    return NULL;
#endif
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
            return obj2->lookupProperty(cx, id, objp, propp);
        }
        obj = obj2;
    } else {
        if (!resolve(cx, obj, id))
            return false;
    }

    if (!obj->nativeEmpty()) {
        if (const Shape *shape = obj->nativeLookup(id)) {
            *objp = obj;
            *propp = (JSProperty *) shape;
        }
    }

    return true;
}

#define SCOPE_DEPTH_ACCUM(bs,val) JS_SCOPE_DEPTH_METERING(JS_BASIC_STATS_ACCUM(bs, val))

static JS_ALWAYS_INLINE bool
LookupPropertyWithFlagsInline(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                              JSObject **objp, JSProperty **propp)
{
    
    JS_ASSERT(id == js_CheckForStringIndex(id));

    
    JSObject *start = obj;
#ifdef JS_SCOPE_DEPTH_METER
    int protoIndex = 0;
#endif
    for (; ; JS_SCOPE_DEPTH_METERING(protoIndex++)) {
        const Shape *shape = obj->nativeLookup(id);
        if (shape) {
            SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
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
                



                SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
                return true;
            }
        }

        JSObject *proto = obj->getProto();
        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!proto->lookupProperty(cx, id, objp, propp))
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
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    JSObject *scopeChain, *obj, *parent, *pobj;
    PropertyCacheEntry *entry;
    int scopeIndex;
    JSProperty *prop;

    JS_ASSERT_IF(cacheResult, !JS_ON_TRACE(cx));
    scopeChain = &js_GetTopStackFrame(cx)->scopeChain();

    
    entry = JS_NO_PROP_CACHE_FILL;
    obj = scopeChain;
    parent = obj->getParent();
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
                Class *clasp = obj->getClass();
                JS_ASSERT(pobj->isNative());
                JS_ASSERT(pobj->getClass() == clasp);
                if (clasp == &js_BlockClass) {
                    




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
         !obj->getParent() || IsCacheableNonGlobalScope(obj);
         scopeIndex++) {
        JSObject *pobj;
        JSProperty *prop;
        if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
            return NULL;
        if (prop) {
            if (!pobj->isNative()) {
                JS_ASSERT(!obj->getParent());
                return obj;
            }
            JS_ASSERT_IF(obj->getParent(), pobj->getClass() == obj->getClass());
            DebugOnly<PropertyCacheEntry*> entry =
                JS_PROPERTY_CACHE(cx).fill(cx, scopeChain, scopeIndex, pobj, (Shape *) prop);
            JS_ASSERT(entry);
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
        if (prop)
            break;

        




        JSObject *parent = obj->getParent();
        if (!parent)
            break;
        obj = parent;
    } while (obj->getParent());
    return obj;
}

static JS_ALWAYS_INLINE JSBool
js_NativeGetInline(JSContext *cx, JSObject *receiver, JSObject *obj, JSObject *pobj,
                   const Shape *shape, uintN getHow, Value *vp)
{
    LeaveTraceIfGlobalObject(cx, pobj);

    uint32 slot;
    int32 sample;

    JS_ASSERT(pobj->isNative());

    slot = shape->slot;
    if (slot != SHAPE_INVALID_SLOT) {
        *vp = pobj->nativeGetSlot(slot);
        JS_ASSERT(!vp->isMagic());
    } else {
        vp->setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    if (JS_UNLIKELY(shape->isMethod()) && (getHow & JSGET_NO_METHOD_BARRIER)) {
        JS_ASSERT(shape->methodObject() == vp->toObject());
        return true;
    }

    sample = cx->runtime->propertyRemovals;
    {
        AutoShapeRooter tvr(cx, shape);
        AutoObjectRooter tvr2(cx, pobj);
        if (!shape->get(cx, receiver, obj, pobj, vp))
            return false;
    }

    if (pobj->containsSlot(slot) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         pobj->nativeContains(*shape))) {
        if (!pobj->methodWriteBarrier(cx, *shape, *vp))
            return false;
        pobj->nativeSetSlot(slot, *vp);
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

    uint32 slot;
    int32 sample;

    JS_ASSERT(obj->isNative());

    slot = shape->slot;
    if (slot != SHAPE_INVALID_SLOT) {
        JS_ASSERT(obj->containsSlot(slot));

        
        if (shape->hasDefaultSetter()) {
            if (!added) {
                AbortRecordingIfUnexpectedGlobalWrite(cx, obj, slot);

                
                if (!obj->methodWriteBarrier(cx, slot, *vp))
                    return false;
            }
            obj->nativeSetSlot(slot, *vp);
            return true;
        }
    } else {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx);
    }

    sample = cx->runtime->propertyRemovals;
    {
        AutoShapeRooter tvr(cx, shape);
        if (!shape->set(cx, obj, strict, vp))
            return false;

        JS_ASSERT_IF(!obj->inDictionaryMode(), shape->slot == slot);
        slot = shape->slot;
    }

    if (obj->containsSlot(slot) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         obj->nativeContains(*shape))) {
        if (!added) {
            AbortRecordingIfUnexpectedGlobalWrite(cx, obj, slot);
            if (!obj->methodWriteBarrier(cx, *shape, *vp))
                return false;
        }
        obj->setSlot(slot, *vp);
    }

    return true;
}

static JS_ALWAYS_INLINE bool
js_GetPropertyHelperWithShapeInline(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                                    uintN getHow, Value *vp,
                                    const Shape **shapeOut, JSObject **holderOut)
{
    JSObject *aobj, *obj2;
    JSProperty *prop;
    const Shape *shape;

    JS_ASSERT_IF(getHow & JSGET_CACHE_RESULT, !JS_ON_TRACE(cx));

    *shapeOut = NULL;

    
    id = js_CheckForStringIndex(id);

    aobj = js_GetProtoIfDenseArray(obj);
    
    if (!LookupPropertyWithFlagsInline(cx, aobj, id, cx->resolveFlags, &obj2, &prop))
        return false;

    *holderOut = obj2;

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
                if (!cx->hasStrictOption() ||
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
               ? JSProxy::get(cx, obj2, receiver, id, vp)
               : obj2->getProperty(cx, id, vp);
    }

    shape = (Shape *) prop;
    *shapeOut = shape;

    if (getHow & JSGET_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        JS_PROPERTY_CACHE(cx).fill(cx, aobj, 0, obj2, shape);
    }

    
    if (!js_NativeGetInline(cx, receiver, obj, obj2, shape, getHow, vp))
        return JS_FALSE;

    return JS_TRUE;
}

bool
js_GetPropertyHelperWithShape(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                              uint32 getHow, Value *vp,
                              const Shape **shapeOut, JSObject **holderOut)
{
    return js_GetPropertyHelperWithShapeInline(cx, obj, receiver, id, getHow, vp,
                                               shapeOut, holderOut);
}

static JS_ALWAYS_INLINE JSBool
js_GetPropertyHelperInline(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                           uint32 getHow, Value *vp)
{
    const Shape *shape;
    JSObject *holder;
    return js_GetPropertyHelperWithShapeInline(cx, obj, receiver, id, getHow, vp, &shape, &holder);
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
    return op(cx, obj, obj, id, vp);
}

JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    StackFrame *const fp = js_GetTopStackFrame(cx);
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
           ExternalInvoke(cx, ObjectValue(*this), fval, argc, argv, vp);
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

    if (!LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags, &pobj, &prop))
        return false;
    if (prop) {
        if (!pobj->isNative()) {
            if (pobj->isProxy()) {
                AutoPropertyDescriptorRooter pd(cx);
                if (!JSProxy::getPropertyDescriptor(cx, pobj, id, true, &pd))
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

        if (!obj->getParent() &&
            (defineHow & DNP_UNQUALIFIED) &&
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
                    shortid = shape->shortid;
                }
                attrs &= ~JSPROP_SHARED;
                getter = shape->getter();
                setter = shape->setter();
            } else {
                
                attrs = JSPROP_ENUMERATE;
            }

            



            shape = NULL;
        }

        JS_ASSERT_IF(shape && shape->isMethod(), pobj->hasMethodBarrier());
        JS_ASSERT_IF(shape && shape->isMethod(),
                     pobj->getSlot(shape->slot).toObject() == shape->methodObject());
        if (shape && (defineHow & DNP_SET_METHOD)) {
            






            bool identical = shape->isMethod() && shape->methodObject() == vp->toObject();
            if (!identical) {
                shape = obj->methodShapeChange(cx, *shape);
                if (!shape)
                    return false;

                JSObject *funobj = &vp->toObject();
                JSFunction *fun = funobj->getFunctionPrivate();
                if (fun == funobj) {
                    funobj = CloneFunctionObject(cx, fun, fun->parent);
                    if (!funobj)
                        return JS_FALSE;
                    vp->setObject(*funobj);
                }
            }
            return identical || js_NativeSet(cx, obj, shape, false, strict, vp);
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

        



        js_PurgeScopeChain(cx, obj, id);

        
        if (!obj->ensureClassReservedSlots(cx))
            return JS_FALSE;

        



        if ((defineHow & DNP_SET_METHOD) && obj->canHaveMethodBarrier()) {
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
        if (!shape)
            return JS_FALSE;

        if (defineHow & DNP_CACHE_RESULT)
            TRACE_1(AddProperty, obj);

        




        if (obj->containsSlot(shape->slot))
            obj->nativeSetSlot(shape->slot, UndefinedValue());

        
        if (!CallAddPropertyHook(cx, clasp, obj, shape, vp)) {
            obj->removeProperty(cx, id);
            return JS_FALSE;
        }
        added = true;
    }

    if (defineHow & DNP_CACHE_RESULT)
        JS_PROPERTY_CACHE(cx).fill(cx, obj, 0, obj, shape, added);

    return js_NativeSet(cx, obj, shape, added, strict, vp);

#ifdef JS_TRACER
  error: 
    return JS_FALSE;
#endif
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
        





        if (prop && proto->isNative()) {
            shape = (Shape *)prop;
            if (shape->isSharedPermanent()) {
                if (strict)
                    return obj->reportNotConfigurable(cx, id);
                rval->setBoolean(false);
                return true;
            }
        }

        




        return CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, id, rval);
    }

    shape = (Shape *)prop;
    if (!shape->configurable()) {
        if (strict)
            return obj->reportNotConfigurable(cx, id);
        rval->setBoolean(false);
        return true;
    }

    if (!CallJSPropertyOp(cx, obj->getClass()->delProperty, obj, SHAPE_USERID(shape), rval))
        return false;

    if (obj->containsSlot(shape->slot)) {
        const Value &v = obj->nativeGetSlot(shape->slot);
        GC_POKE(cx, v);

        










        if (obj->hasMethodBarrier()) {
            JSObject *funobj;

            if (IsFunctionObject(v, &funobj)) {
                JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

                if (fun != funobj) {
                    for (StackFrame *fp = cx->maybefp(); fp; fp = fp->prev()) {
                        if (fp->isFunctionFrame() &&
                            fp->callee() == fun->compiledFunObj() &&
                            fp->thisValue().isObject())
                        {
                            JSObject *tmp = &fp->thisValue().toObject();
                            do {
                                if (tmp == obj) {
                                    fp->calleev().setObject(*funobj);
                                    break;
                                }
                            } while ((tmp = tmp->getProto()) != NULL);
                        }
                    }
                }
            }
        }
    }

    return obj->removeProperty(cx, id) && js_SuppressDeletedProperty(cx, obj, id);
}

namespace js {

JSObject *
HasNativeMethod(JSObject *obj, jsid methodid, Native native)
{
    const Shape *shape = obj->nativeLookup(methodid);
    if (!shape || !shape->hasDefaultGetter() || !obj->containsSlot(shape->slot))
        return NULL;

    const Value &fval = obj->nativeGetSlot(shape->slot);
    JSObject *funobj;
    if (!IsFunctionObject(fval, &funobj) || funobj->getFunctionPrivate()->maybeNative() != native)
        return NULL;

    return funobj;
}

bool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint != JSTYPE_OBJECT && hint != JSTYPE_FUNCTION);

    Value v = ObjectValue(*obj);
    if (hint == JSTYPE_STRING) {
        
        if (obj->getClass() == &js_StringClass &&
            ClassMethodIsNative(cx, obj,
                                 &js_StringClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.toStringAtom),
                                 js_str_toString)) {
            *vp = obj->getPrimitiveThis();
            return true;
        }

        Value fval;
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);
        if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &fval))
            return false;
        if (js_IsCallable(fval)) {
            if (!ExternalInvoke(cx, ObjectValue(*obj), fval, 0, NULL, &v))
                return false;
            if (v.isPrimitive()) {
                *vp = v;
                return true;
            }
        }

        if (!obj->getClass()->convert(cx, obj, hint, &v))
            return false;
    } else {
        
        Class *clasp = obj->getClass();
        if ((clasp == &js_StringClass &&
             ClassMethodIsNative(cx, obj, &js_StringClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom),
                                 js_str_toString)) ||
            (clasp == &js_NumberClass &&
             ClassMethodIsNative(cx, obj, &js_NumberClass,
                                 ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom),
                                 js_num_valueOf))) {
            *vp = obj->getPrimitiveThis();
            return true;
        }

        if (!obj->getClass()->convert(cx, obj, hint, &v))
            return false;
        if (v.isObject()) {
            JS_ASSERT(hint != TypeOfValue(cx, v));
            Value fval;
            jsid id = ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);
            if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &fval))
                return false;
            if (js_IsCallable(fval)) {
                if (!ExternalInvoke(cx, ObjectValue(*obj), fval, 0, NULL, &v))
                    return false;
                if (v.isPrimitive()) {
                    *vp = v;
                    return true;
                }
            }
        }
    }
    if (v.isObject()) {
        
        JSString *str;
        if (hint == JSTYPE_STRING) {
            str = JS_InternString(cx, obj->getClass()->name);
            if (!str)
                return false;
        } else {
            str = NULL;
        }
        vp->setObject(*obj);
        js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO,
                             JSDVG_SEARCH_STACK, *vp, str,
                             (hint == JSTYPE_VOID)
                             ? "primitive type"
                             : JS_TYPE_STR(hint));
        return false;
    }
    *vp = v;
    return true;
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
                *vp = pobj->nativeGetSlot(shape->slot);
            else
                vp->setUndefined();
        }
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
            if (cx->running())
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

JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto, uintN attrs)
{
    





    if (!ctor->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                              ObjectOrNullValue(proto), PropertyStub, StrictPropertyStub, attrs)) {
        return JS_FALSE;
    }

    



    return proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.constructorAtom),
                                 ObjectOrNullValue(ctor), PropertyStub, StrictPropertyStub, 0);
}

JSObject *
PrimitiveToObject(JSContext *cx, const Value &v)
{
    if (v.isString())
        return StringObject::create(cx, v.toString());

    JS_ASSERT(v.isNumber() || v.isBoolean());
    Class *clasp = v.isNumber() ? &js_NumberClass : &js_BooleanClass;
    JSObject *obj = NewBuiltinClassInstance(cx, clasp);
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

JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, Value *rval)
{
    Value fval;
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.valueOfAtom);
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &fval))
        return false;
    if (js_IsCallable(fval)) {
        Value v;
        Value argv[] = { StringValue(cx->runtime->atomState.typeAtoms[type]) };
        if (!ExternalInvoke(cx, ObjectValue(*obj), fval, JS_ARRAY_LENGTH(argv), argv, &v))
            return false;
        if (v.isPrimitive()) {
            *rval = v;
            return true;
        }
    }
    return true;
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

#ifdef JS_DUMP_SCOPE_METERS

#include <stdio.h>

JSBasicStats js_entry_count_bs = JS_INIT_STATIC_BASIC_STATS;

namespace js {

void
MeterEntryCount(uintN count)
{
    JS_BASIC_STATS_ACCUM(&js_entry_count_bs, count);
}

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

    const Shape *shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape->previous() && shape->slot != slot)
            shape = shape->previous();
        if (shape->slot != slot)
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
        jsid propid = shape->propid;
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
        if (!obj->removeProperty(cx, shape->propid))
            return false;
    }

    
    for (Shape::Range r(obj->lastProperty()->all()); !r.empty(); r.popFront()) {
        const Shape *shape = &r.front();
        if (shape->isDataDescriptor() &&
            shape->writable() &&
            shape->hasDefaultSetter() &&
            obj->containsSlot(shape->slot)) {
            obj->setSlot(shape->slot, UndefinedValue());
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

    if (slot < obj->numSlots())
        *vp = obj->getSlot(slot);
    else
        vp->setUndefined();
    return true;
}

bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 slot, const Value &v)
{
    if (!obj->isNative())
        return true;

    Class *clasp = obj->getClass();

    if (slot >= obj->numSlots()) {
        uint32 nslots = JSSLOT_FREE(clasp);
        JS_ASSERT(slot < nslots);
        if (!obj->allocSlots(cx, nslots))
            return false;
    }

    obj->setSlot(slot, v);
    GC_POKE(cx, JS_NULL);
    return true;
}

GlobalObject *
JSObject::getGlobal() const
{
    JSObject *obj = const_cast<JSObject *>(this);
    while (JSObject *parent = obj->getParent())
        obj = parent;
    return obj->asGlobal();
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

JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
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
        JSObject *funobj = &v.toObject();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
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
        fprintf(stderr, " at %p (JSFunction at %p)>", (void *) funobj, (void *) fun);
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
DumpProperty(JSObject *obj, const Shape &shape)
{
    jsid id = shape.propid;
    uint8 attrs = shape.attributes();

    fprintf(stderr, "    ((Shape *) %p) ", (void *) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");
    if (shape.isAlias()) fprintf(stderr, "alias ");
    if (shape.isMethod()) fprintf(stderr, "method=%p ", (void *) &shape.methodObject());

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void *) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void *) shape.setterObject());
    else if (shape.setterOp() == js_watch_set)
        fprintf(stderr, "setterOp=js_watch_set ");
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.setterOp()));

    if (JSID_IS_ATOM(id))
        dumpString(JSID_TO_STRING(id));
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));
    fprintf(stderr, ": slot %d", shape.slot);
    if (obj->containsSlot(shape.slot)) {
        fprintf(stderr, " = ");
        dumpValue(obj->getSlot(shape.slot));
    } else if (shape.slot != SHAPE_INVALID_SLOT) {
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
    uint32 flags = obj->flags;
    if (flags & JSObject::DELEGATE) fprintf(stderr, " delegate");
    if (flags & JSObject::SYSTEM) fprintf(stderr, " system");
    if (flags & JSObject::NOT_EXTENSIBLE) fprintf(stderr, " not_extensible");
    if (flags & JSObject::BRANDED) fprintf(stderr, " branded");
    if (flags & JSObject::GENERIC) fprintf(stderr, " generic");
    if (flags & JSObject::METHOD_BARRIER) fprintf(stderr, " method_barrier");
    if (flags & JSObject::INDEXED) fprintf(stderr, " indexed");
    if (flags & JSObject::OWN_SHAPE) fprintf(stderr, " own_shape");
    if (flags & JSObject::HAS_EQUALITY) fprintf(stderr, " has_equality");

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
        unsigned slots = JS_MIN(obj->getArrayLength(), obj->getDenseArrayCapacity());
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
    dumpValue(ObjectOrNullValue(obj->getParent()));
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


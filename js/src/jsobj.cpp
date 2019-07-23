










































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
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jstracer.h"
#include "jsdbgapi.h"

#if JS_HAS_GENERATORS
#include "jsiter.h"
#endif

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JS_HAS_XDR
#include "jsxdrapi.h"
#endif

#ifdef INCLUDE_MOZILLA_DTRACE
#include "jsdtracef.h"
#endif

#include "jsatominlines.h"

#include "jsautooplen.h"

#ifdef JS_THREADSAFE
#define NATIVE_DROP_PROPERTY js_DropProperty

extern void
js_DropProperty(JSContext *cx, JSObject *obj, JSProperty *prop);
#else
#define NATIVE_DROP_PROPERTY NULL
#endif

JS_FRIEND_DATA(JSObjectOps) js_ObjectOps = {
    NULL,
    js_LookupProperty,      js_DefineProperty,
    js_GetProperty,         js_SetProperty,
    js_GetAttributes,       js_SetAttributes,
    js_DeleteProperty,      js_DefaultValue,
    js_Enumerate,           js_CheckAccess,
    NULL,                   NATIVE_DROP_PROPERTY,
    js_Call,                js_Construct,
    js_HasInstance,         js_TraceObject,
    js_Clear,               js_GetRequiredSlot,
    js_SetRequiredSlot
};

JSClass js_ObjectClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   NULL,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#if JS_HAS_OBJ_PROTO_PROP

static JSBool
obj_getSlot(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
obj_setSlot(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSBool
obj_getCount(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSPropertySpec object_props[] = {
    
    {js_proto_str, JSSLOT_PROTO, JSPROP_PERMANENT|JSPROP_SHARED,
                                                  obj_getSlot,  obj_setSlot},
    {js_parent_str,JSSLOT_PARENT,JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_SHARED,
                                                  obj_getSlot,  obj_setSlot},
    {js_count_str, 0,            JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_SHARED,
                                                  obj_getCount, NULL},
    {0,0,0,0,0}
};


#define JSSLOT_COUNT 2

static JSBool
ReportStrictSlot(JSContext *cx, uint32 slot)
{
    if (slot == JSSLOT_PROTO)
        return JS_TRUE;
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_DEPRECATED_USAGE,
                                        object_props[slot].name);
}

static JSBool
obj_getSlot(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    uint32 slot;
    jsid propid;
    JSAccessMode mode;
    uintN attrs;
    JSObject *pobj;
    JSClass *clasp;

    slot = (uint32) JSVAL_TO_INT(id);
    if (id == INT_TO_JSVAL(JSSLOT_PROTO)) {
        propid = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
        mode = JSACC_PROTO;
    } else {
        propid = ATOM_TO_JSID(cx->runtime->atomState.parentAtom);
        mode = JSACC_PARENT;
    }

    
    if (!OBJ_CHECK_ACCESS(cx, obj, propid, mode, vp, &attrs))
        return JS_FALSE;

    pobj = JSVAL_TO_OBJECT(*vp);
    if (pobj) {
        clasp = OBJ_GET_CLASS(cx, pobj);
        if (clasp == &js_CallClass || clasp == &js_BlockClass) {
            
            *vp = JSVAL_NULL;
        } else {
            



            JS_ASSERT(clasp != &js_DeclEnvClass);
            if (pobj->map->ops->thisObject) {
                pobj = pobj->map->ops->thisObject(cx, pobj);
                if (!pobj)
                    return JS_FALSE;
                *vp = OBJECT_TO_JSVAL(pobj);
            }
        }
    }
    return JS_TRUE;
}

static JSBool
obj_setSlot(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSObject *pobj;
    uint32 slot;
    jsid propid;
    uintN attrs;

    if (!JSVAL_IS_OBJECT(*vp))
        return JS_TRUE;
    pobj = JSVAL_TO_OBJECT(*vp);

    if (pobj) {
        




        OBJ_TO_INNER_OBJECT(cx, pobj);
        if (!pobj)
            return JS_FALSE;
    }
    slot = (uint32) JSVAL_TO_INT(id);
    if (JS_HAS_STRICT_OPTION(cx) && !ReportStrictSlot(cx, slot))
        return JS_FALSE;

    
    propid = ATOM_TO_JSID(cx->runtime->atomState.protoAtom);
    if (!OBJ_CHECK_ACCESS(cx, obj, propid,
                          (JSAccessMode)(JSACC_PROTO|JSACC_WRITE), vp,
                          &attrs)) {
        return JS_FALSE;
    }

    return js_SetProtoOrParent(cx, obj, slot, pobj, JS_TRUE);
}

static JSBool
obj_getCount(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsval iter_state;
    jsid num_properties;
    JSBool ok;

    if (JS_HAS_STRICT_OPTION(cx) && !ReportStrictSlot(cx, JSSLOT_COUNT))
        return JS_FALSE;

    
    iter_state = JSVAL_NULL;
    ok = OBJ_ENUMERATE(cx, obj, JSENUMERATE_INIT, &iter_state, &num_properties);
    if (!ok)
        goto out;

    if (!JSVAL_IS_INT(num_properties)) {
        JS_ASSERT(0);
        *vp = JSVAL_ZERO;
        goto out;
    }
    *vp = num_properties;

out:
    if (iter_state != JSVAL_NULL)
        ok = OBJ_ENUMERATE(cx, obj, JSENUMERATE_DESTROY, &iter_state, 0);
    return ok;
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
        if (OBJ_IS_NATIVE(obj)) {
            JS_LOCK_OBJ(cx, obj);
            bool ok = !!js_GetMutableScope(cx, obj);
            JS_UNLOCK_OBJ(cx, obj);
            if (!ok)
                return JS_FALSE;
        }

        




        JSObject *oldproto = obj;
        while (oldproto && OBJ_IS_NATIVE(oldproto)) {
            JS_LOCK_OBJ(cx, oldproto);
            JSScope *scope = OBJ_SCOPE(oldproto);
            scope->protoShapeChange(cx);
            JSObject *tmp = STOBJ_GET_PROTO(oldproto);
            JS_UNLOCK_OBJ(cx, oldproto);
            oldproto = tmp;
        }
    }

    if (!pobj || !checkForCycles) {
        if (slot == JSSLOT_PROTO)
            STOBJ_SET_PROTO(obj, pobj);
        else
            STOBJ_SET_PARENT(obj, pobj);
    } else {
        



        JSSetSlotRequest ssr;
        ssr.obj = obj;
        ssr.pobj = pobj;
        ssr.slot = (uint16) slot;
        ssr.cycle = false;

        JSRuntime *rt = cx->runtime;
        JS_LOCK_GC(rt);
        ssr.next = rt->setSlotRequests;
        rt->setSlotRequests = &ssr;
        for (;;) {
            js_GC(cx, GC_SET_SLOT_REQUEST);
            JS_UNLOCK_GC(rt);
            if (!rt->setSlotRequests)
                break;
            JS_LOCK_GC(rt);
        }

        if (ssr.cycle) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CYCLIC_VALUE,
#if JS_HAS_OBJ_PROTO_PROP
                                 object_props[slot].name
#else
                                 (slot == JSSLOT_PROTO) ? js_proto_str
                                                        : js_parent_str
#endif
                                 );
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSHashNumber
js_hash_object(const void *key)
{
    return (JSHashNumber)JS_PTR_TO_UINT32(key) >> JSVAL_TAGBITS;
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
#if JS_HAS_GETTER_SETTER
    JSObject *obj2;
    JSProperty *prop;
    uintN attrs;
#endif
    jsval val;

    JS_CHECK_RECURSION(cx, return NULL);

    map = &cx->sharpObjectMap;
    JS_ASSERT(map->depth >= 1);
    table = map->table;
    hash = js_hash_object(obj);
    hep = JS_HashTableRawLookup(table, hash, obj);
    he = *hep;
    if (!he) {
        sharpid = 0;
        he = JS_HashTableRawAdd(table, hep, hash, obj,
                                JS_UINT32_TO_PTR(sharpid));
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
#if JS_HAS_GETTER_SETTER
            ok = OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop);
            if (!ok)
                break;
            if (!prop)
                continue;
            ok = OBJ_GET_ATTRIBUTES(cx, obj2, id, prop, &attrs);
            if (ok) {
                if (OBJ_IS_NATIVE(obj2) &&
                    (attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
                    JSScopeProperty *sprop = (JSScopeProperty *) prop;
                    val = JSVAL_NULL;
                    if (attrs & JSPROP_GETTER)
                        val = js_CastAsObjectJSVal(sprop->getter);
                    if (attrs & JSPROP_SETTER) {
                        if (val != JSVAL_NULL) {
                            
                            ok = (MarkSharpObjects(cx, JSVAL_TO_OBJECT(val),
                                                   NULL)
                                  != NULL);
                        }
                        val = js_CastAsObjectJSVal(sprop->setter);
                    }
                } else {
                    ok = OBJ_GET_PROPERTY(cx, obj, id, &val);
                }
            }
            OBJ_DROP_PROPERTY(cx, obj2, prop);
#else
            ok = OBJ_GET_PROPERTY(cx, obj, id, &val);
#endif
            if (!ok)
                break;
            if (!JSVAL_IS_PRIMITIVE(val) &&
                !MarkSharpObjects(cx, JSVAL_TO_OBJECT(val), NULL)) {
                ok = JS_FALSE;
                break;
            }
        }
        if (!ok || !idap)
            JS_DestroyIdArray(cx, ida);
        if (!ok)
            return NULL;
    } else {
        sharpid = JS_PTR_TO_UINT32(he->value);
        if (sharpid == 0) {
            sharpid = ++map->sharpgen << SHARP_ID_SHIFT;
            he->value = JS_UINT32_TO_PTR(sharpid);
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
        JS_ASSERT((JS_PTR_TO_UINT32(he->value) & SHARP_BIT) == 0);
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

    sharpid = JS_PTR_TO_UINT32(he->value);
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
obj_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok, outermost;
    JSObject *obj;
    JSHashEntry *he;
    JSIdArray *ida;
    jschar *chars, *ochars, *vsharp;
    const jschar *idstrchars, *vchars;
    size_t nchars, idstrlength, gsoplength, vlength, vsharplength, curlen;
    const char *comma;
    jsint i, j, length, valcnt;
    jsid id;
#if JS_HAS_GETTER_SETTER
    JSObject *obj2;
    JSProperty *prop;
    uintN attrs;
#endif
    jsval *val;
    jsval localroot[4] = {JSVAL_NULL, JSVAL_NULL, JSVAL_NULL, JSVAL_NULL};
    JSTempValueRooter tvr;
    JSString *gsopold[2];
    JSString *gsop[2];
    JSString *idstr, *valstr, *str;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    MUST_FLOW_THROUGH("out");
    JS_PUSH_TEMP_ROOT(cx, 4, localroot, &tvr);

    
    outermost = (cx->sharpObjectMap.depth == 0);
    obj = JS_THIS_OBJECT(cx, vp);
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

    for (i = 0, length = ida->length; i < length; i++) {
        JSBool idIsLexicalIdentifier, needOldStyleGetterSetter;

        
        id = ida->vector[i];

#if JS_HAS_GETTER_SETTER
        ok = OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop);
        if (!ok)
            goto error;
#endif

        



        idstr = js_ValueToString(cx, ID_TO_VALUE(id));
        if (!idstr) {
            ok = JS_FALSE;
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            goto error;
        }
        *vp = STRING_TO_JSVAL(idstr);                   
        idIsLexicalIdentifier = js_IsIdentifier(idstr);
        needOldStyleGetterSetter =
            !idIsLexicalIdentifier ||
            js_CheckKeyword(idstr->chars(), idstr->length()) != TOK_EOF;

#if JS_HAS_GETTER_SETTER

        valcnt = 0;
        if (prop) {
            ok = OBJ_GET_ATTRIBUTES(cx, obj2, id, prop, &attrs);
            if (!ok) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                goto error;
            }
            if (OBJ_IS_NATIVE(obj2) &&
                (attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
                JSScopeProperty *sprop = (JSScopeProperty *) prop;
                if (attrs & JSPROP_GETTER) {
                    val[valcnt] = js_CastAsObjectJSVal(sprop->getter);
                    gsopold[valcnt] =
                        ATOM_TO_STRING(cx->runtime->atomState.getterAtom);
                    gsop[valcnt] =
                        ATOM_TO_STRING(cx->runtime->atomState.getAtom);

                    valcnt++;
                }
                if (attrs & JSPROP_SETTER) {
                    val[valcnt] = js_CastAsObjectJSVal(sprop->setter);
                    gsopold[valcnt] =
                        ATOM_TO_STRING(cx->runtime->atomState.setterAtom);
                    gsop[valcnt] =
                        ATOM_TO_STRING(cx->runtime->atomState.setAtom);

                    valcnt++;
                }
            } else {
                valcnt = 1;
                gsop[0] = NULL;
                gsopold[0] = NULL;
                ok = OBJ_GET_PROPERTY(cx, obj, id, &val[0]);
            }
            OBJ_DROP_PROPERTY(cx, obj2, prop);
        }

#else  

        





        valcnt = 1;
        gsop[0] = NULL;
        gsopold[0] = NULL;
        ok = OBJ_GET_PROPERTY(cx, obj, id, &val[0]);

#endif 

        if (!ok)
            goto error;

        



        if (JSID_IS_ATOM(id)
            ? !idIsLexicalIdentifier
            : (!JSID_IS_INT(id) || JSID_TO_INT(id) < 0)) {
            idstr = js_QuoteString(cx, idstr, (jschar)'\'');
            if (!idstr) {
                ok = JS_FALSE;
                goto error;
            }
            *vp = STRING_TO_JSVAL(idstr);               
        }
        idstr->getCharsAndLength(idstrchars, idstrlength);

        for (j = 0; j < valcnt; j++) {
            
            valstr = js_ValueToSource(cx, val[j]);
            if (!valstr) {
                ok = JS_FALSE;
                goto error;
            }
            localroot[j] = STRING_TO_JSVAL(valstr);     
            valstr->getCharsAndLength(vchars, vlength);

            if (vchars[0] == '#')
                needOldStyleGetterSetter = JS_TRUE;

            if (needOldStyleGetterSetter)
                gsop[j] = gsopold[j];

            
            vsharp = NULL;
            vsharplength = 0;
#if JS_HAS_SHARP_VARS
            if (!JSVAL_IS_PRIMITIVE(val[j]) && vchars[0] != '#') {
                he = js_EnterSharpObject(cx, JSVAL_TO_OBJECT(val[j]), NULL,
                                         &vsharp);
                if (!he) {
                    ok = JS_FALSE;
                    goto error;
                }
                if (IS_SHARP(he)) {
                    vchars = vsharp;
                    vlength = js_strlen(vchars);
                    needOldStyleGetterSetter = JS_TRUE;
                    gsop[j] = gsopold[j];
                } else {
                    if (vsharp) {
                        vsharplength = js_strlen(vsharp);
                        MAKE_SHARP(he);
                        needOldStyleGetterSetter = JS_TRUE;
                        gsop[j] = gsopold[j];
                    }
                    js_LeaveSharpObject(cx, NULL);
                }
            }
#endif

#ifndef OLD_GETTER_SETTER
            



            if (gsop[j] && VALUE_IS_FUNCTION(cx, val[j]) &&
                !needOldStyleGetterSetter) {
                JSFunction *fun = JS_ValueToFunction(cx, val[j]);
                const jschar *start = vchars;
                const jschar *end = vchars + vlength;

                uint8 parenChomp = 0;
                if (vchars[0] == '(') {
                    vchars++;
                    parenChomp = 1;
                }

                




                if (JSFUN_GETTER_TEST(fun->flags) ||
                    JSFUN_SETTER_TEST(fun->flags)) { 
                    const jschar *tmp = js_strchr_limit(vchars, ' ', end);
                    if (tmp)
                        vchars = tmp + 1;
                }

                
                if (vchars)
                    vchars = js_strchr_limit(vchars, ' ', end);

                if (vchars) {
                    if (*vchars == ' ')
                        vchars++;
                    vlength = end - vchars - parenChomp;
                } else {
                    gsop[j] = NULL;
                    vchars = start;
                }
            }
#else
            needOldStyleGetterSetter = JS_TRUE;
            gsop[j] = gsopold[j];
#endif

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

            if (curlen > (size_t)-1 / sizeof(jschar))
                goto overflow;

            
            chars = (jschar *)
                js_realloc((ochars = chars), curlen * sizeof(jschar));
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

            if (needOldStyleGetterSetter) {
                js_strncpy(&chars[nchars], idstrchars, idstrlength);
                nchars += idstrlength;
                if (gsop[j]) {
                    chars[nchars++] = ' ';
                    gsoplength = gsop[j]->length();
                    js_strncpy(&chars[nchars], gsop[j]->chars(),
                               gsoplength);
                    nchars += gsoplength;
                }
                chars[nchars++] = ':';
            } else {  
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
            }

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
    *vp = STRING_TO_JSVAL(str);
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;

  overflow:
    cx->free(vsharp);
    js_free(chars);
    chars = NULL;
    goto error;
}
#endif 

static JSBool
obj_toString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jschar *chars;
    size_t nchars;
    const char *clazz, *prefix;
    JSString *str;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    obj = js_GetWrappedObject(cx, obj);
    clazz = OBJ_GET_CLASS(cx, obj)->name;
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
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
obj_toLocaleString(JSContext *cx, uintN argc, jsval *vp)
{
    jsval thisv;
    JSString *str;

    thisv = JS_THIS(cx, vp);
    if (JSVAL_IS_NULL(thisv))
        return JS_FALSE;

    str = js_ValueToString(cx, thisv);
    if (!str)
        return JS_FALSE;

    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
obj_valueOf(JSContext *cx, uintN argc, jsval *vp)
{
    *vp = JS_THIS(cx, vp);
    return !JSVAL_IS_NULL(*vp);
}

#ifdef JS_TRACER
static jsval FASTCALL
Object_p_valueOf(JSContext* cx, JSObject* obj, JSString *hint)
{
    return OBJECT_TO_JSVAL(obj);
}
#endif






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
    JSClass *clasp;
    JSExtendedClass *xclasp;
    JSObject *inner;

    if (!scopeobj)
        goto bad;

    OBJ_TO_INNER_OBJECT(cx, scopeobj);
    if (!scopeobj)
        return NULL;

    inner = scopeobj;

    
    while (scopeobj) {
        clasp = OBJ_GET_CLASS(cx, scopeobj);
        if (clasp->flags & JSCLASS_IS_EXTENDED) {
            xclasp = (JSExtendedClass*)clasp;
            if (xclasp->innerObject &&
                xclasp->innerObject(cx, scopeobj) != scopeobj) {
                goto bad;
            }
        }

        scopeobj = OBJ_GET_PARENT(cx, scopeobj);
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

    if (caller->regs && js_GetOpcode(cx, caller->script, caller->regs->pc) == JSOP_EVAL) {
        JS_ASSERT(js_GetOpcode(cx, caller->script, caller->regs->pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        *linenop = GET_UINT16(caller->regs->pc + JSOP_EVAL_LENGTH);
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
obj_eval(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSStackFrame *fp, *caller;
    JSBool indirectCall;
    uint32 tcflags;
    JSPrincipals *principals;
    const char *file;
    uintN line;
    JSString *str;
    JSScript *script;
    JSBool ok;
    JSScript **bucket = NULL;   
#if JS_HAS_EVAL_THIS_SCOPE
    JSObject *callerScopeChain = NULL, *callerVarObj = NULL;
    JSObject *setCallerScopeChain = NULL;
    JSBool setCallerVarObj = JS_FALSE;
#endif

    fp = js_GetTopStackFrame(cx);
    caller = js_GetScriptedCaller(cx, fp);
    indirectCall = (caller && caller->regs && *caller->regs->pc != JSOP_EVAL);

    








    obj = js_GetWrappedObject(cx, obj);

    




    {
        JSObject *parent = OBJ_GET_PARENT(cx, obj);
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

    if (!JSVAL_IS_STRING(argv[0])) {
        *rval = argv[0];
        return JS_TRUE;
    }

    




    if (caller && !caller->varobj && !js_GetCallObject(cx, caller))
        return JS_FALSE;

    
    JSObject *scopeobj = NULL;
    if (argc >= 2) {
        if (!js_ValueToObject(cx, argv[1], &scopeobj))
            return JS_FALSE;
        argv[1] = OBJECT_TO_JSVAL(scopeobj);
    }

    
    MUST_FLOW_THROUGH("out");
    if (!scopeobj) {
#if JS_HAS_EVAL_THIS_SCOPE
        
        if (indirectCall) {
            callerScopeChain = js_GetScopeChain(cx, caller);
            if (!callerScopeChain) {
                ok = JS_FALSE;
                goto out;
            }
            OBJ_TO_INNER_OBJECT(cx, obj);
            if (!obj) {
                ok = JS_FALSE;
                goto out;
            }
            if (obj != callerScopeChain) {
                ok = js_CheckPrincipalsAccess(cx, obj,
                                              JS_StackFramePrincipals(cx, caller),
                                              cx->runtime->atomState.evalAtom);
                if (!ok)
                    goto out;

                scopeobj = js_NewWithObject(cx, obj, callerScopeChain, -1);
                if (!scopeobj) {
                    ok = JS_FALSE;
                    goto out;
                }

                
                caller->scopeChain = fp->scopeChain = scopeobj;

                
                setCallerScopeChain = scopeobj;
            }

            callerVarObj = caller->varobj;
            if (obj != callerVarObj) {
                
                caller->varobj = fp->varobj = obj;
                setCallerVarObj = JS_TRUE;
            }
        }
#endif

        





        if (caller) {
            scopeobj = js_GetScopeChain(cx, caller);
            if (!scopeobj) {
                ok = JS_FALSE;
                goto out;
            }
        }
    } else {
        scopeobj = js_GetWrappedObject(cx, scopeobj);
        OBJ_TO_INNER_OBJECT(cx, scopeobj);
        if (!scopeobj) {
            ok = JS_FALSE;
            goto out;
        }
        ok = js_CheckPrincipalsAccess(cx, scopeobj,
                                      JS_StackFramePrincipals(cx, caller),
                                      cx->runtime->atomState.evalAtom);
        if (!ok)
            goto out;

        scopeobj = js_NewWithObject(cx, scopeobj,
                                    JS_GetGlobalForObject(cx, scopeobj), -1);
        if (!scopeobj) {
            ok = JS_FALSE;
            goto out;
        }
        argv[1] = OBJECT_TO_JSVAL(scopeobj);
    }

    
    scopeobj = js_CheckScopeChainValidity(cx, scopeobj, js_eval_str);
    if (!scopeobj) {
        ok = JS_FALSE;
        goto out;
    }

    tcflags = TCF_COMPILE_N_GO;
    if (caller) {
        tcflags |= TCF_PUT_STATIC_LEVEL(caller->script->staticLevel + 1);
        principals = JS_EvalFramePrincipals(cx, fp, caller);
        file = js_ComputeFilename(cx, caller, principals, &line);
    } else {
        principals = NULL;
        file = NULL;
        line = 0;
    }

    str = JSVAL_TO_STRING(argv[0]);
    script = NULL;

    
    bucket = EvalCacheHash(cx, str);
    if (caller->fun) {
        uintN count = 0;
        JSScript **scriptp = bucket;

        EVAL_CACHE_METER(probe);
        while ((script = *scriptp) != NULL) {
            if ((script->flags & JSSF_SAVED_CALLER_FUN) &&
                script->version == cx->version &&
                (script->principals == principals ||
                 (principals->subsume(principals, script->principals) &&
                  script->principals->subsume(script->principals, principals)))) {
                



                JSFunction *fun;
                JS_GET_SCRIPT_FUNCTION(script, 0, fun);

                if (fun == caller->fun) {
                    



                    JSString *src = ATOM_TO_STRING(script->atomMap.vector[0]);

                    if (src == str || js_EqualStrings(src, str)) {
                        






                        JSObjectArray *objarray = JS_SCRIPT_OBJECTS(script);
                        int i = 1;
                        if (objarray->length == 1) {
                            if (script->regexpsOffset != 0) {
                                objarray = JS_SCRIPT_REGEXPS(script);
                                i = 0;
                            } else {
                                EVAL_CACHE_METER(noscope);
                                i = -1;
                            }
                        }
                        if (i < 0 ||
                            STOBJ_GET_PARENT(objarray->vector[i]) == scopeobj) {
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

    if (!script) {
        script = JSCompiler::compileScript(cx, scopeobj, caller, principals, tcflags,
                                           str->chars(), str->length(),
                                           NULL, file, line, str);
        if (!script) {
            ok = JS_FALSE;
            goto out;
        }
    }

    if (argc < 2) {
        
        if (caller)
            scopeobj = caller->scopeChain;
    }

    



    ok = js_CheckPrincipalsAccess(cx, scopeobj, principals,
                                  cx->runtime->atomState.evalAtom);
    if (ok)
        ok = js_Execute(cx, scopeobj, script, caller, JSFRAME_EVAL, rval);

    script->u.nextToGC = *bucket;
    *bucket = script;
#ifdef CHECK_SCRIPT_OWNER
    script->owner = NULL;
#endif

out:
#if JS_HAS_EVAL_THIS_SCOPE
    
    if (setCallerScopeChain) {
        caller->scopeChain = callerScopeChain;
        JS_ASSERT(OBJ_GET_CLASS(cx, setCallerScopeChain) == &js_WithClass);
        setCallerScopeChain->setPrivate(NULL);
    }
    if (setCallerVarObj)
        caller->varobj = callerVarObj;
#endif
    return ok;
}

#if JS_HAS_OBJ_WATCHPOINT

static JSBool
obj_watch_handler(JSContext *cx, JSObject *obj, jsval id, jsval old, jsval *nvp,
                  void *closure)
{
    JSObject *callable;
    JSSecurityCallbacks *callbacks;
    JSStackFrame *caller;
    JSPrincipals *subject, *watcher;
    JSResolvingKey key;
    JSResolvingEntry *entry;
    uint32 generation;
    jsval argv[3];
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

    argv[0] = id;
    argv[1] = old;
    argv[2] = *nvp;
    ok = js_InternalCall(cx, obj, OBJECT_TO_JSVAL(callable), 3, argv, nvp);
    js_StopResolving(cx, &key, JSRESFLAG_WATCH, entry, generation);
    return ok;
}

static JSBool
obj_watch(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *callable;
    jsval userid, value;
    jsid propid;
    JSObject *obj;
    uintN attrs;

    if (argc <= 1) {
        js_ReportMissingArg(cx, vp, 1);
        return JS_FALSE;
    }

    callable = js_ValueToCallableObject(cx, &vp[3], 0);
    if (!callable)
        return JS_FALSE;

    
    userid = vp[2];
    if (!JS_ValueToId(cx, userid, &propid))
        return JS_FALSE;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !OBJ_CHECK_ACCESS(cx, obj, propid, JSACC_WATCH, &value, &attrs))
        return JS_FALSE;
    if (attrs & JSPROP_READONLY)
        return JS_TRUE;
    *vp = JSVAL_VOID;

    if (OBJ_IS_DENSE_ARRAY(cx, obj) && !js_MakeArraySlow(cx, obj))
        return JS_FALSE;
    return JS_SetWatchPoint(cx, obj, userid, obj_watch_handler, callable);
}

static JSBool
obj_unwatch(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    *vp = JSVAL_VOID;
    return JS_ClearWatchPoint(cx, obj, argc != 0 ? vp[2] : JSVAL_VOID,
                              NULL, NULL);
}

#endif 







static JSBool
obj_hasOwnProperty(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    return obj &&
           js_HasOwnPropertyHelper(cx, obj->map->ops->lookupProperty, argc, vp);
}

JSBool
js_HasOwnPropertyHelper(JSContext *cx, JSLookupPropOp lookup, uintN argc,
                        jsval *vp)
{
    jsid id;
    JSObject *obj;

    if (!JS_ValueToId(cx, argc != 0 ? vp[2] : JSVAL_VOID, &id))
        return JS_FALSE;
    obj = JS_THIS_OBJECT(cx, vp);
    return obj && js_HasOwnProperty(cx, lookup, obj, id, vp);
}

JSBool
js_HasOwnProperty(JSContext *cx, JSLookupPropOp lookup, JSObject *obj, jsid id,
                  jsval *vp)
{
    JSObject *obj2;
    JSProperty *prop;
    JSScopeProperty *sprop;

    if (!lookup(cx, obj, id, &obj2, &prop))
        return JS_FALSE;
    if (!prop) {
        *vp = JSVAL_FALSE;
    } else if (obj2 == obj) {
        *vp = JSVAL_TRUE;
    } else {
        JSClass *clasp;
        JSExtendedClass *xclasp;
        JSObject *outer;

        clasp = OBJ_GET_CLASS(cx, obj2);
        if (!(clasp->flags & JSCLASS_IS_EXTENDED) ||
            !(xclasp = (JSExtendedClass *) clasp)->outerObject) {
            outer = NULL;
        } else {
            outer = xclasp->outerObject(cx, obj2);
            if (!outer)
                return JS_FALSE;
        }
        if (outer == obj) {
            *vp = JSVAL_TRUE;
        } else if (OBJ_IS_NATIVE(obj2) && OBJ_GET_CLASS(cx, obj) == clasp) {
            














            sprop = (JSScopeProperty *)prop;
            *vp = BOOLEAN_TO_JSVAL(SPROP_IS_SHARED_PERMANENT(sprop));
        } else {
            *vp = JSVAL_FALSE;
        }
    }
    if (prop)
        OBJ_DROP_PROPERTY(cx, obj2, prop);
    return JS_TRUE;
}

#ifdef JS_TRACER
static JSBool FASTCALL
Object_p_hasOwnProperty(JSContext* cx, JSObject* obj, JSString *str)
{
    jsid id;
    jsval v;

    if (!js_ValueToStringId(cx, STRING_TO_JSVAL(str), &id) ||
        !js_HasOwnProperty(cx, obj->map->ops->lookupProperty, obj, id, &v)) {
        js_SetBuiltinError(cx);
        return JSVAL_TO_BOOLEAN(JSVAL_VOID);
    }

    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    return JSVAL_TO_BOOLEAN(v);
}
#endif


static JSBool
obj_isPrototypeOf(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool b;

    if (!js_IsDelegate(cx, JS_THIS_OBJECT(cx, vp),
                       argc != 0 ? vp[2] : JSVAL_VOID, &b)) {
        return JS_FALSE;
    }
    *vp = BOOLEAN_TO_JSVAL(b);
    return JS_TRUE;
}


static JSBool
obj_propertyIsEnumerable(JSContext *cx, uintN argc, jsval *vp)
{
    jsid id;
    JSObject *obj;

    if (!JS_ValueToId(cx, argc != 0 ? vp[2] : JSVAL_VOID, &id))
        return JS_FALSE;

    obj = JS_THIS_OBJECT(cx, vp);
    return obj && js_PropertyIsEnumerable(cx, obj, id, vp);
}

#ifdef JS_TRACER
static JSBool FASTCALL
Object_p_propertyIsEnumerable(JSContext* cx, JSObject* obj, JSString *str)
{
    jsid id = ATOM_TO_JSID(STRING_TO_JSVAL(str));
    jsval v;

    if (!js_PropertyIsEnumerable(cx, obj, id, &v)) {
        js_SetBuiltinError(cx);
        return JSVAL_TO_BOOLEAN(JSVAL_VOID);
    }

    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    return JSVAL_TO_BOOLEAN(v);
}
#endif

JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSObject *pobj;
    uintN attrs;
    JSProperty *prop;
    JSBool ok;

    if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
        return JS_FALSE;

    if (!prop) {
        *vp = JSVAL_FALSE;
        return JS_TRUE;
    }

    










    if (pobj != obj &&
        !(OBJ_IS_NATIVE(pobj) &&
          SPROP_IS_SHARED_PERMANENT((JSScopeProperty *)prop))) {
        OBJ_DROP_PROPERTY(cx, pobj, prop);
        *vp = JSVAL_FALSE;
        return JS_TRUE;
    }

    ok = OBJ_GET_ATTRIBUTES(cx, pobj, id, prop, &attrs);
    OBJ_DROP_PROPERTY(cx, pobj, prop);
    if (ok)
        *vp = BOOLEAN_TO_JSVAL((attrs & JSPROP_ENUMERATE) != 0);
    return ok;
}

#if JS_HAS_GETTER_SETTER
JS_FRIEND_API(JSBool)
js_obj_defineGetter(JSContext *cx, uintN argc, jsval *vp)
{
    jsval fval, junk;
    jsid id;
    JSObject *obj;
    uintN attrs;

    if (argc <= 1 || JS_TypeOfValue(cx, vp[3]) != JSTYPE_FUNCTION) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_getter_str);
        return JS_FALSE;
    }
    fval = vp[3];

    if (!JS_ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_CheckRedeclaration(cx, obj, id, JSPROP_GETTER, NULL, NULL))
        return JS_FALSE;
    



    if (!OBJ_CHECK_ACCESS(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    *vp = JSVAL_VOID;
    return OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                               js_CastAsPropertyOp(JSVAL_TO_OBJECT(fval)),
                               JS_PropertyStub,
                               JSPROP_ENUMERATE | JSPROP_GETTER | JSPROP_SHARED,
                               NULL);
}

JS_FRIEND_API(JSBool)
js_obj_defineSetter(JSContext *cx, uintN argc, jsval *vp)
{
    jsval fval, junk;
    jsid id;
    JSObject *obj;
    uintN attrs;

    if (argc <= 1 || JS_TypeOfValue(cx, vp[3]) != JSTYPE_FUNCTION) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_GETTER_OR_SETTER,
                             js_setter_str);
        return JS_FALSE;
    }
    fval = vp[3];

    if (!JS_ValueToId(cx, vp[2], &id))
        return JS_FALSE;
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_CheckRedeclaration(cx, obj, id, JSPROP_SETTER, NULL, NULL))
        return JS_FALSE;
    



    if (!OBJ_CHECK_ACCESS(cx, obj, id, JSACC_WATCH, &junk, &attrs))
        return JS_FALSE;
    *vp = JSVAL_VOID;
    return OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                               JS_PropertyStub,
                               js_CastAsPropertyOp(JSVAL_TO_OBJECT(fval)),
                               JSPROP_ENUMERATE | JSPROP_SETTER | JSPROP_SHARED,
                               NULL);
}

static JSBool
obj_lookupGetter(JSContext *cx, uintN argc, jsval *vp)
{
    jsid id;
    JSObject *obj, *pobj;
    JSProperty *prop;
    JSScopeProperty *sprop;

    if (!JS_ValueToId(cx, argc != 0 ? vp[2] : JSVAL_VOID, &id))
        return JS_FALSE;
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
        return JS_FALSE;
    *vp = JSVAL_VOID;
    if (prop) {
        if (OBJ_IS_NATIVE(pobj)) {
            sprop = (JSScopeProperty *) prop;
            if (sprop->attrs & JSPROP_GETTER)
                *vp = js_CastAsObjectJSVal(sprop->getter);
        }
        OBJ_DROP_PROPERTY(cx, pobj, prop);
    }
    return JS_TRUE;
}

static JSBool
obj_lookupSetter(JSContext *cx, uintN argc, jsval *vp)
{
    jsid id;
    JSObject *obj, *pobj;
    JSProperty *prop;
    JSScopeProperty *sprop;

    if (!JS_ValueToId(cx, argc != 0 ? vp[2] : JSVAL_VOID, &id))
        return JS_FALSE;
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
        return JS_FALSE;
    *vp = JSVAL_VOID;
    if (prop) {
        if (OBJ_IS_NATIVE(pobj)) {
            sprop = (JSScopeProperty *) prop;
            if (sprop->attrs & JSPROP_SETTER)
                *vp = js_CastAsObjectJSVal(sprop->setter);
        }
        OBJ_DROP_PROPERTY(cx, pobj, prop);
    }
    return JS_TRUE;
}
#endif 

JSBool
obj_getPrototypeOf(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    uintN attrs;

    if (argc == 0) {
        js_ReportMissingArg(cx, vp, 0);
        return JS_FALSE;
    }

    obj = js_ValueToNonNullObject(cx, vp[2]);
    if (!obj)
        return JS_FALSE;
    vp[2] = OBJECT_TO_JSVAL(obj);

    return OBJ_CHECK_ACCESS(cx, obj,
                            ATOM_TO_JSID(cx->runtime->atomState.protoAtom),
                            JSACC_PROTO, vp, &attrs);
}

#if JS_HAS_OBJ_WATCHPOINT
const char js_watch_str[] = "watch";
const char js_unwatch_str[] = "unwatch";
#endif
const char js_hasOwnProperty_str[] = "hasOwnProperty";
const char js_isPrototypeOf_str[] = "isPrototypeOf";
const char js_propertyIsEnumerable_str[] = "propertyIsEnumerable";
#if JS_HAS_GETTER_SETTER
const char js_defineGetter_str[] = "__defineGetter__";
const char js_defineSetter_str[] = "__defineSetter__";
const char js_lookupGetter_str[] = "__lookupGetter__";
const char js_lookupSetter_str[] = "__lookupSetter__";
#endif

JS_DEFINE_TRCINFO_1(obj_valueOf,
    (3, (static, JSVAL,     Object_p_valueOf,               CONTEXT, THIS, STRING,  0, 0)))
JS_DEFINE_TRCINFO_1(obj_hasOwnProperty,
    (3, (static, BOOL_FAIL, Object_p_hasOwnProperty,        CONTEXT, THIS, STRING,  0, 0)))
JS_DEFINE_TRCINFO_1(obj_propertyIsEnumerable,
    (3, (static, BOOL_FAIL, Object_p_propertyIsEnumerable,  CONTEXT, THIS, STRING,  0, 0)))

static JSFunctionSpec object_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,             obj_toSource,                0,0),
#endif
    JS_FN(js_toString_str,             obj_toString,                0,0),
    JS_FN(js_toLocaleString_str,       obj_toLocaleString,          0,0),
    JS_TN(js_valueOf_str,              obj_valueOf,                 0,0,
          obj_valueOf_trcinfo),
#if JS_HAS_OBJ_WATCHPOINT
    JS_FN(js_watch_str,                obj_watch,                   2,0),
    JS_FN(js_unwatch_str,              obj_unwatch,                 1,0),
#endif
    JS_TN(js_hasOwnProperty_str,       obj_hasOwnProperty,          1,0,
          obj_hasOwnProperty_trcinfo),
    JS_FN(js_isPrototypeOf_str,        obj_isPrototypeOf,           1,0),
    JS_TN(js_propertyIsEnumerable_str, obj_propertyIsEnumerable,    1,0,
          obj_propertyIsEnumerable_trcinfo),
#if JS_HAS_GETTER_SETTER
    JS_FN(js_defineGetter_str,         js_obj_defineGetter,         2,0),
    JS_FN(js_defineSetter_str,         js_obj_defineSetter,         2,0),
    JS_FN(js_lookupGetter_str,         obj_lookupGetter,            1,0),
    JS_FN(js_lookupSetter_str,         obj_lookupSetter,            1,0),
#endif
    JS_FS_END
};

static JSFunctionSpec object_static_methods[] = {
    JS_FN("getPrototypeOf",            obj_getPrototypeOf,          1,0),
    JS_FS_END
};

static bool
AllocSlots(JSContext *cx, JSObject *obj, size_t nslots);

static inline bool
InitScopeForObject(JSContext* cx, JSObject* obj, JSObject* proto, JSObjectOps* ops)
{
    JS_ASSERT(OPS_IS_NATIVE(ops));
    JS_ASSERT(proto == OBJ_GET_PROTO(cx, obj));

    
    JSClass *clasp = OBJ_GET_CLASS(cx, obj);
    JSScope *scope;
    if (proto && js_ObjectIsSimilarToProto(cx, obj, ops, clasp, proto)) {
        scope = OBJ_SCOPE(proto)->getEmptyScope(cx, clasp);
        if (!scope)
            goto bad;
    } else {
        scope = JSScope::create(cx, ops, clasp, obj);
        if (!scope)
            goto bad;

        
        JS_ASSERT(scope->freeslot >= JSSLOT_PRIVATE);
        if (scope->freeslot > JS_INITIAL_NSLOTS &&
            !AllocSlots(cx, obj, scope->freeslot)) {
            JSScope::destroy(cx, scope);
            goto bad;
        }
    }
    obj->map = &scope->map;
    return true;

  bad:
    
    obj->map = NULL;
    return false;
}

JSObject *
js_NewObjectWithGivenProto(JSContext *cx, JSClass *clasp, JSObject *proto,
                           JSObject *parent, size_t objectSize)
{
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_START_ENABLED())
        jsdtrace_object_create_start(cx->fp, clasp);
#endif

    
    JS_ASSERT_IF(clasp->flags & JSCLASS_IS_EXTENDED,
                 ((JSExtendedClass *)clasp)->equality);

    
    JSObjectOps *ops = clasp->getObjectOps
                       ? clasp->getObjectOps(cx, clasp)
                       : &js_ObjectOps;

    




    JSObject* obj;
    if (clasp == &js_FunctionClass && !objectSize) {
        obj = (JSObject*) js_NewGCFunction(cx, GCX_OBJECT);
#ifdef DEBUG
        memset((uint8 *) obj + sizeof(JSObject), JS_FREE_PATTERN,
               sizeof(JSFunction) - sizeof(JSObject));
#endif
    } else {
        JS_ASSERT(!objectSize || objectSize == sizeof(JSObject));
        obj = js_NewGCObject(cx, GCX_OBJECT);
    }
    if (!obj)
        goto out;

    



    JS_ASSERT(((jsuword) clasp & 3) == 0);
    obj->classword = jsuword(clasp);
    JS_ASSERT(!STOBJ_IS_DELEGATE(obj));
    JS_ASSERT(!STOBJ_IS_SYSTEM(obj));

    obj->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto);

    



    obj->fslots[JSSLOT_PARENT] = OBJECT_TO_JSVAL((!parent && proto)
                                                 ? OBJ_GET_PARENT(cx, proto)
                                                 : parent);

    
    for (uint32 i = JSSLOT_PRIVATE; i < JS_INITIAL_NSLOTS; ++i)
        obj->fslots[i] = JSVAL_VOID;

    obj->dslots = NULL;

    if (OPS_IS_NATIVE(ops)) {
        if (!InitScopeForObject(cx, obj, proto, ops)) {
            obj = NULL;
            goto out;
        }
    } else {
        JS_ASSERT(ops->objectMap->ops == ops);
        obj->map = const_cast<JSObjectMap *>(ops->objectMap);
    }

    
    JS_ASSERT_IF(!cx->localRootStack, cx->weakRoots.newborn[GCX_OBJECT] == obj);

    



    if (cx->debugHooks->objectHook && !JS_ON_TRACE(cx)) {
        JSAutoTempValueRooter tvr(cx, obj);
        JS_KEEP_ATOMS(cx->runtime);
        cx->debugHooks->objectHook(cx, obj, JS_TRUE,
                                   cx->debugHooks->objectHookData);
        JS_UNKEEP_ATOMS(cx->runtime);
        cx->weakRoots.newborn[GCX_OBJECT] = obj;
    }

out:
#ifdef INCLUDE_MOZILLA_DTRACE
    if (JAVASCRIPT_OBJECT_CREATE_ENABLED())
        jsdtrace_object_create(cx, clasp, obj);
    if (JAVASCRIPT_OBJECT_CREATE_DONE_ENABLED())
        jsdtrace_object_create_done(cx->fp, clasp);
#endif
    return obj;
}

JSObject *
js_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto,
             JSObject *parent, size_t objectSize)
{
    jsid id;

    
    if (!proto) {
        if (!js_GetClassId(cx, clasp, &id))
            return NULL;
        if (!js_GetClassPrototype(cx, parent, id, &proto))
            return NULL;
        if (!proto &&
            !js_GetClassPrototype(cx, parent, INT_TO_JSID(JSProto_Object),
                                  &proto)) {
            return NULL;
        }
    }

    return js_NewObjectWithGivenProto(cx, clasp, proto, parent, objectSize);
}

JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc == 0) {
        
        obj = NULL;
    } else {
        
        if (!js_ValueToObject(cx, argv[0], &obj))
            return JS_FALSE;
    }
    if (!obj) {
        JS_ASSERT(!argc || JSVAL_IS_NULL(argv[0]) || JSVAL_IS_VOID(argv[0]));
        if (JS_IsConstructing(cx))
            return JS_TRUE;
        obj = js_NewObject(cx, &js_ObjectClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
    }
    *rval = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

#ifdef JS_TRACER

static inline JSObject*
NewNativeObject(JSContext* cx, JSClass* clasp, JSObject* proto, JSObject *parent)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    JSObject* obj = js_NewGCObject(cx, GCX_OBJECT);
    if (!obj)
        return NULL;

    obj->classword = jsuword(clasp);
    obj->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto);
    obj->fslots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(parent);
    for (unsigned i = JSSLOT_PRIVATE; i < JS_INITIAL_NSLOTS; ++i)
        obj->fslots[i] = JSVAL_VOID;

    obj->dslots = NULL;
    return InitScopeForObject(cx, obj, proto, &js_ObjectOps) ? obj : NULL;
}

JSObject* FASTCALL
js_Object_tn(JSContext* cx, JSObject* proto)
{
    return NewNativeObject(cx, &js_ObjectClass, proto, JSVAL_TO_OBJECT(proto->fslots[JSSLOT_PARENT]));
}

JS_DEFINE_TRCINFO_1(js_Object,
    (2, (extern, CONSTRUCTOR_RETRY, js_Object_tn, CONTEXT, CALLEE_PROTOTYPE, 0, 0)))

JSObject* FASTCALL
js_NewInstance(JSContext *cx, JSClass *clasp, JSObject *ctor)
{
    JS_ASSERT(HAS_FUNCTION_CLASS(ctor));

    JSAtom *atom = cx->runtime->atomState.classPrototypeAtom;

    JSScope *scope = OBJ_SCOPE(ctor);
#ifdef JS_THREADSAFE
    if (scope->title.ownercx != cx)
        return NULL;
#endif
    if (!scope->owned()) {
        scope = js_GetMutableScope(cx, ctor);
        if (!scope)
            return NULL;
    }

    JSScopeProperty *sprop = scope->lookup(ATOM_TO_JSID(atom));
    jsval pval = sprop ? STOBJ_GET_SLOT(ctor, sprop->slot) : JSVAL_HOLE;

    JSObject *proto;
    if (!JSVAL_IS_PRIMITIVE(pval)) {
        
        proto = JSVAL_TO_OBJECT(pval);
    } else if (pval == JSVAL_HOLE) {
        
        proto = js_NewObject(cx, clasp, NULL, OBJ_GET_PARENT(cx, ctor));
        if (!proto)
            return NULL;
        if (!js_SetClassPrototype(cx, ctor, proto, JSPROP_ENUMERATE | JSPROP_PERMANENT))
            return NULL;
    } else {
        
        if (!js_GetClassPrototype(cx, JSVAL_TO_OBJECT(ctor->fslots[JSSLOT_PARENT]),
                                  INT_TO_JSID(JSProto_Object), &proto)) {
            return NULL;
        }
    }

    return NewNativeObject(cx, clasp, proto, JSVAL_TO_OBJECT(ctor->fslots[JSSLOT_PARENT]));
}

JS_DEFINE_CALLINFO_3(extern, CONSTRUCTOR_RETRY, js_NewInstance, CONTEXT, CLASS, OBJECT, 0, 0)

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
        JS_ASSERT(pc < endpc);

        
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

    JSStackFrame *fp;
    jsbytecode *pc;
    const JSCodeSpec *cs;
    uint32 format;
    uintN flags = 0;

    fp = js_GetTopStackFrame(cx);
    if (!fp || !fp->regs)
        return defaultFlags;
    pc = fp->regs->pc;
    cs = &js_CodeSpec[js_GetOpcode(cx, fp->script, pc)];
    format = cs->format;
    if (JOF_MODE(format) != JOF_NAME)
        flags |= JSRESOLVE_QUALIFIED;
    if ((format & (JOF_SET | JOF_FOR)) ||
        (fp->flags & JSFRAME_ASSIGNING)) {
        flags |= JSRESOLVE_ASSIGNING;
    } else {
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
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_LookupProperty(cx, obj, id, objp, propp);
    return OBJ_LOOKUP_PROPERTY(cx, proto, id, objp, propp);
}

static JSBool
with_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_GetProperty(cx, obj, id, vp);
    return OBJ_GET_PROPERTY(cx, proto, id, vp);
}

static JSBool
with_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_SetProperty(cx, obj, id, vp);
    return OBJ_SET_PROPERTY(cx, proto, id, vp);
}

static JSBool
with_GetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                   uintN *attrsp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_GetAttributes(cx, obj, id, prop, attrsp);
    return OBJ_GET_ATTRIBUTES(cx, proto, id, prop, attrsp);
}

static JSBool
with_SetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                   uintN *attrsp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_SetAttributes(cx, obj, id, prop, attrsp);
    return OBJ_SET_ATTRIBUTES(cx, proto, id, prop, attrsp);
}

static JSBool
with_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *rval)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_DeleteProperty(cx, obj, id, rval);
    return OBJ_DELETE_PROPERTY(cx, proto, id, rval);
}

static JSBool
with_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_DefaultValue(cx, obj, hint, vp);
    return OBJ_DEFAULT_VALUE(cx, proto, hint, vp);
}

static JSBool
with_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
               jsval *statep, jsid *idp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_Enumerate(cx, obj, enum_op, statep, idp);
    return OBJ_ENUMERATE(cx, proto, enum_op, statep, idp);
}

static JSBool
with_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                 jsval *vp, uintN *attrsp)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return js_CheckAccess(cx, obj, id, mode, vp, attrsp);
    return OBJ_CHECK_ACCESS(cx, proto, id, mode, vp, attrsp);
}

static JSObject *
with_ThisObject(JSContext *cx, JSObject *obj)
{
    JSObject *proto = OBJ_GET_PROTO(cx, obj);
    if (!proto)
        return obj;
    return OBJ_THIS_OBJECT(cx, proto);
}

JS_FRIEND_DATA(JSObjectOps) js_WithObjectOps = {
    NULL,
    with_LookupProperty,    js_DefineProperty,
    with_GetProperty,       with_SetProperty,
    with_GetAttributes,     with_SetAttributes,
    with_DeleteProperty,    with_DefaultValue,
    with_Enumerate,         with_CheckAccess,
    with_ThisObject,        NATIVE_DROP_PROPERTY,
    NULL,                   NULL,
    NULL,                   js_TraceObject,
    js_Clear,               NULL,
    NULL
};

static JSObjectOps *
with_getObjectOps(JSContext *cx, JSClass *clasp)
{
    return &js_WithObjectOps;
}

JSClass js_WithClass = {
    "With",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   NULL,
    with_getObjectOps,
    0,0,0,0,0,0,0
};

JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth)
{
    JSObject *obj;

    obj = js_NewObject(cx, &js_WithClass, proto, parent);
    if (!obj)
        return NULL;
    obj->setPrivate(cx->fp);
    OBJ_SET_BLOCK_DEPTH(cx, obj, depth);
    return obj;
}

JSObject *
js_NewBlockObject(JSContext *cx)
{
    



    JSObject *blockObj = js_NewObjectWithGivenProto(cx, &js_BlockClass, NULL, NULL);
    JS_ASSERT_IF(blockObj, !OBJ_IS_CLONED_BLOCK(blockObj));
    return blockObj;
}

JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp)
{
    JS_ASSERT(!OBJ_IS_CLONED_BLOCK(proto));
    JS_ASSERT(STOBJ_GET_CLASS(proto) == &js_BlockClass);

    JSObject *clone = js_NewGCObject(cx, GCX_OBJECT);
    if (!clone)
        return NULL;

    JSScope *scope = OBJ_SCOPE(proto);
    scope->hold();
    clone->map = &scope->map;

    clone->classword = jsuword(&js_BlockClass);
    clone->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto);
    clone->fslots[JSSLOT_PARENT] = JSVAL_NULL;  
    clone->setPrivate(fp);
    clone->fslots[JSSLOT_BLOCK_DEPTH] = proto->fslots[JSSLOT_BLOCK_DEPTH];
    JS_ASSERT(scope->freeslot == JSSLOT_BLOCK_DEPTH + 1);
    for (uint32 i = JSSLOT_BLOCK_DEPTH + 1; i < JS_INITIAL_NSLOTS; ++i)
        clone->fslots[i] = JSVAL_VOID;
    clone->dslots = NULL;
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(clone));
    return clone;
}

JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind)
{
    JSStackFrame *fp;
    JSObject *obj;
    uintN depth, count;

    
    JS_STATIC_ASSERT(JS_INITIAL_NSLOTS == JSSLOT_BLOCK_DEPTH + 2);

    fp = cx->fp;
    obj = fp->scopeChain;
    JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_BlockClass);
    JS_ASSERT(obj->getAssignedPrivate() == cx->fp);
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(obj));

    





    JS_ASSERT(OBJ_SCOPE(obj)->object != obj);

    
    JS_ASSERT(STOBJ_NSLOTS(obj) == JS_INITIAL_NSLOTS);

    
    depth = OBJ_BLOCK_DEPTH(cx, obj);
    count = OBJ_BLOCK_COUNT(cx, obj);
    JS_ASSERT(depth <= (size_t) (fp->regs->sp - StackBase(fp)));
    JS_ASSERT(count <= (size_t) (fp->regs->sp - StackBase(fp) - depth));

    
    JS_ASSERT(count >= 1);

    depth += fp->script->nfixed;
    obj->fslots[JSSLOT_BLOCK_DEPTH + 1] = fp->slots[depth];
    if (normalUnwind && count > 1) {
        --count;
        JS_LOCK_OBJ(cx, obj);
        if (!AllocSlots(cx, obj, JS_INITIAL_NSLOTS + count))
            normalUnwind = JS_FALSE;
        else
            memcpy(obj->dslots, fp->slots + depth + 1, count * sizeof(jsval));
        JS_UNLOCK_OBJ(cx, obj);
    }

    
    obj->setPrivate(NULL);
    fp->scopeChain = OBJ_GET_PARENT(cx, obj);
    return normalUnwind;
}

static JSBool
block_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JS_ASSERT(JS_InstanceOf(cx, obj, &js_BlockClass, NULL));
    JS_ASSERT(OBJ_IS_CLONED_BLOCK(obj));
    if (!JSVAL_IS_INT(id))
        return JS_TRUE;

    uintN index = (uint16) JSVAL_TO_INT(id);
    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        index += fp->script->nfixed + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->script->nslots);
        *vp = fp->slots[index];
        return JS_TRUE;
    }

    
    index += JSSLOT_BLOCK_DEPTH - JSSLOT_PRIVATE;
    return JS_GetReservedSlot(cx, obj, index, vp);
}

static JSBool
block_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JS_ASSERT(JS_InstanceOf(cx, obj, &js_BlockClass, NULL));
    if (!JSVAL_IS_INT(id))
        return JS_TRUE;

    uintN index = (uint16) JSVAL_TO_INT(id);
    JSStackFrame *fp = (JSStackFrame *) obj->getPrivate();
    if (fp) {
        index += fp->script->nfixed + OBJ_BLOCK_DEPTH(cx, obj);
        JS_ASSERT(index < fp->script->nslots);
        fp->slots[index] = *vp;
        return JS_TRUE;
    }

    
    index += JSSLOT_BLOCK_DEPTH - JSSLOT_PRIVATE;
    return JS_SetReservedSlot(cx, obj, index, *vp);
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
    JSTempValueRooter tvr;
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
        parent = OBJ_GET_PARENT(cx, obj);
        parentId = (xdr->script->objectsOffset == 0)
                   ? NO_PARENT_INDEX
                   : FindObjectIndex(JS_SCRIPT_OBJECTS(xdr->script), parent);
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
            JS_GET_SCRIPT_OBJECT(xdr->script, parentId, parent);
        STOBJ_SET_PARENT(obj, parent);
    }

    JS_PUSH_SINGLE_TEMP_ROOT(cx, OBJECT_TO_JSVAL(obj), &tvr);

    if (!JS_XDRUint32(xdr, &tmp)) {
        JS_POP_TEMP_ROOT(cx, &tvr);
        return JS_FALSE;
    }

    if (xdr->mode == JSXDR_DECODE) {
        depth = (uint16)(tmp >> 16);
        count = (uint16)tmp;
        STOBJ_SET_SLOT(obj, JSSLOT_BLOCK_DEPTH, INT_TO_JSVAL(depth));
    }

    




    sprop = NULL;
    ok = JS_TRUE;
    for (i = 0; i < count; i++) {
        if (xdr->mode == JSXDR_ENCODE) {
            
            do {
                
                sprop = sprop ? sprop->parent : OBJ_SCOPE(obj)->lastProp;
            } while (!(sprop->flags & SPROP_HAS_SHORTID));

            JS_ASSERT(sprop->getter == js_BlockClass.getProperty);
            propid = sprop->id;
            JS_ASSERT(JSID_IS_ATOM(propid));
            atom = JSID_TO_ATOM(propid);
            shortid = sprop->shortid;
            JS_ASSERT(shortid >= 0);
        }

        
        if (!js_XDRStringAtom(xdr, &atom) ||
            !JS_XDRUint16(xdr, (uint16 *)&shortid)) {
            ok = JS_FALSE;
            break;
        }

        if (xdr->mode == JSXDR_DECODE) {
            if (!js_DefineNativeProperty(cx, obj, ATOM_TO_JSID(atom),
                                         JSVAL_VOID, NULL, NULL,
                                         JSPROP_ENUMERATE | JSPROP_PERMANENT |
                                         JSPROP_SHARED,
                                         SPROP_HAS_SHORTID, shortid, NULL)) {
                ok = JS_FALSE;
                break;
            }
        }
    }

    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

#endif

static uint32
block_reserveSlots(JSContext *cx, JSObject *obj)
{
    return OBJ_IS_CLONED_BLOCK(obj) ? OBJ_BLOCK_COUNT(cx, obj) : 0;
}

JSClass js_BlockClass = {
    "Block",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,  JS_PropertyStub,  block_getProperty, block_setProperty,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,    NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, block_reserveSlots
};

JSObject *
js_InitEval(JSContext *cx, JSObject *obj)
{
    
    if (!js_DefineFunction(cx, obj, cx->runtime->atomState.evalAtom,
                           obj_eval, 1, 0)) {
        return NULL;
    }

    return obj;
}

JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj)
{
    return js_InitClass(cx, obj, NULL, &js_ObjectClass, js_Object, 1,
                        object_props, object_methods, NULL, object_static_methods);
}

JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             JSClass *clasp, JSNative constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
    JSAtom *atom;
    JSProtoKey key;
    JSObject *proto, *ctor;
    JSTempValueRooter tvr;
    jsval cval, rval;
    JSBool named;
    JSFunction *fun;

    atom = js_Atomize(cx, clasp->name, strlen(clasp->name), 0);
    if (!atom)
        return NULL;

    













    key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !parent_proto &&
        !js_GetClassPrototype(cx, obj, INT_TO_JSID(JSProto_Object),
                              &parent_proto)) {
        return NULL;
    }

    
    proto = js_NewObject(cx, clasp, parent_proto, obj);
    if (!proto)
        return NULL;

    
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &tvr);

    if (!constructor) {
        





        if ((clasp->flags & JSCLASS_IS_ANONYMOUS) &&
            (OBJ_GET_CLASS(cx, obj)->flags & JSCLASS_IS_GLOBAL) &&
            key != JSProto_Null) {
            named = JS_FALSE;
        } else {
            named = OBJ_DEFINE_PROPERTY(cx, obj, ATOM_TO_JSID(atom),
                                        OBJECT_TO_JSVAL(proto),
                                        JS_PropertyStub, JS_PropertyStub,
                                        (clasp->flags & JSCLASS_IS_ANONYMOUS)
                                        ? JSPROP_READONLY | JSPROP_PERMANENT
                                        : 0,
                                        NULL);
            if (!named)
                goto bad;
        }

        ctor = proto;
    } else {
        
        fun = js_DefineFunction(cx, obj, atom, constructor, nargs,
                                JSFUN_STUB_GSOPS);
        named = (fun != NULL);
        if (!fun)
            goto bad;

        




        FUN_CLASP(fun) = clasp;

        





        ctor = FUN_OBJECT(fun);
        if (clasp->flags & JSCLASS_CONSTRUCT_PROTOTYPE) {
            cval = OBJECT_TO_JSVAL(ctor);
            if (!js_InternalConstruct(cx, proto, cval, 0, NULL, &rval))
                goto bad;
            if (!JSVAL_IS_PRIMITIVE(rval) && JSVAL_TO_OBJECT(rval) != proto)
                proto = JSVAL_TO_OBJECT(rval);
        }

        
        if (!js_SetClassPrototype(cx, ctor, proto,
                                  JSPROP_READONLY | JSPROP_PERMANENT)) {
            goto bad;
        }

        
        if (OBJ_GET_CLASS(cx, ctor) == clasp)
            OBJ_SET_PROTO(cx, ctor, proto);
    }

    
    if ((ps && !JS_DefineProperties(cx, proto, ps)) ||
        (fs && !JS_DefineFunctions(cx, proto, fs)) ||
        (static_ps && !JS_DefineProperties(cx, ctor, static_ps)) ||
        (static_fs && !JS_DefineFunctions(cx, ctor, static_fs))) {
        goto bad;
    }

    
    if (key != JSProto_Null && !js_SetClassObject(cx, obj, key, ctor))
        goto bad;

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    return proto;

bad:
    if (named)
        (void) OBJ_DELETE_PROPERTY(cx, obj, ATOM_TO_JSID(atom), &rval);
    proto = NULL;
    goto out;
}

#define SLOTS_TO_DYNAMIC_WORDS(nslots)                                        \
  (JS_ASSERT((nslots) > JS_INITIAL_NSLOTS), (nslots) + 1 - JS_INITIAL_NSLOTS)

#define DYNAMIC_WORDS_TO_SLOTS(words)                                         \
  (JS_ASSERT((words) > 1), (words) - 1 + JS_INITIAL_NSLOTS)


static bool
AllocSlots(JSContext *cx, JSObject *obj, size_t nslots)
{
    JS_ASSERT(!obj->dslots);
    JS_ASSERT(nslots > JS_INITIAL_NSLOTS);

    jsval* slots;
    slots = (jsval*) cx->malloc(SLOTS_TO_DYNAMIC_WORDS(nslots) * sizeof(jsval));
    if (!slots)
        return true;

    *slots++ = nslots;
    
    for (jsuint n = JS_INITIAL_NSLOTS; n < nslots; ++n)
        slots[n - JS_INITIAL_NSLOTS] = JSVAL_VOID;
    obj->dslots = slots;

    return true;
}

bool
js_GrowSlots(JSContext *cx, JSObject *obj, size_t nslots)
{
    


    const size_t MIN_DYNAMIC_WORDS = 4;

    



    const size_t LINEAR_GROWTH_STEP = JS_BIT(16);

    
    if (nslots <= JS_INITIAL_NSLOTS)
        return JS_TRUE;

    size_t nwords = SLOTS_TO_DYNAMIC_WORDS(nslots);

    



    uintN log;
    if (nwords <= MIN_DYNAMIC_WORDS) {
        nwords = MIN_DYNAMIC_WORDS;
    } else if (nwords < LINEAR_GROWTH_STEP) {
        JS_CEILING_LOG2(log, nwords);
        nwords = JS_BIT(log);
    } else {
        nwords = JS_ROUNDUP(nwords, LINEAR_GROWTH_STEP);
    }
    nslots = DYNAMIC_WORDS_TO_SLOTS(nwords);

    



    jsval* slots = obj->dslots;
    if (!slots)
        return AllocSlots(cx, obj, nslots);

    size_t oslots = size_t(slots[-1]);

    slots = (jsval*) cx->realloc(slots - 1, nwords * sizeof(jsval));
    *slots++ = nslots;
    obj->dslots = slots;

    
    JS_ASSERT(nslots > oslots);
    for (size_t i = oslots; i < nslots; i++)
        slots[i - JS_INITIAL_NSLOTS] = JSVAL_VOID;

    return true;
}

void
js_ShrinkSlots(JSContext *cx, JSObject *obj, size_t nslots)
{
    jsval* slots = obj->dslots;

    
    if (!slots)
        return;

    JS_ASSERT(size_t(slots[-1]) > JS_INITIAL_NSLOTS);
    JS_ASSERT(nslots <= size_t(slots[-1]));

    if (nslots <= JS_INITIAL_NSLOTS) {
        cx->free(slots - 1);
        obj->dslots = NULL;
    } else {
        size_t nwords = SLOTS_TO_DYNAMIC_WORDS(nslots);
        slots = (jsval*) cx->realloc(slots - 1, nwords * sizeof(jsval));
        *slots++ = nslots;
        obj->dslots = slots;
    }
}

bool
js_EnsureReservedSlots(JSContext *cx, JSObject *obj, size_t nreserved)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    JS_ASSERT(!obj->dslots);

    uintN nslots = JSSLOT_FREE(STOBJ_GET_CLASS(obj)) + nreserved;
    if (nslots > STOBJ_NSLOTS(obj) && !AllocSlots(cx, obj, nslots))
        return false;

    JSScope *scope = OBJ_SCOPE(obj);
    if (scope->owned()) {
#ifdef JS_THREADSAFE
        JS_ASSERT(scope->title.ownercx->thread == cx->thread);
#endif
        JS_ASSERT(scope->freeslot == JSSLOT_FREE(STOBJ_GET_CLASS(obj)));
        if (scope->freeslot < nslots)
            scope->freeslot = nslots;
    }
    return true;
}

extern JSBool
js_GetClassId(JSContext *cx, JSClass *clasp, jsid *idp)
{
    JSProtoKey key;
    JSAtom *atom;

    key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null) {
        *idp = INT_TO_JSID(key);
    } else if (clasp->flags & JSCLASS_IS_ANONYMOUS) {
        *idp = INT_TO_JSID(JSProto_Object);
    } else {
        atom = js_Atomize(cx, clasp->name, strlen(clasp->name), 0);
        if (!atom)
            return JS_FALSE;
        *idp = ATOM_TO_JSID(atom);
    }
    return JS_TRUE;
}

JSObject*
js_NewNativeObject(JSContext *cx, JSClass *clasp, JSObject *proto, uint32 slot)
{
    JS_ASSERT(!clasp->getObjectOps);
    JS_ASSERT(proto->map->ops == &js_ObjectOps);
    JS_ASSERT(OBJ_GET_CLASS(cx, proto) == clasp);

    JSObject* obj = js_NewGCObject(cx, GCX_OBJECT);
    if (!obj)
        return NULL;

    JSScope *scope = OBJ_SCOPE(proto)->getEmptyScope(cx, clasp);
    if (!scope) {
        obj->map = NULL;
        return NULL;
    }
    obj->map = &scope->map;
    obj->classword = jsuword(clasp);
    obj->fslots[JSSLOT_PROTO] = OBJECT_TO_JSVAL(proto);
    obj->fslots[JSSLOT_PARENT] = proto->fslots[JSSLOT_PARENT];

    JS_ASSERT(slot > JSSLOT_PARENT);
    while (slot < JS_INITIAL_NSLOTS)
        obj->fslots[slot++] = JSVAL_VOID;

    obj->dslots = NULL;
    return obj;
}

JS_BEGIN_EXTERN_C

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

JS_END_EXTERN_C

JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp)
{
    JSBool ok;
    JSObject *tmp, *cobj;
    JSResolvingKey rkey;
    JSResolvingEntry *rentry;
    uint32 generation;
    JSObjectOp init;
    jsval v;

    while ((tmp = OBJ_GET_PARENT(cx, obj)) != NULL)
        obj = tmp;
    if (!(OBJ_GET_CLASS(cx, obj)->flags & JSCLASS_IS_GLOBAL)) {
        *objp = NULL;
        return JS_TRUE;
    }

    ok = JS_GetReservedSlot(cx, obj, key, &v);
    if (!ok)
        return JS_FALSE;
    if (!JSVAL_IS_PRIMITIVE(v)) {
        *objp = JSVAL_TO_OBJECT(v);
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

    cobj = NULL;
    init = lazy_prototype_init[key];
    if (init) {
        if (!init(cx, obj)) {
            ok = JS_FALSE;
        } else {
            ok = JS_GetReservedSlot(cx, obj, key, &v);
            if (ok && !JSVAL_IS_PRIMITIVE(v))
                cobj = JSVAL_TO_OBJECT(v);
        }
    }

    js_StopResolving(cx, &rkey, JSRESFLAG_LOOKUP, rentry, generation);
    *objp = cobj;
    return ok;
}

JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject *cobj)
{
    JS_ASSERT(!OBJ_GET_PARENT(cx, obj));
    if (!(OBJ_GET_CLASS(cx, obj)->flags & JSCLASS_IS_GLOBAL))
        return JS_TRUE;

    return JS_SetReservedSlot(cx, obj, key, OBJECT_TO_JSVAL(cobj));
}

JSBool
js_FindClassObject(JSContext *cx, JSObject *start, jsid id, jsval *vp)
{
    JSStackFrame *fp;
    JSObject *obj, *cobj, *pobj;
    JSProtoKey key;
    JSProperty *prop;
    jsval v;
    JSScopeProperty *sprop;

    




    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (!start && (fp = cx->fp) != NULL)
        start = fp->scopeChain;

    if (start) {
        
        do {
            obj = start;
            start = OBJ_GET_PARENT(cx, obj);
        } while (start);
    } else {
        obj = cx->globalObject;
        if (!obj) {
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }
    }

    OBJ_TO_INNER_OBJECT(cx, obj);
    if (!obj)
        return JS_FALSE;

    if (JSID_IS_INT(id)) {
        key = (JSProtoKey) JSID_TO_INT(id);
        JS_ASSERT(key != JSProto_Null);
        if (!js_GetClassObject(cx, obj, key, &cobj))
            return JS_FALSE;
        if (cobj) {
            *vp = OBJECT_TO_JSVAL(cobj);
            return JS_TRUE;
        }
        id = ATOM_TO_JSID(cx->runtime->atomState.classAtoms[key]);
    }

    JS_ASSERT(OBJ_IS_NATIVE(obj));
    if (js_LookupPropertyWithFlags(cx, obj, id, JSRESOLVE_CLASSNAME,
                                   &pobj, &prop) < 0) {
        return JS_FALSE;
    }
    v = JSVAL_VOID;
    if (prop)  {
        if (OBJ_IS_NATIVE(pobj)) {
            sprop = (JSScopeProperty *) prop;
            if (SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(pobj))) {
                v = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);
                if (JSVAL_IS_PRIMITIVE(v))
                    v = JSVAL_VOID;
            }
        }
        OBJ_DROP_PROPERTY(cx, pobj, prop);
    }
    *vp = v;
    return JS_TRUE;
}

JSObject *
js_ConstructObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, jsval *argv)
{
    jsid id;
    jsval cval, rval;
    JSTempValueRooter argtvr, tvr;
    JSObject *obj, *ctor;

    JS_PUSH_TEMP_ROOT(cx, argc, argv, &argtvr);

    if (!js_GetClassId(cx, clasp, &id) ||
        !js_FindClassObject(cx, parent, id, &cval)) {
        JS_POP_TEMP_ROOT(cx, &argtvr);
        return NULL;
    }

    if (JSVAL_IS_PRIMITIVE(cval)) {
        js_ReportIsNotFunction(cx, &cval, JSV2F_CONSTRUCT | JSV2F_SEARCH_STACK);
        JS_POP_TEMP_ROOT(cx, &argtvr);
        return NULL;
    }

    



    JS_PUSH_SINGLE_TEMP_ROOT(cx, cval, &tvr);
    MUST_FLOW_THROUGH("out");

    



    ctor = JSVAL_TO_OBJECT(cval);
    if (!parent)
        parent = OBJ_GET_PARENT(cx, ctor);
    if (!proto) {
        if (!OBJ_GET_PROPERTY(cx, ctor,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .classPrototypeAtom),
                              &rval)) {
            obj = NULL;
            goto out;
        }
        if (JSVAL_IS_OBJECT(rval))
            proto = JSVAL_TO_OBJECT(rval);
    }

    obj = js_NewObject(cx, clasp, proto, parent);
    if (!obj)
        goto out;

    if (!js_InternalConstruct(cx, obj, cval, argc, argv, &rval))
        goto bad;

    if (JSVAL_IS_PRIMITIVE(rval))
        goto out;
    obj = JSVAL_TO_OBJECT(rval);

    






    if (OBJ_GET_CLASS(cx, obj) != clasp ||
        (!(~clasp->flags & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_CONSTRUCT_PROTOTYPE)) &&
         !obj->getPrivate())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_WRONG_CONSTRUCTOR, clasp->name);
        goto bad;
    }

out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    JS_POP_TEMP_ROOT(cx, &argtvr);
    return obj;

bad:
    cx->weakRoots.newborn[GCX_OBJECT] = NULL;
    obj = NULL;
    goto out;
}



JSBool
js_AllocSlot(JSContext *cx, JSObject *obj, uint32 *slotp)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));

    JSScope *scope = OBJ_SCOPE(obj);
    JSClass *clasp = LOCKED_OBJ_GET_CLASS(obj);
    if (scope->freeslot == JSSLOT_FREE(clasp) && clasp->reserveSlots) {
        
        scope->freeslot += clasp->reserveSlots(cx, obj);
    }

    if (scope->freeslot >= STOBJ_NSLOTS(obj) &&
        !js_GrowSlots(cx, obj, scope->freeslot + 1)) {
        return JS_FALSE;
    }

    
    JS_ASSERT(JSVAL_IS_VOID(STOBJ_GET_SLOT(obj, scope->freeslot)));
    *slotp = scope->freeslot++;
    return JS_TRUE;
}

void
js_FreeSlot(JSContext *cx, JSObject *obj, uint32 slot)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));

    JSScope *scope = OBJ_SCOPE(obj);
    LOCKED_OBJ_SET_SLOT(obj, slot, JSVAL_VOID);
    if (scope->freeslot == slot + 1)
        scope->freeslot = slot;
}



#define JSVAL_INT_MAX_STRING "1073741823"







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
    if (n > sizeof(JSVAL_INT_MAX_STRING) - 1)
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

    if (oldIndex < JSVAL_INT_MAX / 10 ||
        (oldIndex == JSVAL_INT_MAX / 10 && c <= (JSVAL_INT_MAX % 10))) {
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
        if (!OBJ_IS_NATIVE(obj)) {
            obj = OBJ_GET_PROTO(cx, obj);
            continue;
        }
        JS_LOCK_OBJ(cx, obj);
        scope = OBJ_SCOPE(obj);
        sprop = scope->lookup(id);
        if (sprop) {
            PCMETER(JS_PROPERTY_CACHE(cx).pcpurges++);
            scope->shadowingShapeChange(cx, sprop);
            JS_UNLOCK_SCOPE(cx, scope);

            if (!STOBJ_GET_PARENT(obj)) {
                




                js_LeaveTrace(cx);
            }
            return JS_TRUE;
        }
        obj = LOCKED_OBJ_GET_PROTO(obj);
        JS_UNLOCK_SCOPE(cx, scope);
    }
    return JS_FALSE;
}

void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id)
{
    JS_ASSERT(OBJ_IS_DELEGATE(cx, obj));
    PurgeProtoChain(cx, OBJ_GET_PROTO(cx, obj), id);

    





    if (STOBJ_GET_CLASS(obj) == &js_CallClass) {
        while ((obj = OBJ_GET_PARENT(cx, obj)) != NULL) {
            if (PurgeProtoChain(cx, obj, id))
                break;
        }
    }
}

JSScopeProperty *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid)
{
    JSScope *scope;
    JSScopeProperty *sprop;

    




    js_PurgeScopeChain(cx, obj, id);

    JS_LOCK_OBJ(cx, obj);
    scope = js_GetMutableScope(cx, obj);
    if (!scope) {
        sprop = NULL;
    } else {
        
        id = js_CheckForStringIndex(id);
        sprop = scope->add(cx, id, getter, setter, slot, attrs, flags, shortid);
    }
    JS_UNLOCK_OBJ(cx, obj);
    return sprop;
}

JSScopeProperty *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             JSScopeProperty *sprop, uintN attrs, uintN mask,
                             JSPropertyOp getter, JSPropertyOp setter)
{
    JSScope *scope;

    JS_LOCK_OBJ(cx, obj);
    scope = js_GetMutableScope(cx, obj);
    if (!scope) {
        sprop = NULL;
    } else {
        sprop = scope->change(cx, sprop, attrs, mask, getter, setter);
    }
    JS_UNLOCK_OBJ(cx, obj);
    return sprop;
}

JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                  JSPropertyOp getter, JSPropertyOp setter, uintN attrs,
                  JSProperty **propp)
{
    return js_DefineNativeProperty(cx, obj, id, value, getter, setter, attrs,
                                   0, 0, propp);
}










#define ADD_PROPERTY_HELPER(cx,clasp,obj,scope,sprop,vp,cleanup)              \
    JS_BEGIN_MACRO                                                            \
        if ((clasp)->addProperty != JS_PropertyStub) {                        \
            jsval nominal_ = *(vp);                                           \
            if (!(clasp)->addProperty(cx, obj, SPROP_USERID(sprop), vp)) {    \
                cleanup;                                                      \
            }                                                                 \
            if (*(vp) != nominal_) {                                          \
                if (SPROP_HAS_VALID_SLOT(sprop, scope))                       \
                    LOCKED_OBJ_WRITE_SLOT(cx, obj, (sprop)->slot, *(vp));     \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO

JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                        JSPropertyOp getter, JSPropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow )
{
    JSClass *clasp;
    JSScope *scope;
    JSScopeProperty *sprop;
    JSBool added;

    JS_ASSERT((defineHow & ~(JSDNP_CACHE_RESULT | JSDNP_DONT_PURGE)) == 0);
    js_LeaveTraceIfGlobalObject(cx, obj);

    
    id = js_CheckForStringIndex(id);

#if JS_HAS_GETTER_SETTER
    




    sprop = NULL;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        JSObject *pobj;
        JSProperty *prop;

        







        if (!js_LookupProperty(cx, obj, id, &pobj, &prop))
            return JS_FALSE;
        sprop = (JSScopeProperty *) prop;
        if (sprop &&
            pobj == obj &&
            (sprop->attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
            sprop = OBJ_SCOPE(obj)->change(cx, sprop, attrs, sprop->attrs,
                                           (attrs & JSPROP_GETTER)
                                           ? getter
                                           : sprop->getter,
                                           (attrs & JSPROP_SETTER)
                                           ? setter
                                           : sprop->setter);

            
            if (!sprop)
                goto error;
        } else if (prop) {
            
            OBJ_DROP_PROPERTY(cx, pobj, prop);
            prop = NULL;
            sprop = NULL;
        }
    }
#endif 

    




    if (!(defineHow & JSDNP_DONT_PURGE))
        js_PurgeScopeChain(cx, obj, id);

    




    if (OBJ_IS_DELEGATE(cx, obj) && (attrs & (JSPROP_READONLY | JSPROP_SETTER)))
        cx->runtime->protoHazardShape = js_GenerateShape(cx, false);

    
    JS_LOCK_OBJ(cx, obj);

    
    clasp = LOCKED_OBJ_GET_CLASS(obj);
    if (!getter)
        getter = clasp->getProperty;
    if (!setter)
        setter = clasp->setProperty;

    
    scope = js_GetMutableScope(cx, obj);
    if (!scope)
        goto error;

    added = false;
    if (!sprop) {
        
        if (clasp->flags & JSCLASS_SHARE_ALL_PROPERTIES)
            attrs |= JSPROP_SHARED;
        sprop = scope->add(cx, id, getter, setter, SPROP_INVALID_SLOT, attrs,
                           flags, shortid);
        if (!sprop)
            goto error;
        added = true;
    }

    
    if (SPROP_HAS_VALID_SLOT(sprop, scope))
        LOCKED_OBJ_WRITE_SLOT(cx, obj, sprop->slot, value);

    
    ADD_PROPERTY_HELPER(cx, clasp, obj, scope, sprop, &value,
                        scope->remove(cx, id);
                        goto error);

    if (defineHow & JSDNP_CACHE_RESULT) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        JSPropCacheEntry *entry;
        entry = js_FillPropertyCache(cx, obj, 0, 0, obj, sprop, added);
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

int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp)
{
    JSObject *start, *obj2, *proto;
    int protoIndex;
    JSScope *scope;
    JSScopeProperty *sprop;
    JSClass *clasp;
    JSResolveOp resolve;
    JSResolvingKey key;
    JSResolvingEntry *entry;
    uint32 generation;
    JSNewResolveOp newresolve;
    JSBool ok;

    
    id = js_CheckForStringIndex(id);

    
    start = obj;
    for (protoIndex = 0; ; protoIndex++) {
        JS_LOCK_OBJ(cx, obj);
        scope = OBJ_SCOPE(obj);
        sprop = scope->lookup(id);

        
        if (!sprop) {
            clasp = LOCKED_OBJ_GET_CLASS(obj);
            resolve = clasp->resolve;
            if (resolve != JS_ResolveStub) {
                
                key.obj = obj;
                key.id = id;

                





                if (!js_StartResolving(cx, &key, JSRESFLAG_LOOKUP, &entry)) {
                    JS_UNLOCK_OBJ(cx, obj);
                    return -1;
                }
                if (!entry) {
                    
                    JS_UNLOCK_OBJ(cx, obj);
                    goto out;
                }
                generation = cx->resolvingTable->generation;

                
                *propp = NULL;

                if (clasp->flags & JSCLASS_NEW_RESOLVE) {
                    newresolve = (JSNewResolveOp)resolve;
                    if (flags == JSRESOLVE_INFER)
                        flags = js_InferFlags(cx, flags);
                    obj2 = (clasp->flags & JSCLASS_NEW_RESOLVE_GETS_START)
                           ? start
                           : NULL;
                    JS_UNLOCK_OBJ(cx, obj);

                    
                    JS_KEEP_ATOMS(cx->runtime);
                    ok = newresolve(cx, obj, ID_TO_VALUE(id), flags, &obj2);
                    JS_UNKEEP_ATOMS(cx->runtime);
                    if (!ok)
                        goto cleanup;

                    JS_LOCK_OBJ(cx, obj);
                    if (obj2) {
                        
                        if (obj2 != obj) {
                            JS_UNLOCK_OBJ(cx, obj);
                            if (OBJ_IS_NATIVE(obj2))
                                JS_LOCK_OBJ(cx, obj2);
                        }
                        protoIndex = 0;
                        for (proto = start; proto && proto != obj2;
                             proto = OBJ_GET_PROTO(cx, proto)) {
                            protoIndex++;
                        }
                        if (!OBJ_IS_NATIVE(obj2)) {
                            
                            JS_ASSERT(obj2 != obj);
                            ok = OBJ_LOOKUP_PROPERTY(cx, obj2, id, objp, propp);
                            if (!ok || *propp)
                                goto cleanup;
                            JS_LOCK_OBJ(cx, obj2);
                        } else {
                            







                            scope = OBJ_SCOPE(obj2);
                            if (scope->owned())
                                sprop = scope->lookup(id);
                        }
                        if (sprop) {
                            JS_ASSERT(scope == OBJ_SCOPE(obj2));
                            JS_ASSERT(scope->owned());
                            obj = obj2;
                        } else if (obj2 != obj) {
                            if (OBJ_IS_NATIVE(obj2))
                                JS_UNLOCK_OBJ(cx, obj2);
                            JS_LOCK_OBJ(cx, obj);
                        }
                    }
                } else {
                    



                    JS_UNLOCK_OBJ(cx, obj);
                    ok = resolve(cx, obj, ID_TO_VALUE(id));
                    if (!ok)
                        goto cleanup;
                    JS_LOCK_OBJ(cx, obj);
                    JS_ASSERT(OBJ_IS_NATIVE(obj));
                    scope = OBJ_SCOPE(obj);
                    if (scope->owned())
                        sprop = scope->lookup(id);
                }

            cleanup:
                js_StopResolving(cx, &key, JSRESFLAG_LOOKUP, entry, generation);
                if (!ok)
                    return -1;
                if (*propp)
                    return protoIndex;
            }
        }

        if (sprop) {
            SCOPE_DEPTH_ACCUM(&cx->runtime->protoLookupDepthStats, protoIndex);
            JS_ASSERT(OBJ_SCOPE(obj) == scope);
            *objp = obj;

            *propp = (JSProperty *) sprop;
            return protoIndex;
        }

        proto = LOCKED_OBJ_GET_PROTO(obj);
        JS_UNLOCK_OBJ(cx, obj);
        if (!proto)
            break;
        if (!OBJ_IS_NATIVE(proto)) {
            if (!OBJ_LOOKUP_PROPERTY(cx, proto, id, objp, propp))
                return -1;
            return protoIndex + 1;
        }

        








        JS_ASSERT_IF(OBJ_GET_CLASS(cx, obj) != &js_BlockClass,
                     OBJ_SCOPE(obj) != OBJ_SCOPE(proto));

        obj = proto;
    }

out:
    *objp = NULL;
    *propp = NULL;
    return protoIndex;
}






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    JS_ASSERT(STOBJ_GET_PARENT(obj));

    JSClass *clasp = STOBJ_GET_CLASS(obj);
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, obj->map->ops->lookupProperty == js_LookupProperty);
    return cacheable;
}

JSPropCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp)
{
    JSObject *scopeChain, *obj, *parent, *pobj;
    JSPropCacheEntry *entry;
    int scopeIndex, protoIndex;
    JSProperty *prop;

    JS_ASSERT_IF(cacheResult, !JS_ON_TRACE(cx));
    scopeChain = js_GetTopStackFrame(cx)->scopeChain;

    
    entry = JS_NO_PROP_CACHE_FILL;
    obj = scopeChain;
    parent = OBJ_GET_PARENT(cx, obj);
    for (scopeIndex = 0;
         parent
         ? IsCacheableNonGlobalScope(obj)
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
                JSClass *clasp = OBJ_GET_CLASS(cx, obj);
                JS_ASSERT(OBJ_IS_NATIVE(pobj));
                JS_ASSERT(OBJ_GET_CLASS(cx, pobj) == clasp);
                if (clasp == &js_BlockClass) {
                    



                    JS_ASSERT(pobj == obj);
                    JS_ASSERT(protoIndex == 0);
                } else {
                    
                    JS_ASSERT(!OBJ_GET_PROTO(cx, obj));
                    JS_ASSERT(protoIndex == 0);
                }
            }
#endif
            if (cacheResult) {
                entry = js_FillPropertyCache(cx, scopeChain,
                                             scopeIndex, protoIndex, pobj,
                                             (JSScopeProperty *) prop, false);
            }
            SCOPE_DEPTH_ACCUM(&rt->scopeSearchDepthStats, scopeIndex);
            goto out;
        }

        if (!parent) {
            pobj = NULL;
            goto out;
        }
        obj = parent;
        parent = OBJ_GET_PARENT(cx, obj);
    }

    for (;;) {
        if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
            return NULL;
        if (prop) {
            PCMETER(JS_PROPERTY_CACHE(cx).nofills++);
            goto out;
        }

        



        parent = OBJ_GET_PARENT(cx, obj);
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
    



    JS_ASSERT(OBJ_GET_PARENT(cx, scopeChain));
    JS_ASSERT(!JS_ON_TRACE(cx));

    JSObject *obj = scopeChain;

    





    for (int scopeIndex = 0; IsCacheableNonGlobalScope(obj); scopeIndex++) {
        JSObject *pobj;
        JSProperty *prop;
        int protoIndex = js_LookupPropertyWithFlags(cx, obj, id,
                                                    cx->resolveFlags,
                                                    &pobj, &prop);
        if (protoIndex < 0)
            return NULL;
        if (prop) {
            JS_ASSERT(OBJ_IS_NATIVE(pobj));
            JS_ASSERT(OBJ_GET_CLASS(cx, pobj) == OBJ_GET_CLASS(cx, obj));
#ifdef DEBUG
            JSPropCacheEntry *entry =
#endif
            js_FillPropertyCache(cx, scopeChain,
                                 scopeIndex, protoIndex, pobj,
                                 (JSScopeProperty *) prop, false);
            JS_ASSERT(entry);
            JS_UNLOCK_OBJ(cx, pobj);
            return obj;
        }

        
        obj = OBJ_GET_PARENT(cx, obj);
        if (!OBJ_GET_PARENT(cx, obj))
            return obj;
    }

    
    do {
        JSObject *pobj;
        JSProperty *prop;
        if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
            return NULL;
        if (prop) {
            OBJ_DROP_PROPERTY(cx, pobj, prop);
            break;
        }

        




        JSObject *parent = OBJ_GET_PARENT(cx, obj);
        if (!parent)
            break;
        obj = parent;
    } while (OBJ_GET_PARENT(cx, obj));
    return obj;
}

JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj,
             JSScopeProperty *sprop, jsval *vp)
{
    js_LeaveTraceIfGlobalObject(cx, pobj);

    JSScope *scope;
    uint32 slot;
    int32 sample;
    JSTempValueRooter tvr, tvr2;
    JSBool ok;

    JS_ASSERT(OBJ_IS_NATIVE(pobj));
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, pobj));
    scope = OBJ_SCOPE(pobj);

    slot = sprop->slot;
    *vp = (slot != SPROP_INVALID_SLOT)
          ? LOCKED_OBJ_GET_SLOT(pobj, slot)
          : JSVAL_VOID;
    if (SPROP_HAS_STUB_GETTER(sprop))
        return JS_TRUE;

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_SCOPE(cx, scope);
    JS_PUSH_TEMP_ROOT_SPROP(cx, sprop, &tvr);
    JS_PUSH_TEMP_ROOT_OBJECT(cx, pobj, &tvr2);
    ok = js_GetSprop(cx, sprop, obj, vp);
    JS_POP_TEMP_ROOT(cx, &tvr2);
    JS_POP_TEMP_ROOT(cx, &tvr);
    if (!ok)
        return JS_FALSE;

    JS_LOCK_SCOPE(cx, scope);
    if (SLOT_IN_SCOPE(slot, scope) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         scope->has(sprop))) {
        LOCKED_OBJ_SET_SLOT(pobj, slot, *vp);
    }

    return JS_TRUE;
}

JSBool
js_NativeSet(JSContext *cx, JSObject *obj, JSScopeProperty *sprop, jsval *vp)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    JSScope *scope;
    uint32 slot;
    int32 sample;
    JSTempValueRooter tvr;
    JSBool ok;

    JS_ASSERT(OBJ_IS_NATIVE(obj));
    JS_ASSERT(JS_IS_OBJ_LOCKED(cx, obj));
    scope = OBJ_SCOPE(obj);

    slot = sprop->slot;
    if (slot != SPROP_INVALID_SLOT) {
        OBJ_CHECK_SLOT(obj, slot);

        
        if (SPROP_HAS_STUB_SETTER(sprop))
            goto set_slot;
    } else {
        








        if (!(sprop->attrs & JSPROP_GETTER) && SPROP_HAS_STUB_SETTER(sprop)) {
            JS_ASSERT(!(sprop->attrs & JSPROP_SETTER));
            return JS_TRUE;
        }
    }

    sample = cx->runtime->propertyRemovals;
    JS_UNLOCK_SCOPE(cx, scope);
    JS_PUSH_TEMP_ROOT_SPROP(cx, sprop, &tvr);
    ok = js_SetSprop(cx, sprop, obj, vp);
    JS_POP_TEMP_ROOT(cx, &tvr);
    if (!ok)
        return JS_FALSE;

    JS_LOCK_SCOPE(cx, scope);
    if (SLOT_IN_SCOPE(slot, scope) &&
        (JS_LIKELY(cx->runtime->propertyRemovals == sample) ||
         scope->has(sprop))) {
  set_slot:
        LOCKED_OBJ_WRITE_SLOT(cx, obj, slot, *vp);
    }

    return JS_TRUE;
}

JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, JSBool cacheResult,
                     jsval *vp)
{
    JSObject *aobj, *obj2;
    int protoIndex;
    JSProperty *prop;
    JSScopeProperty *sprop;

    JS_ASSERT_IF(cacheResult, !JS_ON_TRACE(cx));
    
    id = js_CheckForStringIndex(id);

    aobj = js_GetProtoIfDenseArray(cx, obj);
    protoIndex = js_LookupPropertyWithFlags(cx, aobj, id, cx->resolveFlags,
                                            &obj2, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (!prop) {
        *vp = JSVAL_VOID;

        if (!OBJ_GET_CLASS(cx, obj)->getProperty(cx, obj, ID_TO_VALUE(id), vp))
            return JS_FALSE;

        PCMETER(cacheResult && JS_PROPERTY_CACHE(cx).nofills++);

        



        jsbytecode *pc;
        if (JSVAL_IS_VOID(*vp) && ((pc = js_GetCurrentBytecodePC(cx)) != NULL)) {
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
                    (op != JSOP_GETPROP && op != JSOP_GETELEM)) {
                    return JS_TRUE;
                }

                



                if (id == ATOM_TO_JSID(cx->runtime->atomState.iteratorAtom))
                    return JS_TRUE;

                
                if (cx->resolveFlags == JSRESOLVE_INFER) {
                    js_LeaveTrace(cx);
                    pc += js_CodeSpec[op].length;
                    if (Detecting(cx, pc))
                        return JS_TRUE;
                } else if (cx->resolveFlags & JSRESOLVE_DETECTING) {
                    return JS_TRUE;
                }

                flags = JSREPORT_WARNING | JSREPORT_STRICT;
            }

            
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, ID_TO_VALUE(id),
                                          NULL, NULL, NULL)) {
                return JS_FALSE;
            }
        }
        return JS_TRUE;
    }

    if (!OBJ_IS_NATIVE(obj2)) {
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        return OBJ_GET_PROPERTY(cx, obj2, id, vp);
    }

    sprop = (JSScopeProperty *) prop;

    if (cacheResult) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        js_FillPropertyCache(cx, aobj, 0, protoIndex, obj2, sprop, false);
    }

    if (!js_NativeGet(cx, obj, obj2, sprop, vp))
        return JS_FALSE;

    JS_UNLOCK_OBJ(cx, obj2);
    return JS_TRUE;
}

JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return js_GetPropertyHelper(cx, obj, id, false, vp);
}

JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, JSBool cacheResult,
             jsval *vp)
{
    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED);

    if (obj->map->ops == &js_ObjectOps ||
        obj->map->ops->getProperty == js_GetProperty) {
        return js_GetPropertyHelper(cx, obj, id, cacheResult, vp);
    }
    JS_ASSERT_IF(cacheResult, OBJ_IS_DENSE_ARRAY(cx, obj));
#if JS_HAS_XML_SUPPORT
    if (OBJECT_IS_XML(cx, obj))
        return js_GetXMLMethod(cx, obj, id, vp);
#endif
    return OBJ_GET_PROPERTY(cx, obj, id, vp);
}

JS_FRIEND_API(JSBool)
js_CheckUndeclaredVarAssignment(JSContext *cx)
{
    JSStackFrame *fp;
    if (!JS_HAS_STRICT_OPTION(cx) ||
        !(fp = js_GetTopStackFrame(cx)) ||
        !fp->regs ||
        js_GetOpcode(cx, fp->script, fp->regs->pc) != JSOP_SETNAME) {
        return JS_TRUE;
    }

    JSAtom *atom;
    GET_ATOM_FROM_BYTECODE(fp->script, fp->regs->pc, 0, atom);

    const char *bytes = js_AtomToPrintableString(cx, atom);
    return bytes &&
           JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING | JSREPORT_STRICT,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_UNDECLARED_VAR, bytes);
}





JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, JSBool cacheResult,
                     jsval *vp)
{
    int protoIndex;
    JSObject *pobj;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSScope *scope;
    uintN attrs, flags;
    intN shortid;
    JSClass *clasp;
    JSPropertyOp getter, setter;
    bool added;

    if (cacheResult)
        JS_ASSERT_NOT_ON_TRACE(cx);

    
    id = js_CheckForStringIndex(id);

    



    if (OBJ_SCOPE(obj)->sealed() && OBJ_SCOPE(obj)->object == obj) {
        flags = JSREPORT_ERROR;
        goto read_only_error;
    }

    protoIndex = js_LookupPropertyWithFlags(cx, obj, id, cx->resolveFlags,
                                            &pobj, &prop);
    if (protoIndex < 0)
        return JS_FALSE;
    if (prop) {
        if (!OBJ_IS_NATIVE(pobj)) {
            OBJ_DROP_PROPERTY(cx, pobj, prop);
            prop = NULL;
        }
    } else {
        
        JS_ASSERT(OBJ_GET_CLASS(cx, obj) != &js_BlockClass);

        if (!OBJ_GET_PARENT(cx, obj) && !js_CheckUndeclaredVarAssignment(cx))
            return JS_FALSE;
    }
    sprop = (JSScopeProperty *) prop;

    







    attrs = JSPROP_ENUMERATE;
    flags = 0;
    shortid = 0;
    clasp = OBJ_GET_CLASS(cx, obj);
    getter = clasp->getProperty;
    setter = clasp->setProperty;

    if (sprop) {
        




        scope = OBJ_SCOPE(pobj);

        attrs = sprop->attrs;
        if ((attrs & JSPROP_READONLY) ||
            (scope->sealed() && (attrs & JSPROP_SHARED))) {
            JS_UNLOCK_SCOPE(cx, scope);

            







            flags = JSREPORT_ERROR;
            if (attrs & JSPROP_READONLY) {
                if (!JS_HAS_STRICT_OPTION(cx)) {
                    
                    PCMETER(cacheResult && JS_PROPERTY_CACHE(cx).rofills++);
                    if (cacheResult)
                        TRACE_2(SetPropHit, JS_NO_PROP_CACHE_FILL, sprop);
                    return JS_TRUE;
                error: 
                    return JS_FALSE;
                }

                
                flags = JSREPORT_STRICT | JSREPORT_WARNING;
            }
            goto read_only_error;
        }

        if (pobj != obj) {
            






            JS_UNLOCK_SCOPE(cx, scope);

            
            if (attrs & JSPROP_SHARED) {
                if (cacheResult) {
                    JSPropCacheEntry *entry;
                    entry = js_FillPropertyCache(cx, obj, 0, protoIndex, pobj, sprop, false);
                    TRACE_2(SetPropHit, entry, sprop);
                }

                if (SPROP_HAS_STUB_SETTER(sprop) &&
                    !(sprop->attrs & JSPROP_GETTER)) {
                    return JS_TRUE;
                }

                return js_SetSprop(cx, sprop, obj, vp);
            }

            
            attrs = JSPROP_ENUMERATE;

            






            if (sprop->flags & SPROP_HAS_SHORTID) {
                flags = SPROP_HAS_SHORTID;
                shortid = sprop->shortid;
                getter = sprop->getter;
                setter = sprop->setter;
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
        if (clasp->flags & JSCLASS_SHARE_ALL_PROPERTIES)
            attrs |= JSPROP_SHARED;
        sprop = scope->add(cx, id, getter, setter, SPROP_INVALID_SLOT, attrs,
                           flags, shortid);
        if (!sprop) {
            JS_UNLOCK_SCOPE(cx, scope);
            return JS_FALSE;
        }

        




        if (SPROP_HAS_VALID_SLOT(sprop, scope))
            LOCKED_OBJ_SET_SLOT(obj, sprop->slot, JSVAL_VOID);

        
        ADD_PROPERTY_HELPER(cx, clasp, obj, scope, sprop, vp,
                            scope->remove(cx, id);
                            JS_UNLOCK_SCOPE(cx, scope);
                            return JS_FALSE);
        added = true;
    }

    if (cacheResult) {
        JSPropCacheEntry *entry;
        entry = js_FillPropertyCache(cx, obj, 0, 0, obj, sprop, added);
        TRACE_2(SetPropHit, entry, sprop);
    }

    if (!js_NativeSet(cx, obj, sprop, vp))
        return NULL;

    JS_UNLOCK_SCOPE(cx, scope);
    return JS_TRUE;

  read_only_error:
    return js_ReportValueErrorFlags(cx, flags, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, ID_TO_VALUE(id), NULL,
                                    NULL, NULL);
}

JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return js_SetPropertyHelper(cx, obj, id, false, vp);
}

JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp)
{
    JSBool noprop, ok;
    JSScopeProperty *sprop;

    noprop = !prop;
    if (noprop) {
        if (!js_LookupProperty(cx, obj, id, &obj, &prop))
            return JS_FALSE;
        if (!prop) {
            *attrsp = 0;
            return JS_TRUE;
        }
        if (!OBJ_IS_NATIVE(obj)) {
            ok = OBJ_GET_ATTRIBUTES(cx, obj, id, prop, attrsp);
            OBJ_DROP_PROPERTY(cx, obj, prop);
            return ok;
        }
    }
    sprop = (JSScopeProperty *)prop;
    *attrsp = sprop->attrs;
    if (noprop)
        OBJ_DROP_PROPERTY(cx, obj, prop);
    return JS_TRUE;
}

JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                 uintN *attrsp)
{
    JSBool noprop, ok;
    JSScopeProperty *sprop;

    noprop = !prop;
    if (noprop) {
        if (!js_LookupProperty(cx, obj, id, &obj, &prop))
            return JS_FALSE;
        if (!prop)
            return JS_TRUE;
        if (!OBJ_IS_NATIVE(obj)) {
            ok = OBJ_SET_ATTRIBUTES(cx, obj, id, prop, attrsp);
            OBJ_DROP_PROPERTY(cx, obj, prop);
            return ok;
        }
    }
    sprop = (JSScopeProperty *)prop;
    sprop = js_ChangeNativePropertyAttrs(cx, obj, sprop, *attrsp, 0,
                                         sprop->getter, sprop->setter);
    if (noprop)
        OBJ_DROP_PROPERTY(cx, obj, prop);
    return (sprop != NULL);
}

JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *rval)
{
    JSObject *proto;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSScope *scope;
    JSBool ok;

    *rval = JSVAL_TRUE;

    
    id = js_CheckForStringIndex(id);

    if (!js_LookupProperty(cx, obj, id, &proto, &prop))
        return JS_FALSE;
    if (!prop || proto != obj) {
        





        if (prop) {
            if (OBJ_IS_NATIVE(proto)) {
                sprop = (JSScopeProperty *)prop;
                if (SPROP_IS_SHARED_PERMANENT(sprop))
                    *rval = JSVAL_FALSE;
            }
            OBJ_DROP_PROPERTY(cx, proto, prop);
            if (*rval == JSVAL_FALSE)
                return JS_TRUE;
        }

        




        return OBJ_GET_CLASS(cx, obj)->delProperty(cx, obj, ID_TO_VALUE(id),
                                                   rval);
    }

    sprop = (JSScopeProperty *)prop;
    if (sprop->attrs & JSPROP_PERMANENT) {
        OBJ_DROP_PROPERTY(cx, obj, prop);
        *rval = JSVAL_FALSE;
        return JS_TRUE;
    }

    
    if (!LOCKED_OBJ_GET_CLASS(obj)->delProperty(cx, obj, SPROP_USERID(sprop),
                                                rval)) {
        OBJ_DROP_PROPERTY(cx, obj, prop);
        return JS_FALSE;
    }

    scope = OBJ_SCOPE(obj);
    if (SPROP_HAS_VALID_SLOT(sprop, scope))
        GC_POKE(cx, LOCKED_OBJ_GET_SLOT(obj, sprop->slot));

    ok = scope->remove(cx, id);
    OBJ_DROP_PROPERTY(cx, obj, prop);
    return ok;
}

JSBool
js_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp)
{
    jsval v, save;
    JSString *str;

    v = save = OBJECT_TO_JSVAL(obj);
    switch (hint) {
      case JSTYPE_STRING:
        




        if (OBJ_GET_CLASS(cx, obj) == &js_StringClass) {
            jsid toStringId = ATOM_TO_JSID(cx->runtime->atomState.toStringAtom);

            JS_LOCK_OBJ(cx, obj);
            JSScope *scope = OBJ_SCOPE(obj);
            JSScopeProperty *sprop = scope->lookup(toStringId);
            JSObject *pobj = obj;

            if (!sprop) {
                pobj = LOCKED_OBJ_GET_PROTO(obj);

                if (pobj && OBJ_GET_CLASS(cx, pobj) == &js_StringClass) {
                    JS_UNLOCK_SCOPE(cx, scope);
                    JS_LOCK_OBJ(cx, pobj);
                    scope = OBJ_SCOPE(pobj);
                    sprop = scope->lookup(toStringId);
                }
            }

            if (sprop &&
                SPROP_HAS_STUB_GETTER(sprop) &&
                SPROP_HAS_VALID_SLOT(sprop, scope)) {
                jsval fval = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);

                if (VALUE_IS_FUNCTION(cx, fval)) {
                    JSObject *funobj = JSVAL_TO_OBJECT(fval);
                    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

                    if (FUN_FAST_NATIVE(fun) == js_str_toString) {
                        JS_UNLOCK_SCOPE(cx, scope);
                        *vp = obj->fslots[JSSLOT_PRIVATE];
                        return JS_TRUE;
                    }
                }
            }
            JS_UNLOCK_SCOPE(cx, scope);
        }

        



        if (!js_TryMethod(cx, obj, cx->runtime->atomState.toStringAtom, 0, NULL,
                          &v)) {
            return JS_FALSE;
        }

        if (!JSVAL_IS_PRIMITIVE(v)) {
            if (!OBJ_GET_CLASS(cx, obj)->convert(cx, obj, hint, &v))
                return JS_FALSE;
        }
        break;

      default:
        if (!OBJ_GET_CLASS(cx, obj)->convert(cx, obj, hint, &v))
            return JS_FALSE;
        if (!JSVAL_IS_PRIMITIVE(v)) {
            JSType type = JS_TypeOfValue(cx, v);
            if (type == hint ||
                (type == JSTYPE_FUNCTION && hint == JSTYPE_OBJECT)) {
                goto out;
            }
            if (!js_TryMethod(cx, obj, cx->runtime->atomState.toStringAtom, 0,
                              NULL, &v)) {
                return JS_FALSE;
            }
        }
        break;
    }
    if (!JSVAL_IS_PRIMITIVE(v)) {
        
        if (hint == JSTYPE_STRING) {
            str = JS_InternString(cx, OBJ_GET_CLASS(cx, obj)->name);
            if (!str)
                return JS_FALSE;
        } else {
            str = NULL;
        }
        *vp = OBJECT_TO_JSVAL(obj);
        js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO,
                             JSDVG_SEARCH_STACK, save, str,
                             (hint == JSTYPE_VOID)
                             ? "primitive type"
                             : JS_TYPE_STR(hint));
        return JS_FALSE;
    }
out:
    *vp = v;
    return JS_TRUE;
}














struct JSNativeEnumerator {
    





    jsword                  cursor;

    uint32                  length;     
    uint32                  shape;      
    JSNativeEnumerator      *next;      
    jsid                    ids[1];     
};


JS_STATIC_ASSERT((jsuword) SHAPE_OVERFLOW_BIT <=
                 ((jsuword) 1 << (JS_BITS_PER_WORD - 1)));

static inline size_t
NativeEnumeratorSize(uint32 length)
{
    JS_ASSERT(length != 0);
    return offsetof(JSNativeEnumerator, ids) + (size_t) length * sizeof(jsid);
}






JSBool
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             jsval *statep, jsid *idp)
{
    JSClass *clasp;
    JSEnumerateOp enumerate;
    JSNativeEnumerator *ne;
    uint32 length, shape;
    size_t allocated;
    JSScope *scope;
    jsuword *cachep, oldcache;
    JSScopeProperty *sprop;
    jsid *ids;
    jsword newcursor;

    clasp = OBJ_GET_CLASS(cx, obj);
    enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((JSNewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
    }

    switch (enum_op) {
      case JSENUMERATE_INIT:
        if (!enumerate(cx, obj))
            return JS_FALSE;

        









        ne = NULL;
        length = 0;
        allocated = (size_t) 0;
        JS_LOCK_OBJ(cx, obj);
        scope = OBJ_SCOPE(obj);
        do {
            ENUM_CACHE_METER(nativeEnumProbes);
            shape = scope->shape;
            JS_ASSERT(shape < SHAPE_OVERFLOW_BIT);
            cachep = &cx->runtime->
                     nativeEnumCache[NATIVE_ENUM_CACHE_HASH(shape)];
            oldcache = *cachep;
            if (oldcache & (jsuword) 1) {
                if ((uint32) (oldcache >> 1) == shape) {
                    
                    break;
                }
            } else if (oldcache != (jsuword) 0) {
                




                ne = (JSNativeEnumerator *) *cachep;
                JS_ASSERT(ne->length >= 1);
                if (ne->shape == shape) {
                    



                    length = ne->length;
                    if (js_CompareAndSwap(&ne->cursor, 0, length))
                        break;
                    length = 0;
                }
                ne = NULL;
            }
            ENUM_CACHE_METER(nativeEnumMisses);

            
            JS_ASSERT(length == 0);
            for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
                if ((sprop->attrs & JSPROP_ENUMERATE) &&
                    !(sprop->flags & SPROP_IS_ALIAS) &&
                    (!scope->hadMiddleDelete() || scope->has(sprop))) {
                    length++;
                }
            }
            if (length == 0) {
                
                *cachep = ((jsuword) shape << 1) | (jsuword) 1;
                break;
            }

            allocated = NativeEnumeratorSize(length);
            ne = (JSNativeEnumerator *) cx->malloc(allocated);
            if (!ne) {
                JS_UNLOCK_SCOPE(cx, scope);
                return JS_FALSE;
            }
            ne->cursor = length;
            ne->length = length;
            ne->shape = shape;
            ids = ne->ids;
            for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
                if ((sprop->attrs & JSPROP_ENUMERATE) &&
                    !(sprop->flags & SPROP_IS_ALIAS) &&
                    (!scope->hadMiddleDelete() || scope->has(sprop))) {
                    JS_ASSERT(ids < ne->ids + length);
                    *ids++ = sprop->id;
                }
            }
            JS_ASSERT(ids == ne->ids + length);
        } while (0);
        JS_UNLOCK_SCOPE(cx, scope);

        if (!ne) {
            JS_ASSERT(length == 0);
            JS_ASSERT(allocated == 0);
            *statep = JSVAL_ZERO;
        } else {
            JS_ASSERT(length != 0);
            JS_ASSERT(ne->cursor == (jsword) length);
            if (allocated != 0) {
                JS_LOCK_GC(cx->runtime);
                ne->next = cx->runtime->nativeEnumerators;
                cx->runtime->nativeEnumerators = ne;
                JS_ASSERT(((jsuword) ne & (jsuword) 1) == (jsuword) 0);
                *cachep = (jsuword) ne;
                JS_UNLOCK_GC(cx->runtime);
            }
            *statep = PRIVATE_TO_JSVAL(ne);
        }
        if (idp)
            *idp = INT_TO_JSVAL(length);
        break;

      case JSENUMERATE_NEXT:
      case JSENUMERATE_DESTROY:
        if (*statep == JSVAL_ZERO) {
            *statep = JSVAL_NULL;
            break;
        }
        ne = (JSNativeEnumerator *) JSVAL_TO_PRIVATE(*statep);
        JS_ASSERT(ne->length >= 1);
        JS_ASSERT(ne->cursor >= 1);

        





        if (enum_op == JSENUMERATE_NEXT) {
            newcursor = ne->cursor - 1;
            *idp = ne->ids[newcursor];
            ne->cursor = newcursor;
            if (newcursor == 0)
                *statep = JSVAL_ZERO;
        } else {
            
            JS_ASSERT(enum_op == JSENUMERATE_DESTROY);
            ne->cursor = 0;

            




            if (cx->runtime->state == JSRTS_LANDING)
                cx->runtime->gcPoke = true;
        }
        break;
    }
    return JS_TRUE;
}

void
js_TraceNativeEnumerators(JSTracer *trc)
{
    JSRuntime *rt;
    JSNativeEnumerator **nep, *ne;
    jsid *cursor, *end;

    




    rt = trc->context->runtime;
    bool doGC = IS_GC_MARKING_TRACER(trc) &&
                (rt->gcRegenShapes || rt->state == JSRTS_LANDING);

    if (doGC) {
        memset(&rt->nativeEnumCache, 0, sizeof rt->nativeEnumCache);
#ifdef JS_DUMP_ENUM_CACHE_STATS
        printf("nativeEnumCache hit rate %g%%\n",
               100.0 * (rt->nativeEnumProbes - rt->nativeEnumMisses) /
               rt->nativeEnumProbes);
#endif
    }

    nep = &rt->nativeEnumerators;
    while ((ne = *nep) != NULL) {
        JS_ASSERT(ne->length != 0);
        if (ne->cursor != 0) {
            
            cursor = ne->ids;
            end = cursor + ne->length;
            do {
                js_TraceId(trc, *cursor);
            } while (++cursor != end);
        } else if (doGC) {
            *nep = ne->next;
            trc->context->free(ne);
            continue;
        }
        nep = &ne->next;
    }
}

JSBool
js_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               jsval *vp, uintN *attrsp)
{
    JSBool writing;
    JSObject *pobj;
    JSProperty *prop;
    JSClass *clasp;
    JSScopeProperty *sprop;
    JSSecurityCallbacks *callbacks;
    JSCheckAccessOp check;

    writing = (mode & JSACC_WRITE) != 0;
    switch (mode & JSACC_TYPEMASK) {
      case JSACC_PROTO:
        pobj = obj;
        if (!writing)
            *vp = OBJECT_TO_JSVAL(OBJ_GET_PROTO(cx, obj));
        *attrsp = JSPROP_PERMANENT;
        break;

      case JSACC_PARENT:
        JS_ASSERT(!writing);
        pobj = obj;
        *vp = OBJECT_TO_JSVAL(OBJ_GET_PARENT(cx, obj));
        *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
        break;

      default:
        if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop))
            return JS_FALSE;
        if (!prop) {
            if (!writing)
                *vp = JSVAL_VOID;
            *attrsp = 0;
            pobj = obj;
            break;
        }

        if (!OBJ_IS_NATIVE(pobj)) {
            OBJ_DROP_PROPERTY(cx, pobj, prop);

            
            if (pobj->map->ops->checkAccess == js_CheckAccess) {
                if (!writing) {
                    *vp = JSVAL_VOID;
                    *attrsp = 0;
                }
                break;
            }
            return OBJ_CHECK_ACCESS(cx, pobj, id, mode, vp, attrsp);
        }

        sprop = (JSScopeProperty *)prop;
        *attrsp = sprop->attrs;
        if (!writing) {
            *vp = (SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(pobj)))
                  ? LOCKED_OBJ_GET_SLOT(pobj, sprop->slot)
                  : JSVAL_VOID;
        }
        OBJ_DROP_PROPERTY(cx, pobj, prop);
    }

    











    clasp = OBJ_GET_CLASS(cx, pobj);
    check = clasp->checkAccess;
    if (!check) {
        callbacks = JS_GetSecurityCallbacks(cx);
        check = callbacks ? callbacks->checkObjectAccess : NULL;
    }
    return !check || check(cx, pobj, ID_TO_VALUE(id), mode, vp);
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
GetCurrentExecutionContext(JSContext *cx, JSObject *obj, jsval *rval)
{
    JSObject *tmp;
    jsval xcval;

    while ((tmp = OBJ_GET_PARENT(cx, obj)) != NULL)
        obj = tmp;
    if (!OBJ_GET_PROPERTY(cx, obj,
                          ATOM_TO_JSID(cx->runtime->atomState
                                       .ExecutionContextAtom),
                          &xcval)) {
        return JS_FALSE;
    }
    if (JSVAL_IS_PRIMITIVE(xcval)) {
        JS_ReportError(cx, "invalid ExecutionContext in global object");
        return JS_FALSE;
    }
    if (!OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(xcval),
                          ATOM_TO_JSID(cx->runtime->atomState.currentAtom),
                          rval)) {
        return JS_FALSE;
    }
    return JS_TRUE;
}
#endif

JSBool
js_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[-2]));
    if (!clasp->call) {
#ifdef NARCISSUS
        JSObject *callee, *args;
        jsval fval, nargv[3];
        JSBool ok;

        callee = JSVAL_TO_OBJECT(argv[-2]);
        if (!OBJ_GET_PROPERTY(cx, callee,
                              ATOM_TO_JSID(cx->runtime->atomState.__call__Atom),
                              &fval)) {
            return JS_FALSE;
        }
        if (VALUE_IS_FUNCTION(cx, fval)) {
            if (!GetCurrentExecutionContext(cx, obj, &nargv[2]))
                return JS_FALSE;
            args = js_GetArgsObject(cx, js_GetTopStackFrame(cx));
            if (!args)
                return JS_FALSE;
            nargv[0] = OBJECT_TO_JSVAL(obj);
            nargv[1] = OBJECT_TO_JSVAL(args);
            return js_InternalCall(cx, callee, fval, 3, nargv, rval);
        }
        if (JSVAL_IS_OBJECT(fval) && JSVAL_TO_OBJECT(fval) != callee) {
            argv[-2] = fval;
            ok = js_Call(cx, obj, argc, argv, rval);
            argv[-2] = OBJECT_TO_JSVAL(callee);
            return ok;
        }
#endif
        js_ReportIsNotFunction(cx, &argv[-2], js_GetTopStackFrame(cx)->flags & JSFRAME_ITERATOR);
        return JS_FALSE;
    }
    return clasp->call(cx, obj, argc, argv, rval);
}

JSBool
js_Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
             jsval *rval)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[-2]));
    if (!clasp->construct) {
#ifdef NARCISSUS
        JSObject *callee, *args;
        jsval cval, nargv[2];
        JSBool ok;

        callee = JSVAL_TO_OBJECT(argv[-2]);
        if (!OBJ_GET_PROPERTY(cx, callee,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .__construct__Atom),
                              &cval)) {
            return JS_FALSE;
        }
        if (VALUE_IS_FUNCTION(cx, cval)) {
            if (!GetCurrentExecutionContext(cx, obj, &nargv[1]))
                return JS_FALSE;
            args = js_GetArgsObject(cx, js_GetTopStackFrame(cx));
            if (!args)
                return JS_FALSE;
            nargv[0] = OBJECT_TO_JSVAL(args);
            return js_InternalCall(cx, callee, cval, 2, nargv, rval);
        }
        if (JSVAL_IS_OBJECT(cval) && JSVAL_TO_OBJECT(cval) != callee) {
            argv[-2] = cval;
            ok = js_Call(cx, obj, argc, argv, rval);
            argv[-2] = OBJECT_TO_JSVAL(callee);
            return ok;
        }
#endif
        js_ReportIsNotFunction(cx, &argv[-2], JSV2F_CONSTRUCT);
        return JS_FALSE;
    }
    return clasp->construct(cx, obj, argc, argv, rval);
}

JSBool
js_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, obj);
    if (clasp->hasInstance)
        return clasp->hasInstance(cx, obj, v, bp);
#ifdef NARCISSUS
    {
        jsval fval, rval;

        if (!OBJ_GET_PROPERTY(cx, obj,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .__hasInstance__Atom),
                              &fval)) {
            return JS_FALSE;
        }
        if (VALUE_IS_FUNCTION(cx, fval)) {
            if (!js_InternalCall(cx, obj, fval, 1, &v, &rval))
                return JS_FALSE;
            *bp = js_ValueToBoolean(rval);
            return JS_TRUE;
        }
    }
#endif
    js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                        JSDVG_SEARCH_STACK, OBJECT_TO_JSVAL(obj), NULL);
    return JS_FALSE;
}

JSBool
js_IsDelegate(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    JSObject *obj2;

    *bp = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(v))
        return JS_TRUE;
    obj2 = JSVAL_TO_OBJECT(v);
    while ((obj2 = OBJ_GET_PROTO(cx, obj2)) != NULL) {
        if (obj2 == obj) {
            *bp = JS_TRUE;
            break;
        }
    }
    return JS_TRUE;
}

JSBool
js_GetClassPrototype(JSContext *cx, JSObject *scope, jsid id,
                     JSObject **protop)
{
    jsval v;
    JSObject *ctor;

    if (!js_FindClassObject(cx, scope, id, &v))
        return JS_FALSE;
    if (VALUE_IS_FUNCTION(cx, v)) {
        ctor = JSVAL_TO_OBJECT(v);
        if (!OBJ_GET_PROPERTY(cx, ctor,
                              ATOM_TO_JSID(cx->runtime->atomState
                                           .classPrototypeAtom),
                              &v)) {
            return JS_FALSE;
        }
        if (!JSVAL_IS_PRIMITIVE(v)) {
            








            cx->weakRoots.newborn[GCX_OBJECT] = JSVAL_TO_GCTHING(v);
        }
    }
    *protop = JSVAL_IS_OBJECT(v) ? JSVAL_TO_OBJECT(v) : NULL;
    return JS_TRUE;
}
















static JSBool
CheckCtorGetAccess(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSAtom *atom;
    uintN attrs;

    atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    return OBJ_CHECK_ACCESS(cx, obj, ATOM_TO_JSID(atom), JSACC_READ,
                            vp, &attrs);
}

static JSBool
CheckCtorSetAccess(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSAtom *atom;
    uintN attrs;

    atom = cx->runtime->atomState.constructorAtom;
    JS_ASSERT(id == ATOM_TO_JSID(atom));
    return OBJ_CHECK_ACCESS(cx, obj, ATOM_TO_JSID(atom), JSACC_WRITE,
                            vp, &attrs);
}

JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto,
                     uintN attrs)
{
    





    if (!OBJ_DEFINE_PROPERTY(cx, ctor,
                             ATOM_TO_JSID(cx->runtime->atomState
                                          .classPrototypeAtom),
                             OBJECT_TO_JSVAL(proto),
                             JS_PropertyStub, JS_PropertyStub,
                             attrs, NULL)) {
        return JS_FALSE;
    }

    



    return OBJ_DEFINE_PROPERTY(cx, proto,
                               ATOM_TO_JSID(cx->runtime->atomState
                                            .constructorAtom),
                               OBJECT_TO_JSVAL(ctor),
                               CheckCtorGetAccess, CheckCtorSetAccess,
                               0, NULL);
}

JSBool
js_PrimitiveToObject(JSContext *cx, jsval *vp)
{
    JSClass *clasp;
    JSObject *obj;

    
    JS_STATIC_ASSERT(JSVAL_INT == 1);
    JS_STATIC_ASSERT(JSVAL_DOUBLE == 2);
    JS_STATIC_ASSERT(JSVAL_STRING == 4);
    JS_STATIC_ASSERT(JSVAL_BOOLEAN == 6);
    static JSClass *const PrimitiveClasses[] = {
        &js_NumberClass,    
        &js_NumberClass,    
        &js_NumberClass,    
        &js_StringClass,    
        &js_NumberClass,    
        &js_BooleanClass,   
        &js_NumberClass     
    };

    JS_ASSERT(!JSVAL_IS_OBJECT(*vp));
    JS_ASSERT(!JSVAL_IS_VOID(*vp));
    clasp = PrimitiveClasses[JSVAL_TAG(*vp) - 1];
    obj = js_NewObject(cx, clasp, NULL, NULL);
    if (!obj)
        return JS_FALSE;
    obj->fslots[JSSLOT_PRIVATE] = *vp;
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

JSBool
js_ValueToObject(JSContext *cx, jsval v, JSObject **objp)
{
    JSObject *obj;

    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v)) {
        obj = NULL;
    } else if (JSVAL_IS_OBJECT(v)) {
        obj = JSVAL_TO_OBJECT(v);
        if (!OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_OBJECT, &v))
            return JS_FALSE;
        if (!JSVAL_IS_PRIMITIVE(v))
            obj = JSVAL_TO_OBJECT(v);
    } else {
        if (!js_PrimitiveToObject(cx, &v))
            return JS_FALSE;
        obj = JSVAL_TO_OBJECT(v);
    }
    *objp = obj;
    return JS_TRUE;
}

JSObject *
js_ValueToNonNullObject(JSContext *cx, jsval v)
{
    JSObject *obj;

    if (!js_ValueToObject(cx, v, &obj))
        return NULL;
    if (!obj)
        js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, v, NULL);
    return obj;
}

JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, jsval *rval)
{
    jsval argv[1];

    argv[0] = ATOM_KEY(cx->runtime->atomState.typeAtoms[type]);
    return js_TryMethod(cx, obj, cx->runtime->atomState.valueOfAtom, 1, argv,
                        rval);
}

JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, jsval *argv, jsval *rval)
{
    JSErrorReporter older;
    jsid id;
    jsval fval;
    JSBool ok;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    




    older = JS_SetErrorReporter(cx, NULL);
    id = ATOM_TO_JSID(atom);
    fval = JSVAL_VOID;
    ok = js_GetMethod(cx, obj, id, false, &fval);
    if (!ok)
        JS_ClearPendingException(cx);
    JS_SetErrorReporter(cx, older);

    if (JSVAL_IS_PRIMITIVE(fval))
        return JS_TRUE;
    return js_InternalCall(cx, obj, fval, argc, argv, rval);
}

#if JS_HAS_XDR

JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp)
{
    JSContext *cx;
    JSAtom *atom;
    JSClass *clasp;
    uint32 classId, classDef;
    JSProtoKey protoKey;
    jsid classKey;
    JSObject *proto;

    cx = xdr->cx;
    atom = NULL;
    if (xdr->mode == JSXDR_ENCODE) {
        clasp = OBJ_GET_CLASS(cx, *objp);
        classId = JS_XDRFindClassIdByName(xdr, clasp->name);
        classDef = !classId;
        if (classDef) {
            if (!JS_XDRRegisterClass(xdr, clasp, &classId))
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
            classKey = (protoKey != JSProto_Null)
                       ? INT_TO_JSID(protoKey)
                       : ATOM_TO_JSID(atom);
            if (!js_GetClassPrototype(cx, NULL, classKey, &proto))
                return JS_FALSE;
            clasp = OBJ_GET_CLASS(cx, proto);
            if (!JS_XDRRegisterClass(xdr, clasp, &classId))
                return JS_FALSE;
        } else {
            clasp = JS_XDRFindClassById(xdr, classId);
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
    JSObject *obj;
    uint32 slot;
    JSScope *scope;
    jsval nval;
    JSScopeProperty *sprop;
    JSClass *clasp;
    uint32 key;
    const char *slotname;

    JS_ASSERT(trc->debugPrinter == js_PrintObjectSlotName);
    obj = (JSObject *)trc->debugPrintArg;
    slot = (uint32)trc->debugPrintIndex;

    if (OBJ_IS_NATIVE(obj)) {
        scope = OBJ_SCOPE(obj);
        sprop = SCOPE_LAST_PROP(scope);
        while (sprop && sprop->slot != slot)
            sprop = sprop->parent;
    } else {
        sprop = NULL;
    }

    if (!sprop) {
        switch (slot) {
          case JSSLOT_PROTO:
            JS_snprintf(buf, bufsize, "__proto__");
            break;
          case JSSLOT_PARENT:
            JS_snprintf(buf, bufsize, "__parent__");
            break;
          default:
            slotname = NULL;
            clasp = LOCKED_OBJ_GET_CLASS(obj);
            if (clasp->flags & JSCLASS_IS_GLOBAL) {
                key = slot - JSSLOT_START(clasp);
#define JS_PROTO(name,code,init) \
    if ((code) == key) { slotname = js_##name##_str; goto found; }
#include "jsproto.tbl"
#undef JS_PROTO
            }
          found:
            if (slotname)
                JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
            else
                JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
            break;
        }
    } else {
        nval = ID_TO_VALUE(sprop->id);
        if (JSVAL_IS_INT(nval)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSVAL_TO_INT(nval));
        } else if (JSVAL_IS_STRING(nval)) {
            js_PutEscapedString(buf, bufsize, JSVAL_TO_STRING(nval), 0);
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}
#endif

void
js_TraceObject(JSTracer *trc, JSObject *obj)
{
    JSContext *cx;
    JSScope *scope;
    JSClass *clasp;
    size_t nslots, i;
    jsval v;

    JS_ASSERT(OBJ_IS_NATIVE(obj));
    cx = trc->context;
    scope = OBJ_SCOPE(obj);

    if (scope->owned() && IS_GC_MARKING_TRACER(trc)) {
        




        size_t slots = scope->freeslot;
        if (STOBJ_NSLOTS(obj) != slots)
            js_ShrinkSlots(cx, obj, slots);
    }

#ifdef JS_DUMP_SCOPE_METERS
    MeterEntryCount(scope->entryCount);
#endif

    scope->trace(trc);

    if (!JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList))
        js_TraceWatchPoints(trc, obj);

    
    clasp = LOCKED_OBJ_GET_CLASS(obj);
    if (clasp->mark) {
        if (clasp->flags & JSCLASS_MARK_IS_TRACE)
            ((JSTraceOp) clasp->mark)(trc, obj);
        else if (IS_GC_MARKING_TRACER(trc))
            (void) clasp->mark(cx, obj, trc);
    }

    








    nslots = STOBJ_NSLOTS(obj);
    if (scope->owned() && scope->freeslot < nslots)
        nslots = scope->freeslot;

    for (i = 0; i != nslots; ++i) {
        v = STOBJ_GET_SLOT(obj, i);
        if (JSVAL_IS_TRACEABLE(v)) {
            JS_SET_TRACING_DETAILS(trc, js_PrintObjectSlotName, obj, i);
            JS_CallTracer(trc, JSVAL_TO_TRACEABLE(v), JSVAL_TRACE_KIND(v));
        }
    }
}

void
js_Clear(JSContext *cx, JSObject *obj)
{
    JSScope *scope;
    uint32 i, n;

    




    JS_LOCK_OBJ(cx, obj);
    scope = OBJ_SCOPE(obj);
    if (scope->owned()) {
        
        scope->clear(cx);

        
        i = STOBJ_NSLOTS(obj);
        n = JSSLOT_FREE(LOCKED_OBJ_GET_CLASS(obj));
        while (--i >= n)
            STOBJ_SET_SLOT(obj, i, JSVAL_VOID);
        scope->freeslot = n;
    }
    JS_UNLOCK_OBJ(cx, obj);
}

jsval
js_GetRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot)
{
    jsval v;

    JS_LOCK_OBJ(cx, obj);
    v = (slot < STOBJ_NSLOTS(obj)) ? STOBJ_GET_SLOT(obj, slot) : JSVAL_VOID;
    JS_UNLOCK_OBJ(cx, obj);
    return v;
}

JSBool
js_SetRequiredSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval v)
{
    JSScope *scope;
    uint32 nslots;
    JSClass *clasp;

    JS_LOCK_OBJ(cx, obj);
    scope = OBJ_SCOPE(obj);
    if (slot >= JS_INITIAL_NSLOTS && !obj->dslots) {
        




        clasp = LOCKED_OBJ_GET_CLASS(obj);
        nslots = JSSLOT_FREE(clasp);
        if (clasp->reserveSlots)
            nslots += clasp->reserveSlots(cx, obj);
        JS_ASSERT(slot < nslots);
        if (!AllocSlots(cx, obj, nslots)) {
            JS_UNLOCK_SCOPE(cx, scope);
            return JS_FALSE;
        }
    }

    







    if (scope->owned() && slot >= scope->freeslot)
        scope->freeslot = slot + 1;

    STOBJ_SET_SLOT(obj, slot, v);
    GC_POKE(cx, JS_NULL);
    JS_UNLOCK_SCOPE(cx, scope);
    return JS_TRUE;
}

JSObject *
js_GetWrappedObject(JSContext *cx, JSObject *obj)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, obj);
    if (clasp->flags & JSCLASS_IS_EXTENDED) {
        JSExtendedClass *xclasp;
        JSObject *obj2;

        xclasp = (JSExtendedClass *)clasp;
        if (xclasp->wrappedObject && (obj2 = xclasp->wrappedObject(cx, obj)))
            return obj2;
    }
    return obj;
}

JSBool
js_IsCallable(JSObject *obj, JSContext *cx)
{
    if (!OBJ_IS_NATIVE(obj))
        return obj->map->ops->call != NULL;

    JS_LOCK_OBJ(cx, obj);
    JSBool callable = (obj->map->ops == &js_ObjectOps)
                      ? HAS_FUNCTION_CLASS(obj) || STOBJ_GET_CLASS(obj)->call
                      : obj->map->ops->call != NULL;
    JS_UNLOCK_OBJ(cx, obj);
    return callable;
}

void
js_ReportGetterOnlyAssignment(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_GETTER_ONLY, NULL);
}


JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    js_ReportGetterOnlyAssignment(cx);
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
    js_DumpValue(ATOM_KEY(atom));
}

void
dumpValue(jsval val)
{
    if (JSVAL_IS_NULL(val)) {
        fprintf(stderr, "null");
    } else if (JSVAL_IS_VOID(val)) {
        fprintf(stderr, "undefined");
    } else if (JSVAL_IS_OBJECT(val) &&
               HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(val))) {
        JSObject *funobj = JSVAL_TO_OBJECT(val);
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
        fprintf(stderr, "<%s %s at %p (JSFunction at %p)>",
                fun->atom ? "function" : "unnamed",
                fun->atom ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom)) : "function",
                (void *) funobj,
                (void *) fun);
    } else if (JSVAL_IS_OBJECT(val)) {
        JSObject *obj = JSVAL_TO_OBJECT(val);
        JSClass *cls = STOBJ_GET_CLASS(obj);
        fprintf(stderr, "<%s%s at %p>",
                cls->name,
                cls == &js_ObjectClass ? "" : " object",
                (void *) obj);
    } else if (JSVAL_IS_INT(val)) {
        fprintf(stderr, "%d", JSVAL_TO_INT(val));
    } else if (JSVAL_IS_STRING(val)) {
        dumpString(JSVAL_TO_STRING(val));
    } else if (JSVAL_IS_DOUBLE(val)) {
        fprintf(stderr, "%g", *JSVAL_TO_DOUBLE(val));
    } else if (val == JSVAL_TRUE) {
        fprintf(stderr, "true");
    } else if (val == JSVAL_FALSE) {
        fprintf(stderr, "false");
    } else if (val == JSVAL_HOLE) {
        fprintf(stderr, "hole");
    } else {
        
        fprintf(stderr, "unrecognized jsval %p", (void *) val);
    }
}

JS_FRIEND_API(void)
js_DumpValue(jsval val)
{
    fprintf(stderr, "jsval %p = ", (void *) val);
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void *) id);
    dumpValue(ID_TO_VALUE(id));
    fputc('\n', stderr);
}

static void
dumpScopeProp(JSScopeProperty *sprop)
{
    jsid id = sprop->id;
    uint8 attrs = sprop->attrs;

    fprintf(stderr, "    ");
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_GETTER) fprintf(stderr, "getter ");
    if (attrs & JSPROP_SETTER) fprintf(stderr, "setter ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");
    if (sprop->flags & SPROP_IS_ALIAS) fprintf(stderr, "alias ");
    if (JSID_IS_ATOM(id))
        dumpString(JSVAL_TO_STRING(ID_TO_VALUE(id)));
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) id);
    fprintf(stderr, ": slot %d", sprop->slot);
    fprintf(stderr, "\n");
}

JS_FRIEND_API(void)
js_DumpObject(JSObject *obj)
{
    uint32 i, slots;
    JSClass *clasp;
    jsuint reservedEnd;
    bool sharesScope = false;

    fprintf(stderr, "object %p\n", (void *) obj);
    clasp = STOBJ_GET_CLASS(obj);
    fprintf(stderr, "class %p %s\n", (void *)clasp, clasp->name);

    
    if (OBJ_IS_DENSE_ARRAY(BOGUS_CX, obj)) {
        slots = JS_MIN((jsuint) obj->fslots[JSSLOT_ARRAY_LENGTH],
                       js_DenseArrayCapacity(obj));
        fprintf(stderr, "elements\n");
        for (i = 0; i < slots; i++) {
            fprintf(stderr, " %3d: ", i);
            dumpValue(obj->dslots[i]);
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        return;
    }

    if (OBJ_IS_NATIVE(obj)) {
        JSScope *scope = OBJ_SCOPE(obj);
        if (scope->sealed())
            fprintf(stderr, "sealed\n");

        fprintf(stderr, "properties:\n");
        for (JSScopeProperty *sprop = SCOPE_LAST_PROP(scope); sprop;
             sprop = sprop->parent) {
            if (!scope->hadMiddleDelete() || scope->has(sprop))
                dumpScopeProp(sprop);
        }
    } else {
        if (!OBJ_IS_NATIVE(obj))
            fprintf(stderr, "not native\n");
    }

    fprintf(stderr, "proto ");
    dumpValue(OBJECT_TO_JSVAL(STOBJ_GET_PROTO(obj)));
    fputc('\n', stderr);

    fprintf(stderr, "parent ");
    dumpValue(OBJECT_TO_JSVAL(STOBJ_GET_PARENT(obj)));
    fputc('\n', stderr);

    i = JSSLOT_PRIVATE;
    if (clasp->flags & JSCLASS_HAS_PRIVATE) {
        i = JSSLOT_PRIVATE + 1;
        fprintf(stderr, "private %p\n", obj->getPrivate());
    }

    fprintf(stderr, "slots:\n");
    reservedEnd = i + JSCLASS_RESERVED_SLOTS(clasp);
    slots = (OBJ_IS_NATIVE(obj) && !sharesScope)
            ? OBJ_SCOPE(obj)->freeslot
            : STOBJ_NSLOTS(obj);
    for (; i < slots; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(STOBJ_GET_SLOT(obj, i));
        fputc('\n', stderr);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char *name, JSObject *obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(OBJECT_TO_JSVAL(obj));
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js_DumpStackFrame(JSStackFrame *fp)
{
    jsval *sp = NULL;

    for (; fp; fp = fp->down) {
        fprintf(stderr, "JSStackFrame at %p\n", (void *) fp);
        if (fp->callee)
            dumpValue(OBJECT_TO_JSVAL(fp->callee));
        else
            fprintf(stderr, "global frame, no callee");
        fputc('\n', stderr);

        if (fp->script)
            fprintf(stderr, "file %s line %u\n", fp->script->filename, (unsigned) fp->script->lineno);

        if (fp->regs) {
            if (!fp->regs->pc) {
                fprintf(stderr, "*** regs && !regs->pc, skipping frame\n\n");
                continue;
            }
            if (!fp->script) {
                fprintf(stderr, "*** regs && !script, skipping frame\n\n");
                continue;
            }
            jsbytecode *pc = fp->regs->pc;
            sp = fp->regs->sp;
            if (fp->imacpc) {
                fprintf(stderr, "  pc in imacro at %p\n  called from ", pc);
                pc = fp->imacpc;
            } else {
                fprintf(stderr, "  ");
            }
            fprintf(stderr, "pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
        }
        if (sp && fp->slots) {
            fprintf(stderr, "  slots: %p\n", (void *) fp->slots);
            fprintf(stderr, "  sp:    %p = slots + %u\n", (void *) sp, (unsigned) (sp - fp->slots));
            if (sp - fp->slots < 10000) { 
                for (jsval *p = fp->slots; p < sp; p++) {
                    fprintf(stderr, "    %p: ", (void *) p);
                    dumpValue(*p);
                    fputc('\n', stderr);
                }
            }
        } else {
            fprintf(stderr, "  sp:    %p\n", (void *) sp);
            fprintf(stderr, "  slots: %p\n", (void *) fp->slots);
        }
        fprintf(stderr, "  argv:  %p (argc: %u)\n", (void *) fp->argv, (unsigned) fp->argc);
        MaybeDumpObject("callobj", fp->callobj);
        MaybeDumpObject("argsobj", JSVAL_TO_OBJECT(fp->argsobj));
        MaybeDumpObject("varobj", fp->varobj);
        MaybeDumpObject("this", fp->thisp);
        fprintf(stderr, "  rval: ");
        dumpValue(fp->rval);
        fputc('\n', stderr);

        fprintf(stderr, "  flags:");
        if (fp->flags == 0)
            fprintf(stderr, " none");
        if (fp->flags & JSFRAME_CONSTRUCTING)
            fprintf(stderr, " constructing");
        if (fp->flags & JSFRAME_COMPUTED_THIS)
            fprintf(stderr, " computed_this");
        if (fp->flags & JSFRAME_ASSIGNING)
            fprintf(stderr, " assigning");
        if (fp->flags & JSFRAME_DEBUGGER)
            fprintf(stderr, " debugger");
        if (fp->flags & JSFRAME_EVAL)
            fprintf(stderr, " eval");
        if (fp->flags & JSFRAME_ROOTED_ARGV)
            fprintf(stderr, " rooted_argv");
        if (fp->flags & JSFRAME_YIELDING)
            fprintf(stderr, " yielding");
        if (fp->flags & JSFRAME_ITERATOR)
            fprintf(stderr, " iterator");
        if (fp->flags & JSFRAME_GENERATOR)
            fprintf(stderr, " generator");
        if ((fp->flags >> JSFRAME_OVERRIDE_SHIFT) & JS_BITMASK(JSFRAME_OVERRIDE_BITS))
            fprintf(stderr, " override_bits(0x%x)", (fp->flags >> JSFRAME_OVERRIDE_SHIFT) & JS_BITMASK(JSFRAME_OVERRIDE_BITS));
        fputc('\n', stderr);

        if (fp->scopeChain)
            fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) fp->scopeChain);
        if (fp->blockChain)
            fprintf(stderr, "  blockChain: (JSObject *) %p\n", (void *) fp->blockChain);

        if (fp->sharpDepth)
            fprintf(stderr, "  sharpDepth: %u\n", (unsigned) fp->sharpDepth);
        if (fp->xmlNamespace)
            fprintf(stderr, "  xmlNamespace: (JSObject *) %p\n", (void *) fp->xmlNamespace);

        if (fp->dormantNext)
            fprintf(stderr, "  dormantNext: (JSStackFrame *) %p\n", (void *) fp->dormantNext);
        if (fp->displaySave)
            fprintf(stderr, "  displaySave: (JSStackFrame *) %p\n", (void *) fp->displaySave);

        fputc('\n', stderr);
    }
}

#endif

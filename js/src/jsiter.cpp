










































#include <string.h>     
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsarena.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsproxy.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "jsvector.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jscntxtinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

using namespace js;

static void iterator_finalize(JSContext *cx, JSObject *obj);
static void iterator_trace(JSTracer *trc, JSObject *obj);
static JSObject *iterator_iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

JSExtendedClass js_IteratorClass = {
  { "Iterator",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator) |
    JSCLASS_MARK_IS_TRACE |
    JSCLASS_IS_EXTENDED,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,   iterator_finalize,
    NULL,             NULL,            NULL,             NULL,
    NULL,             NULL,            JS_CLASS_TRACE(iterator_trace), NULL },
    NULL,             NULL,            NULL,             iterator_iterator,
    NULL,
    JSCLASS_NO_RESERVED_MEMBERS
};

void
NativeIterator::mark(JSTracer *trc)
{
    for (jsval *vp = props_array; vp < props_end; ++vp) {
        JS_SET_TRACING_INDEX(trc, "props", (vp - props_array));
        js_CallValueTracerIfGCThing(trc, *vp);
    }
    JS_CALL_OBJECT_TRACER(trc, obj, "obj");
}





static void
iterator_finalize(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_IteratorClass.base);

    
    NativeIterator *ni = obj->getNativeIterator();
    if (ni) {
        cx->free(ni);
        obj->setNativeIterator(NULL);
    }
}

static void
iterator_trace(JSTracer *trc, JSObject *obj)
{
    NativeIterator *ni = obj->getNativeIterator();

    if (ni)
        ni->mark(trc);
}

static bool
NewKeyValuePair(JSContext *cx, jsid key, jsval val, jsval *rval)
{
    jsval vec[2] = { ID_TO_VALUE(key), val };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vec), vec);

    JSObject *aobj = js_NewArrayObject(cx, 2, vec);
    if (!aobj)
        return false;
    *rval = OBJECT_TO_JSVAL(aobj);
    return true;
}

static inline bool
Enumerate(JSContext *cx, JSObject *obj, JSObject *pobj, jsid id,
          bool enumerable, uintN flags, HashSet<jsid>& ht,
          AutoValueVector& vec)
{
    JS_ASSERT(JSVAL_IS_INT(id) || JSVAL_IS_STRING(id));

    if (JS_LIKELY(!(flags & JSITER_OWNONLY))) {
        HashSet<jsid>::AddPtr p = ht.lookupForAdd(id);
        
        if (JS_UNLIKELY(!!p))
            return true;
        
        if (pobj->getProto() && !ht.add(p, id))
            return false;
    }
    if (enumerable || (flags & JSITER_HIDDEN)) {
        if (!vec.append(ID_TO_VALUE(id)))
            return false;
        if (flags & JSITER_FOREACH) {
            jsval *vp = vec.end() - 1;

            
            if (!obj->getProperty(cx, id, vp))
                return false;
            if ((flags & JSITER_KEYVALUE) && !NewKeyValuePair(cx, id, *vp, vp))
                return false;
        }
    }
    return true;
}

static bool
EnumerateNativeProperties(JSContext *cx, JSObject *obj, JSObject *pobj, uintN flags,
                          HashSet<jsid> &ht, AutoValueVector& props)
{
    AutoValueVector sprops(cx);

    JS_LOCK_OBJ(cx, pobj);

    
    JSScope *scope = pobj->scope();
    for (JSScopeProperty *sprop = scope->lastProperty(); sprop; sprop = sprop->parent) {
        if (sprop->id != JSVAL_VOID &&
            !sprop->isAlias() &&
            !Enumerate(cx, obj, pobj, sprop->id, sprop->enumerable(), flags, ht, sprops)) {
            return false;
        }
    }

    while (sprops.length() > 0) {
        if (!props.append(sprops.back()))
            return false;
        sprops.popBack();
    }

    JS_UNLOCK_SCOPE(cx, scope);

    return true;
}

static bool
EnumerateDenseArrayProperties(JSContext *cx, JSObject *obj, JSObject *pobj, uintN flags,
                              HashSet<jsid> &ht, AutoValueVector& props)
{
    size_t count = pobj->getDenseArrayCount();

    if (count) {
        size_t capacity = pobj->getDenseArrayCapacity();
        jsval *vp = pobj->dslots;
        for (size_t i = 0; i < capacity; ++i, ++vp) {
            if (*vp != JSVAL_HOLE) {
                
                if (!Enumerate(cx, obj, pobj, INT_TO_JSVAL(i), true, flags, ht, props))
                    return false;
            }
        }
    }
    return true;
}

NativeIterator *
NativeIterator::allocate(JSContext *cx, JSObject *obj, uintN flags, uint32 *sarray, uint32 slength,
                         uint32 key, jsval *parray, uint32 plength)
{
    NativeIterator *ni = (NativeIterator *)
        cx->malloc(sizeof(NativeIterator) + plength * sizeof(jsval) + slength * sizeof(uint32));
    if (!ni)
        return NULL;
    ni->obj = obj;
    ni->props_array = ni->props_cursor = (jsval *) (ni + 1);
    ni->props_end = ni->props_array + plength;
    if (plength)
        memcpy(ni->props_array, parray, plength * sizeof(jsval));
    ni->shapes_array = (uint32 *) ni->props_end;
    ni->shapes_length = slength;
    ni->shapes_key = key;
    ni->flags = flags;
    if (slength)
        memcpy(ni->shapes_array, sarray, slength * sizeof(uint32));
    return ni;
}

static bool
Snapshot(JSContext *cx, JSObject *obj, uintN flags, AutoValueVector &props)
{
    HashSet<jsid> ht(cx);
    if (!(flags & JSITER_OWNONLY) && !ht.init(32))
        return false;

    JSObject *pobj = obj;
    do {
        JSClass *clasp = pobj->getClass();
        if (pobj->isNative() &&
            pobj->map->ops->enumerate == js_Enumerate &&
            !(clasp->flags & JSCLASS_NEW_ENUMERATE)) {
            if (!clasp->enumerate(cx, pobj))
                return false;
            if (!EnumerateNativeProperties(cx, obj, pobj, flags, ht, props))
                return false;
        } else if (pobj->isDenseArray()) {
            if (!EnumerateDenseArrayProperties(cx, obj, pobj, flags, ht, props))
                return false;
        } else {
            if (pobj->isProxy()) {
                AutoValueVector proxyProps(cx);
                if (flags & JSITER_OWNONLY) {
                    if (!JSProxy::enumerateOwn(cx, pobj, proxyProps))
                        return false;
                } else {
                    if (!JSProxy::enumerate(cx, pobj, proxyProps))
                        return false;
                }
                for (size_t n = 0, len = proxyProps.length(); n < len; n++) {
                    if (!Enumerate(cx, obj, pobj, (jsid) proxyProps[n], true, flags, ht, props))
                        return false;
                }
                
                break;
            }
            jsval state;
            if (!pobj->enumerate(cx, JSENUMERATE_INIT, &state, NULL))
                return false;
            if (state == JSVAL_NATIVE_ENUMERATE_COOKIE) {
                if (!EnumerateNativeProperties(cx, obj, pobj, flags, ht, props))
                    return false;
            } else {
                while (true) {
                    jsid id;
                    if (!pobj->enumerate(cx, JSENUMERATE_NEXT, &state, &id))
                        return false;
                    if (state == JSVAL_NULL)
                        break;
                    if (!Enumerate(cx, obj, pobj, id, true, flags, ht, props))
                        return false;
                }
            }
        }

        if (JS_UNLIKELY(pobj->isXML() || (flags & JSITER_OWNONLY)))
            break;
    } while ((pobj = pobj->getProto()) != NULL);

    return true;
}

bool
VectorToIdArray(JSContext *cx, AutoValueVector &props, JSIdArray **idap)
{
    JS_STATIC_ASSERT(sizeof(JSIdArray) > sizeof(jsid));
    size_t len = props.length();
    size_t idsz = len * sizeof(jsid);
    size_t sz = (sizeof(JSIdArray) - sizeof(jsid)) + idsz;
    JSIdArray *ida = static_cast<JSIdArray *>(cx->malloc(sz));
    if (!ida)
        return false;

    ida->length = static_cast<jsint>(len);
    memcpy(ida->vector, props.begin(), idsz);
    *idap = ida;
    return true;
}

bool
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, AutoValueVector &props)
{
    return Snapshot(cx, obj, flags & (JSITER_OWNONLY | JSITER_HIDDEN), props);
}

static inline bool
GetCustomIterator(JSContext *cx, JSObject *obj, uintN flags, jsval *vp)
{
    
    JSAtom *atom = cx->runtime->atomState.iteratorAtom;
    if (!js_GetMethod(cx, obj, ATOM_TO_JSID(atom), JSGET_NO_METHOD_BARRIER, vp))
        return false;

    
    if (*vp == JSVAL_VOID)
        return true;

    
    LeaveTrace(cx);
    jsval arg = BOOLEAN_TO_JSVAL((flags & JSITER_FOREACH) == 0);
    if (!js_InternalCall(cx, obj, *vp, 1, &arg, vp))
        return false;
    if (JSVAL_IS_PRIMITIVE(*vp)) {
        



        js_ReportValueError2(cx, JSMSG_BAD_TRAP_RETURN_VALUE,
                             -1, OBJECT_TO_JSVAL(obj), NULL,
                             js_AtomToPrintableString(cx, atom));
        return false;
    }
    return true;
}

template <typename T>
static inline bool
Compare(T *a, T *b, size_t c)
{
    size_t n = (c + size_t(7)) / size_t(8);
    switch (c % 8) {
      case 0: do { if (*a++ != *b++) return false;
      case 7:      if (*a++ != *b++) return false;
      case 6:      if (*a++ != *b++) return false;
      case 5:      if (*a++ != *b++) return false;
      case 4:      if (*a++ != *b++) return false;
      case 3:      if (*a++ != *b++) return false;
      case 2:      if (*a++ != *b++) return false;
      case 1:      if (*a++ != *b++) return false;
              } while (--n > 0);
    }
    return true;
}

static JSObject *
NewIteratorObject(JSContext *cx, uintN flags)
{
    return !(flags & JSITER_ENUMERATE)
           ? NewObject(cx, &js_IteratorClass.base, NULL, NULL)
           : NewObjectWithGivenProto(cx, &js_IteratorClass.base, NULL, NULL);
}

static inline void
RegisterEnumerator(JSContext *cx, JSObject *iterobj, NativeIterator *ni)
{
    
    if (ni->flags & JSITER_ENUMERATE) {
        ni->next = cx->enumerators;
        cx->enumerators = iterobj;
    }
}

bool
IdVectorToIterator(JSContext *cx, JSObject *obj, uintN flags, AutoValueVector &props, jsval *vp)
{
    JSObject *iterobj = NewIteratorObject(cx, flags);
    if (!iterobj)
        return false;

    *vp = OBJECT_TO_JSVAL(iterobj);

    NativeIterator *ni = NativeIterator::allocate(cx, obj, flags, NULL, 0, 0,
                                                  props.begin(), props.length());
    if (!ni)
        return false;

    iterobj->setNativeIterator(ni);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

bool
GetIterator(JSContext *cx, JSObject *obj, uintN flags, jsval *vp)
{
    uint32 hash;
    JSObject **hp;
    Vector<uint32, 8> shapes(cx);
    uint32 key = 0;

    bool keysOnly = (flags == JSITER_ENUMERATE);

    if (obj) {
        if (keysOnly) {
            





            JSObject *pobj = obj;
            do {
                if (!pobj->isNative() ||
                    obj->map->ops->enumerate != js_Enumerate ||
                    pobj->getClass()->enumerate != JS_EnumerateStub) {
                    shapes.clear();
                    goto miss;
                }
                uint32 shape = pobj->shape();
                key = (key + (key << 16)) ^ shape;
                if (!shapes.append(shape))
                    return false;
                pobj = pobj->getProto();
            } while (pobj);

            hash = key % JS_ARRAY_LENGTH(JS_THREAD_DATA(cx)->cachedNativeIterators);
            hp = &JS_THREAD_DATA(cx)->cachedNativeIterators[hash];
            JSObject *iterobj = *hp;
            if (iterobj) {
                NativeIterator *ni = iterobj->getNativeIterator();
                if (ni->shapes_key == key &&
                    ni->shapes_length == shapes.length() &&
                    Compare(ni->shapes_array, shapes.begin(), ni->shapes_length)) {
                    *vp = OBJECT_TO_JSVAL(iterobj);
                    *hp = ni->next;

                    RegisterEnumerator(cx, iterobj, ni);
                    return true;
                }
            }
        }

      miss:
        if (obj->isProxy())
            return JSProxy::iterate(cx, obj, flags, vp);
        if (!GetCustomIterator(cx, obj, flags, vp))
            return false;
        if (*vp != JSVAL_VOID)
            return true;
    }

    JSObject *iterobj = NewIteratorObject(cx, flags);
    if (!iterobj)
        return false;

    
    *vp = OBJECT_TO_JSVAL(iterobj);

    AutoValueVector props(cx);
    if (!Snapshot(cx, obj, flags, props))
        return false;

    NativeIterator *ni =
        NativeIterator::allocate(cx, obj, flags, shapes.begin(), shapes.length(), key,
                                 props.begin(), props.length());
    if (!ni)
        return false;

    iterobj->setNativeIterator(ni);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

static JSObject *
iterator_iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
    return obj;
}

static JSBool
Iterator(JSContext *cx, JSObject *iterobj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool keyonly;
    uintN flags;

    keyonly = js_ValueToBoolean(argv[1]);
    flags = JSITER_OWNONLY | (keyonly ? 0 : (JSITER_FOREACH | JSITER_KEYVALUE));
    *rval = argv[0];
    return js_ValueToIterator(cx, flags, rval);
}

JSBool
js_ThrowStopIteration(JSContext *cx)
{
    jsval v;

    JS_ASSERT(!JS_IsExceptionPending(cx));
    if (js_FindClassObject(cx, NULL, JSProto_StopIteration, &v))
        JS_SetPendingException(cx, v);
    return JS_FALSE;
}

static JSBool
iterator_next(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!JS_InstanceOf(cx, obj, &js_IteratorClass.base, vp + 2))
        return false;

    if (!js_IteratorMore(cx, obj, vp))
        return false;
    if (*vp == JSVAL_FALSE) {
        js_ThrowStopIteration(cx);
        return false;
    }
    JS_ASSERT(*vp == JSVAL_TRUE);
    return js_IteratorNext(cx, obj, vp);
}

#define JSPROP_ROPERM   (JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec iterator_methods[] = {
    JS_FN(js_next_str,      iterator_next,  0,JSPROP_ROPERM),
    JS_FS_END
};





JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp)
{
    JSObject *obj;
    JSClass *clasp;
    JSExtendedClass *xclasp;
    JSObject *iterobj;

    
    JS_ASSERT_IF(flags & JSITER_KEYVALUE, flags & JSITER_FOREACH);

    




    cx->iterValue = JSVAL_HOLE;

    AutoValueRooter tvr(cx);

    if (!JSVAL_IS_PRIMITIVE(*vp)) {
        
        obj = JSVAL_TO_OBJECT(*vp);
    } else {
        






        if ((flags & JSITER_ENUMERATE)) {
            if (!js_ValueToObject(cx, *vp, &obj))
                return false;
            if (!obj)
                return GetIterator(cx, obj, flags, vp);
        } else {
            obj = js_ValueToNonNullObject(cx, *vp);
            if (!obj)
                return false;
        }
    }

    tvr.setObject(obj);

    clasp = obj->getClass();
    if ((clasp->flags & JSCLASS_IS_EXTENDED) &&
        (xclasp = (JSExtendedClass *) clasp)->iteratorObject) {
        
        if (clasp != &js_IteratorClass.base || obj->getNativeIterator()) {
            iterobj = xclasp->iteratorObject(cx, obj, !(flags & JSITER_FOREACH));
            if (!iterobj)
                return false;
            *vp = OBJECT_TO_JSVAL(iterobj);
            return true;
        }
    }

    return GetIterator(cx, obj, flags, vp);
}

#if JS_HAS_GENERATORS
static JS_REQUIRES_STACK JSBool
CloseGenerator(JSContext *cx, JSObject *genobj);
#endif

JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSClass *clasp;

    cx->iterValue = JSVAL_HOLE;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
    obj = JSVAL_TO_OBJECT(v);
    clasp = obj->getClass();

    if (clasp == &js_IteratorClass.base) {
        
        NativeIterator *ni = obj->getNativeIterator();
        if (ni->flags & JSITER_ENUMERATE) {
            JS_ASSERT(cx->enumerators == obj);
            cx->enumerators = ni->next;
        }

        
        if (ni->shapes_length) {
            uint32 hash = ni->shapes_key % NATIVE_ITER_CACHE_SIZE;
            JSObject **hp = &JS_THREAD_DATA(cx)->cachedNativeIterators[hash];
            ni->props_cursor = ni->props_array;
            ni->next = *hp;
            *hp = obj;
        } else {
            iterator_finalize(cx, obj);
        }
    }
#if JS_HAS_GENERATORS
    else if (clasp == &js_GeneratorClass.base) {
        return CloseGenerator(cx, obj);
    }
#endif
    return JS_TRUE;
}











bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id)
{
    JSObject *iterobj = cx->enumerators;
    while (iterobj) {
      again:
        NativeIterator *ni = iterobj->getNativeIterator();
        if (ni->obj == obj && ni->props_cursor < ni->props_end) {
            
            jsid *props_cursor = ni->props_cursor;
            jsid *props_end = ni->props_end;
            for (jsid *idp = props_cursor; idp < props_end; ++idp) {
                if (*idp == id) {
                    



                    if (obj->getProto()) {
                        AutoObjectRooter proto(cx, obj->getProto());
                        AutoObjectRooter obj2(cx);
                        JSProperty *prop;
                        if (!proto.object()->lookupProperty(cx, id, obj2.addr(), &prop))
                            return false;
                        if (prop) {
                            uintN attrs;
                            if (obj2.object()->isNative()) {
                                attrs = ((JSScopeProperty *) prop)->attributes();
                                JS_UNLOCK_OBJ(cx, obj2.object());
                            } else if (!obj2.object()->getAttributes(cx, id, &attrs)) {
                                return false;
                            }
                            if (attrs & JSPROP_ENUMERATE)
                                continue;
                        }
                    }

                    



                    if (props_end != ni->props_end || props_cursor != ni->props_cursor)
                        goto again;

                    




                    if (idp == props_cursor) {
                        ni->props_cursor++;
                    } else {
                        memmove(idp, idp + 1, (props_end - (idp + 1)) * sizeof(jsid));
                        ni->props_end--;
                    }
                    break;
                }
            }
        }
        iterobj = ni->next;
    }
    return true;
}

JSBool
js_IteratorMore(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    
    if (iterobj->getClass() == &js_IteratorClass.base) {
        



        NativeIterator *ni = iterobj->getNativeIterator();
        *rval = BOOLEAN_TO_JSVAL(ni->props_cursor < ni->props_end);
        return true;
    }

    
    if (cx->iterValue != JSVAL_HOLE) {
        *rval = JSVAL_TRUE;
        return true;
    }

    
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.nextAtom);
    if (!JS_GetMethodById(cx, iterobj, id, &iterobj, rval))
        return false;
    if (!js_InternalCall(cx, iterobj, *rval, 0, NULL, rval)) {
        
        if (!cx->throwing || !js_ValueIsStopIteration(cx->exception))
            return false;

        
        cx->throwing = JS_FALSE;
        cx->exception = JSVAL_VOID;
        cx->iterValue = JSVAL_HOLE;
        *rval = JSVAL_FALSE;
        return true;
    }

    
    JS_ASSERT(*rval != JSVAL_HOLE);
    cx->iterValue = *rval;
    *rval = JSVAL_TRUE;
    return true;
}

JSBool
js_IteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval)
{
    
    if (iterobj->getClass() == &js_IteratorClass.base) {
        



        NativeIterator *ni = iterobj->getNativeIterator();
        JS_ASSERT(ni->props_cursor < ni->props_end);
        *rval = *ni->props_cursor++;

        if (JSVAL_IS_STRING(*rval) || (ni->flags & JSITER_FOREACH))
            return true;

        JSString *str;
        jsint i;
        if (JSVAL_IS_INT(*rval) && (jsuint(i = JSVAL_TO_INT(*rval)) < INT_STRING_LIMIT)) {
            str = JSString::intString(i);
        } else {
            str = js_ValueToString(cx, *rval);
            if (!str)
                return false;
        }

        *rval = STRING_TO_JSVAL(str);
        return true;
    }

    JS_ASSERT(cx->iterValue != JSVAL_HOLE);
    *rval = cx->iterValue;
    cx->iterValue = JSVAL_HOLE;

    return true;
}

static JSBool
stopiter_hasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp)
{
    *bp = js_ValueIsStopIteration(v);
    return JS_TRUE;
}

JSClass js_StopIterationClass = {
    js_StopIteration_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration),
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   NULL,
    NULL,             NULL,
    NULL,             NULL,
    NULL,             stopiter_hasInstance,
    NULL,             NULL
};

#if JS_HAS_GENERATORS

static void
generator_finalize(JSContext *cx, JSObject *obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    JS_ASSERT(gen->state == JSGEN_NEWBORN ||
              gen->state == JSGEN_CLOSED ||
              gen->state == JSGEN_OPEN);
    cx->free(gen);
}

static void
generator_trace(JSTracer *trc, JSObject *obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING)
        return;

    JSStackFrame *fp = gen->getFloatingFrame();
    JS_ASSERT(gen->getLiveFrame() == fp);
    TraceValues(trc, gen->floatingStack, fp->argEnd(), "generator slots");
    js_TraceStackFrame(trc, fp);
    TraceValues(trc, fp->slots(), gen->savedRegs.sp, "generator slots");
}

JSExtendedClass js_GeneratorClass = {
  { js_Generator_str,
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Generator) |
    JSCLASS_IS_ANONYMOUS |
    JSCLASS_MARK_IS_TRACE |
    JSCLASS_IS_EXTENDED,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,  JS_ConvertStub,  generator_finalize,
    NULL,             NULL,            NULL,            NULL,
    NULL,             NULL,            JS_CLASS_TRACE(generator_trace), NULL },
    NULL,             NULL,            NULL,            iterator_iterator,
    NULL,
    JSCLASS_NO_RESERVED_MEMBERS
};









JS_REQUIRES_STACK JSObject *
js_NewGenerator(JSContext *cx)
{
    JSObject *obj = NewObject(cx, &js_GeneratorClass.base, NULL, NULL);
    if (!obj)
        return NULL;

    
    JSStackFrame *fp = cx->fp;
    uintN argc = fp->argc;
    uintN nargs = JS_MAX(argc, fp->fun->nargs);
    uintN vplen = 2 + nargs;

    
    uintN nbytes = sizeof(JSGenerator) +
                   (-1 + 
                    vplen +
                    VALUES_PER_STACK_FRAME +
                    fp->script->nslots) * sizeof(jsval);

    JSGenerator *gen = (JSGenerator *) cx->malloc(nbytes);
    if (!gen)
        return NULL;

    
    jsval *vp = gen->floatingStack;
    JSStackFrame *newfp = reinterpret_cast<JSStackFrame *>(vp + vplen);
    jsval *slots = newfp->slots();

    
    gen->obj = obj;
    gen->state = JSGEN_NEWBORN;
    gen->savedRegs.pc = cx->regs->pc;
    JS_ASSERT(cx->regs->sp == fp->slots() + fp->script->nfixed);
    gen->savedRegs.sp = slots + fp->script->nfixed;
    gen->vplen = vplen;
    gen->enumerators = NULL;
    gen->liveFrame = newfp;

    
    newfp->imacpc = NULL;
    newfp->callobj = fp->callobj;
    if (fp->callobj) {      
        fp->callobj->setPrivate(newfp);
        fp->callobj = NULL;
    }
    newfp->argsobj = fp->argsobj;
    if (fp->argsobj) {      
        JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(newfp);
        fp->argsobj = NULL;
    }
    newfp->script = fp->script;
    newfp->fun = fp->fun;
    newfp->thisv = fp->thisv;
    newfp->argc = fp->argc;
    newfp->argv = vp + 2;
    newfp->rval = fp->rval;
    newfp->annotation = NULL;
    newfp->scopeChain = fp->scopeChain;
    JS_ASSERT(!fp->blockChain);
    newfp->blockChain = NULL;
    newfp->flags = fp->flags | JSFRAME_GENERATOR | JSFRAME_FLOATING_GENERATOR;

    
    memcpy(vp, fp->argv - 2, vplen * sizeof(jsval));
    memcpy(slots, fp->slots(), fp->script->nfixed * sizeof(jsval));

    obj->setPrivate(gen);
    return obj;
}

JSGenerator *
js_FloatingFrameToGenerator(JSStackFrame *fp)
{
    JS_ASSERT(fp->isGenerator() && fp->isFloatingGenerator());
    char *floatingStackp = (char *)(fp->argv - 2);
    char *p = floatingStackp - offsetof(JSGenerator, floatingStack);
    return reinterpret_cast<JSGenerator *>(p);
}

typedef enum JSGeneratorOp {
    JSGENOP_NEXT,
    JSGENOP_SEND,
    JSGENOP_THROW,
    JSGENOP_CLOSE
} JSGeneratorOp;





static JS_REQUIRES_STACK JSBool
SendToGenerator(JSContext *cx, JSGeneratorOp op, JSObject *obj,
                JSGenerator *gen, jsval arg)
{
    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        js_ReportValueError(cx, JSMSG_NESTING_GENERATOR,
                            JSDVG_SEARCH_STACK, OBJECT_TO_JSVAL(obj),
                            JS_GetFunctionId(gen->getFloatingFrame()->fun));
        return JS_FALSE;
    }

    
    if (!cx->ensureGeneratorStackSpace())
        return JS_FALSE;

    JS_ASSERT(gen->state ==  JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
    switch (op) {
      case JSGENOP_NEXT:
      case JSGENOP_SEND:
        if (gen->state == JSGEN_OPEN) {
            



            gen->savedRegs.sp[-1] = arg;
        }
        gen->state = JSGEN_RUNNING;
        break;

      case JSGENOP_THROW:
        JS_SetPendingException(cx, arg);
        gen->state = JSGEN_RUNNING;
        break;

      default:
        JS_ASSERT(op == JSGENOP_CLOSE);
        JS_SetPendingException(cx, JSVAL_ARETURN);
        gen->state = JSGEN_CLOSING;
        break;
    }

    JSStackFrame *genfp = gen->getFloatingFrame();
    JSBool ok;
    {
        jsval *genVp = gen->floatingStack;
        uintN vplen = gen->vplen;
        uintN nfixed = genfp->script->nslots;

        



        ExecuteFrameGuard frame;
        if (!cx->stack().getExecuteFrame(cx, cx->fp, vplen, nfixed, frame)) {
            gen->state = JSGEN_CLOSED;
            return JS_FALSE;
        }

        jsval *vp = frame.getvp();
        JSStackFrame *fp = frame.getFrame();

        



        uintN usedBefore = gen->savedRegs.sp - genVp;
        memcpy(vp, genVp, usedBefore * sizeof(jsval));
        fp->flags &= ~JSFRAME_FLOATING_GENERATOR;
        fp->argv = vp + 2;
        gen->savedRegs.sp = fp->slots() + (gen->savedRegs.sp - genfp->slots());
        JS_ASSERT(uintN(gen->savedRegs.sp - fp->slots()) <= fp->script->nslots);

#ifdef DEBUG
        JSObject *callobjBefore = fp->callobj;
        jsval argsobjBefore = fp->argsobj;
#endif

        





        if (genfp->callobj)
            fp->callobj->setPrivate(fp);
        if (genfp->argsobj)
            JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(fp);
        gen->liveFrame = fp;
        (void)cx->enterGenerator(gen); 

        
        cx->stack().pushExecuteFrame(cx, frame, gen->savedRegs, NULL);

        
        JSObject *enumerators = cx->enumerators;
        cx->enumerators = gen->enumerators;

        ok = js_Interpret(cx);

        
        gen->enumerators = cx->enumerators;
        cx->enumerators = enumerators;

        
        cx->leaveGenerator(gen);
        gen->liveFrame = genfp;
        if (fp->argsobj)
            JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(genfp);
        if (fp->callobj)
            fp->callobj->setPrivate(genfp);

        JS_ASSERT_IF(argsobjBefore, argsobjBefore == fp->argsobj);
        JS_ASSERT_IF(callobjBefore, callobjBefore == fp->callobj);

        
        JS_ASSERT(uintN(gen->savedRegs.sp - fp->slots()) <= fp->script->nslots);
        uintN usedAfter = gen->savedRegs.sp - vp;
        memcpy(genVp, vp, usedAfter * sizeof(jsval));
        genfp->flags |= JSFRAME_FLOATING_GENERATOR;
        genfp->argv = genVp + 2;
        gen->savedRegs.sp = genfp->slots() + (gen->savedRegs.sp - fp->slots());
        JS_ASSERT(uintN(gen->savedRegs.sp - genfp->slots()) <= genfp->script->nslots);
    }

    if (gen->getFloatingFrame()->flags & JSFRAME_YIELDING) {
        
        JS_ASSERT(ok);
        JS_ASSERT(!cx->throwing);
        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        genfp->flags &= ~JSFRAME_YIELDING;
        gen->state = JSGEN_OPEN;
        return JS_TRUE;
    }

    genfp->rval = JSVAL_VOID;
    gen->state = JSGEN_CLOSED;
    if (ok) {
        
        if (op == JSGENOP_CLOSE)
            return JS_TRUE;
        return js_ThrowStopIteration(cx);
    }

    



    return JS_FALSE;
}

static JS_REQUIRES_STACK JSBool
CloseGenerator(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_GeneratorClass.base);

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen) {
        
        return JS_TRUE;
    }

    if (gen->state == JSGEN_CLOSED)
        return JS_TRUE;

    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, JSVAL_VOID);
}




static JSBool
generator_op(JSContext *cx, JSGeneratorOp op, jsval *vp, uintN argc)
{
    JSObject *obj;
    jsval arg;

    LeaveTrace(cx);

    obj = JS_THIS_OBJECT(cx, vp);
    if (!JS_InstanceOf(cx, obj, &js_GeneratorClass.base, vp + 2))
        return JS_FALSE;

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen) {
        
        goto closed_generator;
    }

    if (gen->state == JSGEN_NEWBORN) {
        switch (op) {
          case JSGENOP_NEXT:
          case JSGENOP_THROW:
            break;

          case JSGENOP_SEND:
            if (argc >= 1 && !JSVAL_IS_VOID(vp[2])) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_SEND,
                                    JSDVG_SEARCH_STACK, vp[2], NULL);
                return JS_FALSE;
            }
            break;

          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            gen->state = JSGEN_CLOSED;
            return JS_TRUE;
        }
    } else if (gen->state == JSGEN_CLOSED) {
      closed_generator:
        switch (op) {
          case JSGENOP_NEXT:
          case JSGENOP_SEND:
            return js_ThrowStopIteration(cx);
          case JSGENOP_THROW:
            JS_SetPendingException(cx, argc >= 1 ? vp[2] : JSVAL_VOID);
            return JS_FALSE;
          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            return JS_TRUE;
        }
    }

    arg = ((op == JSGENOP_SEND || op == JSGENOP_THROW) && argc != 0)
          ? vp[2]
          : JSVAL_VOID;
    if (!SendToGenerator(cx, op, obj, gen, arg))
        return JS_FALSE;
    *vp = gen->getFloatingFrame()->rval;
    return JS_TRUE;
}

static JSBool
generator_send(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_SEND, vp, argc);
}

static JSBool
generator_next(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_NEXT, vp, argc);
}

static JSBool
generator_throw(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_THROW, vp, argc);
}

static JSBool
generator_close(JSContext *cx, uintN argc, jsval *vp)
{
    return generator_op(cx, JSGENOP_CLOSE, vp, argc);
}

static JSFunctionSpec generator_methods[] = {
    JS_FN(js_next_str,      generator_next,     0,JSPROP_ROPERM),
    JS_FN(js_send_str,      generator_send,     1,JSPROP_ROPERM),
    JS_FN(js_throw_str,     generator_throw,    1,JSPROP_ROPERM),
    JS_FN(js_close_str,     generator_close,    0,JSPROP_ROPERM),
    JS_FS_END
};

#endif 

JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj)
{
    JSObject *proto, *stop;

    
    if (!js_GetClassObject(cx, obj, JSProto_StopIteration, &stop))
        return NULL;
    if (stop)
        return stop;

    proto = JS_InitClass(cx, obj, NULL, &js_IteratorClass.base, Iterator, 2,
                         NULL, iterator_methods, NULL, NULL);
    if (!proto)
        return NULL;

#if JS_HAS_GENERATORS
    
    if (!JS_InitClass(cx, obj, NULL, &js_GeneratorClass.base, NULL, 0,
                      NULL, generator_methods, NULL, NULL)) {
        return NULL;
    }
#endif

    return JS_InitClass(cx, obj, NULL, &js_StopIterationClass, NULL, 0,
                        NULL, NULL, NULL, NULL);
}

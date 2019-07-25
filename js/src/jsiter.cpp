










































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
#include "jsgcmark.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsvector.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jscntxtinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

using namespace js;
using namespace js::gc;

static void iterator_finalize(JSContext *cx, JSObject *obj);
static void iterator_trace(JSTracer *trc, JSObject *obj);
static JSObject *iterator_iterator(JSContext *cx, JSObject *obj, JSBool keysonly);

Class js_IteratorClass = {
    "Iterator",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_CONCURRENT_FINALIZER |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator),
    PropertyStub,         
    PropertyStub,         
    PropertyStub,         
    StrictPropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub,
    iterator_finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    iterator_trace,
    {
        NULL,             
        NULL,             
        NULL,             
        iterator_iterator,
        NULL              
    }
};

void
NativeIterator::mark(JSTracer *trc)
{
    MarkIdRange(trc, begin(), end(), "props");
    if (obj)
        MarkObject(trc, *obj, "obj");
}

static void
iterator_finalize(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_IteratorClass);

    NativeIterator *ni = obj->getNativeIterator();
    if (ni) {
        cx->free_(ni);
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

struct IdHashPolicy {
    typedef jsid Lookup;
    static HashNumber hash(jsid id) {
        return JSID_BITS(id);
    }
    static bool match(jsid id1, jsid id2) {
        return id1 == id2;
    }
};

typedef HashSet<jsid, IdHashPolicy, ContextAllocPolicy> IdSet;

static inline bool
NewKeyValuePair(JSContext *cx, jsid id, const Value &val, Value *rval)
{
    Value vec[2] = { IdToValue(id), val };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vec), vec);

    JSObject *aobj = NewDenseCopiedArray(cx, 2, vec);
    if (!aobj)
        return false;
    rval->setObject(*aobj);
    return true;
}

static inline bool
Enumerate(JSContext *cx, JSObject *obj, JSObject *pobj, jsid id,
          bool enumerable, bool sharedPermanent, uintN flags, IdSet& ht,
          AutoIdVector *props)
{
    IdSet::AddPtr p = ht.lookupForAdd(id);
    JS_ASSERT_IF(obj == pobj && !obj->isProxy(), !p);

    
    if (JS_UNLIKELY(!!p))
        return true;

    




    if ((pobj->getProto() || pobj->isProxy()) && !ht.add(p, id))
        return false;

    if (JS_UNLIKELY(flags & JSITER_OWNONLY)) {
        







        if (!pobj->getProto() && id == ATOM_TO_JSID(cx->runtime->atomState.protoAtom))
            return true;
        if (pobj != obj && !(sharedPermanent && pobj->getClass() == obj->getClass()))
            return true;
    }

    if (enumerable || (flags & JSITER_HIDDEN))
        return props->append(id);

    return true;
}

static bool
EnumerateNativeProperties(JSContext *cx, JSObject *obj, JSObject *pobj, uintN flags, IdSet &ht,
                          AutoIdVector *props)
{
    size_t initialLength = props->length();

    
    for (Shape::Range r = pobj->lastProperty()->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();

        if (!JSID_IS_DEFAULT_XML_NAMESPACE(shape.id) &&
            !shape.isAlias() &&
            !Enumerate(cx, obj, pobj, shape.id, shape.enumerable(),
                       shape.isSharedPermanent(), flags, ht, props))
        {
            return false;
        }
    }

    ::Reverse(props->begin() + initialLength, props->end());
    return true;
}

static bool
EnumerateDenseArrayProperties(JSContext *cx, JSObject *obj, JSObject *pobj, uintN flags,
                              IdSet &ht, AutoIdVector *props)
{
    if (!Enumerate(cx, obj, pobj, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom), false, true,
                   flags, ht, props)) {
        return false;
    }

    if (pobj->getArrayLength() > 0) {
        size_t capacity = pobj->getDenseArrayCapacity();
        Value *vp = pobj->getDenseArrayElements();
        for (size_t i = 0; i < capacity; ++i, ++vp) {
            if (!vp->isMagic(JS_ARRAY_HOLE)) {
                
                if (!Enumerate(cx, obj, pobj, INT_TO_JSID(i), true, false, flags, ht, props))
                    return false;
            }
        }
    }

    return true;
}

static bool
Snapshot(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector *props)
{
    




    IdSet ht(cx);
    if (!ht.init(32))
        return NULL;

    JSObject *pobj = obj;
    do {
        Class *clasp = pobj->getClass();
        if (pobj->isNative() &&
            !pobj->getOps()->enumerate &&
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
                AutoIdVector proxyProps(cx);
                if (flags & JSITER_OWNONLY) {
                    if (flags & JSITER_HIDDEN) {
                        if (!JSProxy::getOwnPropertyNames(cx, pobj, proxyProps))
                            return false;
                    } else {
                        if (!JSProxy::keys(cx, pobj, proxyProps))
                            return false;
                    }
                } else {
                    if (!JSProxy::enumerate(cx, pobj, proxyProps))
                        return false;
                }
                for (size_t n = 0, len = proxyProps.length(); n < len; n++) {
                    if (!Enumerate(cx, obj, pobj, proxyProps[n], true, false, flags, ht, props))
                        return false;
                }
                
                break;
            }
            Value state;
            JSIterateOp op = (flags & JSITER_HIDDEN) ? JSENUMERATE_INIT_ALL : JSENUMERATE_INIT;
            if (!pobj->enumerate(cx, op, &state, NULL))
                return false;
            if (state.isMagic(JS_NATIVE_ENUMERATE)) {
                if (!EnumerateNativeProperties(cx, obj, pobj, flags, ht, props))
                    return false;
            } else {
                while (true) {
                    jsid id;
                    if (!pobj->enumerate(cx, JSENUMERATE_NEXT, &state, &id))
                        return false;
                    if (state.isNull())
                        break;
                    if (!Enumerate(cx, obj, pobj, id, true, false, flags, ht, props))
                        return false;
                }
            }
        }

        if (JS_UNLIKELY(pobj->isXML()))
            break;
    } while ((pobj = pobj->getProto()) != NULL);

    return true;
}

namespace js {

bool
VectorToIdArray(JSContext *cx, AutoIdVector &props, JSIdArray **idap)
{
    JS_STATIC_ASSERT(sizeof(JSIdArray) > sizeof(jsid));
    size_t len = props.length();
    size_t idsz = len * sizeof(jsid);
    size_t sz = (sizeof(JSIdArray) - sizeof(jsid)) + idsz;
    JSIdArray *ida = static_cast<JSIdArray *>(cx->malloc_(sz));
    if (!ida)
        return false;

    ida->length = static_cast<jsint>(len);
    memcpy(ida->vector, props.begin(), idsz);
    *idap = ida;
    return true;
}

JS_FRIEND_API(bool)
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector *props)
{
    return Snapshot(cx, obj, flags & (JSITER_OWNONLY | JSITER_HIDDEN), props);
}

}

static inline bool
GetCustomIterator(JSContext *cx, JSObject *obj, uintN flags, Value *vp)
{
    
    JSAtom *atom = cx->runtime->atomState.iteratorAtom;
    if (!js_GetMethod(cx, obj, ATOM_TO_JSID(atom), JSGET_NO_METHOD_BARRIER, vp))
        return false;

    
    if (!vp->isObject()) {
        vp->setUndefined();
        return true;
    }

    
    LeaveTrace(cx);
    Value arg = BooleanValue((flags & JSITER_FOREACH) == 0);
    if (!ExternalInvoke(cx, ObjectValue(*obj), *vp, 1, &arg, vp))
        return false;
    if (vp->isPrimitive()) {
        



        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, atom, &bytes))
            return false;
        js_ReportValueError2(cx, JSMSG_BAD_TRAP_RETURN_VALUE,
                             -1, ObjectValue(*obj), NULL, bytes.ptr());
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

static inline JSObject *
NewIteratorObject(JSContext *cx, uintN flags)
{
    if (flags & JSITER_ENUMERATE) {
        







        JSObject *obj = js_NewGCObject(cx, FINALIZE_OBJECT0);
        if (!obj)
            return NULL;

        EmptyShape *emptyEnumeratorShape = EmptyShape::getEmptyEnumeratorShape(cx);
        if (!emptyEnumeratorShape)
            return NULL;

        obj->init(cx, &js_IteratorClass, NULL, NULL, NULL, false);
        obj->setMap(emptyEnumeratorShape);
        return obj;
    }

    return NewBuiltinClassInstance(cx, &js_IteratorClass);
}

NativeIterator *
NativeIterator::allocateIterator(JSContext *cx, uint32 slength, const AutoIdVector &props)
{
    size_t plength = props.length();
    NativeIterator *ni = (NativeIterator *)
        cx->malloc_(sizeof(NativeIterator) + plength * sizeof(jsid) + slength * sizeof(uint32));
    if (!ni)
        return NULL;
    ni->props_array = ni->props_cursor = (jsid *) (ni + 1);
    ni->props_end = (jsid *)ni->props_array + plength;
    if (plength)
        memcpy(ni->props_array, props.begin(), plength * sizeof(jsid));
    return ni;
}

inline void
NativeIterator::init(JSObject *obj, uintN flags, uint32 slength, uint32 key)
{
    this->obj = obj;
    this->flags = flags;
    this->shapes_array = (uint32 *) this->props_end;
    this->shapes_length = slength;
    this->shapes_key = key;
}

static inline void
RegisterEnumerator(JSContext *cx, JSObject *iterobj, NativeIterator *ni)
{
    
    if (ni->flags & JSITER_ENUMERATE) {
        ni->next = cx->enumerators;
        cx->enumerators = iterobj;

        JS_ASSERT(!(ni->flags & JSITER_ACTIVE));
        ni->flags |= JSITER_ACTIVE;
    }
}

static inline bool
VectorToKeyIterator(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector &keys,
                    uint32 slength, uint32 key, Value *vp)
{
    JS_ASSERT(!(flags & JSITER_FOREACH));

    JSObject *iterobj = NewIteratorObject(cx, flags);
    if (!iterobj)
        return false;

    NativeIterator *ni = NativeIterator::allocateIterator(cx, slength, keys);
    if (!ni)
        return false;
    ni->init(obj, flags, slength, key);

    if (slength) {
        






        JSObject *pobj = obj;
        size_t ind = 0;
        do {
            ni->shapes_array[ind++] = pobj->shape();
            pobj = pobj->getProto();
        } while (pobj);
        JS_ASSERT(ind == slength);
    }

    iterobj->setNativeIterator(ni);
    vp->setObject(*iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

namespace js {

bool
VectorToKeyIterator(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector &props, Value *vp)
{
    return VectorToKeyIterator(cx, obj, flags, props, 0, 0, vp);
}

bool
VectorToValueIterator(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector &keys,
                      Value *vp)
{
    JS_ASSERT(flags & JSITER_FOREACH);

    JSObject *iterobj = NewIteratorObject(cx, flags);
    if (!iterobj)
        return false;

    NativeIterator *ni = NativeIterator::allocateIterator(cx, 0, keys);
    if (!ni)
        return false;
    ni->init(obj, flags, 0, 0);

    iterobj->setNativeIterator(ni);
    vp->setObject(*iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

bool
EnumeratedIdVectorToIterator(JSContext *cx, JSObject *obj, uintN flags, AutoIdVector &props, Value *vp)
{
    if (!(flags & JSITER_FOREACH))
        return VectorToKeyIterator(cx, obj, flags, props, vp);

    return VectorToValueIterator(cx, obj, flags, props, vp);
}

static inline void
UpdateNativeIterator(NativeIterator *ni, JSObject *obj)
{
    
    
    ni->obj = obj;
}

bool
GetIterator(JSContext *cx, JSObject *obj, uintN flags, Value *vp)
{
    Vector<uint32, 8> shapes(cx);
    uint32 key = 0;

    bool keysOnly = (flags == JSITER_ENUMERATE);

    if (obj) {
        
        JSIteratorOp op = obj->getClass()->ext.iteratorObject;
        if (op && (obj->getClass() != &js_IteratorClass || obj->getNativeIterator())) {
            JSObject *iterobj = op(cx, obj, !(flags & JSITER_FOREACH));
            if (!iterobj)
                return false;
            vp->setObject(*iterobj);
            return true;
        }

        if (keysOnly) {
            





            JSObject *last = cx->compartment->nativeIterCache.last;
            JSObject *proto = obj->getProto();
            if (last) {
                NativeIterator *lastni = last->getNativeIterator();
                if (!(lastni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                    obj->isNative() &&
                    obj->shape() == lastni->shapes_array[0] &&
                    proto && proto->isNative() &&
                    proto->shape() == lastni->shapes_array[1] &&
                    !proto->getProto()) {
                    vp->setObject(*last);
                    UpdateNativeIterator(lastni, obj);
                    RegisterEnumerator(cx, last, lastni);
                    return true;
                }
            }

            





            JSObject *pobj = obj;
            do {
                if (!pobj->isNative() ||
                    obj->getOps()->enumerate ||
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

            JSObject *iterobj = cx->compartment->nativeIterCache.get(key);
            if (iterobj) {
                NativeIterator *ni = iterobj->getNativeIterator();
                if (!(ni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                    ni->shapes_key == key &&
                    ni->shapes_length == shapes.length() &&
                    Compare(ni->shapes_array, shapes.begin(), ni->shapes_length)) {
                    vp->setObject(*iterobj);

                    UpdateNativeIterator(ni, obj);
                    RegisterEnumerator(cx, iterobj, ni);
                    if (shapes.length() == 2)
                        cx->compartment->nativeIterCache.last = iterobj;
                    return true;
                }
            }
        }

      miss:
        if (obj->isProxy())
            return JSProxy::iterate(cx, obj, flags, vp);
        if (!GetCustomIterator(cx, obj, flags, vp))
            return false;
        if (!vp->isUndefined())
            return true;
    }

    

    AutoIdVector keys(cx);
    if (flags & JSITER_FOREACH) {
        if (JS_LIKELY(obj != NULL) && !Snapshot(cx, obj, flags, &keys))
            return false;
        JS_ASSERT(shapes.empty());
        if (!VectorToValueIterator(cx, obj, flags, keys, vp))
            return false;
    } else {
        if (JS_LIKELY(obj != NULL) && !Snapshot(cx, obj, flags, &keys))
            return false;
        if (!VectorToKeyIterator(cx, obj, flags, keys, shapes.length(), key, vp))
            return false;
    }

    JSObject *iterobj = &vp->toObject();

    
    if (shapes.length())
        cx->compartment->nativeIterCache.set(key, iterobj);

    if (shapes.length() == 2)
        cx->compartment->nativeIterCache.last = iterobj;
    return true;
}

}

static JSObject *
iterator_iterator(JSContext *cx, JSObject *obj, JSBool keysonly)
{
    return obj;
}

static JSBool
Iterator(JSContext *cx, uintN argc, Value *vp)
{
    Value *argv = JS_ARGV(cx, vp);
    bool keyonly = argc >= 2 ? js_ValueToBoolean(argv[1]) : false;
    uintN flags = JSITER_OWNONLY | (keyonly ? 0 : (JSITER_FOREACH | JSITER_KEYVALUE));
    *vp = argc >= 1 ? argv[0] : UndefinedValue();
    return js_ValueToIterator(cx, flags, vp);
}

JSBool
js_ThrowStopIteration(JSContext *cx)
{
    Value v;

    JS_ASSERT(!JS_IsExceptionPending(cx));
    if (js_FindClassObject(cx, NULL, JSProto_StopIteration, &v))
        cx->setPendingException(v);
    return JS_FALSE;
}

static JSBool
iterator_next(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    if (obj->getClass() != &js_IteratorClass) {
        ReportIncompatibleMethod(cx, vp, &js_IteratorClass);
        return false;
    }

    if (!js_IteratorMore(cx, obj, vp))
        return false;
    if (!vp->toBoolean()) {
        js_ThrowStopIteration(cx);
        return false;
    }
    return js_IteratorNext(cx, obj, vp);
}

#define JSPROP_ROPERM   (JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec iterator_methods[] = {
    JS_FN(js_next_str,      iterator_next,  0,JSPROP_ROPERM),
    JS_FS_END
};





JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, Value *vp)
{
    
    JS_ASSERT_IF(flags & JSITER_KEYVALUE, flags & JSITER_FOREACH);

    




    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    JSObject *obj;
    if (vp->isObject()) {
        
        obj = &vp->toObject();
    } else {
        






        if ((flags & JSITER_ENUMERATE)) {
            if (!js_ValueToObjectOrNull(cx, *vp, &obj))
                return false;
            
        } else {
            obj = js_ValueToNonNullObject(cx, *vp);
            if (!obj)
                return false;
        }
    }

    return GetIterator(cx, obj, flags, vp);
}

#if JS_HAS_GENERATORS
static JS_REQUIRES_STACK JSBool
CloseGenerator(JSContext *cx, JSObject *genobj);
#endif

JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, JSObject *obj)
{
    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    Class *clasp = obj->getClass();
    if (clasp == &js_IteratorClass) {
        
        NativeIterator *ni = obj->getNativeIterator();

        if (ni->flags & JSITER_ENUMERATE) {
            JS_ASSERT(cx->enumerators == obj);
            cx->enumerators = ni->next;

            JS_ASSERT(ni->flags & JSITER_ACTIVE);
            ni->flags &= ~JSITER_ACTIVE;

            



            ni->props_cursor = ni->props_array;
        }
    }
#if JS_HAS_GENERATORS
    else if (clasp == &js_GeneratorClass) {
        return CloseGenerator(cx, obj);
    }
#endif
    return JS_TRUE;
}


















template<typename IdPredicate>
static bool
SuppressDeletedPropertyHelper(JSContext *cx, JSObject *obj, IdPredicate predicate)
{
    JSObject *iterobj = cx->enumerators;
    while (iterobj) {
      again:
        NativeIterator *ni = iterobj->getNativeIterator();
        
        if (ni->isKeyIter() && ni->obj == obj && ni->props_cursor < ni->props_end) {
            
            jsid *props_cursor = ni->current();
            jsid *props_end = ni->end();
            for (jsid *idp = props_cursor; idp < props_end; ++idp) {
                if (predicate(*idp)) {
                    



                    if (obj->getProto()) {
                        AutoObjectRooter proto(cx, obj->getProto());
                        AutoObjectRooter obj2(cx);
                        JSProperty *prop;
                        if (!proto.object()->lookupProperty(cx, *idp, obj2.addr(), &prop))
                            return false;
                        if (prop) {
                            uintN attrs;
                            if (obj2.object()->isNative())
                                attrs = ((Shape *) prop)->attributes();
                            else if (!obj2.object()->getAttributes(cx, *idp, &attrs))
                                return false;

                            if (attrs & JSPROP_ENUMERATE)
                                continue;
                        }
                    }

                    



                    if (props_end != ni->props_end || props_cursor != ni->props_cursor)
                        goto again;

                    




                    if (idp == props_cursor) {
                        ni->incCursor();
                    } else {
                        memmove(idp, idp + 1, (props_end - (idp + 1)) * sizeof(jsid));
                        ni->props_end = ni->end() - 1;
                    }

                    
                    ni->flags |= JSITER_UNREUSABLE;

                    if (predicate.matchesAtMostOne())
                        break;
                }
            }
        }
        iterobj = ni->next;
    }
    return true;
}

class SingleIdPredicate {
    jsid id;
public:
    SingleIdPredicate(jsid id) : id(id) {}

    bool operator()(jsid id) { return id == this->id; }
    bool matchesAtMostOne() { return true; }
};

bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id)
{
    id = js_CheckForStringIndex(id);
    return SuppressDeletedPropertyHelper(cx, obj, SingleIdPredicate(id));
}

class IndexRangePredicate {
    jsint begin, end;
public:
    IndexRangePredicate(jsint begin, jsint end) : begin(begin), end(end) {}

    bool operator()(jsid id) {
        return JSID_IS_INT(id) && begin <= JSID_TO_INT(id) && JSID_TO_INT(id) < end;
    }
    bool matchesAtMostOne() { return false; }
};

bool
js_SuppressDeletedIndexProperties(JSContext *cx, JSObject *obj, jsint begin, jsint end)
{
    return SuppressDeletedPropertyHelper(cx, obj, IndexRangePredicate(begin, end));
}

JSBool
js_IteratorMore(JSContext *cx, JSObject *iterobj, Value *rval)
{
    
    NativeIterator *ni = NULL;
    if (iterobj->getClass() == &js_IteratorClass) {
        
        ni = iterobj->getNativeIterator();
        if (ni) {
            bool more = ni->props_cursor < ni->props_end;
            if (ni->isKeyIter() || !more) {
                rval->setBoolean(more);
                return true;
            }
        }
    }

    
    if (!cx->iterValue.isMagic(JS_NO_ITER_VALUE)) {
        rval->setBoolean(true);
        return true;
    }

    
    if (!ni) {
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.nextAtom);
        if (!js_GetMethod(cx, iterobj, id, JSGET_METHOD_BARRIER, rval))
            return false;
        if (!ExternalInvoke(cx, ObjectValue(*iterobj), *rval, 0, NULL, rval)) {
            
            if (!cx->isExceptionPending() || !js_ValueIsStopIteration(cx->getPendingException()))
                return false;

            cx->clearPendingException();
            cx->iterValue.setMagic(JS_NO_ITER_VALUE);
            rval->setBoolean(false);
            return true;
        }
    } else {
        JS_ASSERT(!ni->isKeyIter());
        jsid id = *ni->current();
        ni->incCursor();
        if (!ni->obj->getProperty(cx, id, rval))
            return false;
        if ((ni->flags & JSITER_KEYVALUE) && !NewKeyValuePair(cx, id, *rval, rval))
            return false;
    }

    
    JS_ASSERT(!rval->isMagic(JS_NO_ITER_VALUE));
    cx->iterValue = *rval;
    rval->setBoolean(true);
    return true;
}

JSBool
js_IteratorNext(JSContext *cx, JSObject *iterobj, Value *rval)
{
    
    if (iterobj->getClass() == &js_IteratorClass) {
        



        NativeIterator *ni = iterobj->getNativeIterator();
        if (ni && ni->isKeyIter()) {
            JS_ASSERT(ni->props_cursor < ni->props_end);
            *rval = IdToValue(*ni->current());
            ni->incCursor();

            if (rval->isString())
                return true;

            JSString *str;
            jsint i;
            if (rval->isInt32() && JSAtom::hasIntStatic(i = rval->toInt32())) {
                str = &JSAtom::intStatic(i);
            } else {
                str = js_ValueToString(cx, *rval);
                if (!str)
                    return false;
            }

            rval->setString(str);
            return true;
        }
    }

    JS_ASSERT(!cx->iterValue.isMagic(JS_NO_ITER_VALUE));
    *rval = cx->iterValue;
    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    return true;
}

static JSBool
stopiter_hasInstance(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    *bp = js_ValueIsStopIteration(*v);
    return JS_TRUE;
}

Class js_StopIterationClass = {
    js_StopIteration_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration) |
    JSCLASS_FREEZE_PROTO,
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
    stopiter_hasInstance
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
    cx->free_(gen);
}

static void
generator_trace(JSTracer *trc, JSObject *obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING)
        return;

    JSStackFrame *fp = gen->floatingFrame();
    JS_ASSERT(gen->liveFrame() == fp);

    






    MarkStackRangeConservatively(trc, gen->floatingStack, fp->formalArgsEnd());
    js_TraceStackFrame(trc, fp);
    MarkStackRangeConservatively(trc, fp->slots(), gen->regs.sp);
}

Class js_GeneratorClass = {
    js_Generator_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Generator) |
    JSCLASS_IS_ANONYMOUS,
    PropertyStub,         
    PropertyStub,         
    PropertyStub,         
    StrictPropertyStub,   
    EnumerateStub,
    ResolveStub,
    ConvertStub,
    generator_finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    generator_trace,
    {
        NULL,             
        NULL,             
        NULL,             
        iterator_iterator,
        NULL              
    }
};

static inline void
RebaseRegsFromTo(JSFrameRegs *regs, JSStackFrame *from, JSStackFrame *to)
{
    regs->fp = to;
    regs->sp = to->slots() + (regs->sp - from->slots());
}









JS_REQUIRES_STACK JSObject *
js_NewGenerator(JSContext *cx)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &js_GeneratorClass);
    if (!obj)
        return NULL;

    JSStackFrame *stackfp = cx->fp();
    JS_ASSERT(stackfp->base() == cx->regs->sp);
    JS_ASSERT(stackfp->actualArgs() <= stackfp->formalArgs());

    
    Value *stackvp = stackfp->actualArgs() - 2;
    uintN vplen = stackfp->formalArgsEnd() - stackvp;

    
    uintN nbytes = sizeof(JSGenerator) +
                   (-1 + 
                    vplen +
                    VALUES_PER_STACK_FRAME +
                    stackfp->numSlots()) * sizeof(Value);

    JSGenerator *gen = (JSGenerator *) cx->malloc_(nbytes);
    if (!gen)
        return NULL;

    
    Value *genvp = gen->floatingStack;
    JSStackFrame *genfp = reinterpret_cast<JSStackFrame *>(genvp + vplen);

    
    gen->obj = obj;
    gen->state = JSGEN_NEWBORN;
    gen->enumerators = NULL;
    gen->floating = genfp;

    
    gen->regs = *cx->regs;
    RebaseRegsFromTo(&gen->regs, stackfp, genfp);

    
    genfp->stealFrameAndSlots(genvp, stackfp, stackvp, cx->regs->sp);
    genfp->initFloatingGenerator();

    obj->setPrivate(gen);
    return obj;
}

JSGenerator *
js_FloatingFrameToGenerator(JSStackFrame *fp)
{
    JS_ASSERT(fp->isGeneratorFrame() && fp->isFloatingGenerator());
    char *floatingStackp = (char *)(fp->actualArgs() - 2);
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
                JSGenerator *gen, const Value &arg)
{
    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        js_ReportValueError(cx, JSMSG_NESTING_GENERATOR,
                            JSDVG_SEARCH_STACK, ObjectOrNullValue(obj),
                            JS_GetFunctionId(gen->floatingFrame()->fun()));
        return JS_FALSE;
    }

    
    if (!cx->ensureGeneratorStackSpace())
        return JS_FALSE;

    JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
    switch (op) {
      case JSGENOP_NEXT:
      case JSGENOP_SEND:
        if (gen->state == JSGEN_OPEN) {
            



            gen->regs.sp[-1] = arg;
        }
        gen->state = JSGEN_RUNNING;
        break;

      case JSGENOP_THROW:
        cx->setPendingException(arg);
        gen->state = JSGEN_RUNNING;
        break;

      default:
        JS_ASSERT(op == JSGENOP_CLOSE);
        cx->setPendingException(MagicValue(JS_GENERATOR_CLOSING));
        gen->state = JSGEN_CLOSING;
        break;
    }

    JSStackFrame *genfp = gen->floatingFrame();
    Value *genvp = gen->floatingStack;
    uintN vplen = genfp->formalArgsEnd() - genvp;

    JSStackFrame *stackfp;
    Value *stackvp;
    JSBool ok;
    {
        



        GeneratorFrameGuard frame;
        if (!cx->stack().getGeneratorFrame(cx, vplen, genfp->numSlots(), &frame)) {
            gen->state = JSGEN_CLOSED;
            return JS_FALSE;
        }
        stackfp = frame.fp();
        stackvp = frame.vp();

        
        stackfp->stealFrameAndSlots(stackvp, genfp, genvp, gen->regs.sp);
        stackfp->resetGeneratorPrev(cx);
        stackfp->unsetFloatingGenerator();
        RebaseRegsFromTo(&gen->regs, genfp, stackfp);
        MUST_FLOW_THROUGH("restore");

        
        cx->stack().pushGeneratorFrame(cx, &gen->regs, &frame);

        cx->enterGenerator(gen);   
        JSObject *enumerators = cx->enumerators;
        cx->enumerators = gen->enumerators;

        ok = RunScript(cx, stackfp->script(), stackfp);

        gen->enumerators = cx->enumerators;
        cx->enumerators = enumerators;
        cx->leaveGenerator(gen);

        



        genfp->stealFrameAndSlots(genvp, stackfp, stackvp, gen->regs.sp);
        genfp->setFloatingGenerator();
    }
    MUST_FLOW_LABEL(restore)
    RebaseRegsFromTo(&gen->regs, stackfp, genfp);

    if (gen->floatingFrame()->isYielding()) {
        
        JS_ASSERT(ok);
        JS_ASSERT(!cx->isExceptionPending());
        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        genfp->clearYielding();
        gen->state = JSGEN_OPEN;
        return JS_TRUE;
    }

    genfp->clearReturnValue();
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
    JS_ASSERT(obj->getClass() == &js_GeneratorClass);

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen) {
        
        return JS_TRUE;
    }

    if (gen->state == JSGEN_CLOSED)
        return JS_TRUE;

    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, UndefinedValue());
}




static JSBool
generator_op(JSContext *cx, JSGeneratorOp op, Value *vp, uintN argc)
{
    LeaveTrace(cx);

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    if (obj->getClass() != &js_GeneratorClass) {
        ReportIncompatibleMethod(cx, vp, &js_GeneratorClass);
        return JS_FALSE;
    }

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
            if (argc >= 1 && !vp[2].isUndefined()) {
                js_ReportValueError(cx, JSMSG_BAD_GENERATOR_SEND,
                                    JSDVG_SEARCH_STACK, vp[2], NULL);
                return JS_FALSE;
            }
            break;

          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            gen->state = JSGEN_CLOSED;
            JS_SET_RVAL(cx, vp, UndefinedValue());
            return JS_TRUE;
        }
    } else if (gen->state == JSGEN_CLOSED) {
      closed_generator:
        switch (op) {
          case JSGENOP_NEXT:
          case JSGENOP_SEND:
            return js_ThrowStopIteration(cx);
          case JSGENOP_THROW:
            cx->setPendingException(argc >= 1 ? vp[2] : UndefinedValue());
            return JS_FALSE;
          default:
            JS_ASSERT(op == JSGENOP_CLOSE);
            JS_SET_RVAL(cx, vp, UndefinedValue());
            return JS_TRUE;
        }
    }

    bool undef = ((op == JSGENOP_SEND || op == JSGENOP_THROW) && argc != 0);
    if (!SendToGenerator(cx, op, obj, gen, undef ? vp[2] : UndefinedValue()))
        return JS_FALSE;

    JS_SET_RVAL(cx, vp, gen->floatingFrame()->returnValue());
    return JS_TRUE;
}

static JSBool
generator_send(JSContext *cx, uintN argc, Value *vp)
{
    return generator_op(cx, JSGENOP_SEND, vp, argc);
}

static JSBool
generator_next(JSContext *cx, uintN argc, Value *vp)
{
    return generator_op(cx, JSGENOP_NEXT, vp, argc);
}

static JSBool
generator_throw(JSContext *cx, uintN argc, Value *vp)
{
    return generator_op(cx, JSGENOP_THROW, vp, argc);
}

static JSBool
generator_close(JSContext *cx, uintN argc, Value *vp)
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

    proto = js_InitClass(cx, obj, NULL, &js_IteratorClass, Iterator, 2,
                         NULL, iterator_methods, NULL, NULL);
    if (!proto)
        return NULL;

#if JS_HAS_GENERATORS
    
    if (!js_InitClass(cx, obj, NULL, &js_GeneratorClass, NULL, 0,
                      NULL, generator_methods, NULL, NULL)) {
        return NULL;
    }
#endif

    return js_InitClass(cx, obj, NULL, &js_StopIterationClass, NULL, 0,
                        NULL, NULL, NULL, NULL);
}

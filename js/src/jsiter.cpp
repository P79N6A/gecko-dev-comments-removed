









#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsproxy.h"
#include "jsscript.h"

#include "ds/Sort.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "vm/GlobalObject.h"
#include "vm/Shape.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "builtin/Iterator-inl.h"
#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayLength;

typedef Rooted<PropertyIteratorObject*> RootedPropertyIteratorObject;

static const gc::AllocKind ITERATOR_FINALIZE_KIND = gc::FINALIZE_OBJECT2;

void
NativeIterator::mark(JSTracer *trc)
{
    for (HeapPtr<JSFlatString> *str = begin(); str < end(); str++)
        MarkString(trc, str, "prop");
    if (obj)
        MarkObject(trc, &obj, "obj");
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

typedef HashSet<jsid, IdHashPolicy> IdSet;

static inline bool
NewKeyValuePair(JSContext *cx, jsid id, const Value &val, MutableHandleValue rval)
{
    Value vec[2] = { IdToValue(id), val };
    AutoArrayRooter tvr(cx, ArrayLength(vec), vec);

    RawObject aobj = NewDenseCopiedArray(cx, 2, vec);
    if (!aobj)
        return false;
    rval.setObject(*aobj);
    return true;
}

static inline bool
Enumerate(JSContext *cx, HandleObject pobj, jsid id,
          bool enumerable, unsigned flags, IdSet& ht, AutoIdVector *props)
{
    







    if (JS_UNLIKELY(!pobj->getTaggedProto().isObject() && JSID_IS_ATOM(id, cx->names().proto)))
        return true;

    if (!(flags & JSITER_OWNONLY) || pobj->isProxy() || pobj->getOps()->enumerate) {
        
        IdSet::AddPtr p = ht.lookupForAdd(id);
        if (JS_UNLIKELY(!!p))
            return true;

        




        if ((pobj->isProxy() || pobj->getProto() || pobj->getOps()->enumerate) && !ht.add(p, id))
            return false;
    }

    if (enumerable || (flags & JSITER_HIDDEN))
        return props->append(id);

    return true;
}

static bool
EnumerateNativeProperties(JSContext *cx, HandleObject pobj, unsigned flags, IdSet &ht,
                          AutoIdVector *props)
{
    
    size_t initlen = pobj->getDenseInitializedLength();
    const Value *vp = pobj->getDenseElements();
    for (size_t i = 0; i < initlen; ++i, ++vp) {
        if (!vp->isMagic(JS_ELEMENTS_HOLE)) {
            
            if (!Enumerate(cx, pobj, INT_TO_JSID(i), true, flags, ht, props))
                return false;
        }
    }

    size_t initialLength = props->length();

    
    Shape::Range r = pobj->lastProperty()->all();
    Shape::Range::AutoRooter root(cx, &r);
    for (; !r.empty(); r.popFront()) {
        Shape &shape = r.front();

        if (!Enumerate(cx, pobj, shape.propid(), shape.enumerable(), flags, ht, props))
            return false;
    }

    ::Reverse(props->begin() + initialLength, props->end());
    return true;
}

#ifdef JS_MORE_DETERMINISTIC

struct SortComparatorIds
{
    JSContext   *const cx;

    SortComparatorIds(JSContext *cx)
      : cx(cx) {}

    bool operator()(jsid a, jsid b, bool *lessOrEqualp)
    {
        
        JSString *astr = IdToString(cx, a);
	if (!astr)
	    return false;
        JSString *bstr = IdToString(cx, b);
        if (!bstr)
            return false;

        int32_t result;
        if (!CompareStrings(cx, astr, bstr, &result))
            return false;

        *lessOrEqualp = (result <= 0);
        return true;
    }
};

#endif 

static bool
Snapshot(JSContext *cx, RawObject pobj_, unsigned flags, AutoIdVector *props)
{
    IdSet ht(cx);
    if (!ht.init(32))
        return false;

    RootedObject pobj(cx, pobj_);

    do {
        Class *clasp = pobj->getClass();
        if (pobj->isNative() &&
            !pobj->getOps()->enumerate &&
            !(clasp->flags & JSCLASS_NEW_ENUMERATE)) {
            if (!clasp->enumerate(cx, pobj))
                return false;
            if (!EnumerateNativeProperties(cx, pobj, flags, ht, props))
                return false;
        } else {
            if (pobj->isProxy()) {
                AutoIdVector proxyProps(cx);
                if (flags & JSITER_OWNONLY) {
                    if (flags & JSITER_HIDDEN) {
                        if (!Proxy::getOwnPropertyNames(cx, pobj, proxyProps))
                            return false;
                    } else {
                        if (!Proxy::keys(cx, pobj, proxyProps))
                            return false;
                    }
                } else {
                    if (!Proxy::enumerate(cx, pobj, proxyProps))
                        return false;
                }
                for (size_t n = 0, len = proxyProps.length(); n < len; n++) {
                    if (!Enumerate(cx, pobj, proxyProps[n], true, flags, ht, props))
                        return false;
                }
                
                break;
            }
            RootedValue state(cx);
            RootedId id(cx);
            JSIterateOp op = (flags & JSITER_HIDDEN) ? JSENUMERATE_INIT_ALL : JSENUMERATE_INIT;
            if (!JSObject::enumerate(cx, pobj, op, &state, &id))
                return false;
            if (state.isMagic(JS_NATIVE_ENUMERATE)) {
                if (!EnumerateNativeProperties(cx, pobj, flags, ht, props))
                    return false;
            } else {
                while (true) {
                    RootedId id(cx);
                    if (!JSObject::enumerate(cx, pobj, JSENUMERATE_NEXT, &state, &id))
                        return false;
                    if (state.isNull())
                        break;
                    if (!Enumerate(cx, pobj, id, true, flags, ht, props))
                        return false;
                }
            }
        }

        if (flags & JSITER_OWNONLY)
            break;

    } while ((pobj = pobj->getProto()) != NULL);

#ifdef JS_MORE_DETERMINISTIC

    














    jsid *ids = props->begin();
    size_t n = props->length();

    AutoIdVector tmp(cx);
    if (!tmp.resize(n))
        return false;
    PodCopy(tmp.begin(), ids, n);

    if (!MergeSort(ids, n, tmp.begin(), SortComparatorIds(cx)))
        return false;

#endif 

    return true;
}

bool
js::VectorToIdArray(JSContext *cx, AutoIdVector &props, JSIdArray **idap)
{
    JS_STATIC_ASSERT(sizeof(JSIdArray) > sizeof(jsid));
    size_t len = props.length();
    size_t idsz = len * sizeof(jsid);
    size_t sz = (sizeof(JSIdArray) - sizeof(jsid)) + idsz;
    JSIdArray *ida = static_cast<JSIdArray *>(cx->malloc_(sz));
    if (!ida)
        return false;

    ida->length = static_cast<int>(len);
    jsid *v = props.begin();
    for (int i = 0; i < ida->length; i++)
        ida->vector[i].init(v[i]);
    *idap = ida;
    return true;
}

JS_FRIEND_API(bool)
js::GetPropertyNames(JSContext *cx, JSObject *obj, unsigned flags, AutoIdVector *props)
{
    return Snapshot(cx, obj, flags & (JSITER_OWNONLY | JSITER_HIDDEN), props);
}

size_t sCustomIteratorCount = 0;

static inline bool
GetCustomIterator(JSContext *cx, HandleObject obj, unsigned flags, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    
    HandlePropertyName name = cx->names().iteratorIntrinsic;
    if (!GetMethod(cx, obj, name, 0, vp))
        return false;

    
    if (!vp.isObject()) {
        vp.setUndefined();
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sCustomIteratorCount;

    
    Value arg = BooleanValue((flags & JSITER_FOREACH) == 0);
    if (!Invoke(cx, ObjectValue(*obj), vp, 1, &arg, vp.address()))
        return false;
    if (vp.isPrimitive()) {
        



        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;
        RootedValue val(cx, ObjectValue(*obj));
        js_ReportValueError2(cx, JSMSG_BAD_TRAP_RETURN_VALUE,
                             -1, val, NullPtr(), bytes.ptr());
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

static inline PropertyIteratorObject *
NewPropertyIteratorObject(JSContext *cx, unsigned flags)
{
    if (flags & JSITER_ENUMERATE) {
        RootedTypeObject type(cx, cx->compartment->getNewType(cx, &PropertyIteratorObject::class_, NULL));
        if (!type)
            return NULL;

        RootedShape shape(cx, EmptyShape::getInitialShape(cx, &PropertyIteratorObject::class_,
                                                          NULL, NULL, ITERATOR_FINALIZE_KIND));
        if (!shape)
            return NULL;

        RawObject obj = JSObject::create(cx, ITERATOR_FINALIZE_KIND, gc::DefaultHeap, shape, type, NULL);
        if (!obj)
            return NULL;

        JS_ASSERT(obj->numFixedSlots() == JSObject::ITER_CLASS_NFIXED_SLOTS);
        return &obj->asPropertyIterator();
    }

    return &NewBuiltinClassInstance(cx, &PropertyIteratorObject::class_)->asPropertyIterator();
}

NativeIterator *
NativeIterator::allocateIterator(JSContext *cx, uint32_t slength, const AutoIdVector &props)
{
    size_t plength = props.length();
    NativeIterator *ni = (NativeIterator *)
        cx->malloc_(sizeof(NativeIterator)
                    + plength * sizeof(RawString)
                    + slength * sizeof(RawShape));
    if (!ni)
        return NULL;
    AutoValueVector strings(cx);
    ni->props_array = ni->props_cursor = (HeapPtr<JSFlatString> *) (ni + 1);
    ni->props_end = ni->props_array + plength;
    if (plength) {
        for (size_t i = 0; i < plength; i++) {
            JSFlatString *str = IdToString(cx, props[i]);
            if (!str || !strings.append(StringValue(str)))
                return NULL;
            ni->props_array[i].init(str);
        }
    }
    ni->next_ = NULL;
    ni->prev_ = NULL;
    return ni;
}

NativeIterator *
NativeIterator::allocateSentinel(JSContext *cx)
{
    NativeIterator *ni = (NativeIterator *)js_malloc(sizeof(NativeIterator));
    if (!ni)
        return NULL;

    PodZero(ni);

    ni->next_ = ni;
    ni->prev_ = ni;
    return ni;
}

inline void
NativeIterator::init(RawObject obj, RawObject iterObj, unsigned flags, uint32_t slength, uint32_t key)
{
    this->obj.init(obj);
    this->iterObj_ = iterObj;
    this->flags = flags;
    this->shapes_array = (Shape **) this->props_end;
    this->shapes_length = slength;
    this->shapes_key = key;
}

static inline void
RegisterEnumerator(JSContext *cx, PropertyIteratorObject *iterobj, NativeIterator *ni)
{
    
    if (ni->flags & JSITER_ENUMERATE) {
        ni->link(cx->compartment->enumerators);

        JS_ASSERT(!(ni->flags & JSITER_ACTIVE));
        ni->flags |= JSITER_ACTIVE;
    }
}

static inline bool
VectorToKeyIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &keys,
                    uint32_t slength, uint32_t key, MutableHandleValue vp)
{
    JS_ASSERT(!(flags & JSITER_FOREACH));

    if (obj) {
        if (obj->hasSingletonType() && !obj->setIteratedSingleton(cx))
            return false;
        types::MarkTypeObjectFlags(cx, obj, types::OBJECT_FLAG_ITERATED);
    }

    Rooted<PropertyIteratorObject *> iterobj(cx, NewPropertyIteratorObject(cx, flags));
    if (!iterobj)
        return false;

    NativeIterator *ni = NativeIterator::allocateIterator(cx, slength, keys);
    if (!ni)
        return false;
    ni->init(obj, iterobj, flags, slength, key);

    if (slength) {
        






        RawObject pobj = obj;
        size_t ind = 0;
        do {
            ni->shapes_array[ind++] = pobj->lastProperty();
            pobj = pobj->getProto();
        } while (pobj);
        JS_ASSERT(ind == slength);
    }

    iterobj->setNativeIterator(ni);
    vp.setObject(*iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

bool
js::VectorToKeyIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &props,
                        MutableHandleValue vp)
{
    return VectorToKeyIterator(cx, obj, flags, props, 0, 0, vp);
}

bool
js::VectorToValueIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &keys,
                          MutableHandleValue vp)
{
    JS_ASSERT(flags & JSITER_FOREACH);

    if (obj) {
        if (obj->hasSingletonType() && !obj->setIteratedSingleton(cx))
            return false;
        types::MarkTypeObjectFlags(cx, obj, types::OBJECT_FLAG_ITERATED);
    }

    Rooted<PropertyIteratorObject*> iterobj(cx, NewPropertyIteratorObject(cx, flags));
    if (!iterobj)
        return false;

    NativeIterator *ni = NativeIterator::allocateIterator(cx, 0, keys);
    if (!ni)
        return false;
    ni->init(obj, iterobj, flags, 0, 0);

    iterobj->setNativeIterator(ni);
    vp.setObject(*iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

bool
js::EnumeratedIdVectorToIterator(JSContext *cx, HandleObject obj, unsigned flags,
                                 AutoIdVector &props, MutableHandleValue vp)
{
    if (!(flags & JSITER_FOREACH))
        return VectorToKeyIterator(cx, obj, flags, props, vp);

    return VectorToValueIterator(cx, obj, flags, props, vp);
}

static inline void
UpdateNativeIterator(NativeIterator *ni, RawObject obj)
{
    
    
    ni->obj = obj;
}

bool
js::GetIterator(JSContext *cx, HandleObject obj, unsigned flags, MutableHandleValue vp)
{
    if (flags == JSITER_FOR_OF) {
        
        RootedValue method(cx);
        if (!JSObject::getProperty(cx, obj, obj, cx->names().iterator, &method))
            return false;

        
        
        
        if (!method.isObject() || !method.toObject().isCallable()) {
            RootedValue val(cx, ObjectOrNullValue(obj));
            char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, val, NullPtr());
            if (!bytes)
                return false;
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_ITERABLE, bytes);
            js_free(bytes);
            return false;
        }

        if (!Invoke(cx, ObjectOrNullValue(obj), method, 0, NULL, vp.address()))
            return false;

        RawObject obj = ToObject(cx, vp);
        if (!obj)
            return false;
        vp.setObject(*obj);
        return true;
    }

    Vector<RawShape, 8> shapes(cx);
    uint32_t key = 0;

    bool keysOnly = (flags == JSITER_ENUMERATE);

    if (obj) {
        if (JSIteratorOp op = obj->getClass()->ext.iteratorObject) {
            RawObject iterobj = op(cx, obj, !(flags & JSITER_FOREACH));
            if (!iterobj)
                return false;
            vp.setObject(*iterobj);
            types::MarkIteratorUnknown(cx);
            return true;
        }

        if (keysOnly) {
            





            PropertyIteratorObject *last = cx->runtime->nativeIterCache.last;
            if (last) {
                NativeIterator *lastni = last->getNativeIterator();
                if (!(lastni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                    obj->isNative() &&
                    obj->hasEmptyElements() &&
                    obj->lastProperty() == lastni->shapes_array[0])
                {
                    JSObject *proto = obj->getProto();
                    if (proto->isNative() &&
                        proto->hasEmptyElements() &&
                        proto->lastProperty() == lastni->shapes_array[1] &&
                        !proto->getProto())
                    {
                        vp.setObject(*last);
                        UpdateNativeIterator(lastni, obj);
                        RegisterEnumerator(cx, last, lastni);
                        return true;
                    }
                }
            }

            





            {
                AutoAssertNoGC nogc;
                RawObject pobj = obj;
                do {
                    if (!pobj->isNative() ||
                        !pobj->hasEmptyElements() ||
                        pobj->hasUncacheableProto() ||
                        obj->getOps()->enumerate ||
                        pobj->getClass()->enumerate != JS_EnumerateStub) {
                        shapes.clear();
                        goto miss;
                    }
                    RawShape shape = pobj->lastProperty();
                    key = (key + (key << 16)) ^ (uintptr_t(shape) >> 3);
                    if (!shapes.append(shape))
                        return false;
                    pobj = pobj->getProto();
                } while (pobj);
            }

            PropertyIteratorObject *iterobj = cx->runtime->nativeIterCache.get(key);
            if (iterobj) {
                NativeIterator *ni = iterobj->getNativeIterator();
                if (!(ni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                    ni->shapes_key == key &&
                    ni->shapes_length == shapes.length() &&
                    Compare(ni->shapes_array, shapes.begin(), ni->shapes_length)) {
                    vp.setObject(*iterobj);

                    UpdateNativeIterator(ni, obj);
                    RegisterEnumerator(cx, iterobj, ni);
                    if (shapes.length() == 2)
                        cx->runtime->nativeIterCache.last = iterobj;
                    return true;
                }
            }
        }

      miss:
        if (obj->isProxy()) {
            types::MarkIteratorUnknown(cx);
            return Proxy::iterate(cx, obj, flags, vp);
        }
        if (!GetCustomIterator(cx, obj, flags, vp))
            return false;
        if (!vp.isUndefined()) {
            types::MarkIteratorUnknown(cx);
            return true;
        }
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

    PropertyIteratorObject *iterobj = &vp.toObject().asPropertyIterator();

    
    if (shapes.length())
        cx->runtime->nativeIterCache.set(key, iterobj);

    if (shapes.length() == 2)
        cx->runtime->nativeIterCache.last = iterobj;
    return true;
}

JSObject *
js::GetIteratorObject(JSContext *cx, HandleObject obj, uint32_t flags)
{
    RootedValue value(cx);
    if (!GetIterator(cx, obj, flags, &value))
        return NULL;
    return &value.toObject();
}

JSBool
js_ThrowStopIteration(JSContext *cx)
{
    JS_ASSERT(!JS_IsExceptionPending(cx));
    RootedValue v(cx);
    if (js_FindClassObject(cx, JSProto_StopIteration, &v))
        cx->setPendingException(v);
    return false;
}



JSBool
js::IteratorConstructor(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() == 0) {
        js_ReportMissingArg(cx, args.calleev(), 0);
        return false;
    }

    bool keyonly = false;
    if (args.length() >= 2)
        keyonly = ToBoolean(args[1]);
    unsigned flags = JSITER_OWNONLY | (keyonly ? 0 : (JSITER_FOREACH | JSITER_KEYVALUE));

    if (!ValueToIterator(cx, flags, MutableHandleValue::fromMarkedLocation(&args[0])))
        return false;
    args.rval().set(args[0]);
    return true;
}

JS_ALWAYS_INLINE bool
IsIterator(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&PropertyIteratorObject::class_);
}

JS_ALWAYS_INLINE bool
iterator_next_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsIterator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    if (!js_IteratorMore(cx, thisObj, args.rval()))
        return false;

    if (!args.rval().toBoolean()) {
        js_ThrowStopIteration(cx);
        return false;
    }

    return js_IteratorNext(cx, thisObj, args.rval());
}

static JSBool
iterator_iterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(args.thisv());
    return true;
}

JSBool
iterator_next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsIterator, iterator_next_impl>(cx, args);
}

static JSFunctionSpec iterator_methods[] = {
    JS_FN("iterator",  iterator_iterator,   0, 0),
    JS_FN("next",      iterator_next,       0, 0),
    JS_FS_END
};

static JSObject *
iterator_iteratorObject(JSContext *cx, HandleObject obj, JSBool keysonly)
{
    return obj;
}

size_t
PropertyIteratorObject::sizeOfMisc(JSMallocSizeOfFun mallocSizeOf) const
{
    return mallocSizeOf(getPrivate());
}

void
PropertyIteratorObject::trace(JSTracer *trc, RawObject obj)
{
    if (NativeIterator *ni = obj->asPropertyIterator().getNativeIterator())
        ni->mark(trc);
}

void
PropertyIteratorObject::finalize(FreeOp *fop, RawObject obj)
{
    if (NativeIterator *ni = obj->asPropertyIterator().getNativeIterator()) {
        obj->asPropertyIterator().setNativeIterator(NULL);
        fop->free_(ni);
    }
}

Class PropertyIteratorObject::class_ = {
    "Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator) |
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    trace,
    {
        NULL,                
        NULL,                
        iterator_iteratorObject,
    }
};

const uint32_t CLOSED_INDEX = UINT32_MAX;

JSObject *
ElementIteratorObject::create(JSContext *cx, Handle<Value> target)
{
    RootedObject proto(cx, cx->global()->getOrCreateElementIteratorPrototype(cx));
    if (!proto)
        return NULL;
    RootedObject iterobj(cx, NewObjectWithGivenProto(cx, &ElementIteratorClass, proto, cx->global()));
    if (iterobj) {
        iterobj->setReservedSlot(TargetSlot, target);
        iterobj->setReservedSlot(IndexSlot, Int32Value(0));
    }
    return iterobj;
}

static bool
IsElementIterator(const Value &v)
{
    return v.isObject() && v.toObject().isElementIterator();
}

JSBool
ElementIteratorObject::next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, IsElementIterator, next_impl, args);
}

bool
ElementIteratorObject::next_impl(JSContext *cx, CallArgs args)
{
    RootedObject iterobj(cx, &args.thisv().toObject());
    uint32_t i, length;
    RootedValue target(cx, iterobj->getReservedSlot(TargetSlot));
    RootedObject obj(cx);

    
    if (target.isString()) {
        length = uint32_t(target.toString()->length());
    } else {
        obj = ToObjectFromStack(cx, target);
        if (!obj)
            goto close;
        if (!GetLengthProperty(cx, obj, &length))
            goto close;
    }

    
    i = uint32_t(iterobj->getReservedSlot(IndexSlot).toInt32());
    if (i >= length) {
        js_ThrowStopIteration(cx);
        goto close;
    }

    
    JS_ASSERT(i + 1 > i);
    if (target.isString()) {
        JSString *c = cx->runtime->staticStrings.getUnitStringForElement(cx, target.toString(), i);
        if (!c)
            goto close;
        args.rval().setString(c);
    } else {
        if (!JSObject::getElement(cx, obj, obj, i, args.rval()))
            goto close;
    }

    
    iterobj->setReservedSlot(IndexSlot, Int32Value(int32_t(i + 1)));
    return true;

  close:
    
    
    iterobj->setReservedSlot(TargetSlot, UndefinedValue());
    iterobj->setReservedSlot(IndexSlot, Int32Value(int32_t(CLOSED_INDEX)));
    return false;
}

Class js::ElementIteratorClass = {
    "Array Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(ElementIteratorObject::NumSlots),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL                     
};

JSFunctionSpec ElementIteratorObject::methods[] = {
    JS_FN("next", next, 0, 0),
    JS_FS_END
};

#if JS_HAS_GENERATORS
static JSBool
CloseGenerator(JSContext *cx, HandleObject genobj);
#endif

bool
js::ValueToIterator(JSContext *cx, unsigned flags, MutableHandleValue vp)
{
    
    JS_ASSERT_IF(flags & JSITER_KEYVALUE, flags & JSITER_FOREACH);

    




    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    RootedObject obj(cx);
    if (vp.isObject()) {
        
        obj = &vp.toObject();
    } else {
        






        if (flags & JSITER_ENUMERATE) {
            if (!js_ValueToObjectOrNull(cx, vp, &obj))
                return false;
            
        } else {
            obj = js_ValueToNonNullObject(cx, vp);
            if (!obj)
                return false;
        }
    }

    return GetIterator(cx, obj, flags, vp);
}

bool
js::CloseIterator(JSContext *cx, HandleObject obj)
{
    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    if (obj->isPropertyIterator()) {
        
        NativeIterator *ni = obj->asPropertyIterator().getNativeIterator();

        if (ni->flags & JSITER_ENUMERATE) {
            ni->unlink();

            JS_ASSERT(ni->flags & JSITER_ACTIVE);
            ni->flags &= ~JSITER_ACTIVE;

            



            ni->props_cursor = ni->props_array;
        }
    }
#if JS_HAS_GENERATORS
    else if (obj->isGenerator()) {
        return CloseGenerator(cx, obj);
    }
#endif
    return true;
}

bool
js::UnwindIteratorForException(JSContext *cx, HandleObject obj)
{
    RootedValue v(cx, cx->getPendingException());
    cx->clearPendingException();
    if (!CloseIterator(cx, obj))
        return false;
    cx->setPendingException(v);
    return true;
}

void
js::UnwindIteratorForUncatchableException(JSContext *cx, RawObject obj)
{
    if (obj->isPropertyIterator()) {
        NativeIterator *ni = obj->asPropertyIterator().getNativeIterator();
        if (ni->flags & JSITER_ENUMERATE)
            ni->unlink();
    }
}


















template<typename StringPredicate>
static bool
SuppressDeletedPropertyHelper(JSContext *cx, HandleObject obj, StringPredicate predicate)
{
    NativeIterator *enumeratorList = cx->compartment->enumerators;
    NativeIterator *ni = enumeratorList->next();

    while (ni != enumeratorList) {
      again:
        
        if (ni->isKeyIter() && ni->obj == obj && ni->props_cursor < ni->props_end) {
            
            HeapPtr<JSFlatString> *props_cursor = ni->current();
            HeapPtr<JSFlatString> *props_end = ni->end();
            for (HeapPtr<JSFlatString> *idp = props_cursor; idp < props_end; ++idp) {
                if (predicate(*idp)) {
                     



                    AutoObjectRooter iterRoot(cx, ni->iterObj());

                    



                    RootedObject proto(cx);
                    if (!JSObject::getProto(cx, obj, &proto))
                        return false;
                    if (proto) {
                        RootedObject obj2(cx);
                        RootedShape prop(cx);
                        RootedId id(cx);
                        if (!ValueToId<CanGC>(cx, StringValue(*idp), &id))
                            return false;
                        if (!JSObject::lookupGeneric(cx, proto, id, &obj2, &prop))
                            return false;
                        if (prop) {
                            unsigned attrs;
                            if (obj2->isNative())
                                attrs = GetShapeAttributes(prop);
                            else if (!JSObject::getGenericAttributes(cx, obj2, id, &attrs))
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
                        for (HeapPtr<JSFlatString> *p = idp; p + 1 != props_end; p++)
                            *p = *(p + 1);
                        ni->props_end = ni->end() - 1;

                        




                        *ni->props_end = NULL;
                    }

                    
                    ni->flags |= JSITER_UNREUSABLE;

                    if (predicate.matchesAtMostOne())
                        break;
                }
            }
        }
        ni = ni->next();
    }
    return true;
}

class SingleStringPredicate {
    Handle<JSFlatString*> str;
public:
    SingleStringPredicate(Handle<JSFlatString*> str) : str(str) {}

    bool operator()(JSFlatString *str) { return EqualStrings(str, this->str); }
    bool matchesAtMostOne() { return true; }
};

bool
js_SuppressDeletedProperty(JSContext *cx, HandleObject obj, jsid id)
{
    Rooted<JSFlatString*> str(cx, IdToString(cx, id));
    if (!str)
        return false;
    return SuppressDeletedPropertyHelper(cx, obj, SingleStringPredicate(str));
}

bool
js_SuppressDeletedElement(JSContext *cx, HandleObject obj, uint32_t index)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return js_SuppressDeletedProperty(cx, obj, id);
}

class IndexRangePredicate {
    uint32_t begin, end;

  public:
    IndexRangePredicate(uint32_t begin, uint32_t end) : begin(begin), end(end) {}

    bool operator()(JSFlatString *str) {
        uint32_t index;
        return str->isIndex(&index) && begin <= index && index < end;
    }

    bool matchesAtMostOne() { return false; }
};

bool
js_SuppressDeletedElements(JSContext *cx, HandleObject obj, uint32_t begin, uint32_t end)
{
    return SuppressDeletedPropertyHelper(cx, obj, IndexRangePredicate(begin, end));
}

bool
js_IteratorMore(JSContext *cx, HandleObject iterobj, MutableHandleValue rval)
{
    
    NativeIterator *ni = NULL;
    if (iterobj->isPropertyIterator()) {
        
        ni = iterobj->asPropertyIterator().getNativeIterator();
        bool more = ni->props_cursor < ni->props_end;
        if (ni->isKeyIter() || !more) {
            rval.setBoolean(more);
            return true;
        }
    }

    
    if (!cx->iterValue.isMagic(JS_NO_ITER_VALUE)) {
        rval.setBoolean(true);
        return true;
    }

    
    JS_CHECK_RECURSION(cx, return false);

    
    if (ni) {
        JS_ASSERT(!ni->isKeyIter());
        RootedId id(cx);
        if (!ValueToId<CanGC>(cx, StringValue(*ni->current()), &id))
            return false;
        ni->incCursor();
        RootedObject obj(cx, ni->obj);
        if (!JSObject::getGeneric(cx, obj, obj, id, rval))
            return false;
        if ((ni->flags & JSITER_KEYVALUE) && !NewKeyValuePair(cx, id, rval, rval))
            return false;
    } else {
        
        if (!GetMethod(cx, iterobj, cx->names().next, 0, rval))
            return false;
        if (!Invoke(cx, ObjectValue(*iterobj), rval, 0, NULL, rval.address())) {
            
            if (!cx->isExceptionPending() || !IsStopIteration(cx->getPendingException()))
                return false;

            cx->clearPendingException();
            cx->iterValue.setMagic(JS_NO_ITER_VALUE);
            rval.setBoolean(false);
            return true;
        }
    }

    
    JS_ASSERT(!rval.isMagic(JS_NO_ITER_VALUE));
    cx->iterValue = rval;
    rval.setBoolean(true);
    return true;
}

bool
js_IteratorNext(JSContext *cx, HandleObject iterobj, MutableHandleValue rval)
{
    
    if (iterobj->isPropertyIterator()) {
        



        NativeIterator *ni = iterobj->asPropertyIterator().getNativeIterator();
        if (ni->isKeyIter()) {
            JS_ASSERT(ni->props_cursor < ni->props_end);
            rval.setString(*ni->current());
            ni->incCursor();
            return true;
        }
    }

    JS_ASSERT(!cx->iterValue.isMagic(JS_NO_ITER_VALUE));
    rval.set(cx->iterValue);
    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    return true;
}

static JSBool
stopiter_hasInstance(JSContext *cx, HandleObject obj, MutableHandleValue v, JSBool *bp)
{
    *bp = IsStopIteration(v);
    return true;
}

Class js::StopIterationClass = {
    "StopIteration",
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration) |
    JSCLASS_FREEZE_PROTO,
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
    stopiter_hasInstance,
    NULL                     
};



#if JS_HAS_GENERATORS

static void
generator_finalize(FreeOp *fop, RawObject obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    



    JS_ASSERT(gen->state == JSGEN_NEWBORN ||
              gen->state == JSGEN_CLOSED ||
              gen->state == JSGEN_OPEN);
    JS_POISON(gen->fp, JS_FREE_PATTERN, sizeof(StackFrame));
    JS_POISON(gen, JS_FREE_PATTERN, sizeof(JSGenerator));
    fop->free_(gen);
}

static void
MarkGeneratorFrame(JSTracer *trc, JSGenerator *gen)
{
    MarkValueRange(trc,
                   HeapValueify(gen->fp->generatorArgsSnapshotBegin()),
                   HeapValueify(gen->fp->generatorArgsSnapshotEnd()),
                   "Generator Floating Args");
    gen->fp->mark(trc);
    MarkValueRange(trc,
                   HeapValueify(gen->fp->generatorSlotsSnapshotBegin()),
                   HeapValueify(gen->regs.sp),
                   "Generator Floating Stack");
}

static void
GeneratorWriteBarrierPre(JSContext *cx, JSGenerator *gen)
{
    JS::Zone *zone = cx->zone();
    if (zone->needsBarrier())
        MarkGeneratorFrame(zone->barrierTracer(), gen);
}






bool
js::GeneratorHasMarkableFrame(JSGenerator *gen)
{
    return gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN;
}





static void
SetGeneratorClosed(JSContext *cx, JSGenerator *gen)
{
    JS_ASSERT(gen->state != JSGEN_CLOSED);
    if (GeneratorHasMarkableFrame(gen))
        GeneratorWriteBarrierPre(cx, gen);
    gen->state = JSGEN_CLOSED;
}

static void
generator_trace(JSTracer *trc, RawObject obj)
{
    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen)
        return;

    if (GeneratorHasMarkableFrame(gen))
        MarkGeneratorFrame(trc, gen);
}

Class js::GeneratorClass = {
    "Generator",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    generator_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    generator_trace,
    {
        NULL,                
        NULL,                
        iterator_iteratorObject,
    }
};









JSObject *
js_NewGenerator(JSContext *cx)
{
    FrameRegs &stackRegs = cx->regs();
    JS_ASSERT(stackRegs.stackDepth() == 0);
    StackFrame *stackfp = stackRegs.fp();

    Rooted<GlobalObject*> global(cx, &stackfp->global());
    RootedObject obj(cx);
    {
        RawObject proto = global->getOrCreateGeneratorPrototype(cx);
        if (!proto)
            return NULL;
        obj = NewObjectWithGivenProto(cx, &GeneratorClass, proto, global);
    }
    if (!obj)
        return NULL;

    
    Value *stackvp = stackfp->generatorArgsSnapshotBegin();
    unsigned vplen = stackfp->generatorArgsSnapshotEnd() - stackvp;

    
    unsigned nbytes = sizeof(JSGenerator) +
                   (-1 + 
                    vplen +
                    VALUES_PER_STACK_FRAME +
                    stackfp->script()->nslots) * sizeof(HeapValue);

    JS_ASSERT(nbytes % sizeof(Value) == 0);
    JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(HeapValue) == 0);

    JSGenerator *gen = (JSGenerator *) cx->malloc_(nbytes);
    if (!gen)
        return NULL;
    SetValueRangeToUndefined((Value *)gen, nbytes / sizeof(Value));

    
    HeapValue *genvp = gen->stackSnapshot;
    StackFrame *genfp = reinterpret_cast<StackFrame *>(genvp + vplen);

    
    gen->obj.init(obj);
    gen->state = JSGEN_NEWBORN;
    gen->fp = genfp;
    gen->prevGenerator = NULL;

    
    gen->regs.rebaseFromTo(stackRegs, *genfp);
    genfp->copyFrameAndValues<StackFrame::DoPostBarrier>(cx, (Value *)genvp, stackfp,
                                                         stackvp, stackRegs.sp);

    obj->setPrivate(gen);
    return obj;
}

static void
SetGeneratorClosed(JSContext *cx, JSGenerator *gen);

typedef enum JSGeneratorOp {
    JSGENOP_NEXT,
    JSGENOP_SEND,
    JSGENOP_THROW,
    JSGENOP_CLOSE
} JSGeneratorOp;





static JSBool
SendToGenerator(JSContext *cx, JSGeneratorOp op, HandleObject obj,
                JSGenerator *gen, const Value &arg)
{
    AssertCanGC();

    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NESTING_GENERATOR);
        return false;
    }

    










    GeneratorWriteBarrierPre(cx, gen);

    JSGeneratorState futureState;
    JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
    switch (op) {
      case JSGENOP_NEXT:
      case JSGENOP_SEND:
        if (gen->state == JSGEN_OPEN) {
            



            gen->regs.sp[-1] = arg;
        }
        futureState = JSGEN_RUNNING;
        break;

      case JSGENOP_THROW:
        cx->setPendingException(arg);
        futureState = JSGEN_RUNNING;
        break;

      default:
        JS_ASSERT(op == JSGENOP_CLOSE);
        cx->setPendingException(MagicValue(JS_GENERATOR_CLOSING));
        futureState = JSGEN_CLOSING;
        break;
    }

    JSBool ok;
    {
        GeneratorFrameGuard gfg;
        if (!cx->stack.pushGeneratorFrame(cx, gen, &gfg)) {
            SetGeneratorClosed(cx, gen);
            return false;
        }

        



        gen->state = futureState;

        StackFrame *fp = gfg.fp();
        gen->regs = cx->regs();

        cx->enterGenerator(gen);   
        ok = RunScript(cx, fp);
        cx->leaveGenerator(gen);
    }

    if (gen->fp->isYielding()) {
        



        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        gen->fp->clearYielding();
        gen->state = JSGEN_OPEN;
        return ok;
    }

    gen->fp->clearReturnValue();
    SetGeneratorClosed(cx, gen);
    if (ok) {
        
        if (op == JSGENOP_CLOSE)
            return true;
        return js_ThrowStopIteration(cx);
    }

    



    return false;
}

static JSBool
CloseGenerator(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isGenerator());

    JSGenerator *gen = (JSGenerator *) obj->getPrivate();
    if (!gen) {
        
        return true;
    }

    if (gen->state == JSGEN_CLOSED)
        return true;

    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, UndefinedValue());
}

JS_ALWAYS_INLINE bool
IsGenerator(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&GeneratorClass);
}

JS_ALWAYS_INLINE bool
generator_send_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsGenerator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = (JSGenerator *) thisObj->getPrivate();
    if (!gen || gen->state == JSGEN_CLOSED) {
        
        return js_ThrowStopIteration(cx);
    }

    if (gen->state == JSGEN_NEWBORN && args.hasDefined(0)) {
        RootedValue val(cx, args[0]);
        js_ReportValueError(cx, JSMSG_BAD_GENERATOR_SEND,
                            JSDVG_SEARCH_STACK, val, NullPtr());
        return false;
    }

    if (!SendToGenerator(cx, JSGENOP_SEND, thisObj, gen,
                         args.length() > 0 ? args[0] : UndefinedValue()))
    {
        return false;
    }

    args.rval().set(gen->fp->returnValue());
    return true;
}

JSBool
generator_send(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsGenerator, generator_send_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
generator_next_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsGenerator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = (JSGenerator *) thisObj->getPrivate();
    if (!gen || gen->state == JSGEN_CLOSED) {
        
        return js_ThrowStopIteration(cx);
    }

    if (!SendToGenerator(cx, JSGENOP_NEXT, thisObj, gen, UndefinedValue()))
        return false;

    args.rval().set(gen->fp->returnValue());
    return true;
}

JSBool
generator_next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsGenerator, generator_next_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
generator_throw_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsGenerator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = (JSGenerator *) thisObj->getPrivate();
    if (!gen || gen->state == JSGEN_CLOSED) {
        
        cx->setPendingException(args.length() >= 1 ? args[0] : UndefinedValue());
        return false;
    }

    if (!SendToGenerator(cx, JSGENOP_THROW, thisObj, gen,
                         args.length() > 0 ? args[0] : UndefinedValue()))
    {
        return false;
    }

    args.rval().set(gen->fp->returnValue());
    return true;
}

JSBool
generator_throw(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsGenerator, generator_throw_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
generator_close_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsGenerator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = (JSGenerator *) thisObj->getPrivate();
    if (!gen || gen->state == JSGEN_CLOSED) {
        
        args.rval().setUndefined();
        return true;
    }

    if (gen->state == JSGEN_NEWBORN) {
        SetGeneratorClosed(cx, gen);
        args.rval().setUndefined();
        return true;
    }

    if (!SendToGenerator(cx, JSGENOP_CLOSE, thisObj, gen, UndefinedValue()))
        return false;

    args.rval().set(gen->fp->returnValue());
    return true;
}

JSBool
generator_close(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsGenerator, generator_close_impl>(cx, args);
}

#define JSPROP_ROPERM   (JSPROP_READONLY | JSPROP_PERMANENT)

static JSFunctionSpec generator_methods[] = {
    JS_FN("iterator",  iterator_iterator,  0, 0),
    JS_FN("next",      generator_next,     0,JSPROP_ROPERM),
    JS_FN("send",      generator_send,     1,JSPROP_ROPERM),
    JS_FN("throw",     generator_throw,    1,JSPROP_ROPERM),
    JS_FN("close",     generator_close,    0,JSPROP_ROPERM),
    JS_FS_END
};

#endif 

 bool
GlobalObject::initIteratorClasses(JSContext *cx, Handle<GlobalObject *> global)
{
    RootedObject iteratorProto(cx);
    Value iteratorProtoVal = global->getPrototype(JSProto_Iterator);
    if (iteratorProtoVal.isObject()) {
        iteratorProto = &iteratorProtoVal.toObject();
    } else {
        iteratorProto = global->createBlankPrototype(cx, &PropertyIteratorObject::class_);
        if (!iteratorProto)
            return false;

        AutoIdVector blank(cx);
        NativeIterator *ni = NativeIterator::allocateIterator(cx, 0, blank);
        if (!ni)
            return false;
        ni->init(NULL, NULL, 0 , 0, 0);

        iteratorProto->asPropertyIterator().setNativeIterator(ni);

        Rooted<JSFunction*> ctor(cx);
        ctor = global->createConstructor(cx, IteratorConstructor, cx->names().Iterator, 2);
        if (!ctor)
            return false;
        if (!LinkConstructorAndPrototype(cx, ctor, iteratorProto))
            return false;
        if (!DefinePropertiesAndBrand(cx, iteratorProto, NULL, iterator_methods))
            return false;
        if (!DefineConstructorAndPrototype(cx, global, JSProto_Iterator, ctor, iteratorProto))
            return false;
    }

    RootedObject proto(cx);
    if (global->getSlot(ELEMENT_ITERATOR_PROTO).isUndefined()) {
        Class *cls = &ElementIteratorClass;
        proto = global->createBlankPrototypeInheriting(cx, cls, *iteratorProto);
        if (!proto || !DefinePropertiesAndBrand(cx, proto, NULL, ElementIteratorObject::methods))
            return false;
        global->setReservedSlot(ELEMENT_ITERATOR_PROTO, ObjectValue(*proto));
    }

#if JS_HAS_GENERATORS
    if (global->getSlot(GENERATOR_PROTO).isUndefined()) {
        proto = global->createBlankPrototype(cx, &GeneratorClass);
        if (!proto || !DefinePropertiesAndBrand(cx, proto, NULL, generator_methods))
            return false;
        global->setReservedSlot(GENERATOR_PROTO, ObjectValue(*proto));
    }
#endif

    if (global->getPrototype(JSProto_StopIteration).isUndefined()) {
        proto = global->createBlankPrototype(cx, &StopIterationClass);
        if (!proto || !JSObject::freeze(cx, proto))
            return false;

        
        if (!DefineConstructorAndPrototype(cx, global, JSProto_StopIteration, proto, proto))
            return false;

        MarkStandardClassInitializedNoProto(global, &StopIterationClass);
    }

    return true;
}

JSObject *
js_InitIteratorClasses(JSContext *cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->asGlobal());
    if (!GlobalObject::initIteratorClasses(cx, global))
        return NULL;
    return global->getIteratorPrototype();
}

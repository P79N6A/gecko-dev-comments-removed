







#include "jsiter.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jstypes.h"
#include "jsutil.h"

#include "ds/Sort.h"
#include "gc/Marking.h"
#include "vm/GeneratorObject.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/Shape.h"
#include "vm/StopIterationObject.h"
#include "vm/TypedArrayCommon.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;
using JS::ForOfIterator;

using mozilla::ArrayLength;
#ifdef JS_MORE_DETERMINISTIC
using mozilla::PodCopy;
#endif
using mozilla::PodZero;

typedef Rooted<PropertyIteratorObject*> RootedPropertyIteratorObject;

static const gc::AllocKind ITERATOR_FINALIZE_KIND = gc::FINALIZE_OBJECT2_BACKGROUND;

void
NativeIterator::mark(JSTracer *trc)
{
    for (HeapPtrFlatString *str = begin(); str < end(); str++)
        MarkString(trc, str, "prop");
    if (obj)
        MarkObject(trc, &obj, "obj");

    
    
    if (iterObj_)
        MarkObjectUnbarriered(trc, &iterObj_, "iterObj");
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
    JS::AutoValueArray<2> vec(cx);
    vec[0].set(IdToValue(id));
    vec[1].set(val);

    JSObject *aobj = NewDenseCopiedArray(cx, 2, vec.begin());
    if (!aobj)
        return false;
    rval.setObject(*aobj);
    return true;
}

static inline bool
Enumerate(JSContext *cx, HandleObject pobj, jsid id,
          bool enumerable, unsigned flags, IdSet& ht, AutoIdVector *props)
{
    
    
    
    
    
    
    if (MOZ_UNLIKELY(!pobj->getTaggedProto().isObject() && JSID_IS_ATOM(id, cx->names().proto)))
        return true;

    if (!(flags & JSITER_OWNONLY) || pobj->is<ProxyObject>() || pobj->getOps()->enumerate) {
        
        IdSet::AddPtr p = ht.lookupForAdd(id);
        if (MOZ_UNLIKELY(!!p))
            return true;

        
        
        
        if ((pobj->is<ProxyObject>() || pobj->getProto() || pobj->getOps()->enumerate) && !ht.add(p, id))
            return false;
    }

    
    
    
    if (JSID_IS_SYMBOL(id) ? !(flags & JSITER_SYMBOLS) : (flags & JSITER_SYMBOLSONLY))
        return true;
    if (!enumerable && !(flags & JSITER_HIDDEN))
        return true;

    return props->append(id);
}

static bool
EnumerateNativeProperties(JSContext *cx, HandleObject pobj, unsigned flags, IdSet &ht,
                          AutoIdVector *props)
{
    bool enumerateSymbols;
    if (flags & JSITER_SYMBOLSONLY) {
        enumerateSymbols = true;
    } else {
        
        size_t initlen = pobj->getDenseInitializedLength();
        const Value *vp = pobj->getDenseElements();
        for (size_t i = 0; i < initlen; ++i, ++vp) {
            if (!vp->isMagic(JS_ELEMENTS_HOLE)) {
                
                if (!Enumerate(cx, pobj, INT_TO_JSID(i),  true, flags, ht, props))
                    return false;
            }
        }

        
        if (IsAnyTypedArray(pobj)) {
            size_t len = AnyTypedArrayLength(pobj);
            for (size_t i = 0; i < len; i++) {
                if (!Enumerate(cx, pobj, INT_TO_JSID(i),  true, flags, ht, props))
                    return false;
            }
        }

        size_t initialLength = props->length();

        
        bool symbolsFound = false;
        Shape::Range<NoGC> r(pobj->lastProperty());
        for (; !r.empty(); r.popFront()) {
            Shape &shape = r.front();
            jsid id = shape.propid();

            if (JSID_IS_SYMBOL(id)) {
                symbolsFound = true;
                continue;
            }

            if (!Enumerate(cx, pobj, id, shape.enumerable(), flags, ht, props))
                return false;
        }
        ::Reverse(props->begin() + initialLength, props->end());

        enumerateSymbols = symbolsFound && (flags & JSITER_SYMBOLS);
    }

    if (enumerateSymbols) {
        
        
        
        size_t initialLength = props->length();
        for (Shape::Range<NoGC> r(pobj->lastProperty()); !r.empty(); r.popFront()) {
            Shape &shape = r.front();
            jsid id = shape.propid();
            if (JSID_IS_SYMBOL(id)) {
                if (!Enumerate(cx, pobj, id, shape.enumerable(), flags, ht, props))
                    return false;
            }
        }
        ::Reverse(props->begin() + initialLength, props->end());
    }

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
        
        
        if (a == b) {
            *lessOrEqualp = true;
            return true;
        }

        size_t ta = JSID_BITS(a) & JSID_TYPE_MASK;
        size_t tb = JSID_BITS(b) & JSID_TYPE_MASK;
        if (ta != tb) {
            *lessOrEqualp = (ta <= tb);
            return true;
        }

        if (JSID_IS_INT(a)) {
            *lessOrEqualp = (JSID_TO_INT(a) <= JSID_TO_INT(b));
            return true;
        }

        RootedString astr(cx), bstr(cx);
        if (JSID_IS_SYMBOL(a)) {
            MOZ_ASSERT(JSID_IS_SYMBOL(b));
            JS::SymbolCode ca = JSID_TO_SYMBOL(a)->code();
            JS::SymbolCode cb = JSID_TO_SYMBOL(b)->code();
            if (ca != cb) {
                *lessOrEqualp = uint32_t(ca) <= uint32_t(cb);
                return true;
            }
            JS_ASSERT(ca == JS::SymbolCode::InSymbolRegistry || ca == JS::SymbolCode::UniqueSymbol);
            astr = JSID_TO_SYMBOL(a)->description();
            bstr = JSID_TO_SYMBOL(b)->description();
            if (!astr || !bstr) {
                *lessOrEqualp = !astr;
                return true;
            }

            
            
            
        } else {
            astr = IdToString(cx, a);
            if (!astr)
                return false;
            bstr = IdToString(cx, b);
            if (!bstr)
                return false;
        }

        int32_t result;
        if (!CompareStrings(cx, astr, bstr, &result))
            return false;

        *lessOrEqualp = (result <= 0);
        return true;
    }
};

#endif 

static bool
Snapshot(JSContext *cx, JSObject *pobj_, unsigned flags, AutoIdVector *props)
{
    IdSet ht(cx);
    if (!ht.init(32))
        return false;

    RootedObject pobj(cx, pobj_);

    do {
        const Class *clasp = pobj->getClass();
        if (pobj->isNative() &&
            !pobj->getOps()->enumerate &&
            !(clasp->flags & JSCLASS_NEW_ENUMERATE))
        {
            if (!clasp->enumerate(cx, pobj))
                return false;
            if (!EnumerateNativeProperties(cx, pobj, flags, ht, props))
                return false;
        } else {
            if (pobj->is<ProxyObject>()) {
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

    } while ((pobj = pobj->getProto()) != nullptr);

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
    JSIdArray *ida = reinterpret_cast<JSIdArray *>(cx->zone()->pod_malloc<uint8_t>(sz));
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
    return Snapshot(cx, obj,
                    flags & (JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS | JSITER_SYMBOLSONLY),
                    props);
}

size_t sCustomIteratorCount = 0;

static inline bool
GetCustomIterator(JSContext *cx, HandleObject obj, unsigned flags, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    
    HandlePropertyName name = cx->names().iteratorIntrinsic;
    if (!JSObject::getProperty(cx, obj, obj, name, vp))
        return false;

    
    if (!vp.isObject()) {
        vp.setUndefined();
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sCustomIteratorCount;

    
    Value arg = BooleanValue((flags & JSITER_FOREACH) == 0);
    if (!Invoke(cx, ObjectValue(*obj), vp, 1, &arg, vp))
        return false;
    if (vp.isPrimitive()) {
        



        JSAutoByteString bytes;
        if (!AtomToPrintableString(cx, name, &bytes))
            return false;
        RootedValue val(cx, ObjectValue(*obj));
        js_ReportValueError2(cx, JSMSG_BAD_TRAP_RETURN_VALUE,
                             -1, val, js::NullPtr(), bytes.ptr());
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
        RootedTypeObject type(cx, cx->getNewType(&PropertyIteratorObject::class_, TaggedProto(nullptr)));
        if (!type)
            return nullptr;

        JSObject *metadata = nullptr;
        if (!NewObjectMetadata(cx, &metadata))
            return nullptr;

        const Class *clasp = &PropertyIteratorObject::class_;
        RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, TaggedProto(nullptr), nullptr, metadata,
                                                          ITERATOR_FINALIZE_KIND));
        if (!shape)
            return nullptr;

        JSObject *obj = JSObject::create(cx, ITERATOR_FINALIZE_KIND,
                                         GetInitialHeap(GenericObject, clasp), shape, type);
        if (!obj)
            return nullptr;

        JS_ASSERT(obj->numFixedSlots() == JSObject::ITER_CLASS_NFIXED_SLOTS);
        return &obj->as<PropertyIteratorObject>();
    }

    JSObject *obj = NewBuiltinClassInstance(cx, &PropertyIteratorObject::class_);
    if (!obj)
        return nullptr;

    return &obj->as<PropertyIteratorObject>();
}

NativeIterator *
NativeIterator::allocateIterator(JSContext *cx, uint32_t slength, const AutoIdVector &props)
{
    size_t plength = props.length();
    NativeIterator *ni = cx->zone()->pod_malloc_with_extra<NativeIterator, void *>(plength + slength);
    if (!ni)
        return nullptr;

    AutoValueVector strings(cx);
    ni->props_array = ni->props_cursor = reinterpret_cast<HeapPtrFlatString *>(ni + 1);
    ni->props_end = ni->props_array + plength;
    if (plength) {
        for (size_t i = 0; i < plength; i++) {
            JSFlatString *str = IdToString(cx, props[i]);
            if (!str || !strings.append(StringValue(str)))
                return nullptr;
            ni->props_array[i].init(str);
        }
    }
    ni->next_ = nullptr;
    ni->prev_ = nullptr;
    return ni;
}

NativeIterator *
NativeIterator::allocateSentinel(JSContext *cx)
{
    NativeIterator *ni = js_pod_malloc<NativeIterator>();
    if (!ni)
        return nullptr;

    PodZero(ni);

    ni->next_ = ni;
    ni->prev_ = ni;
    return ni;
}

inline void
NativeIterator::init(JSObject *obj, JSObject *iterObj, unsigned flags, uint32_t slength, uint32_t key)
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
        ni->link(cx->compartment()->enumerators);

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
        






        JSObject *pobj = obj;
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
UpdateNativeIterator(NativeIterator *ni, JSObject *obj)
{
    
    
    ni->obj = obj;
}

bool
js::GetIterator(JSContext *cx, HandleObject obj, unsigned flags, MutableHandleValue vp)
{
    Vector<Shape *, 8> shapes(cx);
    uint32_t key = 0;

    bool keysOnly = (flags == JSITER_ENUMERATE);

    if (obj) {
        if (JSIteratorOp op = obj->getClass()->ext.iteratorObject) {
            JSObject *iterobj = op(cx, obj, !(flags & JSITER_FOREACH));
            if (!iterobj)
                return false;
            vp.setObject(*iterobj);
            return true;
        }

        if (keysOnly) {
            





            PropertyIteratorObject *last = cx->runtime()->nativeIterCache.last;
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
                JSObject *pobj = obj;
                do {
                    if (!pobj->isNative() ||
                        !pobj->hasEmptyElements() ||
                        IsAnyTypedArray(pobj) ||
                        pobj->hasUncacheableProto() ||
                        pobj->getOps()->enumerate ||
                        pobj->getClass()->enumerate != JS_EnumerateStub ||
                        pobj->nativeContainsPure(cx->names().iteratorIntrinsic))
                    {
                        shapes.clear();
                        goto miss;
                    }
                    Shape *shape = pobj->lastProperty();
                    key = (key + (key << 16)) ^ (uintptr_t(shape) >> 3);
                    if (!shapes.append(shape))
                        return false;
                    pobj = pobj->getProto();
                } while (pobj);
            }

            PropertyIteratorObject *iterobj = cx->runtime()->nativeIterCache.get(key);
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
                        cx->runtime()->nativeIterCache.last = iterobj;
                    return true;
                }
            }
        }

      miss:
        if (obj->is<ProxyObject>())
            return Proxy::iterate(cx, obj, flags, vp);

        if (!GetCustomIterator(cx, obj, flags, vp))
            return false;
        if (!vp.isUndefined())
            return true;
    }

    

    AutoIdVector keys(cx);
    if (flags & JSITER_FOREACH) {
        if (MOZ_LIKELY(obj != nullptr) && !Snapshot(cx, obj, flags, &keys))
            return false;
        JS_ASSERT(shapes.empty());
        if (!VectorToValueIterator(cx, obj, flags, keys, vp))
            return false;
    } else {
        if (MOZ_LIKELY(obj != nullptr) && !Snapshot(cx, obj, flags, &keys))
            return false;
        if (!VectorToKeyIterator(cx, obj, flags, keys, shapes.length(), key, vp))
            return false;
    }

    PropertyIteratorObject *iterobj = &vp.toObject().as<PropertyIteratorObject>();

    
    if (shapes.length())
        cx->runtime()->nativeIterCache.set(key, iterobj);

    if (shapes.length() == 2)
        cx->runtime()->nativeIterCache.last = iterobj;
    return true;
}

JSObject *
js::GetIteratorObject(JSContext *cx, HandleObject obj, uint32_t flags)
{
    RootedValue value(cx);
    if (!GetIterator(cx, obj, flags, &value))
        return nullptr;
    return &value.toObject();
}

JSObject *
js::CreateItrResultObject(JSContext *cx, HandleValue value, bool done)
{
    
    AssertHeapIsIdle(cx);

    RootedObject proto(cx, cx->global()->getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedObject obj(cx, NewObjectWithGivenProto(cx, &JSObject::class_, proto, cx->global()));
    if (!obj)
        return nullptr;

    if (!JSObject::defineProperty(cx, obj, cx->names().value, value))
        return nullptr;

    RootedValue doneBool(cx, BooleanValue(done));
    if (!JSObject::defineProperty(cx, obj, cx->names().done, doneBool))
        return nullptr;

    return obj;
}

bool
js::ThrowStopIteration(JSContext *cx)
{
    JS_ASSERT(!JS_IsExceptionPending(cx));

    
    
    RootedObject ctor(cx);
    if (GetBuiltinConstructor(cx, JSProto_StopIteration, &ctor))
        cx->setPendingException(ObjectValue(*ctor));
    return false;
}



bool
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

    if (!ValueToIterator(cx, flags, args[0]))
        return false;
    args.rval().set(args[0]);
    return true;
}

MOZ_ALWAYS_INLINE bool
IsIterator(HandleValue v)
{
    return v.isObject() && v.toObject().hasClass(&PropertyIteratorObject::class_);
}

MOZ_ALWAYS_INLINE bool
iterator_next_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsIterator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    bool more;
    if (!IteratorMore(cx, thisObj, &more))
        return false;

    if (!more) {
        ThrowStopIteration(cx);
        return false;
    }

    return IteratorNext(cx, thisObj, args.rval());
}

static bool
iterator_next(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsIterator, iterator_next_impl>(cx, args);
}

static const JSFunctionSpec iterator_methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "LegacyIteratorShim", 0, 0),
    JS_FN("next",      iterator_next,       0, 0),
    JS_FS_END
};

static JSObject *
iterator_iteratorObject(JSContext *cx, HandleObject obj, bool keysonly)
{
    return obj;
}

size_t
PropertyIteratorObject::sizeOfMisc(mozilla::MallocSizeOf mallocSizeOf) const
{
    return mallocSizeOf(getPrivate());
}

void
PropertyIteratorObject::trace(JSTracer *trc, JSObject *obj)
{
    if (NativeIterator *ni = obj->as<PropertyIteratorObject>().getNativeIterator())
        ni->mark(trc);
}

void
PropertyIteratorObject::finalize(FreeOp *fop, JSObject *obj)
{
    if (NativeIterator *ni = obj->as<PropertyIteratorObject>().getNativeIterator())
        fop->free_(ni);
}

const Class PropertyIteratorObject::class_ = {
    "Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_BACKGROUND_FINALIZE,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    finalize,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    trace,
    JS_NULL_CLASS_SPEC,
    {
        nullptr,             
        nullptr,             
        iterator_iteratorObject,
    }
};

enum {
    ArrayIteratorSlotIteratedObject,
    ArrayIteratorSlotNextIndex,
    ArrayIteratorSlotItemKind,
    ArrayIteratorSlotCount
};

const Class ArrayIteratorObject::class_ = {
    "Array Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(ArrayIteratorSlotCount),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr                  
};

static const JSFunctionSpec array_iterator_methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "ArrayIteratorIdentity", 0, 0),
    JS_SELF_HOSTED_FN("next", "ArrayIteratorNext", 0, 0),
    JS_FS_END
};

static const Class StringIteratorPrototypeClass = {
    "String Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr                  
};

enum {
    StringIteratorSlotIteratedObject,
    StringIteratorSlotNextIndex,
    StringIteratorSlotCount
};

const Class StringIteratorObject::class_ = {
    "String Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StringIteratorSlotCount),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr                  
};

static const JSFunctionSpec string_iterator_methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "StringIteratorIdentity", 0, 0),
    JS_SELF_HOSTED_FN("next", "StringIteratorNext", 0, 0),
    JS_FS_END
};

static bool
CloseLegacyGenerator(JSContext *cx, HandleObject genobj);

bool
js::ValueToIterator(JSContext *cx, unsigned flags, MutableHandleValue vp)
{
    
    JS_ASSERT_IF(flags & JSITER_KEYVALUE, flags & JSITER_FOREACH);

    




    cx->iterValue.setMagic(JS_NO_ITER_VALUE);

    RootedObject obj(cx);
    if (vp.isObject()) {
        
        obj = &vp.toObject();
    } else {
        




        if (!(flags & JSITER_ENUMERATE) || !vp.isNullOrUndefined()) {
            obj = ToObject(cx, vp);
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

    if (obj->is<PropertyIteratorObject>()) {
        
        NativeIterator *ni = obj->as<PropertyIteratorObject>().getNativeIterator();

        if (ni->flags & JSITER_ENUMERATE) {
            ni->unlink();

            JS_ASSERT(ni->flags & JSITER_ACTIVE);
            ni->flags &= ~JSITER_ACTIVE;

            



            ni->props_cursor = ni->props_array;
        }
    } else if (obj->is<LegacyGeneratorObject>()) {
        return CloseLegacyGenerator(cx, obj);
    }
    return true;
}

bool
js::UnwindIteratorForException(JSContext *cx, HandleObject obj)
{
    RootedValue v(cx);
    bool getOk = cx->getPendingException(&v);
    cx->clearPendingException();
    if (!CloseIterator(cx, obj))
        return false;
    if (!getOk)
        return false;
    cx->setPendingException(v);
    return true;
}

void
js::UnwindIteratorForUncatchableException(JSContext *cx, JSObject *obj)
{
    if (obj->is<PropertyIteratorObject>()) {
        NativeIterator *ni = obj->as<PropertyIteratorObject>().getNativeIterator();
        if (ni->flags & JSITER_ENUMERATE)
            ni->unlink();
    }
}


















template<typename StringPredicate>
static bool
SuppressDeletedPropertyHelper(JSContext *cx, HandleObject obj, StringPredicate predicate)
{
    NativeIterator *enumeratorList = cx->compartment()->enumerators;
    NativeIterator *ni = enumeratorList->next();

    while (ni != enumeratorList) {
      again:
        
        if (ni->isKeyIter() && ni->obj == obj && ni->props_cursor < ni->props_end) {
            
            HeapPtrFlatString *props_cursor = ni->current();
            HeapPtrFlatString *props_end = ni->end();
            for (HeapPtrFlatString *idp = props_cursor; idp < props_end; ++idp) {
                if (predicate(*idp)) {
                    



                    RootedObject proto(cx);
                    if (!JSObject::getProto(cx, obj, &proto))
                        return false;
                    if (proto) {
                        RootedObject obj2(cx);
                        RootedShape prop(cx);
                        RootedId id(cx);
                        RootedValue idv(cx, StringValue(*idp));
                        if (!ValueToId<CanGC>(cx, idv, &id))
                            return false;
                        if (!JSObject::lookupGeneric(cx, proto, id, &obj2, &prop))
                            return false;
                        if (prop) {
                            unsigned attrs;
                            if (obj2->isNative())
                                attrs = GetShapeAttributes(obj2, prop);
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
                        for (HeapPtrFlatString *p = idp; p + 1 != props_end; p++)
                            *p = *(p + 1);
                        ni->props_end = ni->end() - 1;

                        




                        *ni->props_end = nullptr;
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

namespace {

class SingleStringPredicate {
    Handle<JSFlatString*> str;
public:
    explicit SingleStringPredicate(Handle<JSFlatString*> str) : str(str) {}

    bool operator()(JSFlatString *str) { return EqualStrings(str, this->str); }
    bool matchesAtMostOne() { return true; }
};

} 

bool
js::SuppressDeletedProperty(JSContext *cx, HandleObject obj, jsid id)
{
    if (JSID_IS_SYMBOL(id))
        return true;

    Rooted<JSFlatString*> str(cx, IdToString(cx, id));
    if (!str)
        return false;
    return SuppressDeletedPropertyHelper(cx, obj, SingleStringPredicate(str));
}

bool
js::SuppressDeletedElement(JSContext *cx, HandleObject obj, uint32_t index)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return SuppressDeletedProperty(cx, obj, id);
}

namespace {

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

} 

bool
js::SuppressDeletedElements(JSContext *cx, HandleObject obj, uint32_t begin, uint32_t end)
{
    return SuppressDeletedPropertyHelper(cx, obj, IndexRangePredicate(begin, end));
}

bool
js::IteratorMore(JSContext *cx, HandleObject iterobj, bool *res)
{
    
    NativeIterator *ni = nullptr;
    if (iterobj->is<PropertyIteratorObject>()) {
        
        ni = iterobj->as<PropertyIteratorObject>().getNativeIterator();
        bool more = ni->props_cursor < ni->props_end;
        if (ni->isKeyIter() || !more) {
            *res = more;
            return true;
        }
    }

    
    if (!cx->iterValue.isMagic(JS_NO_ITER_VALUE)) {
        *res = true;
        return true;
    }

    
    JS_CHECK_RECURSION(cx, return false);

    
    RootedValue val(cx);
    if (ni) {
        JS_ASSERT(!ni->isKeyIter());
        RootedId id(cx);
        RootedValue current(cx, StringValue(*ni->current()));
        if (!ValueToId<CanGC>(cx, current, &id))
            return false;
        ni->incCursor();
        RootedObject obj(cx, ni->obj);
        if (!JSObject::getGeneric(cx, obj, obj, id, &val))
            return false;
        if ((ni->flags & JSITER_KEYVALUE) && !NewKeyValuePair(cx, id, val, &val))
            return false;
    } else {
        
        if (!JSObject::getProperty(cx, iterobj, iterobj, cx->names().next, &val))
            return false;
        if (!Invoke(cx, ObjectValue(*iterobj), val, 0, nullptr, &val)) {
            
            if (!cx->isExceptionPending())
                return false;
            RootedValue exception(cx);
            if (!cx->getPendingException(&exception))
                return false;
            if (!JS_IsStopIteration(exception))
                return false;

            cx->clearPendingException();
            cx->iterValue.setMagic(JS_NO_ITER_VALUE);
            *res = false;
            return true;
        }
    }

    
    JS_ASSERT(!val.isMagic(JS_NO_ITER_VALUE));
    cx->iterValue = val;
    *res = true;
    return true;
}

bool
js::IteratorNext(JSContext *cx, HandleObject iterobj, MutableHandleValue rval)
{
    
    if (iterobj->is<PropertyIteratorObject>()) {
        



        NativeIterator *ni = iterobj->as<PropertyIteratorObject>().getNativeIterator();
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

static bool
stopiter_hasInstance(JSContext *cx, HandleObject obj, MutableHandleValue v, bool *bp)
{
    *bp = JS_IsStopIteration(v);
    return true;
}

const Class StopIterationObject::class_ = {
    "StopIteration",
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,                 
    nullptr,                 
    stopiter_hasInstance,
    nullptr                  
};

bool
ForOfIterator::init(HandleValue iterable, NonIterableBehavior nonIterableBehavior)
{
    JSContext *cx = cx_;
    RootedObject iterableObj(cx, ToObject(cx, iterable));
    if (!iterableObj)
        return false;

    JS_ASSERT(index == NOT_ARRAY);

    
    if (iterableObj->is<ArrayObject>()) {
        ForOfPIC::Chain *stubChain = ForOfPIC::getOrCreate(cx);
        if (!stubChain)
            return false;

        bool optimized;
        if (!stubChain->tryOptimizeArray(cx, iterableObj, &optimized))
            return false;

        if (optimized) {
            
            index = 0;
            iterator = iterableObj;
            return true;
        }
    }

    JS_ASSERT(index == NOT_ARRAY);

    
    InvokeArgs args(cx);
    if (!args.init(0))
        return false;
    args.setThis(ObjectValue(*iterableObj));

    RootedValue callee(cx);
    if (!JSObject::getProperty(cx, iterableObj, iterableObj, cx->names().std_iterator, &callee))
        return false;

    
    
    
    
    if (!callee.isObject() || !callee.toObject().isCallable()) {
        if (nonIterableBehavior == AllowNonIterable)
            return true;
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, iterable, NullPtr());
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_ITERABLE, bytes);
        js_free(bytes);
        return false;
    }

    args.setCallee(callee);
    if (!Invoke(cx, args))
        return false;

    iterator = ToObject(cx, args.rval());
    if (!iterator)
        return false;

    return true;
}

bool
ForOfIterator::initWithIterator(HandleValue aIterator)
{
    JSContext *cx = cx_;
    RootedObject iteratorObj(cx, ToObject(cx, aIterator));
    return iterator = iteratorObj;
}

inline bool
ForOfIterator::nextFromOptimizedArray(MutableHandleValue vp, bool *done)
{
    JS_ASSERT(index != NOT_ARRAY);

    if (!CheckForInterrupt(cx_))
        return false;

    JS_ASSERT(iterator->isNative());
    JS_ASSERT(iterator->is<ArrayObject>());

    if (index >= iterator->as<ArrayObject>().length()) {
        vp.setUndefined();
        *done = true;
        return true;
    }
    *done = false;

    
    if (index < iterator->getDenseInitializedLength()) {
        vp.set(iterator->getDenseElement(index));
        if (!vp.isMagic(JS_ELEMENTS_HOLE)) {
            ++index;
            return true;
        }
    }

    return JSObject::getElement(cx_, iterator, iterator, index++, vp);
}

bool
ForOfIterator::next(MutableHandleValue vp, bool *done)
{
    JS_ASSERT(iterator);

    if (index != NOT_ARRAY) {
        ForOfPIC::Chain *stubChain = ForOfPIC::getOrCreate(cx_);
        if (!stubChain)
            return false;

        if (stubChain->isArrayNextStillSane())
            return nextFromOptimizedArray(vp, done);

        
        
        if (!materializeArrayIterator())
            return false;
    }

    RootedValue method(cx_);
    if (!JSObject::getProperty(cx_, iterator, iterator, cx_->names().next, &method))
        return false;

    InvokeArgs args(cx_);
    if (!args.init(1))
        return false;
    args.setCallee(method);
    args.setThis(ObjectValue(*iterator));
    args[0].setUndefined();
    if (!Invoke(cx_, args))
        return false;

    RootedObject resultObj(cx_, ToObject(cx_, args.rval()));
    if (!resultObj)
        return false;
    RootedValue doneVal(cx_);
    if (!JSObject::getProperty(cx_, resultObj, resultObj, cx_->names().done, &doneVal))
        return false;
    *done = ToBoolean(doneVal);
    if (*done) {
        vp.setUndefined();
        return true;
    }
    return JSObject::getProperty(cx_, resultObj, resultObj, cx_->names().value, vp);
}

bool
ForOfIterator::materializeArrayIterator()
{
    JS_ASSERT(index != NOT_ARRAY);

    const char *nameString = "ArrayValuesAt";

    RootedAtom name(cx_, Atomize(cx_, nameString, strlen(nameString)));
    if (!name)
        return false;

    RootedValue val(cx_);
    if (!cx_->global()->getSelfHostedFunction(cx_, name, name, 1, &val))
        return false;

    InvokeArgs args(cx_);
    if (!args.init(1))
        return false;
    args.setCallee(val);
    args.setThis(ObjectValue(*iterator));
    args[0].set(Int32Value(index));
    if (!Invoke(cx_, args))
        return false;

    index = NOT_ARRAY;
    
    iterator = &args.rval().toObject();
    return true;
}



template<typename T>
static void
FinalizeGenerator(FreeOp *fop, JSObject *obj)
{
    JS_ASSERT(obj->is<T>());
    JSGenerator *gen = obj->as<T>().getGenerator();
    JS_ASSERT(gen);
    
    
    JS_ASSERT(gen->state == JSGEN_NEWBORN ||
              gen->state == JSGEN_CLOSED ||
              gen->state == JSGEN_OPEN);
    
    if (gen->fp)
        JS_POISON(gen->fp, JS_SWEPT_FRAME_PATTERN, sizeof(InterpreterFrame));
    JS_POISON(gen, JS_SWEPT_FRAME_PATTERN, sizeof(JSGenerator));
    fop->free_(gen);
}

static void
MarkGeneratorFrame(JSTracer *trc, JSGenerator *gen)
{
    gen->obj = MaybeForwarded(gen->obj.get());
    MarkObject(trc, &gen->obj, "Generator Object");
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
    if (zone->needsIncrementalBarrier())
        MarkGeneratorFrame(zone->barrierTracer(), gen);
}

static void
GeneratorWriteBarrierPost(JSContext *cx, JSGenerator *gen)
{
#ifdef JSGC_GENERATIONAL
    cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(gen->obj);
#endif
}






static bool
GeneratorHasMarkableFrame(JSGenerator *gen)
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

#ifdef DEBUG
    MakeRangeGCSafe(gen->fp->generatorArgsSnapshotBegin(),
                    gen->fp->generatorArgsSnapshotEnd());
    MakeRangeGCSafe(gen->fp->generatorSlotsSnapshotBegin(),
                    gen->regs.sp);
    PodZero(&gen->regs, 1);
    gen->fp = nullptr;
#endif
}

template<typename T>
static void
TraceGenerator(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->is<T>());
    JSGenerator *gen = obj->as<T>().getGenerator();
    JS_ASSERT(gen);
    if (GeneratorHasMarkableFrame(gen))
        MarkGeneratorFrame(trc, gen);
}

GeneratorState::GeneratorState(JSContext *cx, JSGenerator *gen, JSGeneratorState futureState)
  : RunState(cx, Generator, gen->fp->script()),
    cx_(cx),
    gen_(gen),
    futureState_(futureState),
    entered_(false)
{ }

GeneratorState::~GeneratorState()
{
    gen_->fp->setSuspended();

    if (entered_)
        cx_->leaveGenerator(gen_);
}

InterpreterFrame *
GeneratorState::pushInterpreterFrame(JSContext *cx)
{
    










    GeneratorWriteBarrierPre(cx, gen_);
    gen_->state = futureState_;

    gen_->fp->clearSuspended();

    cx->enterGenerator(gen_);   
    entered_ = true;
    return gen_->fp;
}

const Class LegacyGeneratorObject::class_ = {
    "Generator",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    FinalizeGenerator<LegacyGeneratorObject>,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    TraceGenerator<LegacyGeneratorObject>,
    JS_NULL_CLASS_SPEC,
    {
        nullptr,             
        nullptr,             
        iterator_iteratorObject,
    }
};

const Class StarGeneratorObject::class_ = {
    "Generator",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    FinalizeGenerator<StarGeneratorObject>,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    TraceGenerator<StarGeneratorObject>,
};









JSObject *
js_NewGenerator(JSContext *cx, const InterpreterRegs &stackRegs)
{
    JS_ASSERT(stackRegs.stackDepth() == 0);
    InterpreterFrame *stackfp = stackRegs.fp();

    JS_ASSERT(stackfp->script()->isGenerator());

    Rooted<GlobalObject*> global(cx, &stackfp->global());
    RootedObject obj(cx);
    if (stackfp->script()->isStarGenerator()) {
        RootedValue pval(cx);
        RootedObject fun(cx, stackfp->fun());
        
        
        if (!JSObject::getProperty(cx, fun, fun, cx->names().prototype, &pval))
            return nullptr;
        JSObject *proto = pval.isObject() ? &pval.toObject() : nullptr;
        if (!proto) {
            proto = GlobalObject::getOrCreateStarGeneratorObjectPrototype(cx, global);
            if (!proto)
                return nullptr;
        }
        obj = NewObjectWithGivenProto(cx, &StarGeneratorObject::class_, proto, global);
    } else {
        JS_ASSERT(stackfp->script()->isLegacyGenerator());
        JSObject *proto = GlobalObject::getOrCreateLegacyGeneratorObjectPrototype(cx, global);
        if (!proto)
            return nullptr;
        obj = NewObjectWithGivenProto(cx, &LegacyGeneratorObject::class_, proto, global);
    }
    if (!obj)
        return nullptr;

    
    Value *stackvp = stackfp->generatorArgsSnapshotBegin();
    unsigned vplen = stackfp->generatorArgsSnapshotEnd() - stackvp;

    static_assert(sizeof(InterpreterFrame) % sizeof(HeapValue) == 0,
                  "The Values stored after InterpreterFrame must be aligned.");
    unsigned nvals = vplen + VALUES_PER_STACK_FRAME + stackfp->script()->nslots();
    JSGenerator *gen = obj->pod_calloc_with_extra<JSGenerator, HeapValue>(nvals);
    if (!gen)
        return nullptr;

    
    HeapValue *genvp = gen->stackSnapshot();
    SetValueRangeToUndefined((Value *)genvp, vplen);

    InterpreterFrame *genfp = reinterpret_cast<InterpreterFrame *>(genvp + vplen);

    
    gen->obj.init(obj);
    gen->state = JSGEN_NEWBORN;
    gen->fp = genfp;
    gen->prevGenerator = nullptr;

    
    gen->regs.rebaseFromTo(stackRegs, *genfp);
    genfp->copyFrameAndValues<InterpreterFrame::DoPostBarrier>(cx, (Value *)genvp, stackfp,
                                                         stackvp, stackRegs.sp);
    genfp->setSuspended();
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





static bool
SendToGenerator(JSContext *cx, JSGeneratorOp op, HandleObject obj,
                JSGenerator *gen, HandleValue arg, GeneratorKind generatorKind,
                MutableHandleValue rval)
{
    JS_ASSERT(generatorKind == LegacyGenerator || generatorKind == StarGenerator);

    if (gen->state == JSGEN_RUNNING || gen->state == JSGEN_CLOSING) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NESTING_GENERATOR);
        return false;
    }

    JSGeneratorState futureState;
    JS_ASSERT(gen->state == JSGEN_NEWBORN || gen->state == JSGEN_OPEN);
    switch (op) {
      case JSGENOP_NEXT:
      case JSGENOP_SEND:
        if (gen->state == JSGEN_OPEN) {
            




            HeapValue::writeBarrierPre(gen->regs.sp[-1]);
            gen->regs.sp[-1] = arg;
            HeapValue::writeBarrierPost(gen->regs.sp[-1], &gen->regs.sp[-1]);
        }
        futureState = JSGEN_RUNNING;
        break;

      case JSGENOP_THROW:
        cx->setPendingException(arg);
        futureState = JSGEN_RUNNING;
        break;

      default:
        JS_ASSERT(op == JSGENOP_CLOSE);
        JS_ASSERT(generatorKind == LegacyGenerator);
        cx->setPendingException(MagicValue(JS_GENERATOR_CLOSING));
        futureState = JSGEN_CLOSING;
        break;
    }

    bool ok;
    {
        GeneratorState state(cx, gen, futureState);
        ok = RunScript(cx, state);
        if (!ok && gen->state == JSGEN_CLOSED)
            return false;
    }

    if (gen->fp->isYielding()) {
        



        JS_ASSERT(gen->state == JSGEN_RUNNING);
        JS_ASSERT(op != JSGENOP_CLOSE);
        gen->fp->clearYielding();
        gen->state = JSGEN_OPEN;
        GeneratorWriteBarrierPost(cx, gen);
        rval.set(gen->fp->returnValue());
        return ok;
    }

    if (ok) {
        if (generatorKind == StarGenerator) {
            
            rval.set(gen->fp->returnValue());
        } else {
            JS_ASSERT(generatorKind == LegacyGenerator);

            
            
            rval.setUndefined();
            if (op != JSGENOP_CLOSE)
                ok = ThrowStopIteration(cx);
        }
    }

    SetGeneratorClosed(cx, gen);
    return ok;
}

MOZ_ALWAYS_INLINE bool
star_generator_next(JSContext *cx, CallArgs args)
{
    RootedObject thisObj(cx, &args.thisv().toObject());
    JSGenerator *gen = thisObj->as<StarGeneratorObject>().getGenerator();

    if (gen->state == JSGEN_CLOSED) {
        RootedObject obj(cx, CreateItrResultObject(cx, JS::UndefinedHandleValue, true));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    return SendToGenerator(cx, JSGENOP_SEND, thisObj, gen, args.get(0), StarGenerator,
                           args.rval());
}

MOZ_ALWAYS_INLINE bool
star_generator_throw(JSContext *cx, CallArgs args)
{
    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = thisObj->as<StarGeneratorObject>().getGenerator();
    if (gen->state == JSGEN_CLOSED) {
        cx->setPendingException(args.get(0));
        return false;
    }

    return SendToGenerator(cx, JSGENOP_THROW, thisObj, gen, args.get(0), StarGenerator,
                           args.rval());
}

MOZ_ALWAYS_INLINE bool
legacy_generator_next(JSContext *cx, CallArgs args)
{
    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = thisObj->as<LegacyGeneratorObject>().getGenerator();
    if (gen->state == JSGEN_CLOSED)
        return ThrowStopIteration(cx);

    return SendToGenerator(cx, JSGENOP_SEND, thisObj, gen, args.get(0), LegacyGenerator,
                           args.rval());
}

MOZ_ALWAYS_INLINE bool
legacy_generator_throw(JSContext *cx, CallArgs args)
{
    RootedObject thisObj(cx, &args.thisv().toObject());

    JSGenerator *gen = thisObj->as<LegacyGeneratorObject>().getGenerator();
    if (gen->state == JSGEN_CLOSED) {
        cx->setPendingException(args.length() >= 1 ? args[0] : UndefinedValue());
        return false;
    }

    return SendToGenerator(cx, JSGENOP_THROW, thisObj, gen, args.get(0), LegacyGenerator,
                           args.rval());
}

static bool
CloseLegacyGenerator(JSContext *cx, HandleObject obj, MutableHandleValue rval)
{
    JS_ASSERT(obj->is<LegacyGeneratorObject>());

    JSGenerator *gen = obj->as<LegacyGeneratorObject>().getGenerator();

    if (gen->state == JSGEN_CLOSED) {
        rval.setUndefined();
        return true;
    }

    if (gen->state == JSGEN_NEWBORN) {
        SetGeneratorClosed(cx, gen);
        rval.setUndefined();
        return true;
    }

    return SendToGenerator(cx, JSGENOP_CLOSE, obj, gen, JS::UndefinedHandleValue, LegacyGenerator,
                           rval);
}

static bool
CloseLegacyGenerator(JSContext *cx, HandleObject obj)
{
    RootedValue rval(cx);
    return CloseLegacyGenerator(cx, obj, &rval);
}

MOZ_ALWAYS_INLINE bool
legacy_generator_close(JSContext *cx, CallArgs args)
{
    RootedObject thisObj(cx, &args.thisv().toObject());

    return CloseLegacyGenerator(cx, thisObj, args.rval());
}

template<typename T>
MOZ_ALWAYS_INLINE bool
IsObjectOfType(HandleValue v)
{
    return v.isObject() && v.toObject().is<T>();
}

template<typename T, NativeImpl Impl>
static bool
NativeMethod(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsObjectOfType<T>, Impl>(cx, args);
}

#define JSPROP_ROPERM   (JSPROP_READONLY | JSPROP_PERMANENT)
#define JS_METHOD(name, T, impl, len, attrs) JS_FN(name, (NativeMethod<T,impl>), len, attrs)

static const JSFunctionSpec star_generator_methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "IteratorIdentity", 0, 0),
    JS_METHOD("next", StarGeneratorObject, star_generator_next, 1, 0),
    JS_METHOD("throw", StarGeneratorObject, star_generator_throw, 1, 0),
    JS_FS_END
};

static const JSFunctionSpec legacy_generator_methods[] = {
    JS_SELF_HOSTED_FN("@@iterator", "LegacyGeneratorIteratorShim", 0, 0),
    
    JS_METHOD("next", LegacyGeneratorObject, legacy_generator_next, 1, JSPROP_ROPERM),
    JS_METHOD("send", LegacyGeneratorObject, legacy_generator_next, 1, JSPROP_ROPERM),
    JS_METHOD("throw", LegacyGeneratorObject, legacy_generator_throw, 1, JSPROP_ROPERM),
    JS_METHOD("close", LegacyGeneratorObject, legacy_generator_close, 0, JSPROP_ROPERM),
    JS_FS_END
};

static JSObject*
NewSingletonObjectWithObjectPrototype(JSContext *cx, Handle<GlobalObject *> global)
{
    JSObject *proto = global->getOrCreateObjectPrototype(cx);
    if (!proto)
        return nullptr;
    return NewObjectWithGivenProto(cx, &JSObject::class_, proto, global, SingletonObject);
}

static JSObject*
NewSingletonObjectWithFunctionPrototype(JSContext *cx, Handle<GlobalObject *> global)
{
    JSObject *proto = global->getOrCreateFunctionPrototype(cx);
    if (!proto)
        return nullptr;
    return NewObjectWithGivenProto(cx, &JSObject::class_, proto, global, SingletonObject);
}

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
        ni->init(nullptr, nullptr, 0 , 0, 0);

        iteratorProto->as<PropertyIteratorObject>().setNativeIterator(ni);

        Rooted<JSFunction*> ctor(cx);
        ctor = global->createConstructor(cx, IteratorConstructor, cx->names().Iterator, 2);
        if (!ctor)
            return false;
        if (!LinkConstructorAndPrototype(cx, ctor, iteratorProto))
            return false;
        if (!DefinePropertiesAndFunctions(cx, iteratorProto, nullptr, iterator_methods))
            return false;
        if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_Iterator,
                                                  ctor, iteratorProto))
        {
            return false;
        }
    }

    RootedObject proto(cx);
    if (global->getSlot(ARRAY_ITERATOR_PROTO).isUndefined()) {
        const Class *cls = &ArrayIteratorObject::class_;
        proto = global->createBlankPrototypeInheriting(cx, cls, *iteratorProto);
        if (!proto || !DefinePropertiesAndFunctions(cx, proto, nullptr, array_iterator_methods))
            return false;
        global->setReservedSlot(ARRAY_ITERATOR_PROTO, ObjectValue(*proto));
    }

    if (global->getSlot(STRING_ITERATOR_PROTO).isUndefined()) {
        const Class *cls = &StringIteratorPrototypeClass;
        proto = global->createBlankPrototype(cx, cls);
        if (!proto || !DefinePropertiesAndFunctions(cx, proto, nullptr, string_iterator_methods))
            return false;
        global->setReservedSlot(STRING_ITERATOR_PROTO, ObjectValue(*proto));
    }

    if (global->getSlot(LEGACY_GENERATOR_OBJECT_PROTO).isUndefined()) {
        proto = NewSingletonObjectWithObjectPrototype(cx, global);
        if (!proto || !DefinePropertiesAndFunctions(cx, proto, nullptr, legacy_generator_methods))
            return false;
        global->setReservedSlot(LEGACY_GENERATOR_OBJECT_PROTO, ObjectValue(*proto));
    }

    if (global->getSlot(STAR_GENERATOR_OBJECT_PROTO).isUndefined()) {
        RootedObject genObjectProto(cx, NewSingletonObjectWithObjectPrototype(cx, global));
        if (!genObjectProto)
            return false;
        if (!DefinePropertiesAndFunctions(cx, genObjectProto, nullptr, star_generator_methods))
            return false;

        RootedObject genFunctionProto(cx, NewSingletonObjectWithFunctionPrototype(cx, global));
        if (!genFunctionProto)
            return false;
        if (!LinkConstructorAndPrototype(cx, genFunctionProto, genObjectProto))
            return false;

        RootedValue function(cx, global->getConstructor(JSProto_Function));
        if (!function.toObjectOrNull())
            return false;
        RootedAtom name(cx, cx->names().GeneratorFunction);
        RootedObject genFunction(cx, NewFunctionWithProto(cx, NullPtr(), Generator, 1,
                                                          JSFunction::NATIVE_CTOR, global, name,
                                                          &function.toObject()));
        if (!genFunction)
            return false;
        if (!LinkConstructorAndPrototype(cx, genFunction, genFunctionProto))
            return false;

        global->setSlot(STAR_GENERATOR_OBJECT_PROTO, ObjectValue(*genObjectProto));
        global->setConstructor(JSProto_GeneratorFunction, ObjectValue(*genFunction));
        global->setPrototype(JSProto_GeneratorFunction, ObjectValue(*genFunctionProto));
    }

    if (global->getPrototype(JSProto_StopIteration).isUndefined()) {
        proto = global->createBlankPrototype(cx, &StopIterationObject::class_);
        if (!proto || !JSObject::freeze(cx, proto))
            return false;

        
        if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_StopIteration, proto, proto))
            return false;

        global->setConstructor(JSProto_StopIteration, ObjectValue(*proto));
    }

    return true;
}

JSObject *
js_InitIteratorClasses(JSContext *cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    if (!GlobalObject::initIteratorClasses(cx, global))
        return nullptr;
    return global->getIteratorPrototype();
}

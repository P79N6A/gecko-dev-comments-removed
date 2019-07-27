







#include "jsiter.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jstypes.h"
#include "jsutil.h"

#include "ds/Sort.h"
#include "gc/Marking.h"
#include "js/Proxy.h"
#include "vm/GeneratorObject.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/Shape.h"
#include "vm/StopIterationObject.h"
#include "vm/TypedArrayCommon.h"

#include "jsscriptinlines.h"

#include "vm/NativeObject-inl.h"
#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;
using JS::ForOfIterator;

using mozilla::ArrayLength;
using mozilla::Maybe;
#ifdef JS_MORE_DETERMINISTIC
using mozilla::PodCopy;
#endif
using mozilla::PodZero;

typedef Rooted<PropertyIteratorObject*> RootedPropertyIteratorObject;

static const gc::AllocKind ITERATOR_FINALIZE_KIND = gc::AllocKind::OBJECT2_BACKGROUND;

void
NativeIterator::mark(JSTracer* trc)
{
    for (HeapPtrFlatString* str = begin(); str < end(); str++)
        TraceEdge(trc, str, "prop");
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
NewKeyValuePair(JSContext* cx, jsid id, const Value& val, MutableHandleValue rval)
{
    JS::AutoValueArray<2> vec(cx);
    vec[0].set(IdToValue(id));
    vec[1].set(val);

    JSObject* aobj = NewDenseCopiedArray(cx, 2, vec.begin());
    if (!aobj)
        return false;
    rval.setObject(*aobj);
    return true;
}

static inline bool
Enumerate(JSContext* cx, HandleObject pobj, jsid id,
          bool enumerable, unsigned flags, Maybe<IdSet>& ht, AutoIdVector* props)
{
    if (!(flags & JSITER_OWNONLY) || pobj->is<ProxyObject>() || pobj->getOps()->enumerate) {
        if (!ht) {
            ht.emplace(cx);
            
            if (!ht->init(5))
                return false;
        }

        
        IdSet::AddPtr p = ht->lookupForAdd(id);
        if (MOZ_UNLIKELY(!!p))
            return true;

        
        
        
        if ((pobj->is<ProxyObject>() || pobj->getProto() || pobj->getOps()->enumerate) && !ht->add(p, id))
            return false;
    }

    
    
    
    if (JSID_IS_SYMBOL(id) ? !(flags & JSITER_SYMBOLS) : (flags & JSITER_SYMBOLSONLY))
        return true;
    if (!enumerable && !(flags & JSITER_HIDDEN))
        return true;

    return props->append(id);
}

static bool
EnumerateNativeProperties(JSContext* cx, HandleNativeObject pobj, unsigned flags, Maybe<IdSet>& ht,
                          AutoIdVector* props)
{
    bool enumerateSymbols;
    if (flags & JSITER_SYMBOLSONLY) {
        enumerateSymbols = true;
    } else {
        
        size_t initlen = pobj->getDenseInitializedLength();
        const Value* vp = pobj->getDenseElements();
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
            Shape& shape = r.front();
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
            Shape& shape = r.front();
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
    JSContext*  const cx;

    SortComparatorIds(JSContext* cx)
      : cx(cx) {}

    bool operator()(jsid a, jsid b, bool* lessOrEqualp)
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
            MOZ_ASSERT(ca == JS::SymbolCode::InSymbolRegistry || ca == JS::SymbolCode::UniqueSymbol);
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
Snapshot(JSContext* cx, HandleObject pobj_, unsigned flags, AutoIdVector* props)
{
    
    
    Maybe<IdSet> ht;
    RootedObject pobj(cx, pobj_);

    do {
        if (JSNewEnumerateOp enumerate = pobj->getOps()->enumerate) {
            
            AutoIdVector properties(cx);
            if (!enumerate(cx, pobj, properties))
                 return false;

            for (size_t n = 0; n < properties.length(); n++) {
                if (!Enumerate(cx, pobj, properties[n], true, flags, ht, props))
                    return false;
            }

            if (pobj->isNative()) {
                if (!EnumerateNativeProperties(cx, pobj.as<NativeObject>(), flags, ht, props))
                    return false;
            }
        } else if (pobj->isNative()) {
            
            if (JSEnumerateOp enumerate = pobj->getClass()->enumerate) {
                if (!enumerate(cx, pobj.as<NativeObject>()))
                    return false;
            }
            if (!EnumerateNativeProperties(cx, pobj.as<NativeObject>(), flags, ht, props))
                return false;
        } else if (pobj->is<ProxyObject>()) {
            AutoIdVector proxyProps(cx);
            if (flags & JSITER_HIDDEN || flags & JSITER_SYMBOLS) {
                
                
                
                if (!Proxy::ownPropertyKeys(cx, pobj, proxyProps))
                    return false;

                Rooted<PropertyDescriptor> desc(cx);
                for (size_t n = 0, len = proxyProps.length(); n < len; n++) {
                    bool enumerable = false;

                    
                    
                    if (!(flags & JSITER_HIDDEN)) {
                        if (!Proxy::getOwnPropertyDescriptor(cx, pobj, proxyProps[n], &desc))
                            return false;
                        enumerable = desc.enumerable();
                    }

                    if (!Enumerate(cx, pobj, proxyProps[n], enumerable, flags, ht, props))
                        return false;
                }
            } else {
                
                if (!Proxy::getOwnEnumerablePropertyKeys(cx, pobj, proxyProps))
                    return false;

                for (size_t n = 0, len = proxyProps.length(); n < len; n++) {
                    if (!Enumerate(cx, pobj, proxyProps[n], true, flags, ht, props))
                        return false;
                }
            }
        } else {
            MOZ_CRASH("non-native objects must have an enumerate op");
        }

        if (flags & JSITER_OWNONLY)
            break;

        if (!GetPrototype(cx, pobj, &pobj))
            return false;

    } while (pobj != nullptr);

#ifdef JS_MORE_DETERMINISTIC

    














    jsid* ids = props->begin();
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
js::VectorToIdArray(JSContext* cx, AutoIdVector& props, JSIdArray** idap)
{
    JS_STATIC_ASSERT(sizeof(JSIdArray) > sizeof(jsid));
    size_t len = props.length();
    size_t idsz = len * sizeof(jsid);
    size_t sz = (sizeof(JSIdArray) - sizeof(jsid)) + idsz;
    JSIdArray* ida = reinterpret_cast<JSIdArray*>(cx->zone()->pod_malloc<uint8_t>(sz));
    if (!ida)
        return false;

    ida->length = static_cast<int>(len);
    jsid* v = props.begin();
    for (int i = 0; i < ida->length; i++)
        ida->vector[i].init(v[i]);
    *idap = ida;
    return true;
}

JS_FRIEND_API(bool)
js::GetPropertyKeys(JSContext* cx, HandleObject obj, unsigned flags, AutoIdVector* props)
{
    return Snapshot(cx, obj,
                    flags & (JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS | JSITER_SYMBOLSONLY),
                    props);
}

size_t sCustomIteratorCount = 0;

static inline bool
GetCustomIterator(JSContext* cx, HandleObject obj, unsigned flags, MutableHandleObject objp)
{
    JS_CHECK_RECURSION(cx, return false);

    RootedValue rval(cx);
    
    HandlePropertyName name = cx->names().iteratorIntrinsic;
    if (!GetProperty(cx, obj, obj, name, &rval))
        return false;

    
    if (!rval.isObject()) {
        objp.set(nullptr);
        return true;
    }

    if (!cx->runningWithTrustedPrincipals())
        ++sCustomIteratorCount;

    
    Value arg = BooleanValue((flags & JSITER_FOREACH) == 0);
    if (!Invoke(cx, ObjectValue(*obj), rval, 1, &arg, &rval))
        return false;
    if (rval.isPrimitive()) {
        



        JSAutoByteString bytes;
        if (!AtomToPrintableString(cx, name, &bytes))
            return false;
        RootedValue val(cx, ObjectValue(*obj));
        ReportValueError2(cx, JSMSG_BAD_TRAP_RETURN_VALUE,
                          -1, val, js::NullPtr(), bytes.ptr());
        return false;
    }
    objp.set(&rval.toObject());
    return true;
}

template <typename T>
static inline bool
Compare(T* a, T* b, size_t c)
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

static inline PropertyIteratorObject*
NewPropertyIteratorObject(JSContext* cx, unsigned flags)
{
    if (flags & JSITER_ENUMERATE) {
        RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &PropertyIteratorObject::class_,
                                                                 TaggedProto(nullptr)));
        if (!group)
            return nullptr;

        const Class* clasp = &PropertyIteratorObject::class_;
        RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, TaggedProto(nullptr),
                                                          ITERATOR_FINALIZE_KIND));
        if (!shape)
            return nullptr;

        JSObject* obj = JSObject::create(cx, ITERATOR_FINALIZE_KIND,
                                         GetInitialHeap(GenericObject, clasp), shape, group);
        if (!obj)
            return nullptr;
        PropertyIteratorObject* res = &obj->as<PropertyIteratorObject>();

        MOZ_ASSERT(res->numFixedSlots() == JSObject::ITER_CLASS_NFIXED_SLOTS);
        return res;
    }

    return NewBuiltinClassInstance<PropertyIteratorObject>(cx);
}

NativeIterator*
NativeIterator::allocateIterator(JSContext* cx, uint32_t slength, const AutoIdVector& props)
{
    size_t plength = props.length();
    NativeIterator* ni = cx->zone()->pod_malloc_with_extra<NativeIterator, void*>(plength + slength);
    if (!ni)
        return nullptr;

    AutoValueVector strings(cx);
    ni->props_array = ni->props_cursor = reinterpret_cast<HeapPtrFlatString*>(ni + 1);
    ni->props_end = ni->props_array + plength;
    if (plength) {
        for (size_t i = 0; i < plength; i++) {
            JSFlatString* str = IdToString(cx, props[i]);
            if (!str || !strings.append(StringValue(str)))
                return nullptr;
            ni->props_array[i].init(str);
        }
    }
    ni->next_ = nullptr;
    ni->prev_ = nullptr;
    return ni;
}

NativeIterator*
NativeIterator::allocateSentinel(JSContext* cx)
{
    NativeIterator* ni = js_pod_malloc<NativeIterator>();
    if (!ni)
        return nullptr;

    PodZero(ni);

    ni->next_ = ni;
    ni->prev_ = ni;
    return ni;
}

inline void
NativeIterator::init(JSObject* obj, JSObject* iterObj, unsigned flags, uint32_t slength, uint32_t key)
{
    this->obj.init(obj);
    this->iterObj_ = iterObj;
    this->flags = flags;
    this->shapes_array = (Shape**) this->props_end;
    this->shapes_length = slength;
    this->shapes_key = key;
}

static inline void
RegisterEnumerator(JSContext* cx, PropertyIteratorObject* iterobj, NativeIterator* ni)
{
    
    if (ni->flags & JSITER_ENUMERATE) {
        ni->link(cx->compartment()->enumerators);

        MOZ_ASSERT(!(ni->flags & JSITER_ACTIVE));
        ni->flags |= JSITER_ACTIVE;
    }
}

static inline bool
VectorToKeyIterator(JSContext* cx, HandleObject obj, unsigned flags, AutoIdVector& keys,
                    uint32_t slength, uint32_t key, MutableHandleObject objp)
{
    MOZ_ASSERT(!(flags & JSITER_FOREACH));

    if (obj->isSingleton() && !obj->setIteratedSingleton(cx))
        return false;
    MarkObjectGroupFlags(cx, obj, OBJECT_FLAG_ITERATED);

    Rooted<PropertyIteratorObject*> iterobj(cx, NewPropertyIteratorObject(cx, flags));
    if (!iterobj)
        return false;

    NativeIterator* ni = NativeIterator::allocateIterator(cx, slength, keys);
    if (!ni)
        return false;
    ni->init(obj, iterobj, flags, slength, key);

    if (slength) {
        






        JSObject* pobj = obj;
        size_t ind = 0;
        do {
            ni->shapes_array[ind++] = pobj->as<NativeObject>().lastProperty();
            pobj = pobj->getProto();
        } while (pobj);
        MOZ_ASSERT(ind == slength);
    }

    iterobj->setNativeIterator(ni);
    objp.set(iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

static bool
VectorToValueIterator(JSContext* cx, HandleObject obj, unsigned flags, AutoIdVector& keys,
                      MutableHandleObject objp)
{
    MOZ_ASSERT(flags & JSITER_FOREACH);

    if (obj->isSingleton() && !obj->setIteratedSingleton(cx))
        return false;
    MarkObjectGroupFlags(cx, obj, OBJECT_FLAG_ITERATED);

    Rooted<PropertyIteratorObject*> iterobj(cx, NewPropertyIteratorObject(cx, flags));
    if (!iterobj)
        return false;

    NativeIterator* ni = NativeIterator::allocateIterator(cx, 0, keys);
    if (!ni)
        return false;
    ni->init(obj, iterobj, flags, 0, 0);

    iterobj->setNativeIterator(ni);
    objp.set(iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

bool
js::EnumeratedIdVectorToIterator(JSContext* cx, HandleObject obj, unsigned flags,
                                 AutoIdVector& props, MutableHandleObject objp)
{
    if (!(flags & JSITER_FOREACH))
        return VectorToKeyIterator(cx, obj, flags, props, 0, 0, objp);

    return VectorToValueIterator(cx, obj, flags, props, objp);
}


bool
js::NewEmptyPropertyIterator(JSContext* cx, unsigned flags, MutableHandleObject objp)
{
    Rooted<PropertyIteratorObject*> iterobj(cx, NewPropertyIteratorObject(cx, flags));
    if (!iterobj)
        return false;

    AutoIdVector keys(cx); 
    NativeIterator* ni = NativeIterator::allocateIterator(cx, 0, keys);
    if (!ni)
        return false;
    ni->init(nullptr, iterobj, flags, 0, 0);

    iterobj->setNativeIterator(ni);
    objp.set(iterobj);

    RegisterEnumerator(cx, iterobj, ni);
    return true;
}

static inline void
UpdateNativeIterator(NativeIterator* ni, JSObject* obj)
{
    
    
    ni->obj = obj;
}

bool
js::GetIterator(JSContext* cx, HandleObject obj, unsigned flags, MutableHandleObject objp)
{
    if (obj->is<PropertyIteratorObject>() || obj->is<LegacyGeneratorObject>()) {
        objp.set(obj);
        return true;
    }

    
    
    
    
    
    if (flags == 0 || flags == JSITER_ENUMERATE) {
        if (obj->is<ProxyObject>())
            return Proxy::enumerate(cx, obj, objp);
    }

    Vector<Shape*, 8> shapes(cx);
    uint32_t key = 0;
    if (flags == JSITER_ENUMERATE) {
        





        PropertyIteratorObject* last = cx->runtime()->nativeIterCache.last;
        if (last) {
            NativeIterator* lastni = last->getNativeIterator();
            if (!(lastni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                obj->isNative() &&
                obj->as<NativeObject>().hasEmptyElements() &&
                obj->as<NativeObject>().lastProperty() == lastni->shapes_array[0])
            {
                JSObject* proto = obj->getProto();
                if (proto->isNative() &&
                    proto->as<NativeObject>().hasEmptyElements() &&
                    proto->as<NativeObject>().lastProperty() == lastni->shapes_array[1] &&
                    !proto->getProto())
                {
                    objp.set(last);
                    UpdateNativeIterator(lastni, obj);
                    RegisterEnumerator(cx, last, lastni);
                    return true;
                }
            }
        }

        





        {
            JSObject* pobj = obj;
            do {
                if (!pobj->isNative() ||
                    !pobj->as<NativeObject>().hasEmptyElements() ||
                    IsAnyTypedArray(pobj) ||
                    pobj->hasUncacheableProto() ||
                    pobj->getOps()->enumerate ||
                    pobj->getClass()->enumerate ||
                    pobj->as<NativeObject>().containsPure(cx->names().iteratorIntrinsic))
                {
                    shapes.clear();
                    goto miss;
                }
                Shape* shape = pobj->as<NativeObject>().lastProperty();
                key = (key + (key << 16)) ^ (uintptr_t(shape) >> 3);
                if (!shapes.append(shape))
                    return false;
                pobj = pobj->getProto();
            } while (pobj);
        }

        PropertyIteratorObject* iterobj = cx->runtime()->nativeIterCache.get(key);
        if (iterobj) {
            NativeIterator* ni = iterobj->getNativeIterator();
            if (!(ni->flags & (JSITER_ACTIVE|JSITER_UNREUSABLE)) &&
                ni->shapes_key == key &&
                ni->shapes_length == shapes.length() &&
                Compare(ni->shapes_array, shapes.begin(), ni->shapes_length)) {
                objp.set(iterobj);

                UpdateNativeIterator(ni, obj);
                RegisterEnumerator(cx, iterobj, ni);
                if (shapes.length() == 2)
                    cx->runtime()->nativeIterCache.last = iterobj;
                return true;
            }
        }
    }

  miss:
    if (!GetCustomIterator(cx, obj, flags, objp))
        return false;
    if (objp)
        return true;

    AutoIdVector keys(cx);
    if (flags & JSITER_FOREACH) {
        MOZ_ASSERT(shapes.empty());

        if (!Snapshot(cx, obj, flags, &keys))
            return false;
        if (!VectorToValueIterator(cx, obj, flags, keys, objp))
            return false;
    } else {
        if (!Snapshot(cx, obj, flags, &keys))
            return false;
        if (!VectorToKeyIterator(cx, obj, flags, keys, shapes.length(), key, objp))
            return false;
    }

    PropertyIteratorObject* iterobj = &objp->as<PropertyIteratorObject>();

    
    if (shapes.length())
        cx->runtime()->nativeIterCache.set(key, iterobj);

    if (shapes.length() == 2)
        cx->runtime()->nativeIterCache.last = iterobj;
    return true;
}

JSObject*
js::GetIteratorObject(JSContext* cx, HandleObject obj, uint32_t flags)
{
    RootedObject iterator(cx);
    if (!GetIterator(cx, obj, flags, &iterator))
        return nullptr;
    return iterator;
}

JSObject*
js::CreateItrResultObject(JSContext* cx, HandleValue value, bool done)
{
    
    AssertHeapIsIdle(cx);

    RootedObject proto(cx, cx->global()->getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedPlainObject obj(cx, NewObjectWithGivenProto<PlainObject>(cx, proto));
    if (!obj)
        return nullptr;

    if (!DefineProperty(cx, obj, cx->names().value, value))
        return nullptr;

    RootedValue doneBool(cx, BooleanValue(done));
    if (!DefineProperty(cx, obj, cx->names().done, doneBool))
        return nullptr;

    return obj;
}

bool
js::ThrowStopIteration(JSContext* cx)
{
    MOZ_ASSERT(!JS_IsExceptionPending(cx));

    
    
    RootedObject ctor(cx);
    if (GetBuiltinConstructor(cx, JSProto_StopIteration, &ctor))
        cx->setPendingException(ObjectValue(*ctor));
    return false;
}



bool
js::IteratorConstructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() == 0) {
        ReportMissingArg(cx, args.calleev(), 0);
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
NativeIteratorNext(JSContext* cx, NativeIterator* ni, MutableHandleValue rval, bool* done)
{
    *done = false;

    if (ni->props_cursor >= ni->props_end) {
        *done = true;
        return true;
    }

    if (MOZ_LIKELY(ni->isKeyIter())) {
        rval.setString(*ni->current());
        ni->incCursor();
        return true;
    }

    
    RootedId id(cx);
    RootedValue current(cx, StringValue(*ni->current()));
    if (!ValueToId<CanGC>(cx, current, &id))
        return false;
    ni->incCursor();
    RootedObject obj(cx, ni->obj);
    if (!GetProperty(cx, obj, obj, id, rval))
        return false;

    
    if (ni->flags & JSITER_KEYVALUE)
        return NewKeyValuePair(cx, id, rval, rval);
    return true;
}

MOZ_ALWAYS_INLINE bool
IsIterator(HandleValue v)
{
    return v.isObject() && v.toObject().hasClass(&PropertyIteratorObject::class_);
}

MOZ_ALWAYS_INLINE bool
iterator_next_impl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsIterator(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    NativeIterator* ni = thisObj.as<PropertyIteratorObject>()->getNativeIterator();
    RootedValue value(cx);
    bool done;
    if (!NativeIteratorNext(cx, ni, &value, &done))
         return false;

    
    if (done) {
        ThrowStopIteration(cx);
        return false;
    }

    args.rval().set(value);
    return true;
}

static bool
iterator_next(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsIterator, iterator_next_impl>(cx, args);
}

static const JSFunctionSpec iterator_methods[] = {
    JS_SELF_HOSTED_SYM_FN(iterator, "LegacyIteratorShim", 0, 0),
    JS_FN("next",      iterator_next,       0, 0),
    JS_FS_END
};

size_t
PropertyIteratorObject::sizeOfMisc(mozilla::MallocSizeOf mallocSizeOf) const
{
    return mallocSizeOf(getPrivate());
}

void
PropertyIteratorObject::trace(JSTracer* trc, JSObject* obj)
{
    if (NativeIterator* ni = obj->as<PropertyIteratorObject>().getNativeIterator())
        ni->mark(trc);
}

void
PropertyIteratorObject::finalize(FreeOp* fop, JSObject* obj)
{
    if (NativeIterator* ni = obj->as<PropertyIteratorObject>().getNativeIterator())
        fop->free_(ni);
}

const Class PropertyIteratorObject::class_ = {
    "Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Iterator) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_BACKGROUND_FINALIZE,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    finalize,
    nullptr, 
    nullptr, 
    nullptr, 
    trace
};

static const Class ArrayIteratorPrototypeClass = {
    "Array Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS
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
    JSCLASS_HAS_RESERVED_SLOTS(ArrayIteratorSlotCount)
};

static const JSFunctionSpec array_iterator_methods[] = {
    JS_SELF_HOSTED_SYM_FN(iterator, "ArrayIteratorIdentity", 0, 0),
    JS_SELF_HOSTED_FN("next", "ArrayIteratorNext", 0, 0),
    JS_FS_END
};

static const Class StringIteratorPrototypeClass = {
    "String Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS
};

enum {
    StringIteratorSlotIteratedObject,
    StringIteratorSlotNextIndex,
    StringIteratorSlotCount
};

const Class StringIteratorObject::class_ = {
    "String Iterator",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(StringIteratorSlotCount)
};

static const JSFunctionSpec string_iterator_methods[] = {
    JS_SELF_HOSTED_SYM_FN(iterator, "StringIteratorIdentity", 0, 0),
    JS_SELF_HOSTED_FN("next", "StringIteratorNext", 0, 0),
    JS_FS_END
};

bool
js::ValueToIterator(JSContext* cx, unsigned flags, MutableHandleValue vp)
{
    
    MOZ_ASSERT_IF(flags & JSITER_KEYVALUE, flags & JSITER_FOREACH);

    RootedObject obj(cx);
    if (vp.isObject()) {
        
        obj = &vp.toObject();
    } else if ((flags & JSITER_ENUMERATE) && vp.isNullOrUndefined()) {
        




        RootedObject iter(cx);
        if (!NewEmptyPropertyIterator(cx, flags, &iter))
            return false;
        vp.setObject(*iter);
        return true;
    } else {
        obj = ToObject(cx, vp);
        if (!obj)
            return false;
    }

    RootedObject iter(cx);
    if (!GetIterator(cx, obj, flags, &iter))
        return false;
    vp.setObject(*iter);
    return true;
}

bool
js::CloseIterator(JSContext* cx, HandleObject obj)
{
    if (obj->is<PropertyIteratorObject>()) {
        
        NativeIterator* ni = obj->as<PropertyIteratorObject>().getNativeIterator();

        if (ni->flags & JSITER_ENUMERATE) {
            ni->unlink();

            MOZ_ASSERT(ni->flags & JSITER_ACTIVE);
            ni->flags &= ~JSITER_ACTIVE;

            



            ni->props_cursor = ni->props_array;
        }
    } else if (obj->is<LegacyGeneratorObject>()) {
        Rooted<LegacyGeneratorObject*> genObj(cx, &obj->as<LegacyGeneratorObject>());
        if (genObj->isClosed())
            return true;
        if (genObj->isRunning() || genObj->isClosing()) {
            
            return true;
        }
        return LegacyGeneratorObject::close(cx, obj);
    }
    return true;
}

bool
js::UnwindIteratorForException(JSContext* cx, HandleObject obj)
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
js::UnwindIteratorForUncatchableException(JSContext* cx, JSObject* obj)
{
    if (obj->is<PropertyIteratorObject>()) {
        NativeIterator* ni = obj->as<PropertyIteratorObject>().getNativeIterator();
        if (ni->flags & JSITER_ENUMERATE)
            ni->unlink();
    }
}


















template<typename StringPredicate>
static bool
SuppressDeletedPropertyHelper(JSContext* cx, HandleObject obj, StringPredicate predicate)
{
    NativeIterator* enumeratorList = cx->compartment()->enumerators;
    NativeIterator* ni = enumeratorList->next();

    while (ni != enumeratorList) {
      again:
        
        if (ni->isKeyIter() && ni->obj == obj && ni->props_cursor < ni->props_end) {
            
            HeapPtrFlatString* props_cursor = ni->current();
            HeapPtrFlatString* props_end = ni->end();
            for (HeapPtrFlatString* idp = props_cursor; idp < props_end; ++idp) {
                if (predicate(*idp)) {
                    



                    RootedObject proto(cx);
                    if (!GetPrototype(cx, obj, &proto))
                        return false;
                    if (proto) {
                        RootedId id(cx);
                        RootedValue idv(cx, StringValue(*idp));
                        if (!ValueToId<CanGC>(cx, idv, &id))
                            return false;

                        Rooted<PropertyDescriptor> desc(cx);
                        if (!GetPropertyDescriptor(cx, proto, id, &desc))
                            return false;

                        if (desc.object()) {
                            if (desc.enumerable())
                                continue;
                        }
                    }

                    



                    if (props_end != ni->props_end || props_cursor != ni->props_cursor)
                        goto again;

                    




                    if (idp == props_cursor) {
                        ni->incCursor();
                    } else {
                        for (HeapPtrFlatString* p = idp; p + 1 != props_end; p++)
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

    bool operator()(JSFlatString* str) { return EqualStrings(str, this->str); }
    bool matchesAtMostOne() { return true; }
};

} 

bool
js::SuppressDeletedProperty(JSContext* cx, HandleObject obj, jsid id)
{
    if (JSID_IS_SYMBOL(id))
        return true;

    Rooted<JSFlatString*> str(cx, IdToString(cx, id));
    if (!str)
        return false;
    return SuppressDeletedPropertyHelper(cx, obj, SingleStringPredicate(str));
}

bool
js::SuppressDeletedElement(JSContext* cx, HandleObject obj, uint32_t index)
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

    bool operator()(JSFlatString* str) {
        uint32_t index;
        return str->isIndex(&index) && begin <= index && index < end;
    }

    bool matchesAtMostOne() { return false; }
};

} 

bool
js::SuppressDeletedElements(JSContext* cx, HandleObject obj, uint32_t begin, uint32_t end)
{
    return SuppressDeletedPropertyHelper(cx, obj, IndexRangePredicate(begin, end));
}

bool
js::IteratorMore(JSContext* cx, HandleObject iterobj, MutableHandleValue rval)
{
    
    if (iterobj->is<PropertyIteratorObject>()) {
        NativeIterator* ni = iterobj->as<PropertyIteratorObject>().getNativeIterator();
        bool done;
        if (!NativeIteratorNext(cx, ni, rval, &done))
            return false;

        if (done)
            rval.setMagic(JS_NO_ITER_VALUE);
        return true;
    }

    
    JS_CHECK_RECURSION(cx, return false);

    
    if (!GetProperty(cx, iterobj, iterobj, cx->names().next, rval))
        return false;
    
    if (!Invoke(cx, ObjectValue(*iterobj), rval, 0, nullptr, rval)) {
        
        if (!cx->isExceptionPending())
            return false;
        RootedValue exception(cx);
        if (!cx->getPendingException(&exception))
            return false;
        if (!JS_IsStopIteration(exception))
            return false;

        cx->clearPendingException();
        rval.setMagic(JS_NO_ITER_VALUE);
        return true;
    }

    if (!rval.isObject()) {
        
        return true;
    }

    
    
    RootedObject result(cx, &rval.toObject());
    bool found = false;
    if (!HasProperty(cx, result, cx->names().done, &found))
        return false;
    if (!found)
        return true;
    if (!HasProperty(cx, result, cx->names().value, &found))
        return false;
    if (!found)
        return true;

    

    
    
    if (!GetProperty(cx, result, result, cx->names().done, rval))
        return false;

    bool done = ToBoolean(rval);
    if (done) {
         rval.setMagic(JS_NO_ITER_VALUE);
         return true;
     }

    
    return GetProperty(cx, result, result, cx->names().value, rval);
}

static bool
stopiter_hasInstance(JSContext* cx, HandleObject obj, MutableHandleValue v, bool* bp)
{
    *bp = JS_IsStopIteration(v);
    return true;
}

const Class StopIterationObject::class_ = {
    "StopIteration",
    JSCLASS_HAS_CACHED_PROTO(JSProto_StopIteration),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    stopiter_hasInstance
};

 bool
GlobalObject::initIteratorClasses(JSContext* cx, Handle<GlobalObject*> global)
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
        NativeIterator* ni = NativeIterator::allocateIterator(cx, 0, blank);
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
        const Class* cls = &ArrayIteratorPrototypeClass;
        proto = global->createBlankPrototypeInheriting(cx, cls, iteratorProto);
        if (!proto || !DefinePropertiesAndFunctions(cx, proto, nullptr, array_iterator_methods))
            return false;
        global->setReservedSlot(ARRAY_ITERATOR_PROTO, ObjectValue(*proto));
    }

    if (global->getSlot(STRING_ITERATOR_PROTO).isUndefined()) {
        const Class* cls = &StringIteratorPrototypeClass;
        proto = global->createBlankPrototype(cx, cls);
        if (!proto || !DefinePropertiesAndFunctions(cx, proto, nullptr, string_iterator_methods))
            return false;
        global->setReservedSlot(STRING_ITERATOR_PROTO, ObjectValue(*proto));
    }

    return GlobalObject::initStopIterationClass(cx, global);
}

 bool
GlobalObject::initStopIterationClass(JSContext* cx, Handle<GlobalObject*> global)
{
    if (!global->getPrototype(JSProto_StopIteration).isUndefined())
        return true;

    RootedObject proto(cx, global->createBlankPrototype(cx, &StopIterationObject::class_));
    if (!proto || !FreezeObject(cx, proto))
        return false;

    
    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_StopIteration, proto, proto))
        return false;

    global->setConstructor(JSProto_StopIteration, ObjectValue(*proto));

    return true;
}

JSObject*
js::InitIteratorClasses(JSContext* cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    if (!GlobalObject::initIteratorClasses(cx, global))
        return nullptr;
    if (!GlobalObject::initGeneratorClasses(cx, global))
        return nullptr;
    return global->getIteratorPrototype();
}

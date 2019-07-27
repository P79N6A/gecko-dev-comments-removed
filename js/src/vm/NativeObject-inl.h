





#ifndef vm_NativeObject_inl_h
#define vm_NativeObject_inl_h

#include "vm/NativeObject.h"

#include "jscntxt.h"

#include "builtin/TypedObject.h"
#include "proxy/Proxy.h"
#include "vm/ProxyObject.h"
#include "vm/TypedArrayObject.h"

#include "jsobjinlines.h"

namespace js {

inline uint8_t *
NativeObject::fixedData(size_t nslots) const
{
    MOZ_ASSERT(ClassCanHaveFixedData(getClass()));
    MOZ_ASSERT(nslots == numFixedSlots() + (hasPrivate() ? 1 : 0));
    return reinterpret_cast<uint8_t *>(&fixedSlots()[nslots]);
}

 inline bool
NativeObject::changePropertyAttributes(JSContext *cx, HandleNativeObject obj,
                                       HandleShape shape, unsigned attrs)
{
    return !!changeProperty<SequentialExecution>(cx, obj, shape, attrs, 0,
                                                 shape->getter(), shape->setter());
}

inline void
NativeObject::removeLastProperty(ExclusiveContext *cx)
{
    MOZ_ASSERT(canRemoveLastProperty());
    RootedNativeObject self(cx, this);
    RootedShape prev(cx, lastProperty()->previous());
    JS_ALWAYS_TRUE(setLastProperty(cx, self, prev));
}

inline bool
NativeObject::canRemoveLastProperty()
{
    






    MOZ_ASSERT(!inDictionaryMode());
    Shape *previous = lastProperty()->previous().get();
    return previous->getObjectParent() == lastProperty()->getObjectParent()
        && previous->getObjectMetadata() == lastProperty()->getObjectMetadata()
        && previous->getObjectFlags() == lastProperty()->getObjectFlags();
}

inline void
NativeObject::setShouldConvertDoubleElements()
{
    MOZ_ASSERT(is<ArrayObject>() && !hasEmptyElements());
    getElementsHeader()->setShouldConvertDoubleElements();
}

inline void
NativeObject::clearShouldConvertDoubleElements()
{
    MOZ_ASSERT(is<ArrayObject>() && !hasEmptyElements());
    getElementsHeader()->clearShouldConvertDoubleElements();
}

inline bool
NativeObject::setDenseElementIfHasType(uint32_t index, const Value &val)
{
    if (!types::HasTypePropertyId(this, JSID_VOID, val))
        return false;
    setDenseElementMaybeConvertDouble(index, val);
    return true;
}

inline void
NativeObject::setDenseElementWithType(ExclusiveContext *cx, uint32_t index,
                                      const Value &val)
{
    
    
    types::Type thisType = types::GetValueType(val);
    if (index == 0 || types::GetValueType(elements_[index - 1]) != thisType)
        types::AddTypePropertyId(cx, this, JSID_VOID, thisType);
    setDenseElementMaybeConvertDouble(index, val);
}

inline void
NativeObject::initDenseElementWithType(ExclusiveContext *cx, uint32_t index,
                                       const Value &val)
{
    MOZ_ASSERT(!shouldConvertDoubleElements());
    types::AddTypePropertyId(cx, this, JSID_VOID, val);
    initDenseElement(index, val);
}

inline void
NativeObject::setDenseElementHole(ExclusiveContext *cx, uint32_t index)
{
    types::MarkTypeObjectFlags(cx, this, types::OBJECT_FLAG_NON_PACKED);
    setDenseElement(index, MagicValue(JS_ELEMENTS_HOLE));
}

 inline void
NativeObject::removeDenseElementForSparseIndex(ExclusiveContext *cx,
                                               HandleNativeObject obj, uint32_t index)
{
    types::MarkTypeObjectFlags(cx, obj,
                               types::OBJECT_FLAG_NON_PACKED |
                               types::OBJECT_FLAG_SPARSE_INDEXES);
    if (obj->containsDenseElement(index))
        obj->setDenseElement(index, MagicValue(JS_ELEMENTS_HOLE));
}

inline bool
NativeObject::writeToIndexWouldMarkNotPacked(uint32_t index)
{
    return getElementsHeader()->initializedLength < index;
}

inline void
NativeObject::markDenseElementsNotPacked(ExclusiveContext *cx)
{
    MOZ_ASSERT(isNative());
    MarkTypeObjectFlags(cx, this, types::OBJECT_FLAG_NON_PACKED);
}

inline void
NativeObject::ensureDenseInitializedLengthNoPackedCheck(ThreadSafeContext *cx, uint32_t index,
                                                        uint32_t extra)
{
    MOZ_ASSERT(cx->isThreadLocal(this));
    MOZ_ASSERT(!denseElementsAreCopyOnWrite());

    




    MOZ_ASSERT(index + extra <= getDenseCapacity());
    uint32_t &initlen = getElementsHeader()->initializedLength;

    if (initlen < index + extra) {
        size_t offset = initlen;
        for (HeapSlot *sp = elements_ + initlen;
             sp != elements_ + (index + extra);
             sp++, offset++)
        {
            sp->init(this, HeapSlot::Element, offset, MagicValue(JS_ELEMENTS_HOLE));
        }
        initlen = index + extra;
    }
}

inline void
NativeObject::ensureDenseInitializedLength(ExclusiveContext *cx, uint32_t index, uint32_t extra)
{
    if (writeToIndexWouldMarkNotPacked(index))
        markDenseElementsNotPacked(cx);
    ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
}

inline void
NativeObject::ensureDenseInitializedLengthPreservePackedFlag(ThreadSafeContext *cx,
                                                             uint32_t index, uint32_t extra)
{
    MOZ_ASSERT(!writeToIndexWouldMarkNotPacked(index));
    ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
}

NativeObject::EnsureDenseResult
NativeObject::extendDenseElements(ThreadSafeContext *cx,
                                  uint32_t requiredCapacity, uint32_t extra)
{
    MOZ_ASSERT(cx->isThreadLocal(this));
    MOZ_ASSERT(!denseElementsAreCopyOnWrite());

    




    if (!nonProxyIsExtensible() || watched()) {
        MOZ_ASSERT(getDenseCapacity() == 0);
        return ED_SPARSE;
    }

    




    if (isIndexed())
        return ED_SPARSE;

    



    if (requiredCapacity > MIN_SPARSE_INDEX &&
        willBeSparseElements(requiredCapacity, extra)) {
        return ED_SPARSE;
    }

    if (!growElements(cx, requiredCapacity))
        return ED_FAILED;

    return ED_OK;
}

inline NativeObject::EnsureDenseResult
NativeObject::ensureDenseElementsNoPackedCheck(ThreadSafeContext *cx, uint32_t index, uint32_t extra)
{
    MOZ_ASSERT(isNative());

    if (!maybeCopyElementsForWrite(cx))
        return ED_FAILED;

    uint32_t currentCapacity = getDenseCapacity();

    uint32_t requiredCapacity;
    if (extra == 1) {
        
        if (index < currentCapacity) {
            ensureDenseInitializedLengthNoPackedCheck(cx, index, 1);
            return ED_OK;
        }
        requiredCapacity = index + 1;
        if (requiredCapacity == 0) {
            
            return ED_SPARSE;
        }
    } else {
        requiredCapacity = index + extra;
        if (requiredCapacity < index) {
            
            return ED_SPARSE;
        }
        if (requiredCapacity <= currentCapacity) {
            ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
            return ED_OK;
        }
    }

    EnsureDenseResult edr = extendDenseElements(cx, requiredCapacity, extra);
    if (edr != ED_OK)
        return edr;

    ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
    return ED_OK;
}

inline NativeObject::EnsureDenseResult
NativeObject::ensureDenseElements(ExclusiveContext *cx, uint32_t index, uint32_t extra)
{
    if (writeToIndexWouldMarkNotPacked(index))
        markDenseElementsNotPacked(cx);
    return ensureDenseElementsNoPackedCheck(cx, index, extra);
}

inline NativeObject::EnsureDenseResult
NativeObject::ensureDenseElementsPreservePackedFlag(ThreadSafeContext *cx, uint32_t index,
                                                    uint32_t extra)
{
    MOZ_ASSERT(!writeToIndexWouldMarkNotPacked(index));
    return ensureDenseElementsNoPackedCheck(cx, index, extra);
}

inline Value
NativeObject::getDenseOrTypedArrayElement(uint32_t idx)
{
    if (is<TypedArrayObject>())
        return as<TypedArrayObject>().getElement(idx);
    if (is<SharedTypedArrayObject>())
        return as<SharedTypedArrayObject>().getElement(idx);
    return getDenseElement(idx);
}

inline void
NativeObject::initDenseElementsUnbarriered(uint32_t dstStart, const Value *src, uint32_t count) {
    



    MOZ_ASSERT(dstStart + count <= getDenseCapacity());
    MOZ_ASSERT(!denseElementsAreCopyOnWrite());
#if defined(DEBUG) && defined(JSGC_GENERATIONAL)
    



    MOZ_ASSERT(!gc::IsInsideGGCNursery(this));
    for (uint32_t index = 0; index < count; ++index) {
        const Value& value = src[index];
        if (value.isMarkable())
            MOZ_ASSERT(!gc::IsInsideGGCNursery(static_cast<gc::Cell *>(value.toGCThing())));
    }
#endif
    memcpy(&elements_[dstStart], src, count * sizeof(HeapSlot));
}

 inline NativeObject *
NativeObject::copy(ExclusiveContext *cx, gc::AllocKind kind, gc::InitialHeap heap,
                   HandleNativeObject templateObject)
{
    RootedShape shape(cx, templateObject->lastProperty());
    RootedTypeObject type(cx, templateObject->type());
    MOZ_ASSERT(!templateObject->denseElementsAreCopyOnWrite());

    JSObject *baseObj = create(cx, kind, heap, shape, type);
    if (!baseObj)
        return nullptr;
    NativeObject *obj = &baseObj->as<NativeObject>();

    size_t span = shape->slotSpan();
    if (span) {
        uint32_t numFixed = templateObject->numFixedSlots();
        const Value *fixed = &templateObject->getSlot(0);
        
        
        if (span < numFixed)
            numFixed = span;
        obj->copySlotRange(0, fixed, numFixed);

        if (numFixed < span) {
            uint32_t numSlots = span - numFixed;
            const Value *slots = &templateObject->getSlot(numFixed);
            obj->copySlotRange(numFixed, slots, numSlots);
        }
    }

    return obj;
}

inline bool
NativeObject::setSlotIfHasType(Shape *shape, const Value &value, bool overwriting)
{
    if (!types::HasTypePropertyId(this, shape->propid(), value))
        return false;
    setSlot(shape->slot(), value);

    if (overwriting)
        shape->setOverwritten();

    return true;
}

inline void
NativeObject::setSlotWithType(ExclusiveContext *cx, Shape *shape,
                              const Value &value, bool overwriting)
{
    setSlot(shape->slot(), value);

    if (overwriting)
        shape->setOverwritten();

    types::AddTypePropertyId(cx, this, shape->propid(), value);
}


static inline NativeObject *
CopyInitializerObject(JSContext *cx, HandleNativeObject baseobj, NewObjectKind newKind = GenericObject)
{
    MOZ_ASSERT(baseobj->getClass() == &JSObject::class_);
    MOZ_ASSERT(!baseobj->inDictionaryMode());

    gc::AllocKind allocKind = gc::GetGCObjectFixedSlotsKind(baseobj->numFixedSlots());
    allocKind = gc::GetBackgroundAllocKind(allocKind);
    MOZ_ASSERT_IF(baseobj->isTenured(), allocKind == baseobj->asTenured().getAllocKind());
    JSObject *baseObj = NewBuiltinClassInstance(cx, &JSObject::class_, allocKind, newKind);
    if (!baseObj)
        return nullptr;
    RootedNativeObject obj(cx, &baseObj->as<NativeObject>());

    RootedObject metadata(cx, obj->getMetadata());
    RootedShape lastProp(cx, baseobj->lastProperty());
    if (!NativeObject::setLastProperty(cx, obj, lastProp))
        return nullptr;
    if (metadata && !JSObject::setMetadata(cx, obj, metadata))
        return nullptr;

    return obj;
}

inline NativeObject *
NewNativeObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp,
                              TaggedProto proto, JSObject *parent,
                              gc::AllocKind allocKind, NewObjectKind newKind)
{
    return MaybeNativeObject(NewObjectWithGivenProto(cx, clasp, proto, parent, allocKind, newKind));
}

inline NativeObject *
NewNativeObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp,
                              TaggedProto proto, JSObject *parent,
                              NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithGivenProto(cx, clasp, proto, parent, newKind));
}

inline NativeObject *
NewNativeObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp,
                              JSObject *proto, JSObject *parent,
                              NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithGivenProto(cx, clasp, proto, parent, newKind));
}

inline NativeObject *
NewNativeBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp,
                              gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewBuiltinClassInstance(cx, clasp, allocKind, newKind));
}

inline NativeObject *
NewNativeBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp,
                              NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewBuiltinClassInstance(cx, clasp, newKind));
}

inline NativeObject *
NewNativeObjectWithClassProto(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                              gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithClassProto(cx, clasp, proto, parent, allocKind, newKind));
}

inline NativeObject *
NewNativeObjectWithClassProto(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                              NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithClassProto(cx, clasp, proto, parent, newKind));
}

inline NativeObject *
NewNativeObjectWithType(JSContext *cx, HandleTypeObject type, JSObject *parent, gc::AllocKind allocKind,
                        NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithType(cx, type, parent, allocKind, newKind));
}

inline NativeObject *
NewNativeObjectWithType(JSContext *cx, HandleTypeObject type, JSObject *parent,
                        NewObjectKind newKind = GenericObject)
{
    return MaybeNativeObject(NewObjectWithType(cx, type, parent, newKind));
}





















static MOZ_ALWAYS_INLINE bool
CallResolveOp(JSContext *cx, HandleNativeObject obj, HandleId id, MutableHandleObject objp,
              MutableHandleShape propp, bool *recursedp)
{
    







    AutoResolving resolving(cx, obj, id);
    if (resolving.alreadyStarted()) {
        
        *recursedp = true;
        return true;
    }
    *recursedp = false;

    bool resolved = false;
    if (!obj->getClass()->resolve(cx, obj, id, &resolved))
        return false;

    if (!resolved)
        return true;

    objp.set(obj);

    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        MarkDenseOrTypedArrayElementFound<CanGC>(propp);
        return true;
    }

    if (Shape *shape = obj->lookup(cx, id))
        propp.set(shape);
    else
        objp.set(nullptr);

    return true;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
LookupOwnPropertyInline(ExclusiveContext *cx,
                        typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp,
                        bool *donep)
{
    
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        objp.set(obj);
        MarkDenseOrTypedArrayElementFound<allowGC>(propp);
        *donep = true;
        return true;
    }

    
    
    
    if (IsAnyTypedArray(obj)) {
        uint64_t index;
        if (IsTypedArrayIndex(id, &index)) {
            if (index < AnyTypedArrayLength(obj)) {
                objp.set(obj);
                MarkDenseOrTypedArrayElementFound<allowGC>(propp);
            } else {
                objp.set(nullptr);
                propp.set(nullptr);
            }
            *donep = true;
            return true;
        }
    }

    
    if (Shape *shape = obj->lookup(cx, id)) {
        objp.set(obj);
        propp.set(shape);
        *donep = true;
        return true;
    }

    
    if (obj->getClass()->resolve != JS_ResolveStub) {
        if (!cx->shouldBeJSContext() || !allowGC)
            return false;

        bool recursed;
        if (!CallResolveOp(cx->asJSContext(),
                           MaybeRooted<NativeObject*, allowGC>::toHandle(obj),
                           MaybeRooted<jsid, allowGC>::toHandle(id),
                           MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                           MaybeRooted<Shape*, allowGC>::toMutableHandle(propp),
                           &recursed))
        {
            return false;
        }

        if (recursed) {
            objp.set(nullptr);
            propp.set(nullptr);
            *donep = true;
            return true;
        }

        if (propp) {
            *donep = true;
            return true;
        }
    }

    *donep = false;
    return true;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
LookupPropertyInline(ExclusiveContext *cx,
                     typename MaybeRooted<NativeObject*, allowGC>::HandleType obj,
                     typename MaybeRooted<jsid, allowGC>::HandleType id,
                     typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                     typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    




    
    typename MaybeRooted<NativeObject*, allowGC>::RootType current(cx, obj);

    while (true) {
        bool done;
        if (!LookupOwnPropertyInline<allowGC>(cx, current, id, objp, propp, &done))
            return false;
        if (done)
            return true;

        typename MaybeRooted<JSObject*, allowGC>::RootType proto(cx, current->getProto());

        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!cx->shouldBeJSContext() || !allowGC)
                return false;
            return JSObject::lookupGeneric(cx->asJSContext(),
                                           MaybeRooted<JSObject*, allowGC>::toHandle(proto),
                                           MaybeRooted<jsid, allowGC>::toHandle(id),
                                           MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                                           MaybeRooted<Shape*, allowGC>::toMutableHandle(propp));
        }

        current = &proto->template as<NativeObject>();
    }

    objp.set(nullptr);
    propp.set(nullptr);
    return true;
}

inline bool
DefineNativeProperty(ExclusiveContext *cx, HandleNativeObject obj,
                     PropertyName *name, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx, NameToId(name));
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
}

inline bool
WarnIfNotConstructing(JSContext *cx, const CallArgs &args, const char *builtinName)
{
    if (args.isConstructing())
        return true;
    return JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING, js_GetErrorMessage, nullptr,
                                        JSMSG_BUILTIN_CTOR_NO_NEW, builtinName);
}

} 

#endif 

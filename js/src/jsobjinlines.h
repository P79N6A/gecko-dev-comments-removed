





#ifndef jsobjinlines_h
#define jsobjinlines_h

#include "jsobj.h"

#include "vm/ArrayObject.h"
#include "vm/DateObject.h"
#include "vm/NumberObject.h"
#include "vm/Probes.h"
#include "vm/ScopeObject.h"
#include "vm/StringObject.h"

#include "jsatominlines.h"
#include "jscompartmentinlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"

 inline bool
JSObject::setGenericAttributes(JSContext *cx, js::HandleObject obj,
                               js::HandleId id, unsigned *attrsp)
{
    js::types::MarkTypePropertyConfigured(cx, obj, id);
    js::GenericAttributesOp op = obj->getOps()->setGenericAttributes;
    return (op ? op : js::baseops::SetAttributes)(cx, obj, id, attrsp);
}

 inline bool
JSObject::changePropertyAttributes(JSContext *cx, js::HandleObject obj,
                                   js::HandleShape shape, unsigned attrs)
{
    return !!changeProperty<js::SequentialExecution>(cx, obj, shape, attrs, 0,
                                                     shape->getter(), shape->setter());
}

 inline bool
JSObject::deleteProperty(JSContext *cx, js::HandleObject obj, js::HandlePropertyName name,
                         bool *succeeded)
{
    JS::RootedId id(cx, js::NameToId(name));
    js::types::AddTypePropertyId(cx, obj, id, js::types::Type::UndefinedType());
    js::types::MarkTypePropertyConfigured(cx, obj, id);
    js::DeletePropertyOp op = obj->getOps()->deleteProperty;
    return (op ? op : js::baseops::DeleteProperty)(cx, obj, name, succeeded);
}

 inline bool
JSObject::deleteElement(JSContext *cx, js::HandleObject obj, uint32_t index, bool *succeeded)
{
    JS::RootedId id(cx);
    if (!js::IndexToId(cx, index, &id))
        return false;
    js::types::AddTypePropertyId(cx, obj, id, js::types::Type::UndefinedType());
    js::types::MarkTypePropertyConfigured(cx, obj, id);
    js::DeleteElementOp op = obj->getOps()->deleteElement;
    return (op ? op : js::baseops::DeleteElement)(cx, obj, index, succeeded);
}

 inline bool
JSObject::deleteSpecial(JSContext *cx, js::HandleObject obj, js::HandleSpecialId sid,
                        bool *succeeded)
{
    JS::RootedId id(cx, SPECIALID_TO_JSID(sid));
    js::types::AddTypePropertyId(cx, obj, id, js::types::Type::UndefinedType());
    js::types::MarkTypePropertyConfigured(cx, obj, id);
    js::DeleteSpecialOp op = obj->getOps()->deleteSpecial;
    return (op ? op : js::baseops::DeleteSpecial)(cx, obj, sid, succeeded);
}

inline void
JSObject::finalize(js::FreeOp *fop)
{
    js::probes::FinalizeObject(this);

#ifdef DEBUG
    JS_ASSERT(isTenured());
    if (!IsBackgroundFinalized(tenuredGetAllocKind())) {
        
        JS_ASSERT(CurrentThreadCanAccessRuntime(fop->runtime()));
    }
#endif
    const js::Class *clasp = getClass();
    if (clasp->finalize)
        clasp->finalize(fop, this);

    finish(fop);
}

inline void
JSObject::setLastPropertyInfallible(js::Shape *shape)
{
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(shape->compartment() == compartment());
    JS_ASSERT(!inDictionaryMode());
    JS_ASSERT(slotSpan() == shape->slotSpan());
    JS_ASSERT(numFixedSlots() == shape->numFixedSlots());

    shape_ = shape;
}

inline void
JSObject::removeLastProperty(js::ExclusiveContext *cx)
{
    JS_ASSERT(canRemoveLastProperty());
    JS::RootedObject self(cx, this);
    js::RootedShape prev(cx, lastProperty()->previous());
    JS_ALWAYS_TRUE(setLastProperty(cx, self, prev));
}

inline bool
JSObject::canRemoveLastProperty()
{
    






    JS_ASSERT(!inDictionaryMode());
    js::Shape *previous = lastProperty()->previous().get();
    return previous->getObjectParent() == lastProperty()->getObjectParent()
        && previous->getObjectMetadata() == lastProperty()->getObjectMetadata()
        && previous->getObjectFlags() == lastProperty()->getObjectFlags();
}

inline void
JSObject::setShouldConvertDoubleElements()
{
    JS_ASSERT(is<js::ArrayObject>() && !hasEmptyElements());
    getElementsHeader()->setShouldConvertDoubleElements();
}

inline bool
JSObject::setDenseElementIfHasType(uint32_t index, const js::Value &val)
{
    if (!js::types::HasTypePropertyId(this, JSID_VOID, val))
        return false;
    setDenseElementMaybeConvertDouble(index, val);
    return true;
}

 inline void
JSObject::setDenseElementWithType(js::ExclusiveContext *cx, js::HandleObject obj, uint32_t index,
                                  const js::Value &val)
{
    js::types::AddTypePropertyId(cx, obj, JSID_VOID, val);
    obj->setDenseElementMaybeConvertDouble(index, val);
}

 inline void
JSObject::initDenseElementWithType(js::ExclusiveContext *cx, js::HandleObject obj, uint32_t index,
                                   const js::Value &val)
{
    JS_ASSERT(!obj->shouldConvertDoubleElements());
    js::types::AddTypePropertyId(cx, obj, JSID_VOID, val);
    obj->initDenseElement(index, val);
}

 inline void
JSObject::setDenseElementHole(js::ExclusiveContext *cx, js::HandleObject obj, uint32_t index)
{
    js::types::MarkTypeObjectFlags(cx, obj, js::types::OBJECT_FLAG_NON_PACKED);
    obj->setDenseElement(index, js::MagicValue(JS_ELEMENTS_HOLE));
}

 inline void
JSObject::removeDenseElementForSparseIndex(js::ExclusiveContext *cx,
                                           js::HandleObject obj, uint32_t index)
{
    js::types::MarkTypeObjectFlags(cx, obj,
                                   js::types::OBJECT_FLAG_NON_PACKED |
                                   js::types::OBJECT_FLAG_SPARSE_INDEXES);
    if (obj->containsDenseElement(index))
        obj->setDenseElement(index, js::MagicValue(JS_ELEMENTS_HOLE));
}

inline bool
JSObject::writeToIndexWouldMarkNotPacked(uint32_t index)
{
    return getElementsHeader()->initializedLength < index;
}

inline void
JSObject::markDenseElementsNotPacked(js::ExclusiveContext *cx)
{
    JS_ASSERT(isNative());
    MarkTypeObjectFlags(cx, this, js::types::OBJECT_FLAG_NON_PACKED);
}

inline void
JSObject::ensureDenseInitializedLengthNoPackedCheck(js::ThreadSafeContext *cx, uint32_t index,
                                                    uint32_t extra)
{
    JS_ASSERT(cx->isThreadLocal(this));

    




    JS_ASSERT(index + extra <= getDenseCapacity());
    uint32_t &initlen = getElementsHeader()->initializedLength;

    if (initlen < index + extra) {
        JSRuntime *rt = runtimeFromAnyThread();
        size_t offset = initlen;
        for (js::HeapSlot *sp = elements + initlen;
             sp != elements + (index + extra);
             sp++, offset++)
            sp->init(rt, this, js::HeapSlot::Element, offset, js::MagicValue(JS_ELEMENTS_HOLE));
        initlen = index + extra;
    }
}

inline void
JSObject::ensureDenseInitializedLength(js::ExclusiveContext *cx, uint32_t index, uint32_t extra)
{
    if (writeToIndexWouldMarkNotPacked(index))
        markDenseElementsNotPacked(cx);
    ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
}

inline void
JSObject::ensureDenseInitializedLengthPreservePackedFlag(js::ThreadSafeContext *cx,
                                                         uint32_t index, uint32_t extra)
{
    JS_ASSERT(!writeToIndexWouldMarkNotPacked(index));
    ensureDenseInitializedLengthNoPackedCheck(cx, index, extra);
}

JSObject::EnsureDenseResult
JSObject::extendDenseElements(js::ThreadSafeContext *cx,
                              uint32_t requiredCapacity, uint32_t extra)
{
    JS_ASSERT(cx->isThreadLocal(this));

    




    if (!nonProxyIsExtensible() || watched()) {
        JS_ASSERT(getDenseCapacity() == 0);
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

inline JSObject::EnsureDenseResult
JSObject::ensureDenseElementsNoPackedCheck(js::ThreadSafeContext *cx, uint32_t index, uint32_t extra)
{
    JS_ASSERT(isNative());

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

inline JSObject::EnsureDenseResult
JSObject::ensureDenseElements(js::ExclusiveContext *cx, uint32_t index, uint32_t extra)
{
    if (writeToIndexWouldMarkNotPacked(index))
        markDenseElementsNotPacked(cx);
    return ensureDenseElementsNoPackedCheck(cx, index, extra);
}

inline JSObject::EnsureDenseResult
JSObject::ensureDenseElementsPreservePackedFlag(js::ThreadSafeContext *cx, uint32_t index,
                                                uint32_t extra)
{
    JS_ASSERT(!writeToIndexWouldMarkNotPacked(index));
    return ensureDenseElementsNoPackedCheck(cx, index, extra);
}

 inline bool
JSObject::setSingletonType(js::ExclusiveContext *cx, js::HandleObject obj)
{
    if (!cx->typeInferenceEnabled())
        return true;

    JS_ASSERT_IF(cx->isJSContext(),
                 !IsInsideNursery(cx->asJSContext()->runtime(), obj.get()));

    js::types::TypeObject *type = cx->getLazyType(obj->getClass(), obj->getTaggedProto());
    if (!type)
        return false;

    obj->type_ = type;
    return true;
}

inline js::types::TypeObject*
JSObject::getType(JSContext *cx)
{
    JS_ASSERT(cx->compartment() == compartment());
    if (hasLazyType()) {
        JS::RootedObject self(cx, this);
        if (cx->compartment() != compartment())
            MOZ_CRASH();
        return makeLazyType(cx, self);
    }
    return static_cast<js::types::TypeObject*>(type_);
}

 inline bool
JSObject::clearType(JSContext *cx, js::HandleObject obj)
{
    JS_ASSERT(!obj->hasSingletonType());
    JS_ASSERT(cx->compartment() == obj->compartment());

    js::types::TypeObject *type = cx->getNewType(obj->getClass(), nullptr);
    if (!type)
        return false;

    obj->type_ = type;
    return true;
}

inline void
JSObject::setType(js::types::TypeObject *newType)
{
    JS_ASSERT(newType);
    JS_ASSERT(!hasSingletonType());
    type_ = newType;
}

 inline bool
JSObject::getProto(JSContext *cx, js::HandleObject obj, js::MutableHandleObject protop)
{
    if (obj->getTaggedProto().isLazy()) {
        JS_ASSERT(obj->is<js::ProxyObject>());
        return js::Proxy::getPrototypeOf(cx, obj, protop);
    } else {
        protop.set(obj->js::ObjectImpl::getProto());
        return true;
    }
}

inline bool JSObject::isVarObj()
{
    if (is<js::DebugScopeObject>())
        return as<js::DebugScopeObject>().scope().isVarObj();
    return lastProperty()->hasObjectFlag(js::BaseShape::VAROBJ);
}

 inline JSObject *
JSObject::create(js::ExclusiveContext *cx, js::gc::AllocKind kind, js::gc::InitialHeap heap,
                 js::HandleShape shape, js::HandleTypeObject type,
                 js::HeapSlot *extantSlots )
{
    




    JS_ASSERT(shape && type);
    JS_ASSERT(type->clasp == shape->getObjectClass());
    JS_ASSERT(type->clasp != &js::ArrayObject::class_);
    JS_ASSERT(js::gc::GetGCKindSlots(kind, type->clasp) == shape->numFixedSlots());
    JS_ASSERT_IF(type->clasp->flags & JSCLASS_BACKGROUND_FINALIZE, IsBackgroundFinalized(kind));
    JS_ASSERT_IF(type->clasp->finalize, heap == js::gc::TenuredHeap);
    JS_ASSERT_IF(extantSlots, dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan()));

    js::HeapSlot *slots = extantSlots;
    if (!slots) {
        size_t nDynamicSlots = dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan());
        if (nDynamicSlots) {
            slots = cx->pod_malloc<js::HeapSlot>(nDynamicSlots);
            if (!slots)
                return nullptr;
            js::Debug_SetSlotRangeToCrashOnTouch(slots, nDynamicSlots);
        }
    }

    JSObject *obj = js_NewGCObject<js::CanGC>(cx, kind, heap);
    if (!obj) {
        js_free(slots);
        return nullptr;
    }

#ifdef JSGC_GENERATIONAL
    if (slots && heap != js::gc::TenuredHeap)
        cx->asJSContext()->runtime()->gcNursery.notifyInitialSlots(obj, slots);
#endif

    obj->shape_.init(shape);
    obj->type_.init(type);
    obj->slots = slots;
    obj->elements = js::emptyObjectElements;

    const js::Class *clasp = type->clasp;
    if (clasp->hasPrivate())
        obj->privateRef(shape->numFixedSlots()) = nullptr;

    size_t span = shape->slotSpan();
    if (span && clasp != &js::ArrayBufferObject::class_)
        obj->initializeSlotRange(0, span);

    return obj;
}

 inline js::ArrayObject *
JSObject::createArray(js::ExclusiveContext *cx, js::gc::AllocKind kind, js::gc::InitialHeap heap,
                      js::HandleShape shape, js::HandleTypeObject type,
                      uint32_t length)
{
    JS_ASSERT(shape && type);
    JS_ASSERT(type->clasp == shape->getObjectClass());
    JS_ASSERT(type->clasp == &js::ArrayObject::class_);
    JS_ASSERT_IF(type->clasp->finalize, heap == js::gc::TenuredHeap);

    




    JS_ASSERT(shape->numFixedSlots() == 0);

    



    JS_ASSERT(js::gc::GetGCKindSlots(kind) >= js::ObjectElements::VALUES_PER_HEADER);

    uint32_t capacity = js::gc::GetGCKindSlots(kind) - js::ObjectElements::VALUES_PER_HEADER;

    JSObject *obj = js_NewGCObject<js::CanGC>(cx, kind, heap);
    if (!obj)
        return nullptr;

    obj->shape_.init(shape);
    obj->type_.init(type);
    obj->slots = nullptr;
    obj->setFixedElements();
    new (obj->getElementsHeader()) js::ObjectElements(capacity, length);

    return &obj->as<js::ArrayObject>();
}

inline void
JSObject::finish(js::FreeOp *fop)
{
    if (hasDynamicSlots())
        fop->free_(slots);
    if (hasDynamicElements()) {
        js::ObjectElements *elements = getElementsHeader();
        if (JS_UNLIKELY(elements->isAsmJSArrayBuffer()))
            js::ArrayBufferObject::releaseAsmJSArrayBuffer(fop, this);
        else
            fop->free_(elements);
    }
}

 inline bool
JSObject::hasProperty(JSContext *cx, js::HandleObject obj,
                      js::HandleId id, bool *foundp, unsigned flags)
{
    JS::RootedObject pobj(cx);
    js::RootedShape prop(cx);
    JSAutoResolveFlags rf(cx, flags);
    if (!lookupGeneric(cx, obj, id, &pobj, &prop)) {
        *foundp = false;  
        return false;
    }
    *foundp = !!prop;
    return true;
}

inline bool
JSObject::nativeSetSlotIfHasType(js::Shape *shape, const js::Value &value)
{
    if (!js::types::HasTypePropertyId(this, shape->propid(), value))
        return false;
    nativeSetSlot(shape->slot(), value);
    return true;
}

 inline void
JSObject::nativeSetSlotWithType(js::ExclusiveContext *cx, js::HandleObject obj, js::Shape *shape,
                                const js::Value &value)
{
    obj->nativeSetSlot(shape->slot(), value);
    js::types::AddTypePropertyId(cx, obj, shape->propid(), value);
}

 inline bool
JSObject::getElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                     uint32_t index, js::MutableHandleValue vp)
{
    js::ElementIdOp op = obj->getOps()->getElement;
    if (op)
        return op(cx, obj, receiver, index, vp);

    JS::RootedId id(cx);
    if (!js::IndexToId(cx, index, &id))
        return false;
    return getGeneric(cx, obj, receiver, id, vp);
}

 inline bool
JSObject::getElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                         uint32_t index, js::Value *vp)
{
    js::ElementIdOp op = obj->getOps()->getElement;
    if (op)
        return false;

    jsid id;
    if (!js::IndexToIdNoGC(cx, index, &id))
        return false;
    return getGenericNoGC(cx, obj, receiver, id, vp);
}

 inline bool
JSObject::getElementIfPresent(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                              uint32_t index, js::MutableHandleValue vp,
                              bool *present)
{
    js::ElementIfPresentOp op = obj->getOps()->getElementIfPresent;
    if (op)
        return op(cx, obj, receiver, index, vp, present);

    




    JS::RootedId id(cx);
    if (!js::IndexToId(cx, index, &id))
        return false;

    JS::RootedObject obj2(cx);
    js::RootedShape prop(cx);
    if (!lookupGeneric(cx, obj, id, &obj2, &prop))
        return false;

    if (!prop) {
        *present = false;
        return true;
    }

    *present = true;
    return getGeneric(cx, obj, receiver, id, vp);
}

inline js::GlobalObject &
JSObject::global() const
{
#ifdef DEBUG
    JSObject *obj = const_cast<JSObject *>(this);
    while (JSObject *parent = obj->getParent())
        obj = parent;
#endif
    return *compartment()->maybeGlobal();
}

namespace js {

PropDesc::PropDesc(const Value &getter, const Value &setter,
                   Enumerability enumerable, Configurability configurable)
  : pd_(UndefinedValue()),
    value_(UndefinedValue()),
    get_(getter), set_(setter),
    attrs(JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED |
          (enumerable ? JSPROP_ENUMERATE : 0) |
          (configurable ? 0 : JSPROP_PERMANENT)),
    hasGet_(true), hasSet_(true),
    hasValue_(false), hasWritable_(false), hasEnumerable_(true), hasConfigurable_(true),
    isUndefined_(false)
{
    MOZ_ASSERT(getter.isUndefined() || js_IsCallable(getter));
    MOZ_ASSERT(setter.isUndefined() || js_IsCallable(setter));
}

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v)
{
    return v.isObject() && v.toObject().is<JSFunction>();
}

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v, JSFunction **fun)
{
    if (v.isObject() && v.toObject().is<JSFunction>()) {
        *fun = &v.toObject().as<JSFunction>();
        return true;
    }
    return false;
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->isNative();
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSFunction **fun)
{
    return IsFunctionObject(v, fun) && (*fun)->isNative();
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSNative native)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->maybeNative() == native;
}









static JS_ALWAYS_INLINE bool
ClassMethodIsNative(JSContext *cx, JSObject *obj, const Class *clasp, jsid methodid, JSNative native)
{
    JS_ASSERT(!obj->is<ProxyObject>());
    JS_ASSERT(obj->getClass() == clasp);

    Value v;
    if (!HasDataProperty(cx, obj, methodid, &v)) {
        JSObject *proto = obj->getProto();
        if (!proto || proto->getClass() != clasp || !HasDataProperty(cx, proto, methodid, &v))
            return false;
    }

    return js::IsNativeFunction(v, native);
}


static JS_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, MutableHandleValue vp)
{
    if (vp.isPrimitive())
        return true;

    JSObject *obj = &vp.toObject();

    
    if (obj->is<StringObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        if (ClassMethodIsNative(cx, obj, &StringObject::class_, id, js_str_toString)) {
            vp.setString(obj->as<StringObject>().unbox());
            return true;
        }
    }

    
    if (obj->is<NumberObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        if (ClassMethodIsNative(cx, obj, &NumberObject::class_, id, js_num_valueOf)) {
            vp.setNumber(obj->as<NumberObject>().unbox());
            return true;
        }
    }

    RootedObject objRoot(cx, obj);
    return JSObject::defaultValue(cx, objRoot, JSTYPE_VOID, vp);
}


static JS_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, JSType preferredType, MutableHandleValue vp)
{
    JS_ASSERT(preferredType != JSTYPE_VOID); 
    if (vp.isPrimitive())
        return true;
    RootedObject obj(cx, &vp.toObject());
    return JSObject::defaultValue(cx, obj, preferredType, vp);
}






inline bool
IsInternalFunctionObject(JSObject *funobj)
{
    JSFunction *fun = &funobj->as<JSFunction>();
    return fun->isLambda() && !funobj->getParent();
}

class AutoPropDescArrayRooter : private AutoGCRooter
{
  public:
    AutoPropDescArrayRooter(JSContext *cx)
      : AutoGCRooter(cx, DESCRIPTORS), descriptors(cx), skip(cx, &descriptors)
    { }

    PropDesc *append() {
        if (!descriptors.append(PropDesc()))
            return nullptr;
        return &descriptors.back();
    }

    bool reserve(size_t n) {
        return descriptors.reserve(n);
    }

    PropDesc& operator[](size_t i) {
        JS_ASSERT(i < descriptors.length());
        return descriptors[i];
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    PropDescArray descriptors;
    SkipRoot skip;
};





JSObject *
NewObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp, TaggedProto proto, JSObject *parent,
                        gc::AllocKind allocKind, NewObjectKind newKind);

inline JSObject *
NewObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp, TaggedProto proto, JSObject *parent,
                        NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithGivenProto(cx, clasp, proto, parent, allocKind, newKind);
}

inline JSObject *
NewObjectWithGivenProto(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithGivenProto(cx, clasp, TaggedProto(proto), parent, newKind);
}

inline JSProtoKey
GetClassProtoKey(const js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null)
        return key;
    if (clasp->flags & JSCLASS_IS_ANONYMOUS)
        return JSProto_Object;
    return JSProto_Null;
}

inline bool
FindProto(ExclusiveContext *cx, const js::Class *clasp, MutableHandleObject proto)
{
    JSProtoKey protoKey = GetClassProtoKey(clasp);
    if (!js_GetClassPrototype(cx, protoKey, proto, clasp))
        return false;
    if (!proto && !js_GetClassPrototype(cx, JSProto_Object, proto))
        return false;
    return true;
}


















JSObject *
NewObjectWithClassProtoCommon(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                              gc::AllocKind allocKind, NewObjectKind newKind);

inline JSObject *
NewObjectWithClassProto(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                        gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProtoCommon(cx, clasp, proto, parent, allocKind, newKind);
}

inline JSObject *
NewObjectWithClassProto(ExclusiveContext *cx, const js::Class *clasp, JSObject *proto, JSObject *parent,
                        NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithClassProto(cx, clasp, proto, parent, allocKind, newKind);
}





inline JSObject *
NewBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp, gc::AllocKind allocKind,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProto(cx, clasp, nullptr, nullptr, allocKind, newKind);
}

inline JSObject *
NewBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewBuiltinClassInstance(cx, clasp, allocKind, newKind);
}


bool
NewObjectScriptedCall(JSContext *cx, MutableHandleObject obj);


static inline JSObject *
CopyInitializerObject(JSContext *cx, HandleObject baseobj, NewObjectKind newKind = GenericObject)
{
    JS_ASSERT(baseobj->getClass() == &JSObject::class_);
    JS_ASSERT(!baseobj->inDictionaryMode());

    gc::AllocKind allocKind = gc::GetGCObjectFixedSlotsKind(baseobj->numFixedSlots());
    allocKind = gc::GetBackgroundAllocKind(allocKind);
    JS_ASSERT_IF(baseobj->isTenured(), allocKind == baseobj->tenuredGetAllocKind());
    RootedObject obj(cx);
    obj = NewBuiltinClassInstance(cx, &JSObject::class_, allocKind, newKind);
    if (!obj)
        return nullptr;

    RootedObject metadata(cx, obj->getMetadata());
    RootedShape lastProp(cx, baseobj->lastProperty());
    if (!JSObject::setLastProperty(cx, obj, lastProp))
        return nullptr;
    if (metadata && !JSObject::setMetadata(cx, obj, metadata))
        return nullptr;

    return obj;
}

JSObject *
NewReshapedObject(JSContext *cx, HandleTypeObject type, JSObject *parent,
                  gc::AllocKind kind, HandleShape shape);






static inline gc::AllocKind
GuessObjectGCKind(size_t numSlots)
{
    if (numSlots)
        return gc::GetGCObjectKind(numSlots);
    return gc::FINALIZE_OBJECT4;
}

static inline gc::AllocKind
GuessArrayGCKind(size_t numSlots)
{
    if (numSlots)
        return gc::GetGCArrayKind(numSlots);
    return gc::FINALIZE_OBJECT8;
}

inline bool
DefineConstructorAndPrototype(JSContext *cx, Handle<GlobalObject*> global,
                              JSProtoKey key, HandleObject ctor, HandleObject proto)
{
    JS_ASSERT(!global->nativeEmpty()); 
    JS_ASSERT(ctor);
    JS_ASSERT(proto);

    RootedId id(cx, NameToId(ClassName(key, cx)));
    JS_ASSERT(!global->nativeLookup(cx, id));

    
    global->setSlot(key, ObjectValue(*ctor));
    global->setSlot(key + JSProto_LIMIT, ObjectValue(*proto));
    global->setSlot(key + JSProto_LIMIT * 2, ObjectValue(*ctor));

    types::AddTypePropertyId(cx, global, id, ObjectValue(*ctor));
    if (!global->addDataProperty(cx, id, key + JSProto_LIMIT * 2, 0)) {
        global->setSlot(key, UndefinedValue());
        global->setSlot(key + JSProto_LIMIT, UndefinedValue());
        global->setSlot(key + JSProto_LIMIT * 2, UndefinedValue());
        return false;
    }

    return true;
}

inline bool
ObjectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx)
{
    if (JS_UNLIKELY(obj->is<ProxyObject>()))
        return Proxy::objectClassIs(obj, classValue, cx);

    switch (classValue) {
      case ESClass_Array: return obj->is<ArrayObject>();
      case ESClass_Number: return obj->is<NumberObject>();
      case ESClass_String: return obj->is<StringObject>();
      case ESClass_Boolean: return obj->is<BooleanObject>();
      case ESClass_RegExp: return obj->is<RegExpObject>();
      case ESClass_ArrayBuffer: return obj->is<ArrayBufferObject>();
      case ESClass_Date: return obj->is<DateObject>();
    }
    MOZ_ASSUME_UNREACHABLE("bad classValue");
}

inline bool
IsObjectWithClass(const Value &v, ESClassValue classValue, JSContext *cx)
{
    if (!v.isObject())
        return false;
    RootedObject obj(cx, &v.toObject());
    return ObjectClassIs(obj, classValue, cx);
}

static JS_ALWAYS_INLINE bool
ValueMightBeSpecial(const Value &propval)
{
    return propval.isObject();
}

static JS_ALWAYS_INLINE bool
ValueIsSpecial(JSObject *obj, MutableHandleValue propval, MutableHandle<SpecialId> sidp,
               JSContext *cx)
{
    return false;
}

JSObject *
DefineConstructorAndPrototype(JSContext *cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                              JSObject *protoProto, const Class *clasp,
                              Native constructor, unsigned nargs,
                              const JSPropertySpec *ps, const JSFunctionSpec *fs,
                              const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
                              JSObject **ctorp = nullptr,
                              gc::AllocKind ctorKind = JSFunction::FinalizeKind);

static JS_ALWAYS_INLINE bool
NewObjectMetadata(ExclusiveContext *cxArg, JSObject **pmetadata)
{
    
    
    JS_ASSERT(!*pmetadata);
    if (JSContext *cx = cxArg->maybeJSContext()) {
        if (JS_UNLIKELY((size_t)cx->compartment()->objectMetadataCallback) &&
            !cx->compartment()->activeAnalysis &&
            !cx->runtime()->mainThread.activeCompilations)
        {
            gc::AutoSuppressGC suppress(cx);
            return cx->compartment()->objectMetadataCallback(cx, pmetadata);
        }
    }
    return true;
}

inline bool
DefineNativeProperty(ExclusiveContext *cx, HandleObject obj, PropertyName *name, HandleValue value,
                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                     unsigned flags, int shortid, unsigned defineHow = 0)
{
    Rooted<jsid> id(cx, NameToId(name));
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs, flags,
                                shortid, defineHow);
}

inline bool
LookupPropertyWithFlags(ExclusiveContext *cx, HandleObject obj, PropertyName *name, unsigned flags,
                        js::MutableHandleObject objp, js::MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return LookupPropertyWithFlags(cx, obj, id, flags, objp, propp);
}

namespace baseops {

inline bool
LookupProperty(ExclusiveContext *cx, HandleObject obj, PropertyName *name,
               MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return LookupProperty<CanGC>(cx, obj, id, objp, propp);
}

inline bool
DefineProperty(ExclusiveContext *cx, HandleObject obj, PropertyName *name, HandleValue value,
               JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return DefineGeneric(cx, obj, id, value, getter, setter, attrs);
}

} 

} 

extern JSObject *
js_InitClass(JSContext *cx, js::HandleObject obj, JSObject *parent_proto,
             const js::Class *clasp, JSNative constructor, unsigned nargs,
             const JSPropertySpec *ps, const JSFunctionSpec *fs,
             const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
             JSObject **ctorp = nullptr,
             js::gc::AllocKind ctorKind = JSFunction::FinalizeKind);

#endif 







#ifndef jsobjinlines_h
#define jsobjinlines_h

#include "jsobj.h"

#include "builtin/MapObject.h"
#include "builtin/TypedObject.h"
#include "gc/Allocator.h"
#include "vm/ArrayObject.h"
#include "vm/DateObject.h"
#include "vm/NumberObject.h"
#include "vm/Probes.h"
#include "vm/ScopeObject.h"
#include "vm/StringObject.h"
#include "vm/TypedArrayCommon.h"

#include "jsatominlines.h"
#include "jscompartmentinlines.h"
#include "jsgcinlines.h"

#include "vm/TypeInference-inl.h"

inline js::Shape*
JSObject::maybeShape() const
{
    if (is<js::UnboxedPlainObject>())
        return nullptr;
    return *reinterpret_cast<js::Shape**>(uintptr_t(this) + offsetOfShape());
}

inline js::Shape*
JSObject::ensureShape(js::ExclusiveContext* cx)
{
    if (is<js::UnboxedPlainObject>() && !js::UnboxedPlainObject::convertToNative(cx->asJSContext(), this))
        return nullptr;
    js::Shape* shape = maybeShape();
    MOZ_ASSERT(shape);
    return shape;
}

inline void
JSObject::finalize(js::FreeOp* fop)
{
    js::probes::FinalizeObject(this);

#ifdef DEBUG
    MOZ_ASSERT(isTenured());
    if (!IsBackgroundFinalized(asTenured().getAllocKind())) {
        
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(fop->runtime()));
    }
#endif
    const js::Class* clasp = getClass();
    if (clasp->finalize)
        clasp->finalize(fop, this);

    if (!clasp->isNative())
        return;

    js::NativeObject* nobj = &as<js::NativeObject>();

    if (nobj->hasDynamicSlots())
        fop->free_(nobj->slots_);

    if (nobj->hasDynamicElements()) {
        js::ObjectElements* elements = nobj->getElementsHeader();
        if (elements->isCopyOnWrite()) {
            if (elements->ownerObject() == this) {
                
                
                
                fop->freeLater(elements);
            }
        } else {
            fop->free_(elements);
        }
    }

    
    
    
    
    if (nobj->shape_->listp == &nobj->shape_)
        nobj->shape_->listp = nullptr;
}

 inline bool
JSObject::setSingleton(js::ExclusiveContext* cx, js::HandleObject obj)
{
    MOZ_ASSERT_IF(cx->isJSContext(), !IsInsideNursery(obj));

    js::ObjectGroup* group = js::ObjectGroup::lazySingletonGroup(cx, obj->getClass(),
                                                                 obj->getTaggedProto());
    if (!group)
        return false;

    obj->group_ = group;
    return true;
}

inline js::ObjectGroup*
JSObject::getGroup(JSContext* cx)
{
    MOZ_ASSERT(cx->compartment() == compartment());
    if (hasLazyGroup()) {
        JS::RootedObject self(cx, this);
        if (cx->compartment() != compartment())
            MOZ_CRASH();
        return makeLazyGroup(cx, self);
    }
    return group_;
}

inline void
JSObject::setGroup(js::ObjectGroup* group)
{
    MOZ_ASSERT(group);
    MOZ_ASSERT(!isSingleton());
    group_ = group;
}




inline bool
js::GetPrototype(JSContext* cx, js::HandleObject obj, js::MutableHandleObject protop)
{
    if (obj->getTaggedProto().isLazy()) {
        MOZ_ASSERT(obj->is<js::ProxyObject>());
        return js::Proxy::getPrototype(cx, obj, protop);
    } else {
        protop.set(obj->getTaggedProto().toObjectOrNull());
        return true;
    }
}

inline bool
js::IsExtensible(ExclusiveContext* cx, HandleObject obj, bool* extensible)
{
    if (obj->is<ProxyObject>()) {
        if (!cx->shouldBeJSContext())
            return false;
        return Proxy::isExtensible(cx->asJSContext(), obj, extensible);
    }

    *extensible = obj->nonProxyIsExtensible();
    return true;
}

inline bool
js::HasProperty(JSContext* cx, HandleObject obj, PropertyName* name, bool* found)
{
    RootedId id(cx, NameToId(name));
    return HasProperty(cx, obj, id, found);
}

inline bool
js::GetElement(JSContext* cx, HandleObject obj, HandleObject receiver, uint32_t index,
               MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return GetProperty(cx, obj, receiver, id, vp);
}

inline bool
js::GetElementNoGC(JSContext* cx, JSObject* obj, JSObject* receiver, uint32_t index, Value* vp)
{
    if (obj->getOps()->getProperty)
        return false;

    if (index > JSID_INT_MAX)
        return false;
    return GetPropertyNoGC(cx, obj, receiver, INT_TO_JSID(index), vp);
}

inline bool
js::DeleteProperty(JSContext* cx, HandleObject obj, HandleId id, ObjectOpResult& result)
{
    MarkTypePropertyNonData(cx, obj, id);
    if (DeletePropertyOp op = obj->getOps()->deleteProperty)
        return op(cx, obj, id, result);
    return NativeDeleteProperty(cx, obj.as<NativeObject>(), id, result);
}

inline bool
js::DeleteElement(JSContext* cx, HandleObject obj, uint32_t index, ObjectOpResult& result)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DeleteProperty(cx, obj, id, result);
}




inline bool
JSObject::isQualifiedVarObj()
{
    if (is<js::DebugScopeObject>())
        return as<js::DebugScopeObject>().scope().isQualifiedVarObj();
    return hasAllFlags(js::BaseShape::QUALIFIED_VAROBJ);
}

inline bool
JSObject::isUnqualifiedVarObj()
{
    if (is<js::DebugScopeObject>())
        return as<js::DebugScopeObject>().scope().isUnqualifiedVarObj();
    return hasAllFlags(js::BaseShape::UNQUALIFIED_VAROBJ);
}

namespace js {

inline bool
ClassCanHaveFixedData(const Class* clasp)
{
    
    
    
    
    
    return !clasp->isNative()
        || clasp == &js::ArrayBufferObject::class_
        || js::IsTypedArrayClass(clasp);
}

static MOZ_ALWAYS_INLINE void
SetNewObjectMetadata(ExclusiveContext* cxArg, JSObject* obj)
{
    
    
    if (JSContext* cx = cxArg->maybeJSContext()) {
        if (MOZ_UNLIKELY((size_t)cx->compartment()->hasObjectMetadataCallback()) &&
            !cx->zone()->types.activeAnalysis)
        {
            
            
            AutoEnterAnalysis enter(cx);

            RootedObject hobj(cx, obj);
            cx->compartment()->setNewObjectMetadata(cx, hobj);
        }
    }
}

} 

 inline JSObject*
JSObject::create(js::ExclusiveContext* cx, js::gc::AllocKind kind, js::gc::InitialHeap heap,
                 js::HandleShape shape, js::HandleObjectGroup group)
{
    MOZ_ASSERT(shape && group);
    MOZ_ASSERT(group->clasp() == shape->getObjectClass());
    MOZ_ASSERT(group->clasp() != &js::ArrayObject::class_);
    MOZ_ASSERT_IF(!js::ClassCanHaveFixedData(group->clasp()),
                  js::gc::GetGCKindSlots(kind, group->clasp()) == shape->numFixedSlots());
    MOZ_ASSERT_IF(group->clasp()->flags & JSCLASS_BACKGROUND_FINALIZE,
                  IsBackgroundFinalized(kind));
    MOZ_ASSERT_IF(group->clasp()->finalize,
                  heap == js::gc::TenuredHeap ||
                  (group->clasp()->flags & JSCLASS_FINALIZE_FROM_NURSERY));

    
    
    
    MOZ_ASSERT_IF(!group->clasp()->isNative(), JSCLASS_RESERVED_SLOTS(group->clasp()) == 0);
    MOZ_ASSERT_IF(!group->clasp()->isNative(), !group->clasp()->hasPrivate());
    MOZ_ASSERT_IF(!group->clasp()->isNative(), shape->numFixedSlots() == 0);
    MOZ_ASSERT_IF(!group->clasp()->isNative(), shape->slotSpan() == 0);

    const js::Class* clasp = group->clasp();
    size_t nDynamicSlots =
        js::NativeObject::dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan(), clasp);

    JSObject* obj = js::Allocate<JSObject>(cx, kind, nDynamicSlots, heap, clasp);
    if (!obj)
        return nullptr;

    obj->group_.init(group);

    obj->setInitialShapeMaybeNonNative(shape);

    
    obj->setInitialElementsMaybeNonNative(js::emptyObjectElements);

    if (clasp->hasPrivate())
        obj->as<js::NativeObject>().privateRef(shape->numFixedSlots()) = nullptr;

    if (size_t span = shape->slotSpan())
        obj->as<js::NativeObject>().initializeSlotRange(0, span);

    
    if (group->clasp()->isJSFunction())
        memset(obj->as<JSFunction>().fixedSlots(), 0, sizeof(js::HeapSlot) * GetGCKindSlots(kind));

    SetNewObjectMetadata(cx, obj);

    js::gc::TraceCreateObject(obj);

    return obj;
}

inline void
JSObject::setInitialShapeMaybeNonNative(js::Shape* shape)
{
    static_cast<js::NativeObject*>(this)->shape_.init(shape);
}

inline void
JSObject::setShapeMaybeNonNative(js::Shape* shape)
{
    MOZ_ASSERT(!is<js::UnboxedPlainObject>());
    static_cast<js::NativeObject*>(this)->shape_ = shape;
}

inline void
JSObject::setInitialSlotsMaybeNonNative(js::HeapSlot* slots)
{
    static_cast<js::NativeObject*>(this)->slots_ = slots;
}

inline void
JSObject::setInitialElementsMaybeNonNative(js::HeapSlot* elements)
{
    static_cast<js::NativeObject*>(this)->elements_ = elements;
}

inline js::GlobalObject&
JSObject::global() const
{
    





    return *compartment()->unsafeUnbarrieredMaybeGlobal();
}

inline bool
JSObject::isOwnGlobal() const
{
    return &global() == this;
}

inline bool
JSObject::hasAllFlags(js::BaseShape::Flag flags) const
{
    MOZ_ASSERT(flags);
    if (js::Shape* shape = maybeShape())
        return shape->hasAllObjectFlags(flags);
    return false;
}

inline bool
JSObject::nonProxyIsExtensible() const
{
    MOZ_ASSERT(!uninlinedIsProxy());

    
    return !hasAllFlags(js::BaseShape::NOT_EXTENSIBLE);
}

inline bool
JSObject::isBoundFunction() const
{
    return hasAllFlags(js::BaseShape::BOUND_FUNCTION);
}

inline bool
JSObject::watched() const
{
    return hasAllFlags(js::BaseShape::WATCHED);
}

inline bool
JSObject::isDelegate() const
{
    return hasAllFlags(js::BaseShape::DELEGATE);
}

inline bool
JSObject::hasUncacheableProto() const
{
    return hasAllFlags(js::BaseShape::UNCACHEABLE_PROTO);
}

inline bool
JSObject::hadElementsAccess() const
{
    return hasAllFlags(js::BaseShape::HAD_ELEMENTS_ACCESS);
}

inline bool
JSObject::isIndexed() const
{
    return hasAllFlags(js::BaseShape::INDEXED);
}

inline bool
JSObject::nonLazyPrototypeIsImmutable() const
{
    MOZ_ASSERT(!hasLazyPrototype());
    return hasAllFlags(js::BaseShape::IMMUTABLE_PROTOTYPE);
}

inline bool
JSObject::isIteratedSingleton() const
{
    return hasAllFlags(js::BaseShape::ITERATED_SINGLETON);
}

inline bool
JSObject::isNewGroupUnknown() const
{
    return hasAllFlags(js::BaseShape::NEW_GROUP_UNKNOWN);
}

inline bool
JSObject::wasNewScriptCleared() const
{
    return hasAllFlags(js::BaseShape::NEW_SCRIPT_CLEARED);
}

namespace js {

static MOZ_ALWAYS_INLINE bool
IsFunctionObject(const js::Value& v)
{
    return v.isObject() && v.toObject().is<JSFunction>();
}

static MOZ_ALWAYS_INLINE bool
IsFunctionObject(const js::Value& v, JSFunction** fun)
{
    if (v.isObject() && v.toObject().is<JSFunction>()) {
        *fun = &v.toObject().as<JSFunction>();
        return true;
    }
    return false;
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value& v)
{
    JSFunction* fun;
    return IsFunctionObject(v, &fun) && fun->isNative();
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value& v, JSFunction** fun)
{
    return IsFunctionObject(v, fun) && (*fun)->isNative();
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value& v, JSNative native)
{
    JSFunction* fun;
    return IsFunctionObject(v, &fun) && fun->maybeNative() == native;
}









static MOZ_ALWAYS_INLINE bool
ClassMethodIsNative(JSContext* cx, NativeObject* obj, const Class* clasp, jsid methodid, JSNative native)
{
    MOZ_ASSERT(obj->getClass() == clasp);

    Value v;
    if (!HasDataProperty(cx, obj, methodid, &v)) {
        JSObject* proto = obj->getProto();
        if (!proto || proto->getClass() != clasp || !HasDataProperty(cx, &proto->as<NativeObject>(), methodid, &v))
            return false;
    }

    return IsNativeFunction(v, native);
}




static MOZ_ALWAYS_INLINE bool
HasObjectValueOf(JSObject* obj, JSContext* cx)
{
    if (obj->is<ProxyObject>() || !obj->isNative())
        return false;

    jsid valueOf = NameToId(cx->names().valueOf);

    Value v;
    while (!HasDataProperty(cx, &obj->as<NativeObject>(), valueOf, &v)) {
        obj = obj->getProto();
        if (!obj || obj->is<ProxyObject>() || !obj->isNative())
            return false;
    }

    return IsNativeFunction(v, obj_valueOf);
}


MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext* cx, MutableHandleValue vp)
{
    if (vp.isPrimitive())
        return true;

    JSObject* obj = &vp.toObject();

    
    if (obj->is<StringObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        StringObject* nobj = &obj->as<StringObject>();
        if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, str_toString)) {
            vp.setString(nobj->unbox());
            return true;
        }
    }

    
    if (obj->is<NumberObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        NumberObject* nobj = &obj->as<NumberObject>();
        if (ClassMethodIsNative(cx, nobj, &NumberObject::class_, id, num_valueOf)) {
            vp.setNumber(nobj->unbox());
            return true;
        }
    }

    RootedObject objRoot(cx, obj);
    return ToPrimitive(cx, objRoot, JSTYPE_VOID, vp);
}


MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext* cx, JSType preferredType, MutableHandleValue vp)
{
    MOZ_ASSERT(preferredType != JSTYPE_VOID); 
    if (vp.isPrimitive())
        return true;
    RootedObject obj(cx, &vp.toObject());
    return ToPrimitive(cx, obj, preferredType, vp);
}






inline bool
IsInternalFunctionObject(JSObject* funobj)
{
    JSFunction* fun = &funobj->as<JSFunction>();
    MOZ_ASSERT_IF(fun->isLambda(),
                  fun->isInterpreted() || fun->isAsmJSNative());
    return fun->isLambda() && fun->isInterpreted() && !fun->environment();
}

typedef AutoVectorRooter<PropertyDescriptor> AutoPropertyDescriptorVector;





JSObject*
NewObjectWithGivenTaggedProto(ExclusiveContext* cx, const Class* clasp, Handle<TaggedProto> proto,
                              gc::AllocKind allocKind, NewObjectKind newKind);

inline JSObject*
NewObjectWithGivenTaggedProto(ExclusiveContext* cx, const Class* clasp, Handle<TaggedProto> proto,
                              NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithGivenTaggedProto(cx, clasp, proto, allocKind, newKind);
}

template <typename T>
inline T*
NewObjectWithGivenTaggedProto(ExclusiveContext* cx, Handle<TaggedProto> proto,
                              NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewObjectWithGivenTaggedProto(cx, &T::class_, proto, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

inline JSObject*
NewObjectWithGivenProto(ExclusiveContext* cx, const Class* clasp, HandleObject proto,
                        gc::AllocKind allocKind, NewObjectKind newKind)
{
    return NewObjectWithGivenTaggedProto(cx, clasp, AsTaggedProto(proto), allocKind,
                                         newKind);
}

inline JSObject*
NewObjectWithGivenProto(ExclusiveContext* cx, const Class* clasp, HandleObject proto,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithGivenTaggedProto(cx, clasp, AsTaggedProto(proto), newKind);
}

template <typename T>
inline T*
NewObjectWithGivenProto(ExclusiveContext* cx, HandleObject proto,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithGivenTaggedProto<T>(cx, AsTaggedProto(proto), newKind);
}

template <typename T>
inline T*
NewObjectWithGivenProto(ExclusiveContext* cx, HandleObject proto,
                        gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewObjectWithGivenTaggedProto(cx, &T::class_, AsTaggedProto(proto),
                                                  allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}



JSObject*
NewObjectWithClassProtoCommon(ExclusiveContext* cx, const Class* clasp, HandleObject proto,
                              gc::AllocKind allocKind, NewObjectKind newKind);

inline JSObject*
NewObjectWithClassProto(ExclusiveContext* cx, const Class* clasp, HandleObject proto,
                        gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProtoCommon(cx, clasp, proto, allocKind, newKind);
}

inline JSObject*
NewObjectWithClassProto(ExclusiveContext* cx, const Class* clasp, HandleObject proto,
                        NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithClassProto(cx, clasp, proto, allocKind, newKind);
}

template<typename T>
inline T*
NewObjectWithProto(ExclusiveContext* cx, HandleObject proto,
                   gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewObjectWithClassProto(cx, &T::class_, proto, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template<typename T>
inline T*
NewObjectWithProto(ExclusiveContext* cx, HandleObject proto,
                   NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewObjectWithClassProto(cx, &T::class_, proto, newKind);
    return obj ? &obj->as<T>() : nullptr;
}





inline JSObject*
NewBuiltinClassInstance(ExclusiveContext* cx, const Class* clasp, gc::AllocKind allocKind,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProto(cx, clasp, NullPtr(), allocKind, newKind);
}

inline JSObject*
NewBuiltinClassInstance(ExclusiveContext* cx, const Class* clasp, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewBuiltinClassInstance(cx, clasp, allocKind, newKind);
}

template<typename T>
inline T*
NewBuiltinClassInstance(ExclusiveContext* cx, NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewBuiltinClassInstance(cx, &T::class_, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template<typename T>
inline T*
NewBuiltinClassInstance(ExclusiveContext* cx, gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewBuiltinClassInstance(cx, &T::class_, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}


bool
NewObjectScriptedCall(JSContext* cx, MutableHandleObject obj);

JSObject*
NewObjectWithGroupCommon(ExclusiveContext* cx, HandleObjectGroup group,
                         gc::AllocKind allocKind, NewObjectKind newKind);

template <typename T>
inline T*
NewObjectWithGroup(ExclusiveContext* cx, HandleObjectGroup group,
                   gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject* obj = NewObjectWithGroupCommon(cx, group, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template <typename T>
inline T*
NewObjectWithGroup(ExclusiveContext* cx, HandleObjectGroup group,
                   NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(group->clasp());
    return NewObjectWithGroup<T>(cx, group, allocKind, newKind);
}






static inline gc::AllocKind
GuessObjectGCKind(size_t numSlots)
{
    if (numSlots)
        return gc::GetGCObjectKind(numSlots);
    return gc::AllocKind::OBJECT4;
}

static inline gc::AllocKind
GuessArrayGCKind(size_t numSlots)
{
    if (numSlots)
        return gc::GetGCArrayKind(numSlots);
    return gc::AllocKind::OBJECT8;
}

inline bool
ObjectClassIs(HandleObject obj, ESClassValue classValue, JSContext* cx)
{
    if (MOZ_UNLIKELY(obj->is<ProxyObject>()))
        return Proxy::objectClassIs(obj, classValue, cx);

    switch (classValue) {
      case ESClass_Object: return obj->is<PlainObject>();
      case ESClass_Array:
      case ESClass_IsArray:
        
        return obj->is<ArrayObject>();
      case ESClass_Number: return obj->is<NumberObject>();
      case ESClass_String: return obj->is<StringObject>();
      case ESClass_Boolean: return obj->is<BooleanObject>();
      case ESClass_RegExp: return obj->is<RegExpObject>();
      case ESClass_ArrayBuffer: return obj->is<ArrayBufferObject>();
      case ESClass_SharedArrayBuffer: return obj->is<SharedArrayBufferObject>();
      case ESClass_Date: return obj->is<DateObject>();
      case ESClass_Set: return obj->is<SetObject>();
      case ESClass_Map: return obj->is<MapObject>();
    }
    MOZ_CRASH("bad classValue");
}

inline bool
IsObjectWithClass(const Value& v, ESClassValue classValue, JSContext* cx)
{
    if (!v.isObject())
        return false;
    RootedObject obj(cx, &v.toObject());
    return ObjectClassIs(obj, classValue, cx);
}


inline bool
IsArray(HandleObject obj, JSContext* cx)
{
    if (obj->is<ArrayObject>())
        return true;

    return ObjectClassIs(obj, ESClass_IsArray, cx);
}

inline bool
Unbox(JSContext* cx, HandleObject obj, MutableHandleValue vp)
{
    if (MOZ_UNLIKELY(obj->is<ProxyObject>()))
        return Proxy::boxedValue_unbox(cx, obj, vp);

    if (obj->is<BooleanObject>())
        vp.setBoolean(obj->as<BooleanObject>().unbox());
    else if (obj->is<NumberObject>())
        vp.setNumber(obj->as<NumberObject>().unbox());
    else if (obj->is<StringObject>())
        vp.setString(obj->as<StringObject>().unbox());
    else if (obj->is<DateObject>())
        vp.set(obj->as<DateObject>().UTCTime());
    else
        vp.setUndefined();

    return true;
}

extern NativeObject*
InitClass(JSContext* cx, HandleObject obj, HandleObject parent_proto,
          const Class* clasp, JSNative constructor, unsigned nargs,
          const JSPropertySpec* ps, const JSFunctionSpec* fs,
          const JSPropertySpec* static_ps, const JSFunctionSpec* static_fs,
          NativeObject** ctorp = nullptr,
          gc::AllocKind ctorKind = JSFunction::FinalizeKind);

} 

#endif 

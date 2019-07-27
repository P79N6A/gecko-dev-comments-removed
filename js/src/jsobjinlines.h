





#ifndef jsobjinlines_h
#define jsobjinlines_h

#include "jsobj.h"

#include "builtin/MapObject.h"
#include "builtin/TypedObject.h"
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

inline void
JSObject::finalize(js::FreeOp *fop)
{
    js::probes::FinalizeObject(this);

#ifdef DEBUG
    MOZ_ASSERT(isTenured());
    if (!IsBackgroundFinalized(asTenured().getAllocKind())) {
        
        MOZ_ASSERT(CurrentThreadCanAccessRuntime(fop->runtime()));
    }
#endif
    const js::Class *clasp = getClass();
    if (clasp->finalize)
        clasp->finalize(fop, this);

    if (!clasp->isNative())
        return;

    js::NativeObject *nobj = &as<js::NativeObject>();

    if (nobj->hasDynamicSlots())
        fop->free_(nobj->slots_);

    if (nobj->hasDynamicElements()) {
        js::ObjectElements *elements = nobj->getElementsHeader();
        if (elements->isCopyOnWrite()) {
            if (elements->ownerObject() == this) {
                
                
                
                fop->freeLater(elements);
            }
        } else {
            fop->free_(elements);
        }
    }

    
    
    
    
    if (shape_->listp == &shape_)
        shape_->listp = nullptr;
}

 inline bool
JSObject::setSingleton(js::ExclusiveContext *cx, js::HandleObject obj)
{
    MOZ_ASSERT_IF(cx->isJSContext(), !IsInsideNursery(obj));

    js::ObjectGroup *group = js::ObjectGroup::lazySingletonGroup(cx, obj->getClass(),
                                                                 obj->getTaggedProto());
    if (!group)
        return false;

    obj->group_ = group;
    return true;
}

inline js::ObjectGroup*
JSObject::getGroup(JSContext *cx)
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
JSObject::setGroup(js::ObjectGroup *group)
{
    MOZ_ASSERT(group);
    MOZ_ASSERT(!isSingleton());
    group_ = group;
}




inline bool
js::GetPrototype(JSContext *cx, js::HandleObject obj, js::MutableHandleObject protop)
{
    if (obj->getTaggedProto().isLazy()) {
        MOZ_ASSERT(obj->is<js::ProxyObject>());
        return js::Proxy::getPrototypeOf(cx, obj, protop);
    } else {
        protop.set(obj->getTaggedProto().toObjectOrNull());
        return true;
    }
}

inline bool
js::IsExtensible(ExclusiveContext *cx, HandleObject obj, bool *extensible)
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
js::HasProperty(JSContext *cx, HandleObject obj, PropertyName *name, bool *found)
{
    RootedId id(cx, NameToId(name));
    return HasProperty(cx, obj, id, found);
}

inline bool
js::GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
               MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return GetProperty(cx, obj, receiver, id, vp);
}

inline bool
js::GetElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index, Value *vp)
{
    if (obj->getOps()->getProperty)
        return false;

    if (index > JSID_INT_MAX)
        return false;
    return GetPropertyNoGC(cx, obj, receiver, INT_TO_JSID(index), vp);
}

inline bool
js::DeleteProperty(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    MarkTypePropertyNonData(cx, obj, id);
    if (DeletePropertyOp op = obj->getOps()->deleteProperty)
        return op(cx, obj, id, succeeded);
    return NativeDeleteProperty(cx, obj.as<NativeObject>(), id, succeeded);
}

inline bool
js::DeleteElement(JSContext *cx, HandleObject obj, uint32_t index, bool *succeeded)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DeleteProperty(cx, obj, id, succeeded);
}




inline bool
JSObject::isQualifiedVarObj()
{
    if (is<js::DebugScopeObject>())
        return as<js::DebugScopeObject>().scope().isQualifiedVarObj();
    return lastProperty()->hasObjectFlag(js::BaseShape::QUALIFIED_VAROBJ);
}

inline bool
JSObject::isUnqualifiedVarObj()
{
    if (is<js::DebugScopeObject>())
        return as<js::DebugScopeObject>().scope().isUnqualifiedVarObj();
    return lastProperty()->hasObjectFlag(js::BaseShape::UNQUALIFIED_VAROBJ);
}

namespace js {

inline bool
ClassCanHaveFixedData(const Class *clasp)
{
    
    
    
    
    
    return !clasp->isNative()
        || clasp == &js::ArrayBufferObject::class_
        || js::IsTypedArrayClass(clasp);
}

} 

 inline JSObject *
JSObject::create(js::ExclusiveContext *cx, js::gc::AllocKind kind, js::gc::InitialHeap heap,
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

    const js::Class *clasp = group->clasp();
    size_t nDynamicSlots =
        js::NativeObject::dynamicSlotsCount(shape->numFixedSlots(), shape->slotSpan(), clasp);

    JSObject *obj = js::NewGCObject<js::CanGC>(cx, kind, nDynamicSlots, heap, clasp);
    if (!obj)
        return nullptr;

    obj->shape_.init(shape);
    obj->group_.init(group);

    
    obj->setInitialElementsMaybeNonNative(js::emptyObjectElements);

    if (clasp->hasPrivate())
        obj->as<js::NativeObject>().privateRef(shape->numFixedSlots()) = nullptr;

    if (size_t span = shape->slotSpan())
        obj->as<js::NativeObject>().initializeSlotRange(0, span);

    
    if (group->clasp()->isJSFunction())
        memset(obj->as<JSFunction>().fixedSlots(), 0, sizeof(js::HeapSlot) * GetGCKindSlots(kind));

    js::gc::TraceCreateObject(obj);

    return obj;
}

inline void
JSObject::setInitialSlotsMaybeNonNative(js::HeapSlot *slots)
{
    static_cast<js::NativeObject *>(this)->slots_ = slots;
}

inline void
JSObject::setInitialElementsMaybeNonNative(js::HeapSlot *elements)
{
    static_cast<js::NativeObject *>(this)->elements_ = elements;
}

inline js::GlobalObject &
JSObject::global() const
{
#ifdef DEBUG
    JSObject *obj = const_cast<JSObject *>(this);
    while (JSObject *parent = obj->getParent())
        obj = parent;
#endif
    





    return *compartment()->unsafeUnbarrieredMaybeGlobal();
}

inline bool
JSObject::isOwnGlobal() const
{
    return &global() == this;
}

namespace js {

PropDesc::PropDesc(const Value &getter, const Value &setter,
                   Enumerability enumerable, Configurability configurable)
  : value_(UndefinedValue()),
    get_(getter), set_(setter),
    attrs(JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED |
          (enumerable ? JSPROP_ENUMERATE : 0) |
          (configurable ? 0 : JSPROP_PERMANENT)),
    hasGet_(true), hasSet_(true),
    hasValue_(false), hasWritable_(false), hasEnumerable_(true), hasConfigurable_(true),
    isUndefined_(false)
{
    MOZ_ASSERT(getter.isUndefined() || IsCallable(getter));
    MOZ_ASSERT(setter.isUndefined() || IsCallable(setter));
}

static MOZ_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v)
{
    return v.isObject() && v.toObject().is<JSFunction>();
}

static MOZ_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v, JSFunction **fun)
{
    if (v.isObject() && v.toObject().is<JSFunction>()) {
        *fun = &v.toObject().as<JSFunction>();
        return true;
    }
    return false;
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->isNative();
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSFunction **fun)
{
    return IsFunctionObject(v, fun) && (*fun)->isNative();
}

static MOZ_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSNative native)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->maybeNative() == native;
}









static MOZ_ALWAYS_INLINE bool
ClassMethodIsNative(JSContext *cx, NativeObject *obj, const Class *clasp, jsid methodid, JSNative native)
{
    MOZ_ASSERT(obj->getClass() == clasp);

    Value v;
    if (!HasDataProperty(cx, obj, methodid, &v)) {
        JSObject *proto = obj->getProto();
        if (!proto || proto->getClass() != clasp || !HasDataProperty(cx, &proto->as<NativeObject>(), methodid, &v))
            return false;
    }

    return IsNativeFunction(v, native);
}




static MOZ_ALWAYS_INLINE bool
HasObjectValueOf(JSObject *obj, JSContext *cx)
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
ToPrimitive(JSContext *cx, MutableHandleValue vp)
{
    if (vp.isPrimitive())
        return true;

    JSObject *obj = &vp.toObject();

    
    if (obj->is<StringObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        StringObject *nobj = &obj->as<StringObject>();
        if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, js_str_toString)) {
            vp.setString(nobj->unbox());
            return true;
        }
    }

    
    if (obj->is<NumberObject>()) {
        jsid id = NameToId(cx->names().valueOf);
        NumberObject *nobj = &obj->as<NumberObject>();
        if (ClassMethodIsNative(cx, nobj, &NumberObject::class_, id, js_num_valueOf)) {
            vp.setNumber(nobj->unbox());
            return true;
        }
    }

    RootedObject objRoot(cx, obj);
    return ToPrimitive(cx, objRoot, JSTYPE_VOID, vp);
}


MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, JSType preferredType, MutableHandleValue vp)
{
    MOZ_ASSERT(preferredType != JSTYPE_VOID); 
    if (vp.isPrimitive())
        return true;
    RootedObject obj(cx, &vp.toObject());
    return ToPrimitive(cx, obj, preferredType, vp);
}






inline bool
IsInternalFunctionObject(JSObject *funobj)
{
    JSFunction *fun = &funobj->as<JSFunction>();
    return fun->isLambda() && !funobj->getParent();
}

class AutoPropDescVector : public AutoVectorRooter<PropDesc>
{
  public:
    explicit AutoPropDescVector(JSContext *cx
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<PropDesc>(cx, DESCVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};





JSObject *
NewObjectWithGivenTaggedProto(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                              HandleObject parent, gc::AllocKind allocKind, NewObjectKind newKind);

inline JSObject *
NewObjectWithGivenTaggedProto(ExclusiveContext *cx, const Class *clasp, Handle<TaggedProto> proto,
                              HandleObject parent, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithGivenTaggedProto(cx, clasp, proto, parent, allocKind, newKind);
}

template <typename T>
inline T *
NewObjectWithGivenTaggedProto(ExclusiveContext *cx, Handle<TaggedProto> proto, HandleObject parent,
                              NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewObjectWithGivenTaggedProto(cx, &T::class_, proto, parent, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

inline JSObject *
NewObjectWithGivenProto(ExclusiveContext *cx, const Class *clasp, HandleObject proto,
                        HandleObject parent, gc::AllocKind allocKind, NewObjectKind newKind)
{
    return NewObjectWithGivenTaggedProto(cx, clasp, AsTaggedProto(proto), parent, allocKind,
                                         newKind);
}

inline JSObject *
NewObjectWithGivenProto(ExclusiveContext *cx, const Class *clasp, HandleObject proto,
                        HandleObject parent, NewObjectKind newKind = GenericObject)
{
    return NewObjectWithGivenTaggedProto(cx, clasp, AsTaggedProto(proto), parent, newKind);
}

template <typename T>
inline T *
NewObjectWithGivenProto(ExclusiveContext *cx, HandleObject proto, HandleObject parent,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithGivenTaggedProto<T>(cx, AsTaggedProto(proto), parent, newKind);
}

template <typename T>
inline T *
NewObjectWithGivenProto(ExclusiveContext *cx, HandleObject proto, HandleObject parent,
                        gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewObjectWithGivenTaggedProto(cx, &T::class_, AsTaggedProto(proto), parent,
                                                  allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}


















JSObject *
NewObjectWithClassProtoCommon(ExclusiveContext *cx, const Class *clasp, HandleObject proto,
                              HandleObject parent, gc::AllocKind allocKind,
                              NewObjectKind newKind);

inline JSObject *
NewObjectWithClassProto(ExclusiveContext *cx, const Class *clasp, HandleObject proto,
                        HandleObject parent, gc::AllocKind allocKind,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProtoCommon(cx, clasp, proto, parent, allocKind, newKind);
}

inline JSObject *
NewObjectWithClassProto(ExclusiveContext *cx, const Class *clasp, HandleObject proto,
                        HandleObject parent, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewObjectWithClassProto(cx, clasp, proto, parent, allocKind, newKind);
}

template<typename T>
inline T *
NewObjectWithProto(ExclusiveContext *cx, HandleObject proto, HandleObject parent,
                   gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewObjectWithClassProto(cx, &T::class_, proto, parent, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template<typename T>
inline T *
NewObjectWithProto(ExclusiveContext *cx, HandleObject proto, HandleObject parent,
                   NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewObjectWithClassProto(cx, &T::class_, proto, parent, newKind);
    return obj ? &obj->as<T>() : nullptr;
}





inline JSObject *
NewBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp, gc::AllocKind allocKind,
                        NewObjectKind newKind = GenericObject)
{
    return NewObjectWithClassProto(cx, clasp, NullPtr(), NullPtr(), allocKind, newKind);
}

inline JSObject *
NewBuiltinClassInstance(ExclusiveContext *cx, const Class *clasp, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(clasp);
    return NewBuiltinClassInstance(cx, clasp, allocKind, newKind);
}

template<typename T>
inline T *
NewBuiltinClassInstance(ExclusiveContext *cx, NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &T::class_, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template<typename T>
inline T *
NewBuiltinClassInstance(ExclusiveContext *cx, gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &T::class_, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}


bool
NewObjectScriptedCall(JSContext *cx, MutableHandleObject obj);

JSObject *
NewObjectWithGroupCommon(JSContext *cx, HandleObjectGroup group, HandleObject parent,
                         gc::AllocKind allocKind, NewObjectKind newKind);

template <typename T>
inline T *
NewObjectWithGroup(JSContext *cx, HandleObjectGroup group, HandleObject parent,
                   gc::AllocKind allocKind, NewObjectKind newKind = GenericObject)
{
    JSObject *obj = NewObjectWithGroupCommon(cx, group, parent, allocKind, newKind);
    return obj ? &obj->as<T>() : nullptr;
}

template <typename T>
inline T *
NewObjectWithGroup(JSContext *cx, HandleObjectGroup group, HandleObject parent,
                   NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = gc::GetGCObjectKind(group->clasp());
    return NewObjectWithGroup<T>(cx, group, parent, allocKind, newKind);
}

JSObject *
NewReshapedObject(JSContext *cx, HandleObjectGroup group, HandleObject parent,
                  gc::AllocKind allocKind, HandleShape shape,
                  NewObjectKind newKind = GenericObject);






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
ObjectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx)
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
IsObjectWithClass(const Value &v, ESClassValue classValue, JSContext *cx)
{
    if (!v.isObject())
        return false;
    RootedObject obj(cx, &v.toObject());
    return ObjectClassIs(obj, classValue, cx);
}


inline bool
IsArray(HandleObject obj, JSContext *cx)
{
    if (obj->is<ArrayObject>())
        return true;

    return ObjectClassIs(obj, ESClass_IsArray, cx);
}

inline bool
Unbox(JSContext *cx, HandleObject obj, MutableHandleValue vp)
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

static MOZ_ALWAYS_INLINE bool
NewObjectMetadata(ExclusiveContext *cxArg, JSObject **pmetadata)
{
    
    
    
    MOZ_ASSERT(!*pmetadata);
    if (JSContext *cx = cxArg->maybeJSContext()) {
        if (MOZ_UNLIKELY((size_t)cx->compartment()->hasObjectMetadataCallback()) &&
            !cx->zone()->types.activeAnalysis)
        {
            
            
            AutoEnterAnalysis enter(cx);

            if (!cx->compartment()->callObjectMetadataCallback(cx, pmetadata))
                return false;
        }
    }
    return true;
}

static inline unsigned
ApplyAttributes(unsigned attrs, bool enumerable, bool writable, bool configurable)
{
    



    if (attrs & JSPROP_IGNORE_ENUMERATE) {
        attrs &= ~JSPROP_IGNORE_ENUMERATE;
        if (enumerable)
            attrs |= JSPROP_ENUMERATE;
        else
            attrs &= ~JSPROP_ENUMERATE;
    }
    if (attrs & JSPROP_IGNORE_READONLY) {
        attrs &= ~JSPROP_IGNORE_READONLY;
        
        if (!(attrs & (JSPROP_GETTER | JSPROP_SETTER))) {
            if (!writable)
                attrs |= JSPROP_READONLY;
            else
                attrs &= ~JSPROP_READONLY;
        }
    }
    if (attrs & JSPROP_IGNORE_PERMANENT) {
        attrs &= ~JSPROP_IGNORE_PERMANENT;
        if (!configurable)
            attrs |= JSPROP_PERMANENT;
        else
            attrs &= ~JSPROP_PERMANENT;
    }
    return attrs;
}

} 

extern js::NativeObject *
js_InitClass(JSContext *cx, js::HandleObject obj, js::HandleObject parent_proto,
             const js::Class *clasp, JSNative constructor, unsigned nargs,
             const JSPropertySpec *ps, const JSFunctionSpec *fs,
             const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
             js::NativeObject **ctorp = nullptr,
             js::gc::AllocKind ctorKind = JSFunction::FinalizeKind);

#endif 

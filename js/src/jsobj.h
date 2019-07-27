





#ifndef jsobj_h
#define jsobj_h










#include "mozilla/MemoryReporting.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/Conversions.h"
#include "js/GCAPI.h"
#include "js/HeapAPI.h"
#include "vm/Shape.h"
#include "vm/String.h"
#include "vm/Xdr.h"

namespace JS {
struct ClassInfo;
}

namespace js {

class AutoPropDescVector;
class GCMarker;
struct NativeIterator;
class Nursery;
class ObjectElements;
struct StackShape;

namespace gc {
class RelocationOverlay;
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline JSObject *
CastAsObject(StrictPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(StrictPropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}



typedef Vector<PropDesc, 1> PropDescArray;

extern const Class IntlClass;
extern const Class JSONClass;
extern const Class MathClass;

class GlobalObject;
class MapObject;
class NewObjectCache;
class NormalArgumentsObject;
class SetObject;
class StrictArgumentsObject;


bool PreventExtensions(JSContext *cx, JS::HandleObject obj, bool *succeeded);
bool SetImmutablePrototype(js::ExclusiveContext *cx, JS::HandleObject obj, bool *succeeded);

}  













class JSObject : public js::gc::Cell
{
  protected:
    js::HeapPtrShape shape_;
    js::HeapPtrObjectGroup group_;

  private:
    friend class js::Shape;
    friend class js::GCMarker;
    friend class js::NewObjectCache;
    friend class js::Nursery;
    friend class js::gc::RelocationOverlay;
    friend bool js::PreventExtensions(JSContext *cx, JS::HandleObject obj, bool *succeeded);
    friend bool js::SetImmutablePrototype(js::ExclusiveContext *cx, JS::HandleObject obj,
                                          bool *succeeded);

    
    static js::ObjectGroup *makeLazyGroup(JSContext *cx, js::HandleObject obj);

  public:
    js::Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    bool isNative() const {
        return lastProperty()->isNative();
    }

    const js::Class *getClass() const {
        return group_->clasp();
    }
    const JSClass *getJSClass() const {
        return Jsvalify(getClass());
    }
    bool hasClass(const js::Class *c) const {
        return getClass() == c;
    }
    const js::ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    js::ObjectGroup *group() const {
        MOZ_ASSERT(!hasLazyGroup());
        return groupRaw();
    }

    js::ObjectGroup *groupRaw() const {
        return group_;
    }

    



    bool isSingleton() const {
        return !!group_->singleton();
    }

    



    bool hasLazyGroup() const {
        return group_->lazy();
    }

    JSCompartment *compartment() const {
        return lastProperty()->base()->compartment();
    }

    



    static inline JSObject *create(js::ExclusiveContext *cx,
                                   js::gc::AllocKind kind,
                                   js::gc::InitialHeap heap,
                                   js::HandleShape shape,
                                   js::HandleObjectGroup group);

    
    
    
    
    inline void setInitialSlotsMaybeNonNative(js::HeapSlot *slots);
    inline void setInitialElementsMaybeNonNative(js::HeapSlot *elements);

  protected:
    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(js::ExclusiveContext *cx,  uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);

  public:
    








    bool isDelegate() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::DELEGATE);
    }

    bool setDelegate(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::DELEGATE, GENERATE_SHAPE);
    }

    bool isBoundFunction() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::BOUND_FUNCTION);
    }

    inline bool hasSpecialEquality() const;

    bool watched() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::WATCHED);
    }
    bool setWatched(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::WATCHED, GENERATE_SHAPE);
    }

    
    inline bool isQualifiedVarObj();
    bool setQualifiedVarObj(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::QUALIFIED_VAROBJ);
    }

    inline bool isUnqualifiedVarObj();
    bool setUnqualifiedVarObj(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::UNQUALIFIED_VAROBJ);
    }

    





    bool hasUncacheableProto() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::UNCACHEABLE_PROTO);
    }
    bool setUncacheableProto(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::UNCACHEABLE_PROTO, GENERATE_SHAPE);
    }

    



    bool hadElementsAccess() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::HAD_ELEMENTS_ACCESS);
    }
    bool setHadElementsAccess(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::HAD_ELEMENTS_ACCESS);
    }

    



    bool isIndexed() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::INDEXED);
    }

    uint32_t propertyCount() const {
        return lastProperty()->entryCount();
    }

    bool hasShapeTable() const {
        return lastProperty()->hasTable();
    }

    

    void markChildren(JSTracer *trc);

    void fixupAfterMovingGC();

    static js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }
    static const size_t MaxTagBits = 3;
    static bool isNullLike(const JSObject *obj) { return uintptr_t(obj) < (1 << MaxTagBits); }

    MOZ_ALWAYS_INLINE JS::Zone *zone() const {
        return shape_->zone();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZone() const {
        return JS::shadow::Zone::asShadowZone(zone());
    }
    MOZ_ALWAYS_INLINE JS::Zone *zoneFromAnyThread() const {
        return shape_->zoneFromAnyThread();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone *shadowZoneFromAnyThread() const {
        return JS::shadow::Zone::asShadowZone(zoneFromAnyThread());
    }
    static MOZ_ALWAYS_INLINE void readBarrier(JSObject *obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPre(JSObject *obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPost(JSObject *obj, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRelocate(JSObject *obj, void *cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRemove(JSObject *obj, void *cellp);

    size_t tenuredSizeOfThis() const {
        MOZ_ASSERT(isTenured());
        return js::gc::Arena::thingSize(asTenured().getAllocKind());
    }

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ClassInfo *info);

    bool hasIdempotentProtoChain() const;

    



    static inline bool setSingleton(js::ExclusiveContext *cx, js::HandleObject obj);

    inline js::ObjectGroup* getGroup(JSContext *cx);

    const js::HeapPtrObjectGroup &groupFromGC() const {
        
        return group_;
    }

    












    js::TaggedProto getTaggedProto() const {
        return group_->proto();
    }

    bool hasTenuredProto() const;

    bool uninlinedIsProxy() const;

    JSObject *getProto() const {
        MOZ_ASSERT(!uninlinedIsProxy());
        return getTaggedProto().toObjectOrNull();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool hasLazyPrototype() const {
        bool lazy = getTaggedProto().isLazy();
        MOZ_ASSERT_IF(lazy, uninlinedIsProxy());
        return lazy;
    }

    
    
    bool nonLazyPrototypeIsImmutable() const {
        MOZ_ASSERT(!hasLazyPrototype());
        return lastProperty()->hasObjectFlag(js::BaseShape::IMMUTABLE_PROTOTYPE);
    }

    inline void setGroup(js::ObjectGroup *group);

    




    bool isIteratedSingleton() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::ITERATED_SINGLETON);
    }
    bool setIteratedSingleton(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::ITERATED_SINGLETON);
    }

    



    bool isNewGroupUnknown() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::NEW_GROUP_UNKNOWN);
    }
    static bool setNewGroupUnknown(JSContext *cx, const js::Class *clasp, JS::HandleObject obj);

    
    bool wasNewScriptCleared() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::NEW_SCRIPT_CLEARED);
    }
    bool setNewScriptCleared(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::NEW_SCRIPT_CLEARED);
    }

    
    bool splicePrototype(JSContext *cx, const js::Class *clasp, js::Handle<js::TaggedProto> proto);

    



    bool shouldSplicePrototype(JSContext *cx);

    

























    
    JSObject *getParent() const {
        return lastProperty()->getObjectParent();
    }
    static bool setParent(JSContext *cx, js::HandleObject obj, js::HandleObject newParent);

    




    inline JSObject *enclosingScope();

    
    inline JSObject *getMetadata() const {
        return lastProperty()->getObjectMetadata();
    }
    static bool setMetadata(JSContext *cx, js::HandleObject obj, js::HandleObject newMetadata);

    inline js::GlobalObject &global() const;
    inline bool isOwnGlobal() const;

    



  public:
    
    
    
    
    bool nonProxyIsExtensible() const {
        MOZ_ASSERT(!uninlinedIsProxy());

        
        return !lastProperty()->hasObjectFlag(js::BaseShape::NOT_EXTENSIBLE);
    }

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    bool isCallable() const;
    bool isConstructor() const;
    JSNative callHook() const;
    JSNative constructHook() const;

    MOZ_ALWAYS_INLINE void finalize(js::FreeOp *fop);

  public:
    static bool reportReadOnly(JSContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, js::HandleId id, unsigned argc, js::Value *argv,
                    js::MutableHandleValue vp);

    static bool nonNativeSetProperty(JSContext *cx, js::HandleObject obj,
                                     js::HandleObject receiver, js::HandleId id,
                                     js::MutableHandleValue vp, bool strict);
    static bool nonNativeSetElement(JSContext *cx, js::HandleObject obj,
                                    js::HandleObject receiver, uint32_t index,
                                    js::MutableHandleValue vp, bool strict);

    static bool swap(JSContext *cx, JS::HandleObject a, JS::HandleObject b);

  private:
    void fixDictionaryShapeAfterSwap();

  public:
    inline void initArrayClass();

    






















    template <class T>
    inline bool is() const { return getClass() == &T::class_; }

    template <class T>
    T &as() {
        MOZ_ASSERT(this->is<T>());
        return *static_cast<T *>(this);
    }

    template <class T>
    const T &as() const {
        MOZ_ASSERT(this->is<T>());
        return *static_cast<const T *>(this);
    }

#ifdef DEBUG
    void dump();
#endif

    

    static size_t offsetOfShape() { return offsetof(JSObject, shape_); }
    static size_t offsetOfGroup() { return offsetof(JSObject, group_); }

    
    static const size_t MAX_BYTE_SIZE = 4 * sizeof(void *) + 16 * sizeof(JS::Value);

  private:
    JSObject() = delete;
    JSObject(const JSObject &other) = delete;
    void operator=(const JSObject &other) = delete;
};

template <class U>
MOZ_ALWAYS_INLINE JS::Handle<U*>
js::RootedBase<JSObject*>::as() const
{
    const JS::Rooted<JSObject*> &self = *static_cast<const JS::Rooted<JSObject*>*>(this);
    MOZ_ASSERT(self->is<U>());
    return Handle<U*>::fromMarkedLocation(reinterpret_cast<U* const*>(self.address()));
}

template <class U>
MOZ_ALWAYS_INLINE JS::Handle<U*>
js::HandleBase<JSObject*>::as() const
{
    const JS::Handle<JSObject*> &self = *static_cast<const JS::Handle<JSObject*>*>(this);
    MOZ_ASSERT(self->is<U>());
    return Handle<U*>::fromMarkedLocation(reinterpret_cast<U* const*>(self.address()));
}






static MOZ_ALWAYS_INLINE bool
operator==(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs == &rhs;
}

static MOZ_ALWAYS_INLINE bool
operator!=(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs != &rhs;
}


struct JSObject_Slots0 : JSObject { void *data[2]; };
struct JSObject_Slots2 : JSObject { void *data[2]; js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { void *data[2]; js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { void *data[2]; js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { void *data[2]; js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { void *data[2]; js::Value fslots[16]; };

 MOZ_ALWAYS_INLINE void
JSObject::readBarrier(JSObject *obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured().readBarrier(&obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPre(JSObject *obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured().writeBarrierPre(&obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPost(JSObject *obj, void *cellp)
{
    MOZ_ASSERT(cellp);
    if (IsNullTaggedPointer(obj))
        return;
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
    js::gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putCellFromAnyThread(static_cast<js::gc::Cell **>(cellp));
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRelocate(JSObject *obj, void *cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
    js::gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(static_cast<js::gc::Cell **>(cellp));
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRemove(JSObject *obj, void *cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->removeRelocatableCellFromAnyThread(
        static_cast<js::gc::Cell **>(cellp));
}

namespace js {

inline bool
IsCallable(const Value &v)
{
    return v.isObject() && v.toObject().isCallable();
}


inline bool
IsConstructor(const Value &v)
{
    return v.isObject() && v.toObject().isConstructor();
}

} 

class JSValueArray {
  public:
    const jsval *array;
    size_t length;

    JSValueArray(const jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};

namespace js {























inline bool
GetPrototype(JSContext *cx, HandleObject obj, MutableHandleObject protop);








extern bool
SetPrototype(JSContext *cx, HandleObject obj, HandleObject proto, bool *succeeded);






inline bool
IsExtensible(ExclusiveContext *cx, HandleObject obj, bool *extensible);






extern bool
PreventExtensions(JSContext *cx, HandleObject obj, bool *succeeded);








extern bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                         MutableHandle<PropertyDescriptor> desc);












extern bool
StandardDefineProperty(JSContext *cx, HandleObject obj, HandleId id,
                       const PropDesc &desc, bool throwError, bool *rval);

extern bool
StandardDefineProperty(JSContext *cx, HandleObject obj, HandleId id,
                       Handle<PropertyDescriptor> descriptor, bool *bp);

extern bool
DefineProperty(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
               JSPropertyOp getter = nullptr,
               JSStrictPropertyOp setter = nullptr,
               unsigned attrs = JSPROP_ENUMERATE);

extern bool
DefineProperty(ExclusiveContext *cx, HandleObject obj, PropertyName *name, HandleValue value,
               JSPropertyOp getter = nullptr,
               JSStrictPropertyOp setter = nullptr,
               unsigned attrs = JSPROP_ENUMERATE);

extern bool
DefineElement(ExclusiveContext *cx, HandleObject obj, uint32_t index, HandleValue value,
              JSPropertyOp getter = nullptr,
              JSStrictPropertyOp setter = nullptr,
              unsigned attrs = JSPROP_ENUMERATE);





inline bool
HasProperty(JSContext *cx, HandleObject obj, HandleId id, bool *foundp);

inline bool
HasProperty(JSContext *cx, HandleObject obj, PropertyName *name, bool *foundp);









inline bool
GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
            MutableHandleValue vp);

inline bool
GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, PropertyName *name,
            MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return GetProperty(cx, obj, receiver, id, vp);
}

inline bool
GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
           MutableHandleValue vp);

inline bool
GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

inline bool
GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name, Value *vp)
{
    return GetPropertyNoGC(cx, obj, receiver, NameToId(name), vp);
}

inline bool
GetElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index, Value *vp);

















inline bool
SetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
            MutableHandleValue vp, bool strict);

inline bool
SetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, PropertyName *name,
            MutableHandleValue vp, bool strict)
{
    RootedId id(cx, NameToId(name));
    return SetProperty(cx, obj, receiver, id, vp, strict);
}

inline bool
SetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
           MutableHandleValue vp, bool strict);




inline bool
DeleteProperty(JSContext *cx, js::HandleObject obj, js::HandleId id, bool *succeeded);

inline bool
DeleteElement(JSContext *cx, js::HandleObject obj, uint32_t index, bool *succeeded);










extern bool
SetImmutablePrototype(js::ExclusiveContext *cx, JS::HandleObject obj, bool *succeeded);

extern bool
GetPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                      MutableHandle<PropertyDescriptor> desc);






extern bool
LookupProperty(JSContext *cx, HandleObject obj, HandleId id,
               MutableHandleObject objp, MutableHandleShape propp);

inline bool
LookupProperty(JSContext *cx, HandleObject obj, PropertyName *name,
               MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return LookupProperty(cx, obj, id, objp, propp);
}


extern bool
HasOwnProperty(JSContext *cx, HandleObject obj, HandleId id, bool *result);









extern bool
WatchProperty(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable);


extern bool
UnwatchProperty(JSContext *cx, HandleObject obj, HandleId id);





extern bool
ToPrimitive(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp);

MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, MutableHandleValue vp);

MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext *cx, JSType preferredType, MutableHandleValue vp);





extern const char *
GetObjectClassName(JSContext *cx, HandleObject obj);



























inline JSObject *
GetInnerObject(JSObject *obj)
{
    if (InnerObjectOp op = obj->getClass()->ext.innerObject) {
        JS::AutoSuppressGCAnalysis nogc;
        return op(obj);
    }
    return obj;
}








inline JSObject *
GetOuterObject(JSContext *cx, HandleObject obj)
{
    if (ObjectOp op = obj->getClass()->ext.outerObject)
        return op(cx, obj);
    return obj;
}












inline JSObject *
GetThisObject(JSContext *cx, HandleObject obj)
{
    if (ObjectOp op = obj->getOps()->thisObject)
        return op(cx, obj);
    return obj;
}




typedef JSObject *(*ClassInitializerOp)(JSContext *cx, JS::HandleObject obj);


bool
GetBuiltinConstructor(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject objp);

bool
GetBuiltinPrototype(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject objp);

JSObject *
GetBuiltinPrototypePure(GlobalObject *global, JSProtoKey protoKey);

extern bool
SetClassAndProto(JSContext *cx, HandleObject obj,
                 const Class *clasp, Handle<TaggedProto> proto);

} 




extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef JS_OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

namespace js {





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    MaybeSingletonObject,

    




    TenuredObject
};

inline gc::InitialHeap
GetInitialHeap(NewObjectKind newKind, const Class *clasp)
{
    if (newKind != GenericObject)
        return gc::TenuredHeap;
    if (clasp->finalize && !(clasp->flags & JSCLASS_FINALIZE_FROM_NURSERY))
        return gc::TenuredHeap;
    return gc::DefaultHeap;
}



extern JSObject *
CreateThisForFunctionWithProto(JSContext *cx, js::HandleObject callee, JSObject *proto,
                               NewObjectKind newKind = GenericObject);


extern JSObject *
CreateThisForFunction(JSContext *cx, js::HandleObject callee, NewObjectKind newKind);


extern JSObject *
CreateThis(JSContext *cx, const js::Class *clasp, js::HandleObject callee);

extern JSObject *
CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent);

extern NativeObject *
DeepCloneObjectLiteral(JSContext *cx, HandleNativeObject obj, NewObjectKind newKind = GenericObject);

extern bool
DefineProperties(JSContext *cx, HandleObject obj, HandleObject props);





extern bool
ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescVector *descs);


extern bool
LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
           MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp);

extern bool
LookupNameNoGC(JSContext *cx, PropertyName *name, JSObject *scopeChain,
               JSObject **objp, JSObject **pobjp, Shape **propp);








extern bool
LookupNameWithGlobalDefault(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                            MutableHandleObject objp);









extern bool
LookupNameUnqualified(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
                      MutableHandleObject objp);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);


namespace js {

bool
LookupPropertyPure(ExclusiveContext *cx, JSObject *obj, jsid id, JSObject **objp,
                   Shape **propp);

bool
GetPropertyPure(ExclusiveContext *cx, JSObject *obj, jsid id, Value *vp);

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                         MutableHandle<PropertyDescriptor> desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp);

bool
NewPropertyDescriptorObject(JSContext *cx, Handle<PropertyDescriptor> desc, MutableHandleValue vp);

extern bool
IsDelegate(JSContext *cx, HandleObject obj, const Value &v, bool *result);



extern bool
IsDelegateOfObject(JSContext *cx, HandleObject protoObj, JSObject* obj, bool *result);


extern JSObject *
PrimitiveToObject(JSContext *cx, const Value &v);

} 

namespace js {


MOZ_ALWAYS_INLINE JSObject *
ToObjectFromStack(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return js::ToObjectSlow(cx, vp, true);
}

template<XDRMode mode>
bool
XDRObjectLiteral(XDRState<mode> *xdr, MutableHandleNativeObject obj);

extern JSObject *
CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj);

} 

extern void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern bool
js_ReportGetterOnlyAssignment(JSContext *cx, bool strict);


namespace js {

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);

extern bool
GetFirstArgumentAsObject(JSContext *cx, const CallArgs &args, const char *method,
                         MutableHandleObject objp);


extern bool
Throw(JSContext *cx, jsid id, unsigned errorNumber);

extern bool
Throw(JSContext *cx, JSObject *obj, unsigned errorNumber);

enum class IntegrityLevel {
    Sealed,
    Frozen
};







extern bool
SetIntegrityLevel(JSContext *cx, HandleObject obj, IntegrityLevel level);

inline bool
FreezeObject(JSContext *cx, HandleObject obj)
{
    return SetIntegrityLevel(cx, obj, IntegrityLevel::Frozen);
}





extern bool
TestIntegrityLevel(JSContext *cx, HandleObject obj, IntegrityLevel level, bool *resultp);

}  

#endif

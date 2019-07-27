





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

class AutoPropertyDescriptorVector;
class GCMarker;
struct NativeIterator;
class Nursery;
class ObjectElements;
struct StackShape;

namespace gc {
class RelocationOverlay;
}

inline JSObject*
CastAsObject(GetterOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject*, op);
}

inline JSObject*
CastAsObject(SetterOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject*, op);
}

inline Value
CastAsObjectJsval(GetterOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(SetterOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}



extern const Class IntlClass;
extern const Class JSONClass;
extern const Class MathClass;

class GlobalObject;
class MapObject;
class NewObjectCache;
class NormalArgumentsObject;
class SetObject;
class StrictArgumentsObject;


bool PreventExtensions(JSContext* cx, JS::HandleObject obj, JS::ObjectOpResult& result);
bool SetImmutablePrototype(js::ExclusiveContext* cx, JS::HandleObject obj, bool* succeeded);

}  















class JSObject : public js::gc::Cell
{
  protected:
    js::HeapPtrObjectGroup group_;

  private:
    friend class js::Shape;
    friend class js::GCMarker;
    friend class js::NewObjectCache;
    friend class js::Nursery;
    friend class js::gc::RelocationOverlay;
    friend bool js::PreventExtensions(JSContext* cx, JS::HandleObject obj, JS::ObjectOpResult& result);
    friend bool js::SetImmutablePrototype(js::ExclusiveContext* cx, JS::HandleObject obj,
                                          bool* succeeded);

    
    static js::ObjectGroup* makeLazyGroup(JSContext* cx, js::HandleObject obj);

  public:
    bool isNative() const {
        return getClass()->isNative();
    }

    const js::Class* getClass() const {
        return group_->clasp();
    }
    const JSClass* getJSClass() const {
        return Jsvalify(getClass());
    }
    bool hasClass(const js::Class* c) const {
        return getClass() == c;
    }
    const js::ObjectOps* getOps() const {
        return &getClass()->ops;
    }

    js::ObjectGroup* group() const {
        MOZ_ASSERT(!hasLazyGroup());
        return groupRaw();
    }

    js::ObjectGroup* groupRaw() const {
        return group_;
    }

    



    bool isSingleton() const {
        return group_->singleton();
    }

    



    bool hasLazyGroup() const {
        return group_->lazy();
    }

    JSCompartment* compartment() const {
        return group_->compartment();
    }

    inline js::Shape* maybeShape() const;
    inline js::Shape* ensureShape(js::ExclusiveContext* cx);

    



    static inline JSObject* create(js::ExclusiveContext* cx,
                                   js::gc::AllocKind kind,
                                   js::gc::InitialHeap heap,
                                   js::HandleShape shape,
                                   js::HandleObjectGroup group);

    
    
    
    
    inline void setInitialShapeMaybeNonNative(js::Shape* shape);
    inline void setShapeMaybeNonNative(js::Shape* shape);

    
    
    
    
    inline void setInitialSlotsMaybeNonNative(js::HeapSlot* slots);
    inline void setInitialElementsMaybeNonNative(js::HeapSlot* elements);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlags(js::ExclusiveContext* cx, js::BaseShape::Flag flags,
                  GenerateShape generateShape = GENERATE_NONE);
    inline bool hasAllFlags(js::BaseShape::Flag flags) const;

    








    inline bool isDelegate() const;
    bool setDelegate(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::DELEGATE, GENERATE_SHAPE);
    }

    inline bool isBoundFunction() const;
    inline bool hasSpecialEquality() const;

    inline bool watched() const;
    bool setWatched(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::WATCHED, GENERATE_SHAPE);
    }

    
    inline bool isQualifiedVarObj();
    bool setQualifiedVarObj(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::QUALIFIED_VAROBJ);
    }

    inline bool isUnqualifiedVarObj();
    bool setUnqualifiedVarObj(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::UNQUALIFIED_VAROBJ);
    }

    





    inline bool hasUncacheableProto() const;
    bool setUncacheableProto(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::UNCACHEABLE_PROTO, GENERATE_SHAPE);
    }

    



    inline bool hadElementsAccess() const;
    bool setHadElementsAccess(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::HAD_ELEMENTS_ACCESS);
    }

    



    inline bool isIndexed() const;

    

    void traceChildren(JSTracer* trc);

    void fixupAfterMovingGC();

    static js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }
    static const size_t MaxTagBits = 3;
    static bool isNullLike(const JSObject* obj) { return uintptr_t(obj) < (1 << MaxTagBits); }

    MOZ_ALWAYS_INLINE JS::Zone* zone() const {
        return group_->zone();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone* shadowZone() const {
        return JS::shadow::Zone::asShadowZone(zone());
    }
    MOZ_ALWAYS_INLINE JS::Zone* zoneFromAnyThread() const {
        return group_->zoneFromAnyThread();
    }
    MOZ_ALWAYS_INLINE JS::shadow::Zone* shadowZoneFromAnyThread() const {
        return JS::shadow::Zone::asShadowZone(zoneFromAnyThread());
    }
    static MOZ_ALWAYS_INLINE void readBarrier(JSObject* obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPre(JSObject* obj);
    static MOZ_ALWAYS_INLINE void writeBarrierPost(JSObject* obj, void* cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRelocate(JSObject* obj, void* cellp);
    static MOZ_ALWAYS_INLINE void writeBarrierPostRemove(JSObject* obj, void* cellp);

    
    js::gc::AllocKind allocKindForTenure(const js::Nursery &nursery) const;

    size_t tenuredSizeOfThis() const {
        MOZ_ASSERT(isTenured());
        return js::gc::Arena::thingSize(asTenured().getAllocKind());
    }

    void addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ClassInfo* info);

    
    
    
    
    size_t sizeOfIncludingThisInNursery() const;

    



    static inline bool setSingleton(js::ExclusiveContext* cx, js::HandleObject obj);

    inline js::ObjectGroup* getGroup(JSContext* cx);

    const js::HeapPtrObjectGroup& groupFromGC() const {
        
        return group_;
    }

    












    js::TaggedProto getTaggedProto() const {
        return group_->proto();
    }

    bool hasTenuredProto() const;

    bool uninlinedIsProxy() const;

    JSObject* getProto() const {
        MOZ_ASSERT(!uninlinedIsProxy());
        return getTaggedProto().toObjectOrNull();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool hasLazyPrototype() const {
        bool lazy = getTaggedProto().isLazy();
        MOZ_ASSERT_IF(lazy, uninlinedIsProxy());
        return lazy;
    }

    
    
    inline bool nonLazyPrototypeIsImmutable() const;

    inline void setGroup(js::ObjectGroup* group);

    




    inline bool isIteratedSingleton() const;
    bool setIteratedSingleton(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::ITERATED_SINGLETON);
    }

    



    inline bool isNewGroupUnknown() const;
    static bool setNewGroupUnknown(JSContext* cx, const js::Class* clasp, JS::HandleObject obj);

    
    inline bool wasNewScriptCleared() const;
    bool setNewScriptCleared(js::ExclusiveContext* cx) {
        return setFlags(cx, js::BaseShape::NEW_SCRIPT_CLEARED);
    }

    
    bool splicePrototype(JSContext* cx, const js::Class* clasp, js::Handle<js::TaggedProto> proto);

    



    bool shouldSplicePrototype(JSContext* cx);

    
















    




    inline JSObject* enclosingScope();

    inline js::GlobalObject& global() const;
    inline bool isOwnGlobal() const;

    



  public:
    
    
    
    
    inline bool nonProxyIsExtensible() const;

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    bool isCallable() const;
    bool isConstructor() const;
    JSNative callHook() const;
    JSNative constructHook() const;

    MOZ_ALWAYS_INLINE void finalize(js::FreeOp* fop);

  public:
    static bool reportReadOnly(JSContext* cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext* cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext* cx, js::HandleId id, unsigned argc, js::Value* argv,
                    js::MutableHandleValue vp);

    static bool nonNativeSetProperty(JSContext* cx, js::HandleObject obj, js::HandleId id,
                                     js::HandleValue v, js::HandleValue receiver,
                                     JS::ObjectOpResult& result);
    static bool nonNativeSetElement(JSContext* cx, js::HandleObject obj, uint32_t index,
                                    js::HandleValue v, js::HandleValue receiver,
                                    JS::ObjectOpResult& result);

    static bool swap(JSContext* cx, JS::HandleObject a, JS::HandleObject b);

  private:
    void fixDictionaryShapeAfterSwap();

  public:
    inline void initArrayClass();

    






















    template <class T>
    inline bool is() const { return getClass() == &T::class_; }

    template <class T>
    T& as() {
        MOZ_ASSERT(this->is<T>());
        return *static_cast<T*>(this);
    }

    template <class T>
    const T& as() const {
        MOZ_ASSERT(this->is<T>());
        return *static_cast<const T*>(this);
    }

#ifdef DEBUG
    void dump();
#endif

    

    static size_t offsetOfGroup() { return offsetof(JSObject, group_); }
    static size_t offsetOfShape() { return sizeof(JSObject); }

    
    static const size_t MAX_BYTE_SIZE = 4 * sizeof(void*) + 16 * sizeof(JS::Value);

  private:
    JSObject() = delete;
    JSObject(const JSObject& other) = delete;
    void operator=(const JSObject& other) = delete;
};

template <class U>
MOZ_ALWAYS_INLINE JS::Handle<U*>
js::RootedBase<JSObject*>::as() const
{
    const JS::Rooted<JSObject*>& self = *static_cast<const JS::Rooted<JSObject*>*>(this);
    MOZ_ASSERT(self->is<U>());
    return Handle<U*>::fromMarkedLocation(reinterpret_cast<U* const*>(self.address()));
}

template <class U>
MOZ_ALWAYS_INLINE JS::Handle<U*>
js::HandleBase<JSObject*>::as() const
{
    const JS::Handle<JSObject*>& self = *static_cast<const JS::Handle<JSObject*>*>(this);
    MOZ_ASSERT(self->is<U>());
    return Handle<U*>::fromMarkedLocation(reinterpret_cast<U* const*>(self.address()));
}






static MOZ_ALWAYS_INLINE bool
operator==(const JSObject& lhs, const JSObject& rhs)
{
    return &lhs == &rhs;
}

static MOZ_ALWAYS_INLINE bool
operator!=(const JSObject& lhs, const JSObject& rhs)
{
    return &lhs != &rhs;
}


struct JSObject_Slots0 : JSObject { void* data[3]; };
struct JSObject_Slots2 : JSObject { void* data[3]; js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { void* data[3]; js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { void* data[3]; js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { void* data[3]; js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { void* data[3]; js::Value fslots[16]; };

 MOZ_ALWAYS_INLINE void
JSObject::readBarrier(JSObject* obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured().readBarrier(&obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPre(JSObject* obj)
{
    if (!isNullLike(obj) && obj->isTenured())
        obj->asTenured().writeBarrierPre(&obj->asTenured());
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPost(JSObject* obj, void* cellp)
{
    MOZ_ASSERT(cellp);
    if (IsNullTaggedPointer(obj))
        return;
    MOZ_ASSERT(obj == *static_cast<JSObject**>(cellp));
    js::gc::StoreBuffer* storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putCellFromAnyThread(static_cast<js::gc::Cell**>(cellp));
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRelocate(JSObject* obj, void* cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject**>(cellp));
    js::gc::StoreBuffer* storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(static_cast<js::gc::Cell**>(cellp));
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRemove(JSObject* obj, void* cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject**>(cellp));
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->removeRelocatableCellFromAnyThread(
        static_cast<js::gc::Cell**>(cellp));
}

namespace js {

inline bool
IsCallable(const Value& v)
{
    return v.isObject() && v.toObject().isCallable();
}


inline bool
IsConstructor(const Value& v)
{
    return v.isObject() && v.toObject().isConstructor();
}

} 

class JSValueArray {
  public:
    const jsval* array;
    size_t length;

    JSValueArray(const jsval* v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value* array;
    size_t length;

    ValueArray(js::Value* v, size_t c) : array(v), length(c) {}
};

namespace js {























inline bool
GetPrototype(JSContext* cx, HandleObject obj, MutableHandleObject protop);








extern bool
SetPrototype(JSContext* cx, HandleObject obj, HandleObject proto,
             ObjectOpResult& result);


extern bool
SetPrototype(JSContext* cx, HandleObject obj, HandleObject proto);






inline bool
IsExtensible(ExclusiveContext* cx, HandleObject obj, bool* extensible);






extern bool
PreventExtensions(JSContext* cx, HandleObject obj, ObjectOpResult& result);


extern bool
PreventExtensions(JSContext* cx, HandleObject obj);








extern bool
GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                         MutableHandle<PropertyDescriptor> desc);












extern bool
StandardDefineProperty(JSContext* cx, HandleObject obj, HandleId id,
                       Handle<PropertyDescriptor> descriptor, ObjectOpResult& result);





extern bool
StandardDefineProperty(JSContext* cx, HandleObject obj, HandleId id,
                       Handle<PropertyDescriptor> desc);

extern bool
DefineProperty(JSContext* cx, HandleObject obj, HandleId id,
               Handle<PropertyDescriptor> desc, ObjectOpResult& result);

extern bool
DefineProperty(ExclusiveContext* cx, HandleObject obj, HandleId id, HandleValue value,
               JSGetterOp getter, JSSetterOp setter, unsigned attrs, ObjectOpResult& result);

extern bool
DefineProperty(ExclusiveContext* cx, HandleObject obj, PropertyName* name, HandleValue value,
               JSGetterOp getter, JSSetterOp setter, unsigned attrs, ObjectOpResult& result);

extern bool
DefineElement(ExclusiveContext* cx, HandleObject obj, uint32_t index, HandleValue value,
              JSGetterOp getter, JSSetterOp setter, unsigned attrs, ObjectOpResult& result);





extern bool
DefineProperty(ExclusiveContext* cx, HandleObject obj, HandleId id, HandleValue value,
               JSGetterOp getter = nullptr,
               JSSetterOp setter = nullptr,
               unsigned attrs = JSPROP_ENUMERATE);

extern bool
DefineProperty(ExclusiveContext* cx, HandleObject obj, PropertyName* name, HandleValue value,
               JSGetterOp getter = nullptr,
               JSSetterOp setter = nullptr,
               unsigned attrs = JSPROP_ENUMERATE);

extern bool
DefineElement(ExclusiveContext* cx, HandleObject obj, uint32_t index, HandleValue value,
              JSGetterOp getter = nullptr,
              JSSetterOp setter = nullptr,
              unsigned attrs = JSPROP_ENUMERATE);





inline bool
HasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp);

inline bool
HasProperty(JSContext* cx, HandleObject obj, PropertyName* name, bool* foundp);









inline bool
GetProperty(JSContext* cx, HandleObject obj, HandleObject receiver, HandleId id,
            MutableHandleValue vp);

inline bool
GetProperty(JSContext* cx, HandleObject obj, HandleObject receiver, PropertyName* name,
            MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return GetProperty(cx, obj, receiver, id, vp);
}

inline bool
GetElement(JSContext* cx, HandleObject obj, HandleObject receiver, uint32_t index,
           MutableHandleValue vp);

inline bool
GetPropertyNoGC(JSContext* cx, JSObject* obj, JSObject* receiver, jsid id, Value* vp);

inline bool
GetPropertyNoGC(JSContext* cx, JSObject* obj, JSObject* receiver, PropertyName* name, Value* vp)
{
    return GetPropertyNoGC(cx, obj, receiver, NameToId(name), vp);
}

inline bool
GetElementNoGC(JSContext* cx, JSObject* obj, JSObject* receiver, uint32_t index, Value* vp);














inline bool
SetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
            HandleValue receiver, ObjectOpResult& result);

inline bool
SetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v)
{
    RootedValue receiver(cx, ObjectValue(*obj));
    ObjectOpResult result;
    return SetProperty(cx, obj, id, v, receiver, result) &&
           result.checkStrict(cx, obj, id);
}

inline bool
SetProperty(JSContext* cx, HandleObject obj, PropertyName* name, HandleValue v,
            HandleValue receiver, ObjectOpResult& result)
{
    RootedId id(cx, NameToId(name));
    return SetProperty(cx, obj, id, v, receiver, result);
}

inline bool
SetProperty(JSContext* cx, HandleObject obj, PropertyName* name, HandleValue v)
{
    RootedId id(cx, NameToId(name));
    RootedValue receiver(cx, ObjectValue(*obj));
    ObjectOpResult result;
    return SetProperty(cx, obj, id, v, receiver, result) &&
           result.checkStrict(cx, obj, id);
}

inline bool
SetElement(JSContext* cx, HandleObject obj, uint32_t index, HandleValue v,
           HandleValue receiver, ObjectOpResult& result);






inline bool
PutProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v, bool strict)
{
    RootedValue receiver(cx, ObjectValue(*obj));
    ObjectOpResult result;
    return SetProperty(cx, obj, id, v, receiver, result) &&
           result.checkStrictErrorOrWarning(cx, obj, id, strict);
}




inline bool
DeleteProperty(JSContext* cx, HandleObject obj, HandleId id, ObjectOpResult& result);

inline bool
DeleteElement(JSContext* cx, HandleObject obj, uint32_t index, ObjectOpResult& result);










extern bool
SetImmutablePrototype(js::ExclusiveContext* cx, JS::HandleObject obj, bool* succeeded);

extern bool
GetPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                      MutableHandle<PropertyDescriptor> desc);






extern bool
LookupProperty(JSContext* cx, HandleObject obj, HandleId id,
               MutableHandleObject objp, MutableHandleShape propp);

inline bool
LookupProperty(JSContext* cx, HandleObject obj, PropertyName* name,
               MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return LookupProperty(cx, obj, id, objp, propp);
}


extern bool
HasOwnProperty(JSContext* cx, HandleObject obj, HandleId id, bool* result);









extern bool
WatchProperty(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable);


extern bool
UnwatchProperty(JSContext* cx, HandleObject obj, HandleId id);





extern bool
ToPrimitive(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp);

MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext* cx, MutableHandleValue vp);

MOZ_ALWAYS_INLINE bool
ToPrimitive(JSContext* cx, JSType preferredType, MutableHandleValue vp);





extern const char*
GetObjectClassName(JSContext* cx, HandleObject obj);



























inline JSObject*
GetInnerObject(JSObject* obj)
{
    if (InnerObjectOp op = obj->getClass()->ext.innerObject) {
        JS::AutoSuppressGCAnalysis nogc;
        return op(obj);
    }
    return obj;
}








inline JSObject*
GetOuterObject(JSContext* cx, HandleObject obj)
{
    if (ObjectOp op = obj->getClass()->ext.outerObject)
        return op(cx, obj);
    return obj;
}












inline JSObject*
GetThisObject(JSContext* cx, HandleObject obj)
{
    if (ObjectOp op = obj->getOps()->thisObject)
        return op(cx, obj);
    return obj;
}




typedef JSObject* (*ClassInitializerOp)(JSContext* cx, JS::HandleObject obj);


bool
GetBuiltinConstructor(ExclusiveContext* cx, JSProtoKey key, MutableHandleObject objp);

bool
GetBuiltinPrototype(ExclusiveContext* cx, JSProtoKey key, MutableHandleObject objp);

JSObject*
GetBuiltinPrototypePure(GlobalObject* global, JSProtoKey protoKey);

extern bool
SetClassAndProto(JSContext* cx, HandleObject obj,
                 const Class* clasp, Handle<TaggedProto> proto);

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

inline gc::InitialHeap
GetInitialHeap(NewObjectKind newKind, const Class* clasp)
{
    if (newKind != GenericObject)
        return gc::TenuredHeap;
    if (clasp->finalize && !(clasp->flags & JSCLASS_FINALIZE_FROM_NURSERY))
        return gc::TenuredHeap;
    return gc::DefaultHeap;
}



extern JSObject*
CreateThisForFunctionWithProto(JSContext* cx, js::HandleObject callee, HandleObject proto,
                               NewObjectKind newKind = GenericObject);


extern JSObject*
CreateThisForFunction(JSContext* cx, js::HandleObject callee, NewObjectKind newKind);


extern JSObject*
CreateThis(JSContext* cx, const js::Class* clasp, js::HandleObject callee);

extern JSObject*
CloneObject(JSContext* cx, HandleObject obj, Handle<js::TaggedProto> proto);

extern JSObject*
DeepCloneObjectLiteral(JSContext* cx, HandleObject obj, NewObjectKind newKind = GenericObject);

extern bool
DefineProperties(JSContext* cx, HandleObject obj, HandleObject props);

inline JSGetterOp
CastAsGetterOp(JSObject* object)
{
    return JS_DATA_TO_FUNC_PTR(JSGetterOp, object);
}

inline JSSetterOp
CastAsSetterOp(JSObject* object)
{
    return JS_DATA_TO_FUNC_PTR(JSSetterOp, object);
}


bool
ToPropertyDescriptor(JSContext* cx, HandleValue descval, bool checkAccessors,
                     MutableHandle<PropertyDescriptor> desc);






bool
CheckPropertyDescriptorAccessors(JSContext* cx, Handle<PropertyDescriptor> desc);

void
CompletePropertyDescriptor(MutableHandle<PropertyDescriptor> desc);





extern bool
ReadPropertyDescriptors(JSContext* cx, HandleObject props, bool checkAccessors,
                        AutoIdVector* ids, AutoPropertyDescriptorVector* descs);


extern bool
LookupName(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
           MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp);

extern bool
LookupNameNoGC(JSContext* cx, PropertyName* name, JSObject* scopeChain,
               JSObject** objp, JSObject** pobjp, Shape** propp);








extern bool
LookupNameWithGlobalDefault(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
                            MutableHandleObject objp);









extern bool
LookupNameUnqualified(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
                      MutableHandleObject objp);

}

namespace js {

extern JSObject*
FindVariableScope(JSContext* cx, JSFunction** funp);

bool
LookupPropertyPure(ExclusiveContext* cx, JSObject* obj, jsid id, JSObject** objp,
                   Shape** propp);

bool
GetPropertyPure(ExclusiveContext* cx, JSObject* obj, jsid id, Value* vp);

bool
GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                         MutableHandle<PropertyDescriptor> desc);

bool
GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp);


bool
FromPropertyDescriptor(JSContext* cx, Handle<PropertyDescriptor> desc, MutableHandleValue vp);

extern bool
IsDelegate(JSContext* cx, HandleObject obj, const Value& v, bool* result);



extern bool
IsDelegateOfObject(JSContext* cx, HandleObject protoObj, JSObject* obj, bool* result);


extern JSObject*
PrimitiveToObject(JSContext* cx, const Value& v);

} 

namespace js {


MOZ_ALWAYS_INLINE JSObject*
ToObjectFromStack(JSContext* cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return js::ToObjectSlow(cx, vp, true);
}

template<XDRMode mode>
bool
XDRObjectLiteral(XDRState<mode>* xdr, MutableHandleObject obj);

extern JSObject*
CloneObjectLiteral(JSContext* cx, HandleObject srcObj);

extern bool
ReportGetterOnlyAssignment(JSContext* cx, bool strict);

extern JSObject*
NonNullObject(JSContext* cx, const Value& v);

extern const char*
InformalValueTypeName(const Value& v);

extern bool
GetFirstArgumentAsObject(JSContext* cx, const CallArgs& args, const char* method,
                         MutableHandleObject objp);


extern bool
Throw(JSContext* cx, jsid id, unsigned errorNumber);

extern bool
Throw(JSContext* cx, JSObject* obj, unsigned errorNumber);

enum class IntegrityLevel {
    Sealed,
    Frozen
};







extern bool
SetIntegrityLevel(JSContext* cx, HandleObject obj, IntegrityLevel level);

inline bool
FreezeObject(JSContext* cx, HandleObject obj)
{
    return SetIntegrityLevel(cx, obj, IntegrityLevel::Frozen);
}





extern bool
TestIntegrityLevel(JSContext* cx, HandleObject obj, IntegrityLevel level, bool* resultp);

}  

#endif

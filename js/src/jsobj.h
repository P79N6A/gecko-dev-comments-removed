





#ifndef jsobj_h
#define jsobj_h










#include "mozilla/MemoryReporting.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
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

namespace gc {
class ForkJoinNursery;
}

}  













class JSObject : public js::gc::Cell
{
  protected:
    



    js::HeapPtrShape shape_;

    




    js::HeapPtrTypeObject type_;

  private:
    friend class js::Shape;
    friend class js::GCMarker;
    friend class js::NewObjectCache;
    friend class js::Nursery;
    friend class js::gc::ForkJoinNursery;

    
    static js::types::TypeObject *makeLazyType(JSContext *cx, js::HandleObject obj);

  public:
    static const js::Class class_;

    js::Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    bool isNative() const {
        return lastProperty()->isNative();
    }

    const js::Class *getClass() const {
        return type_->clasp();
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

    js::types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return typeRaw();
    }

    js::types::TypeObject *typeRaw() const {
        return type_;
    }

    



    bool hasSingletonType() const {
        return !!type_->singleton();
    }

    



    bool hasLazyType() const {
        return type_->lazy();
    }

    JSCompartment *compartment() const {
        return lastProperty()->base()->compartment();
    }

    



    static inline JSObject *create(js::ExclusiveContext *cx,
                                   js::gc::AllocKind kind,
                                   js::gc::InitialHeap heap,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type);

    
    
    
    
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

    



    static inline bool setSingletonType(js::ExclusiveContext *cx, js::HandleObject obj);

    
    inline js::types::TypeObject* getType(JSContext *cx);
    js::types::TypeObject* uninlinedGetType(JSContext *cx);

    const js::HeapPtrTypeObject &typeFromGC() const {
        
        return type_;
    }

    












    js::TaggedProto getTaggedProto() const {
        return type_->proto();
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

    
    
    
    
    static bool
    setImmutablePrototype(js::ExclusiveContext *cx, JS::HandleObject obj, bool *succeeded);

    static inline bool getProto(JSContext *cx, js::HandleObject obj,
                                js::MutableHandleObject protop);
    
    static inline bool setProto(JSContext *cx, JS::HandleObject obj,
                                JS::HandleObject proto, bool *succeeded);

    
    inline void setType(js::types::TypeObject *newType);
    void uninlinedSetType(js::types::TypeObject *newType);

#ifdef DEBUG
    bool hasNewType(const js::Class *clasp, js::types::TypeObject *newType);
#endif

    




    bool isIteratedSingleton() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::ITERATED_SINGLETON);
    }
    bool setIteratedSingleton(js::ExclusiveContext *cx) {
        return setFlag(cx, js::BaseShape::ITERATED_SINGLETON);
    }

    



    bool isNewTypeUnknown() const {
        return lastProperty()->hasObjectFlag(js::BaseShape::NEW_TYPE_UNKNOWN);
    }
    static bool setNewTypeUnknown(JSContext *cx, const js::Class *clasp, JS::HandleObject obj);

    
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

    
    static inline bool clearType(JSContext *cx, js::HandleObject obj);
    static bool clearParent(JSContext *cx, js::HandleObject obj);

    



  public:
    static inline bool
    isExtensible(js::ExclusiveContext *cx, js::HandleObject obj, bool *extensible);

    
    
    
    
    bool nonProxyIsExtensible() const {
        MOZ_ASSERT(!uninlinedIsProxy());

        
        return !lastProperty()->hasObjectFlag(js::BaseShape::NOT_EXTENSIBLE);
    }

    
    
    
    static bool
    preventExtensions(JSContext *cx, js::HandleObject obj, bool *succeeded);

  private:
    enum ImmutabilityType { SEAL, FREEZE };

    





    static bool sealOrFreeze(JSContext *cx, js::HandleObject obj, ImmutabilityType it);

    static bool isSealedOrFrozen(JSContext *cx, js::HandleObject obj, ImmutabilityType it, bool *resultp);

    static inline unsigned getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it);

  public:
    
    static inline bool seal(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, SEAL); }
    
    static inline bool freeze(JSContext *cx, js::HandleObject obj) { return sealOrFreeze(cx, obj, FREEZE); }

    static inline bool isSealed(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, SEAL, resultp);
    }
    static inline bool isFrozen(JSContext *cx, js::HandleObject obj, bool *resultp) {
        return isSealedOrFrozen(cx, obj, FREEZE, resultp);
    }

    
    static const char *className(JSContext *cx, js::HandleObject obj);

  public:
    



    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    


    bool isCallable() const;
    bool isConstructor() const;
    JSNative callHook() const;
    JSNative constructHook() const;

    MOZ_ALWAYS_INLINE void finalize(js::FreeOp *fop);

    static inline bool hasProperty(JSContext *cx, js::HandleObject obj,
                                   js::HandleId id, bool *foundp);

  public:
    static bool reportReadOnly(js::ThreadSafeContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotConfigurable(js::ThreadSafeContext *cx, jsid id, unsigned report = JSREPORT_ERROR);
    bool reportNotExtensible(js::ThreadSafeContext *cx, unsigned report = JSREPORT_ERROR);

    





    bool callMethod(JSContext *cx, js::HandleId id, unsigned argc, js::Value *argv,
                    js::MutableHandleValue vp);

    static bool lookupGeneric(JSContext *cx, js::HandleObject obj, js::HandleId id,
                              js::MutableHandleObject objp, js::MutableHandleShape propp);

    static bool lookupProperty(JSContext *cx, js::HandleObject obj, js::PropertyName *name,
                               js::MutableHandleObject objp, js::MutableHandleShape propp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return lookupGeneric(cx, obj, id, objp, propp);
    }

    static inline bool lookupElement(JSContext *cx, js::HandleObject obj, uint32_t index,
                                     js::MutableHandleObject objp, js::MutableHandleShape propp);

    static bool defineGeneric(js::ExclusiveContext *cx, js::HandleObject obj,
                              js::HandleId id, js::HandleValue value,
                              JSPropertyOp getter = JS_PropertyStub,
                              JSStrictPropertyOp setter = JS_StrictPropertyStub,
                              unsigned attrs = JSPROP_ENUMERATE);

    static bool defineProperty(js::ExclusiveContext *cx, js::HandleObject obj,
                               js::PropertyName *name, js::HandleValue value,
                               JSPropertyOp getter = JS_PropertyStub,
                               JSStrictPropertyOp setter = JS_StrictPropertyStub,
                               unsigned attrs = JSPROP_ENUMERATE);

    static bool defineElement(js::ExclusiveContext *cx, js::HandleObject obj,
                              uint32_t index, js::HandleValue value,
                              JSPropertyOp getter = JS_PropertyStub,
                              JSStrictPropertyOp setter = JS_StrictPropertyStub,
                              unsigned attrs = JSPROP_ENUMERATE);

    static inline bool getGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                  js::HandleId id, js::MutableHandleValue vp);

    static inline bool getGenericNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                      jsid id, js::Value *vp);

    static bool getProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                            js::PropertyName *name, js::MutableHandleValue vp)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return getGeneric(cx, obj, receiver, id, vp);
    }

    static bool getPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                js::PropertyName *name, js::Value *vp)
    {
        return getGenericNoGC(cx, obj, receiver, js::NameToId(name), vp);
    }

    static inline bool getElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                  uint32_t index, js::MutableHandleValue vp);
    static inline bool getElementNoGC(JSContext *cx, JSObject *obj, JSObject *receiver,
                                      uint32_t index, js::Value *vp);

    static inline bool setGeneric(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                  js::HandleId id, js::MutableHandleValue vp, bool strict);

    static bool setProperty(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                            js::PropertyName *name,
                            js::MutableHandleValue vp, bool strict)
    {
        JS::RootedId id(cx, js::NameToId(name));
        return setGeneric(cx, obj, receiver, id, vp, strict);
    }

    static inline bool setElement(JSContext *cx, js::HandleObject obj, js::HandleObject receiver,
                                  uint32_t index, js::MutableHandleValue vp, bool strict);

    static bool nonNativeSetProperty(JSContext *cx, js::HandleObject obj,
                                     js::HandleId id, js::MutableHandleValue vp, bool strict);
    static bool nonNativeSetElement(JSContext *cx, js::HandleObject obj,
                                    uint32_t index, js::MutableHandleValue vp, bool strict);

    static inline bool getGenericAttributes(JSContext *cx, js::HandleObject obj,
                                            js::HandleId id, unsigned *attrsp);

    static inline bool setGenericAttributes(JSContext *cx, js::HandleObject obj,
                                            js::HandleId id, unsigned *attrsp);

    static inline bool deleteGeneric(JSContext *cx, js::HandleObject obj, js::HandleId id,
                                     bool *succeeded);
    static inline bool deleteElement(JSContext *cx, js::HandleObject obj, uint32_t index,
                                     bool *succeeded);

    static inline bool watch(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                             JS::HandleObject callable);
    static inline bool unwatch(JSContext *cx, JS::HandleObject obj, JS::HandleId id);

    static bool enumerate(JSContext *cx, JS::HandleObject obj, JSIterateOp iterop,
                          JS::MutableHandleValue statep, JS::MutableHandleId idp)
    {
        JSNewEnumerateOp op = obj->getOps()->enumerate;
        return (op ? op : JS_EnumerateState)(cx, obj, iterop, statep, idp);
    }

    static bool defaultValue(JSContext *cx, js::HandleObject obj, JSType hint,
                             js::MutableHandleValue vp)
    {
        JSConvertOp op = obj->getClass()->convert;
        bool ok;
        if (op == JS_ConvertStub)
            ok = js::DefaultValue(cx, obj, hint, vp);
        else
            ok = op(cx, obj, hint, vp);
        MOZ_ASSERT_IF(ok, vp.isPrimitive());
        return ok;
    }

    static JSObject *thisObject(JSContext *cx, js::HandleObject obj)
    {
        if (js::ObjectOp op = obj->getOps()->thisObject)
            return op(cx, obj);
        return obj;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

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
    js::HeapPtrShape *addressOfShape() { return &shape_; }

    static size_t offsetOfType() { return offsetof(JSObject, type_); }
    js::HeapPtrTypeObject *addressOfType() { return &type_; }

  private:
    JSObject() MOZ_DELETE;
    JSObject(const JSObject &other) MOZ_DELETE;
    void operator=(const JSObject &other) MOZ_DELETE;
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
#ifdef JSGC_GENERATIONAL
    if (IsNullTaggedPointer(obj))
        return;
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
    js::gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putCellFromAnyThread(static_cast<js::gc::Cell **>(cellp));
#endif
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRelocate(JSObject *obj, void *cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
#ifdef JSGC_GENERATIONAL
    js::gc::StoreBuffer *storeBuffer = obj->storeBuffer();
    if (storeBuffer)
        storeBuffer->putRelocatableCellFromAnyThread(static_cast<js::gc::Cell **>(cellp));
#endif
}

 MOZ_ALWAYS_INLINE void
JSObject::writeBarrierPostRemove(JSObject *obj, void *cellp)
{
    MOZ_ASSERT(cellp);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(obj == *static_cast<JSObject **>(cellp));
#ifdef JSGC_GENERATIONAL
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->removeRelocatableCellFromAnyThread(
        static_cast<js::gc::Cell **>(cellp));
#endif
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

inline JSObject *
GetInnerObject(JSObject *obj)
{
    if (js::InnerObjectOp op = obj->getClass()->ext.innerObject) {
        JS::AutoSuppressGCAnalysis nogc;
        return op(obj);
    }
    return obj;
}

inline JSObject *
GetOuterObject(JSContext *cx, js::HandleObject obj)
{
    if (js::ObjectOp op = obj->getClass()->ext.outerObject)
        return op(cx, obj);
    return obj;
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


bool
HasOwnProperty(JSContext *cx, HandleObject obj, HandleId id, bool *resultp);

template <AllowGC allowGC>
extern bool
HasOwnProperty(JSContext *cx, LookupGenericOp lookup,
               typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
               typename MaybeRooted<jsid, allowGC>::HandleType id,
               typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
               typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp);

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







bool
FindClassObject(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp);

extern bool
FindClassPrototype(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp);

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

extern bool
js_PopulateObject(JSContext *cx, js::HandleObject newborn, js::HandleObject props);


namespace js {

extern bool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::HandleValue descriptor, bool *bp);

extern bool
DefineOwnProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::Handle<js::PropertyDescriptor> descriptor, bool *bp);





enum NewObjectKind {
    
    GenericObject,

    




    SingletonObject,

    




    MaybeSingletonObject,

    




    TenuredObject
};

inline gc::InitialHeap
GetInitialHeap(NewObjectKind newKind, const Class *clasp)
{
    if (clasp->finalize || newKind != GenericObject)
        return gc::TenuredHeap;
    return gc::DefaultHeap;
}



extern NativeObject *
CreateThisForFunctionWithProto(JSContext *cx, js::HandleObject callee, JSObject *proto,
                               NewObjectKind newKind = GenericObject);


extern NativeObject *
CreateThisForFunction(JSContext *cx, js::HandleObject callee, NewObjectKind newKind);


extern JSObject *
CreateThis(JSContext *cx, const js::Class *clasp, js::HandleObject callee);

extern JSObject *
CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent);

extern NativeObject *
DeepCloneObjectLiteral(JSContext *cx, HandleNativeObject obj, NewObjectKind newKind = GenericObject);









extern bool
DefineProperty(JSContext *cx, js::HandleObject obj,
               js::HandleId id, const PropDesc &desc, bool throwError,
               bool *rval);

bool
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
LookupPropertyPure(JSObject *obj, jsid id, NativeObject **objp, Shape **propp);

bool
GetPropertyPure(ThreadSafeContext *cx, JSObject *obj, jsid id, Value *vp);

inline bool
GetPropertyPure(ThreadSafeContext *cx, JSObject *obj, PropertyName *name, Value *vp)
{
    return GetPropertyPure(cx, obj, NameToId(name), vp);
}

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

bool
GetObjectElementOperationPure(ThreadSafeContext *cx, JSObject *obj, const Value &prop, Value *vp);


extern JSObject *
PrimitiveToObject(JSContext *cx, const Value &v);

} 

namespace js {






extern JSObject *
ToObjectSlow(JSContext *cx, HandleValue vp, bool reportScanStack);


MOZ_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, false);
}


MOZ_ALWAYS_INLINE JSObject *
ToObjectFromStack(JSContext *cx, HandleValue vp)
{
    if (vp.isObject())
        return &vp.toObject();
    return ToObjectSlow(cx, vp, true);
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

namespace baseops {

extern bool
Watch(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable);

extern bool
Unwatch(JSContext *cx, JS::HandleObject obj, JS::HandleId id);

} 

}  

#endif 

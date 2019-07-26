





#ifndef vm_PropDesc_h
#define vm_PropDesc_h

#include "jsapi.h"
#include "NamespaceImports.h"

namespace js {

class Debugger;

static inline JSPropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSPropertyOp, object);
}

static inline JSStrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, object);
}





struct PropDesc {
  private:
    



    Value pd_;

    Value value_, get_, set_;

    
    uint8_t attrs;

    
    bool hasGet_ : 1;
    bool hasSet_ : 1;
    bool hasValue_ : 1;
    bool hasWritable_ : 1;
    bool hasEnumerable_ : 1;
    bool hasConfigurable_ : 1;

    
    bool isUndefined_ : 1;

    explicit PropDesc(const Value &v)
      : pd_(UndefinedValue()),
        value_(v),
        get_(UndefinedValue()), set_(UndefinedValue()),
        attrs(0),
        hasGet_(false), hasSet_(false),
        hasValue_(true), hasWritable_(false), hasEnumerable_(false), hasConfigurable_(false),
        isUndefined_(false)
    {
    }

  public:
    friend class AutoPropDescRooter;
    friend void JS::AutoGCRooter::trace(JSTracer *trc);
    friend struct GCMethods<PropDesc>;

    void trace(JSTracer *trc);

    enum Enumerability { Enumerable = true, NonEnumerable = false };
    enum Configurability { Configurable = true, NonConfigurable = false };
    enum Writability { Writable = true, NonWritable = false };

    PropDesc();

    static PropDesc undefined() { return PropDesc(); }
    static PropDesc valueOnly(const Value &v) { return PropDesc(v); }

    PropDesc(const Value &v, Writability writable,
             Enumerability enumerable, Configurability configurable)
      : pd_(UndefinedValue()),
        value_(v),
        get_(UndefinedValue()), set_(UndefinedValue()),
        attrs((writable ? 0 : JSPROP_READONLY) |
              (enumerable ? JSPROP_ENUMERATE : 0) |
              (configurable ? 0 : JSPROP_PERMANENT)),
        hasGet_(false), hasSet_(false),
        hasValue_(true), hasWritable_(true), hasEnumerable_(true), hasConfigurable_(true),
        isUndefined_(false)
    {}

    inline PropDesc(const Value &getter, const Value &setter,
                    Enumerability enumerable, Configurability configurable);

    









    bool initialize(JSContext *cx, const Value &v, bool checkAccessors = true);

    






    void complete();

    








    void initFromPropertyDescriptor(Handle<JSPropertyDescriptor> desc);
    bool makeObject(JSContext *cx);

    void setUndefined() { isUndefined_ = true; }

    bool isUndefined() const { return isUndefined_; }

    bool hasGet() const { MOZ_ASSERT(!isUndefined()); return hasGet_; }
    bool hasSet() const { MOZ_ASSERT(!isUndefined()); return hasSet_; }
    bool hasValue() const { MOZ_ASSERT(!isUndefined()); return hasValue_; }
    bool hasWritable() const { MOZ_ASSERT(!isUndefined()); return hasWritable_; }
    bool hasEnumerable() const { MOZ_ASSERT(!isUndefined()); return hasEnumerable_; }
    bool hasConfigurable() const { MOZ_ASSERT(!isUndefined()); return hasConfigurable_; }

    Value pd() const { MOZ_ASSERT(!isUndefined()); return pd_; }
    void clearPd() { pd_ = UndefinedValue(); }

    uint8_t attributes() const { MOZ_ASSERT(!isUndefined()); return attrs; }

    
    bool isAccessorDescriptor() const {
        return !isUndefined() && (hasGet() || hasSet());
    }

    
    bool isDataDescriptor() const {
        return !isUndefined() && (hasValue() || hasWritable());
    }

    
    bool isGenericDescriptor() const {
        return !isUndefined() && !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasConfigurable());
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasEnumerable());
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasWritable());
        return (attrs & JSPROP_READONLY) == 0;
    }

    HandleValue value() const {
        MOZ_ASSERT(hasValue());
        return HandleValue::fromMarkedLocation(&value_);
    }

    JSObject * getterObject() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasGet());
        return get_.isUndefined() ? nullptr : &get_.toObject();
    }
    JSObject * setterObject() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasSet());
        return set_.isUndefined() ? nullptr : &set_.toObject();
    }

    HandleValue getterValue() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasGet());
        return HandleValue::fromMarkedLocation(&get_);
    }
    HandleValue setterValue() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasSet());
        return HandleValue::fromMarkedLocation(&set_);
    }

    



    JSPropertyOp getter() const {
        return CastAsPropertyOp(get_.isUndefined() ? nullptr : &get_.toObject());
    }
    JSStrictPropertyOp setter() const {
        return CastAsStrictPropertyOp(set_.isUndefined() ? nullptr : &set_.toObject());
    }

    




    bool checkGetter(JSContext *cx);
    bool checkSetter(JSContext *cx);

    bool unwrapDebuggerObjectsInto(JSContext *cx, Debugger *dbg, HandleObject obj,
                                   PropDesc *unwrapped) const;

    bool wrapInto(JSContext *cx, HandleObject obj, const jsid &id, jsid *wrappedId,
                  PropDesc *wrappedDesc) const;
};

class AutoPropDescRooter : private JS::CustomAutoRooter
{
  public:
    explicit AutoPropDescRooter(JSContext *cx
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : CustomAutoRooter(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    PropDesc& getPropDesc() { return propDesc; }

    void initFromPropertyDescriptor(Handle<JSPropertyDescriptor> desc) {
        propDesc.initFromPropertyDescriptor(desc);
    }

    bool makeObject(JSContext *cx) {
        return propDesc.makeObject(cx);
    }

    void setUndefined() { propDesc.setUndefined(); }
    bool isUndefined() const { return propDesc.isUndefined(); }

    bool hasGet() const { return propDesc.hasGet(); }
    bool hasSet() const { return propDesc.hasSet(); }
    bool hasValue() const { return propDesc.hasValue(); }
    bool hasWritable() const { return propDesc.hasWritable(); }
    bool hasEnumerable() const { return propDesc.hasEnumerable(); }
    bool hasConfigurable() const { return propDesc.hasConfigurable(); }

    Value pd() const { return propDesc.pd(); }
    void clearPd() { propDesc.clearPd(); }

    uint8_t attributes() const { return propDesc.attributes(); }

    bool isAccessorDescriptor() const { return propDesc.isAccessorDescriptor(); }
    bool isDataDescriptor() const { return propDesc.isDataDescriptor(); }
    bool isGenericDescriptor() const { return propDesc.isGenericDescriptor(); }
    bool configurable() const { return propDesc.configurable(); }
    bool enumerable() const { return propDesc.enumerable(); }
    bool writable() const { return propDesc.writable(); }

    HandleValue value() const { return propDesc.value(); }
    JSObject *getterObject() const { return propDesc.getterObject(); }
    JSObject *setterObject() const { return propDesc.setterObject(); }
    HandleValue getterValue() const { return propDesc.getterValue(); }
    HandleValue setterValue() const { return propDesc.setterValue(); }

    JSPropertyOp getter() const { return propDesc.getter(); }
    JSStrictPropertyOp setter() const { return propDesc.setter(); }

  private:
    virtual void trace(JSTracer *trc);

    PropDesc propDesc;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

namespace JS {

template <typename Outer>
class PropDescOperations
{
    const js::PropDesc * desc() const { return static_cast<const Outer*>(this)->extract(); }

  public:
    bool isUndefined() const { return desc()->isUndefined(); }

    bool hasGet() const { return desc()->hasGet(); }
    bool hasSet() const { return desc()->hasSet(); }
    bool hasValue() const { return desc()->hasValue(); }
    bool hasWritable() const { return desc()->hasWritable(); }
    bool hasEnumerable() const { return desc()->hasEnumerable(); }
    bool hasConfigurable() const { return desc()->hasConfigurable(); }

    Value pd() const { return desc()->pd(); }

    uint8_t attributes() const { return desc()->attributes(); }

    bool isAccessorDescriptor() const { return desc()->isAccessorDescriptor(); }
    bool isDataDescriptor() const { return desc()->isDataDescriptor(); }
    bool isGenericDescriptor() const { return desc()->isGenericDescriptor(); }
    bool configurable() const { return desc()->configurable(); }
    bool enumerable() const { return desc()->enumerable(); }
    bool writable() const { return desc()->writable(); }

    HandleValue value() const { return desc()->value(); }
    JSObject *getterObject() const { return desc()->getterObject(); }
    JSObject *setterObject() const { return desc()->setterObject(); }
    HandleValue getterValue() const { return desc()->getterValue(); }
    HandleValue setterValue() const { return desc()->setterValue(); }

    JSPropertyOp getter() const { return desc()->getter(); }
    JSStrictPropertyOp setter() const { return desc()->setter(); }

    
    
    
};

template <typename Outer>
class MutablePropDescOperations : public PropDescOperations<Outer>
{
    js::PropDesc * desc() { return static_cast<Outer*>(this)->extractMutable(); }

  public:

    bool initialize(JSContext *cx, const Value &v, bool checkAccessors = true) {
        return desc()->initialize(cx, v, checkAccessors);
    }
    void complete() {
        desc()->complete();
    }

    bool checkGetter(JSContext *cx) { return desc()->checkGetter(cx); }
    bool checkSetter(JSContext *cx) { return desc()->checkSetter(cx); }

    void initFromPropertyDescriptor(Handle<JSPropertyDescriptor> descriptor) {
        desc()->initFromPropertyDescriptor(descriptor);
    }
    bool makeObject(JSContext *cx) {
        return desc()->makeObject(cx);
    }

    void setUndefined() { desc()->setUndefined(); }
    void clearPd() { desc()->clearPd(); }
};

} 

namespace js {

template <>
struct GCMethods<PropDesc> {
    static PropDesc initial() { return PropDesc(); }
    static ThingRootKind kind() { return THING_ROOT_PROP_DESC; }
    static bool poisoned(const PropDesc &desc) {
        return (desc.pd_.isGCThing() &&
                JS::IsPoisonedPtr(desc.pd_.toGCThing())) ||
               (desc.value_.isGCThing() &&
                JS::IsPoisonedPtr(desc.value_.toGCThing())) ||
               (desc.get_.isGCThing() &&
                JS::IsPoisonedPtr(desc.get_.toGCThing())) ||
               (desc.set_.isGCThing() &&
                JS::IsPoisonedPtr(desc.set_.toGCThing()));
    }
};

template <>
class RootedBase<PropDesc>
  : public JS::MutablePropDescOperations<JS::Rooted<PropDesc> >
{
    friend class JS::PropDescOperations<JS::Rooted<PropDesc> >;
    friend class JS::MutablePropDescOperations<JS::Rooted<PropDesc> >;
    const PropDesc *extract() const {
        return static_cast<const JS::Rooted<PropDesc>*>(this)->address();
    }
    PropDesc *extractMutable() {
        return static_cast<JS::Rooted<PropDesc>*>(this)->address();
    }
};

template <>
class HandleBase<PropDesc>
  : public JS::PropDescOperations<JS::Handle<PropDesc> >
{
    friend class JS::PropDescOperations<JS::Handle<PropDesc> >;
    const PropDesc *extract() const {
        return static_cast<const JS::Handle<PropDesc>*>(this)->address();
    }
};

template <>
class MutableHandleBase<PropDesc>
  : public JS::MutablePropDescOperations<JS::MutableHandle<PropDesc> >
{
    friend class JS::PropDescOperations<JS::MutableHandle<PropDesc> >;
    friend class JS::MutablePropDescOperations<JS::MutableHandle<PropDesc> >;
    const PropDesc *extract() const {
        return static_cast<const JS::MutableHandle<PropDesc>*>(this)->address();
    }
    PropDesc *extractMutable() {
        return static_cast<JS::MutableHandle<PropDesc>*>(this)->address();
    }
};

} 

#endif 

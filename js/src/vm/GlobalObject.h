





#ifndef vm_GlobalObject_h
#define vm_GlobalObject_h

#include "jsarray.h"
#include "jsbool.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsnum.h"

#include "builtin/RegExp.h"
#include "js/Vector.h"
#include "vm/ArrayBufferObject.h"
#include "vm/ErrorObject.h"
#include "vm/Runtime.h"

namespace js {

extern JSObject*
InitSharedArrayBufferClass(JSContext* cx, HandleObject obj);

class Debugger;
class TypedObjectModuleObject;
































class GlobalObject : public NativeObject
{
    
    static const unsigned APPLICATION_SLOTS = JSCLASS_GLOBAL_APPLICATION_SLOTS;

    



    static const unsigned STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const unsigned EVAL                    = APPLICATION_SLOTS + STANDARD_CLASS_SLOTS;
    static const unsigned CREATE_DATAVIEW_FOR_THIS = EVAL + 1;
    static const unsigned THROWTYPEERROR          = CREATE_DATAVIEW_FOR_THIS + 1;

    



    static const unsigned FROM_BUFFER_UINT8 = THROWTYPEERROR + 1;
    static const unsigned FROM_BUFFER_INT8 = FROM_BUFFER_UINT8 + 1;
    static const unsigned FROM_BUFFER_UINT16 = FROM_BUFFER_INT8 + 1;
    static const unsigned FROM_BUFFER_INT16 = FROM_BUFFER_UINT16 + 1;
    static const unsigned FROM_BUFFER_UINT32 = FROM_BUFFER_INT16 + 1;
    static const unsigned FROM_BUFFER_INT32 = FROM_BUFFER_UINT32 + 1;
    static const unsigned FROM_BUFFER_FLOAT32 = FROM_BUFFER_INT32 + 1;
    static const unsigned FROM_BUFFER_FLOAT64 = FROM_BUFFER_FLOAT32 + 1;
    static const unsigned FROM_BUFFER_UINT8CLAMPED = FROM_BUFFER_FLOAT64 + 1;

    
    static const unsigned ARRAY_ITERATOR_PROTO  = FROM_BUFFER_UINT8CLAMPED + 1;
    static const unsigned STRING_ITERATOR_PROTO  = ARRAY_ITERATOR_PROTO + 1;
    static const unsigned LEGACY_GENERATOR_OBJECT_PROTO = STRING_ITERATOR_PROTO + 1;
    static const unsigned STAR_GENERATOR_OBJECT_PROTO = LEGACY_GENERATOR_OBJECT_PROTO + 1;
    static const unsigned MAP_ITERATOR_PROTO      = STAR_GENERATOR_OBJECT_PROTO + 1;
    static const unsigned SET_ITERATOR_PROTO      = MAP_ITERATOR_PROTO + 1;
    static const unsigned COLLATOR_PROTO          = SET_ITERATOR_PROTO + 1;
    static const unsigned NUMBER_FORMAT_PROTO     = COLLATOR_PROTO + 1;
    static const unsigned DATE_TIME_FORMAT_PROTO  = NUMBER_FORMAT_PROTO + 1;
    static const unsigned REGEXP_STATICS          = DATE_TIME_FORMAT_PROTO + 1;
    static const unsigned WARNED_ONCE_FLAGS       = REGEXP_STATICS + 1;
    static const unsigned RUNTIME_CODEGEN_ENABLED = WARNED_ONCE_FLAGS + 1;
    static const unsigned DEBUGGERS               = RUNTIME_CODEGEN_ENABLED + 1;
    static const unsigned INTRINSICS              = DEBUGGERS + 1;
    static const unsigned FLOAT32X4_TYPE_DESCR    = INTRINSICS + 1;
    static const unsigned FLOAT64X2_TYPE_DESCR    = FLOAT32X4_TYPE_DESCR + 1;
    static const unsigned INT32X4_TYPE_DESCR      = FLOAT64X2_TYPE_DESCR + 1;
    static const unsigned FOR_OF_PIC_CHAIN        = INT32X4_TYPE_DESCR + 1;

    
    static const unsigned RESERVED_SLOTS = FOR_OF_PIC_CHAIN + 1;

    




    static_assert(JSCLASS_GLOBAL_SLOT_COUNT == RESERVED_SLOTS,
                  "global object slot counts are inconsistent");

    enum WarnOnceFlag : int32_t {
        WARN_WATCH_DEPRECATED                   = 0x00000001,
        WARN_PROTO_SETTING_SLOW                 = 0x00000002,
        WARN_STRING_CONTAINS_DEPRECATED         = 0x00000004
    };

    
    
    
    
    static bool
    warnOnceAbout(JSContext* cx, HandleObject obj, WarnOnceFlag flag, unsigned errorNumber);


  public:
    void setThrowTypeError(JSFunction* fun) {
        MOZ_ASSERT(getSlotRef(THROWTYPEERROR).isUndefined());
        setSlot(THROWTYPEERROR, ObjectValue(*fun));
    }

    void setOriginalEval(JSObject* evalobj) {
        MOZ_ASSERT(getSlotRef(EVAL).isUndefined());
        setSlot(EVAL, ObjectValue(*evalobj));
    }

    void setIntrinsicsHolder(JSObject* obj) {
        MOZ_ASSERT(getSlotRef(INTRINSICS).isUndefined());
        setSlot(INTRINSICS, ObjectValue(*obj));
    }

    Value getConstructor(JSProtoKey key) const {
        MOZ_ASSERT(key <= JSProto_LIMIT);
        return getSlot(APPLICATION_SLOTS + key);
    }
    static bool ensureConstructor(JSContext* cx, Handle<GlobalObject*> global, JSProtoKey key);
    static bool resolveConstructor(JSContext* cx, Handle<GlobalObject*> global, JSProtoKey key);
    static bool initBuiltinConstructor(JSContext* cx, Handle<GlobalObject*> global,
                                       JSProtoKey key, HandleObject ctor, HandleObject proto);

    void setConstructor(JSProtoKey key, const Value& v) {
        MOZ_ASSERT(key <= JSProto_LIMIT);
        setSlot(APPLICATION_SLOTS + key, v);
    }

    Value getPrototype(JSProtoKey key) const {
        MOZ_ASSERT(key <= JSProto_LIMIT);
        return getSlot(APPLICATION_SLOTS + JSProto_LIMIT + key);
    }

    void setPrototype(JSProtoKey key, const Value& value) {
        MOZ_ASSERT(key <= JSProto_LIMIT);
        setSlot(APPLICATION_SLOTS + JSProto_LIMIT + key, value);
    }

    static uint32_t constructorPropertySlot(JSProtoKey key) {
        MOZ_ASSERT(key <= JSProto_LIMIT);
        return APPLICATION_SLOTS + JSProto_LIMIT * 2 + key;
    }

    Value getConstructorPropertySlot(JSProtoKey key) {
        return getSlot(constructorPropertySlot(key));
    }

    void setConstructorPropertySlot(JSProtoKey key, const Value& ctor) {
        setSlot(constructorPropertySlot(key), ctor);
    }

    bool classIsInitialized(JSProtoKey key) const {
        bool inited = !getConstructor(key).isUndefined();
        MOZ_ASSERT(inited == !getPrototype(key).isUndefined());
        return inited;
    }

    bool functionObjectClassesInitialized() const {
        bool inited = classIsInitialized(JSProto_Function);
        MOZ_ASSERT(inited == classIsInitialized(JSProto_Object));
        return inited;
    }

    









    bool isStandardClassResolved(JSProtoKey key) const {
        
        MOZ_ASSERT(getConstructor(key).isUndefined() ||
                   getConstructor(key).isObject());
        return !getConstructor(key).isUndefined();
    }

    









    static HandleObject upcast(Handle<GlobalObject*> global) {
        return HandleObject::fromMarkedLocation(
                reinterpret_cast<JSObject * const*>(global.address()));
    }

  private:
    bool arrayClassInitialized() const {
        return classIsInitialized(JSProto_Array);
    }

    bool booleanClassInitialized() const {
        return classIsInitialized(JSProto_Boolean);
    }
    bool numberClassInitialized() const {
        return classIsInitialized(JSProto_Number);
    }
    bool stringClassInitialized() const {
        return classIsInitialized(JSProto_String);
    }
    bool regexpClassInitialized() const {
        return classIsInitialized(JSProto_RegExp);
    }
    bool arrayBufferClassInitialized() const {
        return classIsInitialized(JSProto_ArrayBuffer);
    }
    bool sharedArrayBufferClassInitialized() const {
        return classIsInitialized(JSProto_SharedArrayBuffer);
    }
    bool errorClassesInitialized() const {
        return classIsInitialized(JSProto_Error);
    }
    bool dataViewClassInitialized() const {
        return classIsInitialized(JSProto_DataView);
    }

    Value createArrayFromBufferHelper(uint32_t slot) const {
        MOZ_ASSERT(FROM_BUFFER_UINT8 <= slot && slot <= FROM_BUFFER_UINT8CLAMPED);
        MOZ_ASSERT(!getSlot(slot).isUndefined());
        return getSlot(slot);
    }

    void setCreateArrayFromBufferHelper(uint32_t slot, Handle<JSFunction*> fun) {
        MOZ_ASSERT(getSlotRef(slot).isUndefined());
        setSlot(slot, ObjectValue(*fun));
    }

  public:
    
    void setCreateDataViewForThis(Handle<JSFunction*> fun) {
        MOZ_ASSERT(getSlotRef(CREATE_DATAVIEW_FOR_THIS).isUndefined());
        setSlot(CREATE_DATAVIEW_FOR_THIS, ObjectValue(*fun));
    }

    template<typename T>
    inline void setCreateArrayFromBuffer(Handle<JSFunction*> fun);

  private:
    
    static GlobalObject* create(...) = delete;

    friend struct ::JSRuntime;
    static GlobalObject* createInternal(JSContext* cx, const Class* clasp);

  public:
    static GlobalObject*
    new_(JSContext* cx, const Class* clasp, JSPrincipals* principals,
         JS::OnNewGlobalHookOption hookOption, const JS::CompartmentOptions& options);

    



    JSFunction*
    createConstructor(JSContext* cx, JSNative ctor, JSAtom* name, unsigned length,
                      gc::AllocKind kind = JSFunction::FinalizeKind);

    







    NativeObject* createBlankPrototype(JSContext* cx, const js::Class* clasp);

    



    NativeObject* createBlankPrototypeInheriting(JSContext* cx, const js::Class* clasp,
                                                 HandleObject proto);

    template <typename T>
    T* createBlankPrototype(JSContext* cx) {
        NativeObject* res = createBlankPrototype(cx, &T::class_);
        return res ? &res->template as<T>() : nullptr;
    }

    NativeObject* getOrCreateObjectPrototype(JSContext* cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Object).toObject().as<NativeObject>();
        RootedGlobalObject self(cx, this);
        if (!ensureConstructor(cx, self, JSProto_Object))
            return nullptr;
        return &self->getPrototype(JSProto_Object).toObject().as<NativeObject>();
    }

    NativeObject* getOrCreateFunctionPrototype(JSContext* cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Function).toObject().as<NativeObject>();
        RootedGlobalObject self(cx, this);
        if (!ensureConstructor(cx, self, JSProto_Object))
            return nullptr;
        return &self->getPrototype(JSProto_Function).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateArrayPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_Array))
            return nullptr;
        return &global->getPrototype(JSProto_Array).toObject().as<NativeObject>();
    }

    NativeObject* maybeGetArrayPrototype() {
        if (arrayClassInitialized())
            return &getPrototype(JSProto_Array).toObject().as<NativeObject>();
        return nullptr;
    }

    static NativeObject* getOrCreateBooleanPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_Boolean))
            return nullptr;
        return &global->getPrototype(JSProto_Boolean).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateNumberPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_Number))
            return nullptr;
        return &global->getPrototype(JSProto_Number).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateStringPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_String))
            return nullptr;
        return &global->getPrototype(JSProto_String).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateSymbolPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_Symbol))
            return nullptr;
        return &global->getPrototype(JSProto_Symbol).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateRegExpPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_RegExp))
            return nullptr;
        return &global->getPrototype(JSProto_RegExp).toObject().as<NativeObject>();
    }

    JSObject* maybeGetRegExpPrototype() {
        if (regexpClassInitialized())
            return &getPrototype(JSProto_RegExp).toObject();
        return nullptr;
    }

    static NativeObject* getOrCreateSavedFramePrototype(JSContext* cx,
                                                        Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_SavedFrame))
            return nullptr;
        return &global->getPrototype(JSProto_SavedFrame).toObject().as<NativeObject>();
    }

    static JSObject* getOrCreateArrayBufferPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_ArrayBuffer))
            return nullptr;
        return &global->getPrototype(JSProto_ArrayBuffer).toObject();
    }

    JSObject* getOrCreateSharedArrayBufferPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_SharedArrayBuffer))
            return nullptr;
        return &global->getPrototype(JSProto_SharedArrayBuffer).toObject();
    }

    static JSObject* getOrCreateCustomErrorPrototype(JSContext* cx,
                                                     Handle<GlobalObject*> global,
                                                     JSExnType exnType)
    {
        JSProtoKey key = GetExceptionProtoKey(exnType);
        if (!ensureConstructor(cx, global, key))
            return nullptr;
        return &global->getPrototype(key).toObject();
    }

    JSObject* getOrCreateIntlObject(JSContext* cx) {
        return getOrCreateObject(cx, APPLICATION_SLOTS + JSProto_Intl, initIntlObject);
    }

    JSObject* getOrCreateTypedObjectModule(JSContext* cx) {
        return getOrCreateObject(cx, APPLICATION_SLOTS + JSProto_TypedObject, initTypedObjectModule);
    }

    void setFloat32x4TypeDescr(JSObject& obj) {
        MOZ_ASSERT(getSlotRef(FLOAT32X4_TYPE_DESCR).isUndefined());
        setSlot(FLOAT32X4_TYPE_DESCR, ObjectValue(obj));
    }

    JSObject& float32x4TypeDescr() {
        MOZ_ASSERT(getSlotRef(FLOAT32X4_TYPE_DESCR).isObject());
        return getSlotRef(FLOAT32X4_TYPE_DESCR).toObject();
    }

    void setFloat64x2TypeDescr(JSObject& obj) {
        MOZ_ASSERT(getSlotRef(FLOAT64X2_TYPE_DESCR).isUndefined());
        setSlot(FLOAT64X2_TYPE_DESCR, ObjectValue(obj));
    }

    JSObject& float64x2TypeDescr() {
        MOZ_ASSERT(getSlotRef(FLOAT64X2_TYPE_DESCR).isObject());
        return getSlotRef(FLOAT64X2_TYPE_DESCR).toObject();
    }

    void setInt32x4TypeDescr(JSObject& obj) {
        MOZ_ASSERT(getSlotRef(INT32X4_TYPE_DESCR).isUndefined());
        setSlot(INT32X4_TYPE_DESCR, ObjectValue(obj));
    }

    JSObject& int32x4TypeDescr() {
        MOZ_ASSERT(getSlotRef(INT32X4_TYPE_DESCR).isObject());
        return getSlotRef(INT32X4_TYPE_DESCR).toObject();
    }

    TypedObjectModuleObject& getTypedObjectModule() const;

    JSObject* getIteratorPrototype() {
        return &getPrototype(JSProto_Iterator).toObject();
    }

    JSObject* getOrCreateCollatorPrototype(JSContext* cx) {
        return getOrCreateObject(cx, COLLATOR_PROTO, initCollatorProto);
    }

    JSObject* getOrCreateNumberFormatPrototype(JSContext* cx) {
        return getOrCreateObject(cx, NUMBER_FORMAT_PROTO, initNumberFormatProto);
    }

    JSObject* getOrCreateDateTimeFormatPrototype(JSContext* cx) {
        return getOrCreateObject(cx, DATE_TIME_FORMAT_PROTO, initDateTimeFormatProto);
    }

    static JSFunction*
    getOrCreateTypedArrayConstructor(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_TypedArray))
            return nullptr;
        return &global->getConstructor(JSProto_TypedArray).toObject().as<JSFunction>();
    }

    static JSObject*
    getOrCreateTypedArrayPrototype(JSContext* cx, Handle<GlobalObject*> global) {
        if (!ensureConstructor(cx, global, JSProto_TypedArray))
            return nullptr;
        return &global->getPrototype(JSProto_TypedArray).toObject();
    }

  private:
    typedef bool (*ObjectInitOp)(JSContext* cx, Handle<GlobalObject*> global);

    JSObject* getOrCreateObject(JSContext* cx, unsigned slot, ObjectInitOp init) {
        Value v = getSlotRef(slot);
        if (v.isObject())
            return &v.toObject();
        RootedGlobalObject self(cx, this);
        if (!init(cx, self))
            return nullptr;
        return &self->getSlot(slot).toObject();
    }

  public:
    static NativeObject* getOrCreateIteratorPrototype(JSContext* cx,
                                                      Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        size_t slot = APPLICATION_SLOTS + JSProto_LIMIT + JSProto_Iterator;
        return &global->getSlot(slot).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateArrayIteratorPrototype(JSContext* cx,
                                                           Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        return &global->getSlot(ARRAY_ITERATOR_PROTO).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateStringIteratorPrototype(JSContext* cx,
                                                            Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        return &global->getSlot(STRING_ITERATOR_PROTO).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateLegacyGeneratorObjectPrototype(JSContext* cx,
                                                                   Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        return &global->getSlot(LEGACY_GENERATOR_OBJECT_PROTO).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateStarGeneratorObjectPrototype(JSContext* cx,
                                                                 Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        return &global->getSlot(STAR_GENERATOR_OBJECT_PROTO).toObject().as<NativeObject>();
    }

    static NativeObject* getOrCreateStarGeneratorFunctionPrototype(JSContext* cx,
                                                                   Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        size_t slot = APPLICATION_SLOTS + JSProto_LIMIT + JSProto_GeneratorFunction;
        return &global->getSlot(slot).toObject().as<NativeObject>();
    }

    static JSObject* getOrCreateStarGeneratorFunction(JSContext* cx,
                                                      Handle<GlobalObject*> global)
    {
        if (!ensureConstructor(cx, global, JSProto_Iterator))
            return nullptr;
        return &global->getSlot(APPLICATION_SLOTS + JSProto_GeneratorFunction).toObject();
    }

    static JSObject* getOrCreateMapIteratorPrototype(JSContext* cx,
                                                     Handle<GlobalObject*> global)
    {
        return global->getOrCreateObject(cx, MAP_ITERATOR_PROTO, initMapIteratorProto);
    }

    static JSObject* getOrCreateSetIteratorPrototype(JSContext* cx,
                                                     Handle<GlobalObject*> global)
    {
        return global->getOrCreateObject(cx, SET_ITERATOR_PROTO, initSetIteratorProto);
    }

    JSObject* getOrCreateDataViewPrototype(JSContext* cx) {
        RootedGlobalObject self(cx, this);
        if (!ensureConstructor(cx, self, JSProto_DataView))
            return nullptr;
        return &self->getPrototype(JSProto_DataView).toObject();
    }

    NativeObject* intrinsicsHolder() {
        MOZ_ASSERT(!getSlot(INTRINSICS).isUndefined());
        return &getSlot(INTRINSICS).toObject().as<NativeObject>();
    }

    bool maybeGetIntrinsicValue(jsid id, Value* vp) {
        NativeObject* holder = intrinsicsHolder();

        if (Shape* shape = holder->lookupPure(id)) {
            *vp = holder->getSlot(shape->slot());
            return true;
        }
        return false;
    }
    bool maybeGetIntrinsicValue(PropertyName* name, Value* vp) {
        return maybeGetIntrinsicValue(NameToId(name), vp);
    }

    static bool getIntrinsicValue(JSContext* cx, Handle<GlobalObject*> global,
                                  HandlePropertyName name, MutableHandleValue value)
    {
        if (global->maybeGetIntrinsicValue(name, value.address()))
            return true;
        if (!cx->runtime()->cloneSelfHostedValue(cx, name, value))
            return false;
        RootedId id(cx, NameToId(name));
        return global->addIntrinsicValue(cx, id, value);
    }

    bool addIntrinsicValue(JSContext* cx, HandleId id, HandleValue value);

    bool setIntrinsicValue(JSContext* cx, PropertyName* name, HandleValue value) {
#ifdef DEBUG
        RootedObject self(cx, this);
        MOZ_ASSERT(cx->runtime()->isSelfHostingGlobal(self));
#endif
        RootedObject holder(cx, intrinsicsHolder());
        return SetProperty(cx, holder, name, value);
    }

    bool getSelfHostedFunction(JSContext* cx, HandleAtom selfHostedName, HandleAtom name,
                               unsigned nargs, MutableHandleValue funVal);

    bool hasRegExpStatics() const;
    RegExpStatics* getRegExpStatics(ExclusiveContext* cx) const;
    RegExpStatics* getAlreadyCreatedRegExpStatics() const;

    JSObject* getThrowTypeError() const {
        const Value v = getReservedSlot(THROWTYPEERROR);
        MOZ_ASSERT(v.isObject(),
                   "attempting to access [[ThrowTypeError]] too early");
        return &v.toObject();
    }

    Value createDataViewForThis() const {
        MOZ_ASSERT(dataViewClassInitialized());
        return getSlot(CREATE_DATAVIEW_FOR_THIS);
    }

    template<typename T>
    inline Value createArrayFromBuffer() const;

    static bool isRuntimeCodeGenEnabled(JSContext* cx, Handle<GlobalObject*> global);

    
    
    static bool warnOnceAboutWatch(JSContext* cx, HandleObject obj) {
        
        
        
        return true;
    }

    
    
    static bool warnOnceAboutPrototypeMutation(JSContext* cx, HandleObject protoSetter) {
        return warnOnceAbout(cx, protoSetter, WARN_PROTO_SETTING_SLOW, JSMSG_PROTO_SETTING_SLOW);
    }

    
    static bool warnOnceAboutStringContains(JSContext *cx, HandleObject strContains) {
        return warnOnceAbout(cx, strContains, WARN_STRING_CONTAINS_DEPRECATED,
                             JSMSG_DEPRECATED_STRING_CONTAINS);
    }

    static bool getOrCreateEval(JSContext* cx, Handle<GlobalObject*> global,
                                MutableHandleObject eval);

    
    bool valueIsEval(Value val);

    
    static bool initIteratorClasses(JSContext* cx, Handle<GlobalObject*> global);
    static bool initStopIterationClass(JSContext* cx, Handle<GlobalObject*> global);

    
    static bool initGeneratorClasses(JSContext* cx, Handle<GlobalObject*> global);

    
    static bool initMapIteratorProto(JSContext* cx, Handle<GlobalObject*> global);
    static bool initSetIteratorProto(JSContext* cx, Handle<GlobalObject*> global);

    
    static bool initIntlObject(JSContext* cx, Handle<GlobalObject*> global);
    static bool initCollatorProto(JSContext* cx, Handle<GlobalObject*> global);
    static bool initNumberFormatProto(JSContext* cx, Handle<GlobalObject*> global);
    static bool initDateTimeFormatProto(JSContext* cx, Handle<GlobalObject*> global);

    
    static bool initTypedObjectModule(JSContext* cx, Handle<GlobalObject*> global);

    static bool initStandardClasses(JSContext* cx, Handle<GlobalObject*> global);
    static bool initSelfHostingBuiltins(JSContext* cx, Handle<GlobalObject*> global,
                                        const JSFunctionSpec* builtins);

    typedef js::Vector<js::Debugger*, 0, js::SystemAllocPolicy> DebuggerVector;

    



    DebuggerVector* getDebuggers() const;

    



    static DebuggerVector* getOrCreateDebuggers(JSContext* cx, Handle<GlobalObject*> global);

    inline NativeObject* getForOfPICObject() {
        Value forOfPIC = getReservedSlot(FOR_OF_PIC_CHAIN);
        if (forOfPIC.isUndefined())
            return nullptr;
        return &forOfPIC.toObject().as<NativeObject>();
    }
    static NativeObject* getOrCreateForOfPICObject(JSContext* cx, Handle<GlobalObject*> global);
};

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<uint8_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_UINT8, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<int8_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_INT8, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<uint16_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_UINT16, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<int16_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_INT16, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<uint32_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_UINT32, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<int32_t>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_INT32, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<float>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_FLOAT32, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<double>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_FLOAT64, fun);
}

template<>
inline void
GlobalObject::setCreateArrayFromBuffer<uint8_clamped>(Handle<JSFunction*> fun)
{
    setCreateArrayFromBufferHelper(FROM_BUFFER_UINT8CLAMPED, fun);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<uint8_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_UINT8);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<int8_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_INT8);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<uint16_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_UINT16);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<int16_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_INT16);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<uint32_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_UINT32);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<int32_t>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_INT32);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<float>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_FLOAT32);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<double>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_FLOAT64);
}

template<>
inline Value
GlobalObject::createArrayFromBuffer<uint8_clamped>() const
{
    return createArrayFromBufferHelper(FROM_BUFFER_UINT8CLAMPED);
}






extern bool
LinkConstructorAndPrototype(JSContext* cx, JSObject* ctor, JSObject* proto);





extern bool
DefinePropertiesAndFunctions(JSContext* cx, HandleObject obj,
                             const JSPropertySpec* ps, const JSFunctionSpec* fs);

typedef HashSet<GlobalObject*, DefaultHasher<GlobalObject*>, SystemAllocPolicy> GlobalObjectSet;






template<JSNative ctor, unsigned length, gc::AllocKind kind>
JSObject*
GenericCreateConstructor(JSContext* cx, JSProtoKey key)
{
    
    
    PropertyName* name = (&cx->names().Null)[key];
    return cx->global()->createConstructor(cx, ctor, name, length, kind);
}

inline JSObject*
GenericCreatePrototype(JSContext* cx, JSProtoKey key)
{
    MOZ_ASSERT(key != JSProto_Object);
    const Class* clasp = ProtoKeyToClass(key);
    JSProtoKey parentKey = ParentKeyForStandardClass(key);
    if (!GlobalObject::ensureConstructor(cx, cx->global(), parentKey))
        return nullptr;
    RootedObject parentProto(cx, &cx->global()->getPrototype(parentKey).toObject());
    return cx->global()->createBlankPrototypeInheriting(cx, clasp, parentProto);
}

inline JSProtoKey
StandardProtoKeyOrNull(const JSObject* obj)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(obj->getClass());
    if (key == JSProto_Error)
        return GetExceptionProtoKey(obj->as<ErrorObject>().type());
    return key;
}

} 

template<>
inline bool
JSObject::is<js::GlobalObject>() const
{
    return !!(getClass()->flags & JSCLASS_IS_GLOBAL);
}

#endif 








#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "jsarray.h"
#include "jsbool.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"

#include "js/Vector.h"

#include "builtin/RegExp.h"

extern JSObject *
js_InitObjectClass(JSContext *cx, js::HandleObject obj);

extern JSObject *
js_InitFunctionClass(JSContext *cx, js::HandleObject obj);

extern JSObject *
js_InitTypedArrayClasses(JSContext *cx, js::HandleObject obj);

namespace js {

class Debugger;




























class GlobalObject : public JSObject
{
    



    static const unsigned STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const unsigned BOOLEAN_VALUEOF         = STANDARD_CLASS_SLOTS;
    static const unsigned EVAL                    = BOOLEAN_VALUEOF + 1;
    static const unsigned CREATE_DATAVIEW_FOR_THIS = EVAL + 1;
    static const unsigned THROWTYPEERROR          = CREATE_DATAVIEW_FOR_THIS + 1;
    static const unsigned PROTO_GETTER            = THROWTYPEERROR + 1;

    



    static const unsigned FROM_BUFFER_UINT8 = PROTO_GETTER + 1;
    static const unsigned FROM_BUFFER_INT8 = FROM_BUFFER_UINT8 + 1;
    static const unsigned FROM_BUFFER_UINT16 = FROM_BUFFER_INT8 + 1;
    static const unsigned FROM_BUFFER_INT16 = FROM_BUFFER_UINT16 + 1;
    static const unsigned FROM_BUFFER_UINT32 = FROM_BUFFER_INT16 + 1;
    static const unsigned FROM_BUFFER_INT32 = FROM_BUFFER_UINT32 + 1;
    static const unsigned FROM_BUFFER_FLOAT32 = FROM_BUFFER_INT32 + 1;
    static const unsigned FROM_BUFFER_FLOAT64 = FROM_BUFFER_FLOAT32 + 1;
    static const unsigned FROM_BUFFER_UINT8CLAMPED = FROM_BUFFER_FLOAT64 + 1;

    
    static const unsigned ELEMENT_ITERATOR_PROTO  = FROM_BUFFER_UINT8CLAMPED + 1;
    static const unsigned GENERATOR_PROTO         = ELEMENT_ITERATOR_PROTO + 1;
    static const unsigned MAP_ITERATOR_PROTO      = GENERATOR_PROTO + 1;
    static const unsigned SET_ITERATOR_PROTO      = MAP_ITERATOR_PROTO + 1;
    static const unsigned COLLATOR_PROTO          = SET_ITERATOR_PROTO + 1;
    static const unsigned NUMBER_FORMAT_PROTO     = COLLATOR_PROTO + 1;
    static const unsigned DATE_TIME_FORMAT_PROTO  = NUMBER_FORMAT_PROTO + 1;
    static const unsigned REGEXP_STATICS          = DATE_TIME_FORMAT_PROTO + 1;
    static const unsigned FUNCTION_NS             = REGEXP_STATICS + 1;
    static const unsigned RUNTIME_CODEGEN_ENABLED = FUNCTION_NS + 1;
    static const unsigned DEBUGGERS               = RUNTIME_CODEGEN_ENABLED + 1;
    static const unsigned INTRINSICS              = DEBUGGERS + 1;

    
    static const unsigned RESERVED_SLOTS = INTRINSICS + 1;

    void staticAsserts() {
        




        JS_STATIC_ASSERT(JSCLASS_GLOBAL_SLOT_COUNT == RESERVED_SLOTS);
    }

    friend JSObject *
    ::js_InitObjectClass(JSContext *cx, js::HandleObject);
    friend JSObject *
    ::js_InitFunctionClass(JSContext *cx, js::HandleObject);

    
    JSObject *
    initFunctionAndObjectClasses(JSContext *cx);

    inline void setDetailsForKey(JSProtoKey key, JSObject *ctor, JSObject *proto);
    inline void setObjectClassDetails(JSFunction *ctor, JSObject *proto);
    inline void setFunctionClassDetails(JSFunction *ctor, JSObject *proto);

    inline void setThrowTypeError(JSFunction *fun);
    inline void setOriginalEval(JSObject *evalobj);
    inline void setProtoGetter(JSFunction *protoGetter);

    inline void setIntrinsicsHolder(JSObject *obj);

    Value getConstructor(JSProtoKey key) const {
        JS_ASSERT(key <= JSProto_LIMIT);
        return getSlot(key);
    }

    Value getPrototype(JSProtoKey key) const {
        JS_ASSERT(key <= JSProto_LIMIT);
        return getSlot(JSProto_LIMIT + key);
    }

    bool classIsInitialized(JSProtoKey key) const {
        bool inited = !getConstructor(key).isUndefined();
        JS_ASSERT(inited == !getPrototype(key).isUndefined());
        return inited;
    }

    bool functionObjectClassesInitialized() const {
        bool inited = classIsInitialized(JSProto_Function);
        JS_ASSERT(inited == classIsInitialized(JSProto_Object));
        return inited;
    }

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
    bool errorClassesInitialized() const {
        return classIsInitialized(JSProto_Error);
    }
    bool dataViewClassInitialized() const {
        return classIsInitialized(JSProto_DataView);
    }
    bool typedArrayClassesInitialized() const {
        
        
        return classIsInitialized(JSProto_DataView);
    }

    Value createArrayFromBufferHelper(uint32_t slot) const {
        JS_ASSERT(typedArrayClassesInitialized());
        JS_ASSERT(FROM_BUFFER_UINT8 <= slot && slot <= FROM_BUFFER_UINT8CLAMPED);
        return getSlot(slot);
    }

    inline void setCreateArrayFromBufferHelper(uint32_t slot, Handle<JSFunction*> fun);

  public:
    
    inline void setBooleanValueOf(Handle<JSFunction*> valueOfFun);
    inline void setCreateDataViewForThis(Handle<JSFunction*> fun);

    template<typename T>
    inline void setCreateArrayFromBuffer(Handle<JSFunction*> fun);

  public:
    static GlobalObject *create(JSContext *cx, Class *clasp);

    



    JSFunction *
    createConstructor(JSContext *cx, JSNative ctor, JSAtom *name, unsigned length,
                      gc::AllocKind kind = JSFunction::FinalizeKind);

    







    JSObject *createBlankPrototype(JSContext *cx, js::Class *clasp);

    



    JSObject *createBlankPrototypeInheriting(JSContext *cx, js::Class *clasp, JSObject &proto);

    JSObject *getOrCreateObjectPrototype(JSContext *cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Object).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!initFunctionAndObjectClasses(cx))
            return NULL;
        return &self->getPrototype(JSProto_Object).toObject();
    }

    JSObject *getOrCreateFunctionPrototype(JSContext *cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Function).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!initFunctionAndObjectClasses(cx))
            return NULL;
        return &self->getPrototype(JSProto_Function).toObject();
    }

    JSObject *getOrCreateArrayPrototype(JSContext *cx) {
        if (arrayClassInitialized())
            return &getPrototype(JSProto_Array).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitArrayClass(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_Array).toObject();
    }

    JSObject *getOrCreateBooleanPrototype(JSContext *cx) {
        if (booleanClassInitialized())
            return &getPrototype(JSProto_Boolean).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitBooleanClass(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_Boolean).toObject();
    }

    JSObject *getOrCreateNumberPrototype(JSContext *cx) {
        if (numberClassInitialized())
            return &getPrototype(JSProto_Number).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitNumberClass(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_Number).toObject();
    }

    JSObject *getOrCreateStringPrototype(JSContext *cx) {
        if (stringClassInitialized())
            return &getPrototype(JSProto_String).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitStringClass(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_String).toObject();
    }

    JSObject *getOrCreateRegExpPrototype(JSContext *cx) {
        if (regexpClassInitialized())
            return &getPrototype(JSProto_RegExp).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitRegExpClass(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_RegExp).toObject();
    }

    JSObject *getOrCreateArrayBufferPrototype(JSContext *cx) {
        if (arrayBufferClassInitialized())
            return &getPrototype(JSProto_ArrayBuffer).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitTypedArrayClasses(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_ArrayBuffer).toObject();
    }

    JSObject *getOrCreateCustomErrorPrototype(JSContext *cx, int exnType) {
        JSProtoKey key = GetExceptionProtoKey(exnType);
        if (errorClassesInitialized())
            return &getPrototype(key).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitExceptionClasses(cx, self))
            return NULL;
        return &self->getPrototype(key).toObject();
    }

    JSObject *getOrCreateIntlObject(JSContext *cx) {
        return getOrCreateObject(cx, JSProto_Intl, initIntlObject);
    }

    JSObject *getOrCreateCollatorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, COLLATOR_PROTO, initCollatorProto);
    }

    JSObject *getOrCreateNumberFormatPrototype(JSContext *cx) {
        return getOrCreateObject(cx, NUMBER_FORMAT_PROTO, initNumberFormatProto);
    }

    JSObject *getOrCreateDateTimeFormatPrototype(JSContext *cx) {
        return getOrCreateObject(cx, DATE_TIME_FORMAT_PROTO, initDateTimeFormatProto);
    }

    JSObject *getIteratorPrototype() {
        return &getPrototype(JSProto_Iterator).toObject();
    }

  private:
    typedef bool (*ObjectInitOp)(JSContext *cx, Handle<GlobalObject*> global);

    JSObject *getOrCreateObject(JSContext *cx, unsigned slot, ObjectInitOp init) {
        Value v = getSlotRef(slot);
        if (v.isObject())
            return &v.toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!init(cx, self))
            return NULL;
        return &self->getSlot(slot).toObject();
    }

  public:
    JSObject *getOrCreateIteratorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, JSProto_LIMIT + JSProto_Iterator, initIteratorClasses);
    }

    JSObject *getOrCreateElementIteratorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, ELEMENT_ITERATOR_PROTO, initIteratorClasses);
    }

    JSObject *getOrCreateGeneratorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, GENERATOR_PROTO, initIteratorClasses);
    }

    JSObject *getOrCreateMapIteratorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, MAP_ITERATOR_PROTO, initMapIteratorProto);
    }

    JSObject *getOrCreateSetIteratorPrototype(JSContext *cx) {
        return getOrCreateObject(cx, SET_ITERATOR_PROTO, initSetIteratorProto);
    }

    JSObject *getOrCreateDataViewPrototype(JSContext *cx) {
        if (dataViewClassInitialized())
            return &getPrototype(JSProto_DataView).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitTypedArrayClasses(cx, self))
            return NULL;
        return &self->getPrototype(JSProto_DataView).toObject();
    }

    JSObject *intrinsicsHolder() {
        JS_ASSERT(!getSlotRef(INTRINSICS).isUndefined());
        return &getSlotRef(INTRINSICS).toObject();
    }

    bool getIntrinsicValue(JSContext *cx, HandlePropertyName name, MutableHandleValue value) {
        RootedObject holder(cx, intrinsicsHolder());
        RootedId id(cx, NameToId(name));
        if (HasDataProperty(cx, holder, id, value.address()))
            return true;
        if (!cx->runtime->cloneSelfHostedValue(cx, name, value))
            return false;
        mozilla::DebugOnly<bool> ok = JS_DefinePropertyById(cx, holder, id, value, NULL, NULL, 0);
        JS_ASSERT(ok);
        return true;
    }

    inline bool setIntrinsicValue(JSContext *cx, PropertyName *name, HandleValue value);

    inline RegExpStatics *getRegExpStatics() const;

    JSObject *getThrowTypeError() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return &getSlot(THROWTYPEERROR).toObject();
    }

    Value booleanValueOf() const {
        JS_ASSERT(booleanClassInitialized());
        return getSlot(BOOLEAN_VALUEOF);
    }

    Value createDataViewForThis() const {
        JS_ASSERT(dataViewClassInitialized());
        return getSlot(CREATE_DATAVIEW_FOR_THIS);
    }

    template<typename T>
    inline Value createArrayFromBuffer() const;

    Value protoGetter() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return getSlot(PROTO_GETTER);
    }

    bool isRuntimeCodeGenEnabled(JSContext *cx);

    const Value &getOriginalEval() const {
        JS_ASSERT(getSlot(EVAL).isObject());
        return getSlot(EVAL);
    }

    bool getFunctionNamespace(JSContext *cx, Value *vp);

    
    static bool initIteratorClasses(JSContext *cx, Handle<GlobalObject*> global);

    
    static bool initMapIteratorProto(JSContext *cx, Handle<GlobalObject*> global);
    static bool initSetIteratorProto(JSContext *cx, Handle<GlobalObject*> global);

    
    static bool initIntlObject(JSContext *cx, Handle<GlobalObject*> global);
    static bool initCollatorProto(JSContext *cx, Handle<GlobalObject*> global);
    static bool initNumberFormatProto(JSContext *cx, Handle<GlobalObject*> global);
    static bool initDateTimeFormatProto(JSContext *cx, Handle<GlobalObject*> global);

    static bool initStandardClasses(JSContext *cx, Handle<GlobalObject*> global);

    typedef js::Vector<js::Debugger *, 0, js::SystemAllocPolicy> DebuggerVector;

    



    DebuggerVector *getDebuggers();

    



    static DebuggerVector *getOrCreateDebuggers(JSContext *cx, Handle<GlobalObject*> global);

    static bool addDebugger(JSContext *cx, Handle<GlobalObject*> global, Debugger *dbg);
};






extern bool
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto);





extern bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj,
                         const JSPropertySpec *ps, const JSFunctionSpec *fs);

typedef HashSet<GlobalObject *, DefaultHasher<GlobalObject *>, SystemAllocPolicy> GlobalObjectSet;

} 

inline bool
JSObject::isGlobal() const
{
    return !!(js::GetObjectClass(const_cast<JSObject*>(this))->flags & JSCLASS_IS_GLOBAL);
}

js::GlobalObject &
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return *static_cast<js::GlobalObject *>(this);
}

#endif 

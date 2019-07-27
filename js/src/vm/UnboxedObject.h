





#ifndef vm_UnboxedObject_h
#define vm_UnboxedObject_h

#include "jsgc.h"
#include "jsobj.h"

#include "vm/TypeInference.h"

namespace js {



static inline size_t
UnboxedTypeSize(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_BOOLEAN: return 1;
      case JSVAL_TYPE_INT32:   return 4;
      case JSVAL_TYPE_DOUBLE:  return 8;
      case JSVAL_TYPE_STRING:  return sizeof(void*);
      case JSVAL_TYPE_OBJECT:  return sizeof(void*);
      default:                 return 0;
    }
}

static inline bool
UnboxedTypeNeedsPreBarrier(JSValueType type)
{
    return type == JSVAL_TYPE_STRING || type == JSVAL_TYPE_OBJECT;
}


class UnboxedLayout : public mozilla::LinkedListElement<UnboxedLayout>
{
  public:
    struct Property {
        PropertyName* name;
        uint32_t offset;
        JSValueType type;

        Property()
          : name(nullptr), offset(UINT32_MAX), type(JSVAL_TYPE_MAGIC)
        {}
    };

    typedef Vector<Property, 0, SystemAllocPolicy> PropertyVector;

  private:
    
    
    
    HeapPtrObjectGroup nativeGroup_;
    HeapPtrShape nativeShape_;

    

    
    PropertyVector properties_;

    
    size_t size_;

    
    TypeNewScript* newScript_;

    
    
    int32_t* traceList_;

    
    
    
    
    
    
    HeapPtrObjectGroup replacementNewGroup_;

    
    
    
    HeapPtrJitCode constructorCode_;

    

    
    JSValueType elementType_;

  public:
    UnboxedLayout()
      : nativeGroup_(nullptr), nativeShape_(nullptr), size_(0), newScript_(nullptr),
        traceList_(nullptr), replacementNewGroup_(nullptr), constructorCode_(nullptr),
        elementType_(JSVAL_TYPE_MAGIC)
    {}

    bool initProperties(const PropertyVector& properties, size_t size) {
        size_ = size;
        return properties_.appendAll(properties);
    }

    void initArray(JSValueType elementType) {
        elementType_ = elementType;
    }

    ~UnboxedLayout() {
        js_delete(newScript_);
        js_free(traceList_);
    }

    bool isArray() {
        return elementType_ != JSVAL_TYPE_MAGIC;
    }

    void detachFromCompartment();

    const PropertyVector& properties() const {
        return properties_;
    }

    TypeNewScript* newScript() const {
        return newScript_;
    }

    void setNewScript(TypeNewScript* newScript, bool writeBarrier = true);

    const int32_t* traceList() const {
        return traceList_;
    }

    void setTraceList(int32_t* traceList) {
        traceList_ = traceList;
    }

    const Property* lookup(JSAtom* atom) const {
        for (size_t i = 0; i < properties_.length(); i++) {
            if (properties_[i].name == atom)
                return &properties_[i];
        }
        return nullptr;
    }

    const Property* lookup(jsid id) const {
        if (JSID_IS_STRING(id))
            return lookup(JSID_TO_ATOM(id));
        return nullptr;
    }

    size_t size() const {
        return size_;
    }

    ObjectGroup* nativeGroup() const {
        return nativeGroup_;
    }

    Shape* nativeShape() const {
        return nativeShape_;
    }

    jit::JitCode* constructorCode() const {
        return constructorCode_;
    }

    void setConstructorCode(jit::JitCode* code) {
        constructorCode_ = code;
    }

    JSValueType elementType() const {
        return elementType_;
    }

    inline gc::AllocKind getAllocKind() const;

    void trace(JSTracer* trc);

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

    static bool makeNativeGroup(JSContext* cx, ObjectGroup* group);
    static bool makeConstructorCode(JSContext* cx, HandleObjectGroup group);
};




class UnboxedExpandoObject : public NativeObject
{
  public:
    static const Class class_;
};





class UnboxedPlainObject : public JSObject
{
    
    
    
    UnboxedExpandoObject* expando_;

    
    uint8_t data_[1];

  public:
    static const Class class_;

    static bool obj_lookupProperty(JSContext* cx, HandleObject obj,
                                   HandleId id, MutableHandleObject objp,
                                   MutableHandleShape propp);

    static bool obj_defineProperty(JSContext* cx, HandleObject obj, HandleId id,
                                   Handle<JSPropertyDescriptor> desc,
                                   ObjectOpResult& result);

    static bool obj_hasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp);

    static bool obj_getProperty(JSContext* cx, HandleObject obj, HandleObject receiver,
                                HandleId id, MutableHandleValue vp);

    static bool obj_setProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                                HandleValue receiver, ObjectOpResult& result);

    static bool obj_getOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                             MutableHandle<JSPropertyDescriptor> desc);

    static bool obj_deleteProperty(JSContext* cx, HandleObject obj, HandleId id,
                                   ObjectOpResult& result);

    static bool obj_enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties);
    static bool obj_watch(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable);

    const UnboxedLayout& layout() const {
        return group()->unboxedLayout();
    }

    const UnboxedLayout& layoutDontCheckGeneration() const {
        return group()->unboxedLayoutDontCheckGeneration();
    }

    uint8_t* data() {
        return &data_[0];
    }

    UnboxedExpandoObject* maybeExpando() const {
        return expando_;
    }

    void initExpando() {
        expando_ = nullptr;
    }

    
    JSObject** addressOfExpando() {
        return reinterpret_cast<JSObject**>(&expando_);
    }

    bool containsUnboxedOrExpandoProperty(ExclusiveContext* cx, jsid id) const;

    static UnboxedExpandoObject* ensureExpando(JSContext* cx, Handle<UnboxedPlainObject*> obj);

    bool setValue(ExclusiveContext* cx, const UnboxedLayout::Property& property, const Value& v);
    Value getValue(const UnboxedLayout::Property& property);

    static bool convertToNative(JSContext* cx, JSObject* obj);
    static UnboxedPlainObject* create(ExclusiveContext* cx, HandleObjectGroup group,
                                      NewObjectKind newKind);
    static JSObject* createWithProperties(ExclusiveContext* cx, HandleObjectGroup group,
                                          NewObjectKind newKind, IdValuePair* properties);

    void fillAfterConvert(ExclusiveContext* cx,
                          const AutoValueVector& values, size_t* valueCursor);

    static void trace(JSTracer* trc, JSObject* object);

    static size_t offsetOfExpando() {
        return offsetof(UnboxedPlainObject, expando_);
    }

    static size_t offsetOfData() {
        return offsetof(UnboxedPlainObject, data_[0]);
    }
};




bool
TryConvertToUnboxedLayout(ExclusiveContext* cx, Shape* templateShape,
                          ObjectGroup* group, PreliminaryObjectArray* objects);

inline gc::AllocKind
UnboxedLayout::getAllocKind() const
{
    MOZ_ASSERT(size());
    return gc::GetGCObjectKindForBytes(UnboxedPlainObject::offsetOfData() + size());
}


class UnboxedArrayObject : public JSObject
{
    
    uint8_t* elements_;

    
    uint32_t length_;

    
    
    
    
    uint32_t capacityIndexAndInitializedLength_;

    
    uint8_t inlineElements_[1];

  public:
    static const uint32_t CapacityBits = 6;
    static const uint32_t CapacityShift = 26;

    static const uint32_t CapacityMask = uint32_t(-1) << CapacityShift;
    static const uint32_t InitializedLengthMask = (1 << CapacityShift) - 1;

    static const uint32_t MaximumCapacity = InitializedLengthMask;
    static const uint32_t MinimumDynamicCapacity = 8;

    static const uint32_t CapacityArray[];

    
    static const uint32_t CapacityMatchesLengthIndex = 0;

  private:
    static inline uint32_t computeCapacity(uint32_t index, uint32_t length) {
        if (index == CapacityMatchesLengthIndex)
            return length;
        return CapacityArray[index];
    }

    static uint32_t chooseCapacityIndex(uint32_t capacity, uint32_t length);
    static uint32_t exactCapacityIndex(uint32_t capacity);

  public:
    static const Class class_;

    static bool obj_lookupProperty(JSContext* cx, HandleObject obj,
                                   HandleId id, MutableHandleObject objp,
                                   MutableHandleShape propp);

    static bool obj_defineProperty(JSContext* cx, HandleObject obj, HandleId id,
                                   Handle<JSPropertyDescriptor> desc,
                                   ObjectOpResult& result);

    static bool obj_hasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp);

    static bool obj_getProperty(JSContext* cx, HandleObject obj, HandleObject receiver,
                                HandleId id, MutableHandleValue vp);

    static bool obj_setProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                                HandleValue receiver, ObjectOpResult& result);

    static bool obj_getOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                             MutableHandle<JSPropertyDescriptor> desc);

    static bool obj_deleteProperty(JSContext* cx, HandleObject obj, HandleId id,
                                   ObjectOpResult& result);

    static bool obj_enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties);
    static bool obj_watch(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable);

    const UnboxedLayout& layout() const {
        return group()->unboxedLayout();
    }

    const UnboxedLayout& layoutDontCheckGeneration() const {
        return group()->unboxedLayoutDontCheckGeneration();
    }

    JSValueType elementType() const {
        return layoutDontCheckGeneration().elementType();
    }

    uint32_t elementSize() const {
        return UnboxedTypeSize(elementType());
    }

    static bool convertToNative(JSContext* cx, JSObject* obj);
    static UnboxedArrayObject* create(ExclusiveContext* cx, HandleObjectGroup group,
                                      uint32_t length, NewObjectKind newKind);

    void fillAfterConvert(ExclusiveContext* cx,
                          const AutoValueVector& values, size_t* valueCursor);

    static void trace(JSTracer* trc, JSObject* object);
    static void objectMoved(JSObject* obj, const JSObject* old);
    static void finalize(FreeOp* fop, JSObject* obj);

    uint8_t* elements() {
        return elements_;
    }

    bool hasInlineElements() const {
        return elements_ == &inlineElements_[0];
    }

    uint32_t length() const {
        return length_;
    }

    uint32_t initializedLength() const {
        return capacityIndexAndInitializedLength_ & InitializedLengthMask;
    }

    uint32_t capacityIndex() const {
        return (capacityIndexAndInitializedLength_ & CapacityMask) >> CapacityShift;
    }

    uint32_t capacity() const {
        return computeCapacity(capacityIndex(), length());
    }

    bool containsProperty(JSContext* cx, jsid id);

    bool setElement(ExclusiveContext* cx, size_t index, const Value& v);
    bool initElement(ExclusiveContext* cx, size_t index, const Value& v);
    Value getElement(size_t index);

    bool growElements(ExclusiveContext* cx, size_t cap);
    void shrinkElements(ExclusiveContext* cx, size_t cap);

    static uint32_t offsetOfElements() {
        return offsetof(UnboxedArrayObject, elements_);
    }
    static uint32_t offsetOfLength() {
        return offsetof(UnboxedArrayObject, length_);
    }
    static uint32_t offsetOfCapacityIndexAndInitializedLength() {
        return offsetof(UnboxedArrayObject, capacityIndexAndInitializedLength_);
    }
    static uint32_t offsetOfInlineElements() {
        return offsetof(UnboxedArrayObject, inlineElements_);
    }

  private:
    void setInlineElements() {
        elements_ = &inlineElements_[0];
    }

    void setLength(uint32_t len) {
        MOZ_ASSERT(len <= INT32_MAX);
        length_ = len;
    }

    void setInitializedLength(uint32_t initlen) {
        MOZ_ASSERT(initlen <= InitializedLengthMask);
        capacityIndexAndInitializedLength_ =
            (capacityIndexAndInitializedLength_ & CapacityMask) | initlen;
    }

    void setCapacityIndex(uint32_t index) {
        MOZ_ASSERT(index <= (CapacityMask >> CapacityShift));
        capacityIndexAndInitializedLength_ =
            (index << CapacityShift) | initializedLength();
    }
};

} 

#endif 

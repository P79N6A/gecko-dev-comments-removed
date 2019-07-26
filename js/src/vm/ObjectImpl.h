





#ifndef vm_ObjectImpl_h
#define vm_ObjectImpl_h

#include "mozilla/Assertions.h"

#include <stdint.h>

#include "jsfriendapi.h"
#include "jsinfer.h"
#include "NamespaceImports.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "gc/Marking.h"
#include "js/Value.h"
#include "vm/NumericConversions.h"
#include "vm/Shape.h"
#include "vm/String.h"

namespace js {

class ObjectImpl;
class Nursery;
class Shape;







static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *beg, Value *end)
{
#ifdef DEBUG
    for (Value *v = beg; v != end; ++v)
        v->setObject(*reinterpret_cast<JSObject *>(0x42));
#endif
}

static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(Value *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(vec, vec + len);
#endif
}

static JS_ALWAYS_INLINE void
Debug_SetValueRangeToCrashOnTouch(HeapValue *vec, size_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot *vec, uint32_t len)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) vec, len);
#endif
}

static MOZ_ALWAYS_INLINE void
Debug_SetSlotRangeToCrashOnTouch(HeapSlot *begin, HeapSlot *end)
{
#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch((Value *) begin, end - begin);
#endif
}











class PropertyId
{
    jsid id;

  public:
    bool isName() const {
        MOZ_ASSERT(JSID_IS_STRING(id) || JSID_IS_SPECIAL(id));
        return JSID_IS_STRING(id);
    }
    bool isSpecial() const {
        MOZ_ASSERT(JSID_IS_STRING(id) || JSID_IS_SPECIAL(id));
        return !isName();
    }

    PropertyId() {
        *this = PropertyId(SpecialId());
    }
    explicit PropertyId(PropertyName *name)
      : id(NON_INTEGER_ATOM_TO_JSID(name))
    { }
    explicit PropertyId(const SpecialId &sid)
      : id(SPECIALID_TO_JSID(sid))
    { }

    PropertyName * asName() const {
        return JSID_TO_STRING(id)->asAtom().asPropertyName();
    }
    SpecialId asSpecial() const {
        return JSID_TO_SPECIALID(id);
    }
    const jsid &asId() const {
        return id;
    }
    jsid &asId() {
        return id;
    }

    bool operator==(const PropertyId &rhs) const { return id == rhs.id; }
    bool operator!=(const PropertyId &rhs) const { return id != rhs.id; }
};

class DenseElementsHeader;
class SparseElementsHeader;
class Uint8ElementsHeader;
class Int8ElementsHeader;
class Uint16ElementsHeader;
class Int16ElementsHeader;
class Uint32ElementsHeader;
class Int32ElementsHeader;
class Uint8ClampedElementsHeader;
class Float32ElementsHeader;
class Float64ElementsHeader;
class Uint8ClampedElementsHeader;
class ArrayBufferElementsHeader;

enum ElementsKind {
    DenseElements,
    SparseElements,

    ArrayBufferElements,

    
    Uint8Elements,
    Int8Elements,
    Uint16Elements,
    Int16Elements,
    Uint32Elements,
    Int32Elements,
    Uint8ClampedElements,
    Float32Elements,
    Float64Elements
};

class ElementsHeader
{
  protected:
    uint32_t type;
    uint32_t length; 

    union {
        class {
            friend class DenseElementsHeader;
            uint32_t initializedLength;
            uint32_t capacity;
        } dense;
        class {
            friend class SparseElementsHeader;
            Shape *shape;
        } sparse;
        class {
            friend class ArrayBufferElementsHeader;
            JSObject * views;
        } buffer;
    };

    void staticAsserts() {
        static_assert(sizeof(ElementsHeader) == ValuesPerHeader * sizeof(Value),
                      "Elements size and values-per-Elements mismatch");
    }

  public:
    ElementsKind kind() const {
        MOZ_ASSERT(type <= ArrayBufferElements);
        return ElementsKind(type);
    }

    inline bool isDenseElements() const { return kind() == DenseElements; }
    inline bool isSparseElements() const { return kind() == SparseElements; }
    inline bool isArrayBufferElements() const { return kind() == ArrayBufferElements; }
    inline bool isUint8Elements() const { return kind() == Uint8Elements; }
    inline bool isInt8Elements() const { return kind() == Int8Elements; }
    inline bool isUint16Elements() const { return kind() == Uint16Elements; }
    inline bool isInt16Elements() const { return kind() == Int16Elements; }
    inline bool isUint32Elements() const { return kind() == Uint32Elements; }
    inline bool isInt32Elements() const { return kind() == Int32Elements; }
    inline bool isUint8ClampedElements() const { return kind() == Uint8ClampedElements; }
    inline bool isFloat32Elements() const { return kind() == Float32Elements; }
    inline bool isFloat64Elements() const { return kind() == Float64Elements; }

    inline DenseElementsHeader & asDenseElements();
    inline SparseElementsHeader & asSparseElements();
    inline ArrayBufferElementsHeader & asArrayBufferElements();
    inline Uint8ElementsHeader & asUint8Elements();
    inline Int8ElementsHeader & asInt8Elements();
    inline Uint16ElementsHeader & asUint16Elements();
    inline Int16ElementsHeader & asInt16Elements();
    inline Uint32ElementsHeader & asUint32Elements();
    inline Int32ElementsHeader & asInt32Elements();
    inline Uint8ClampedElementsHeader & asUint8ClampedElements();
    inline Float32ElementsHeader & asFloat32Elements();
    inline Float64ElementsHeader & asFloat64Elements();

    static ElementsHeader * fromElements(HeapSlot *elems) {
        return reinterpret_cast<ElementsHeader *>(uintptr_t(elems) - sizeof(ElementsHeader));
    }

    static const size_t ValuesPerHeader = 2;
};

class DenseElementsHeader : public ElementsHeader
{
  public:
    uint32_t capacity() const {
        MOZ_ASSERT(ElementsHeader::isDenseElements());
        return dense.capacity;
    }

    uint32_t initializedLength() const {
        MOZ_ASSERT(ElementsHeader::isDenseElements());
        return dense.initializedLength;
    }

    uint32_t length() const {
        MOZ_ASSERT(ElementsHeader::isDenseElements());
        return ElementsHeader::length;
    }

    bool getOwnElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       unsigned resolveFlags, PropDesc *desc);

    bool defineElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       const PropDesc &desc, bool shouldThrow, unsigned resolveFlags,
                       bool *succeeded);

    bool setElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
                    uint32_t index, const Value &v, unsigned resolveFlags, bool *succeeded);

  private:
    inline bool isDenseElements() const MOZ_DELETE;
    inline DenseElementsHeader & asDenseElements() MOZ_DELETE;

    DenseElementsHeader(const DenseElementsHeader &other) MOZ_DELETE;
    void operator=(const DenseElementsHeader &other) MOZ_DELETE;
};

class SparseElementsHeader : public ElementsHeader
{
  public:
    Shape *shape() {
        MOZ_ASSERT(ElementsHeader::isSparseElements());
        return sparse.shape;
    }

    uint32_t length() const {
        MOZ_ASSERT(ElementsHeader::isSparseElements());
        return ElementsHeader::length;
    }

    bool getOwnElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       unsigned resolveFlags, PropDesc *desc);

    bool defineElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       const PropDesc &desc, bool shouldThrow, unsigned resolveFlags,
                       bool *succeeded);

    bool setElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
                    uint32_t index, const Value &v, unsigned resolveFlags, bool *succeeded);

  private:
    inline bool isSparseElements() const MOZ_DELETE;
    inline SparseElementsHeader & asSparseElements() MOZ_DELETE;

    SparseElementsHeader(const SparseElementsHeader &other) MOZ_DELETE;
    void operator=(const SparseElementsHeader &other) MOZ_DELETE;
};

extern uint32_t JS_FASTCALL
ClampDoubleToUint8(const double x);

struct uint8_clamped {
    uint8_t val;

    uint8_clamped() { }
    uint8_clamped(const uint8_clamped& other) : val(other.val) { }

    
    uint8_clamped(uint8_t x)    { *this = x; }
    uint8_clamped(uint16_t x)   { *this = x; }
    uint8_clamped(uint32_t x)   { *this = x; }
    uint8_clamped(int8_t x)     { *this = x; }
    uint8_clamped(int16_t x)    { *this = x; }
    uint8_clamped(int32_t x)    { *this = x; }
    uint8_clamped(double x)     { *this = x; }

    uint8_clamped& operator=(const uint8_clamped& x) {
        val = x.val;
        return *this;
    }

    uint8_clamped& operator=(uint8_t x) {
        val = x;
        return *this;
    }

    uint8_clamped& operator=(uint16_t x) {
        val = (x > 255) ? 255 : uint8_t(x);
        return *this;
    }

    uint8_clamped& operator=(uint32_t x) {
        val = (x > 255) ? 255 : uint8_t(x);
        return *this;
    }

    uint8_clamped& operator=(int8_t x) {
        val = (x >= 0) ? uint8_t(x) : 0;
        return *this;
    }

    uint8_clamped& operator=(int16_t x) {
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8_t(x)
                 : 255)
              : 0;
        return *this;
    }

    uint8_clamped& operator=(int32_t x) {
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8_t(x)
                 : 255)
              : 0;
        return *this;
    }

    uint8_clamped& operator=(const double x) {
        val = uint8_t(ClampDoubleToUint8(x));
        return *this;
    }

    operator uint8_t() const {
        return val;
    }

    void staticAsserts() {
        static_assert(sizeof(uint8_clamped) == 1,
                      "uint8_clamped must be layout-compatible with uint8_t");
    }
};


template<typename T> inline const bool TypeIsFloatingPoint() { return false; }
template<> inline const bool TypeIsFloatingPoint<float>() { return true; }
template<> inline const bool TypeIsFloatingPoint<double>() { return true; }

template<typename T> inline const bool TypeIsUnsigned() { return false; }
template<> inline const bool TypeIsUnsigned<uint8_t>() { return true; }
template<> inline const bool TypeIsUnsigned<uint16_t>() { return true; }
template<> inline const bool TypeIsUnsigned<uint32_t>() { return true; }

template <typename T>
class TypedElementsHeader : public ElementsHeader
{
    T getElement(uint32_t index) {
        MOZ_ASSERT(index < length());
        return reinterpret_cast<T *>(this + 1)[index];
    }

    inline void assign(uint32_t index, double d);

    void setElement(uint32_t index, T value) {
        MOZ_ASSERT(index < length());
        reinterpret_cast<T *>(this + 1)[index] = value;
    }

  public:
    uint32_t length() const {
        MOZ_ASSERT(Uint8Elements <= kind());
        MOZ_ASSERT(kind() <= Float64Elements);
        return ElementsHeader::length;
    }

    bool getOwnElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       unsigned resolveFlags, PropDesc *desc);

    bool defineElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       const PropDesc &desc, bool shouldThrow, unsigned resolveFlags,
                       bool *succeeded);

    bool setElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
                    uint32_t index, const Value &v, unsigned resolveFlags, bool *succeeded);

  private:
    TypedElementsHeader(const TypedElementsHeader &other) MOZ_DELETE;
    void operator=(const TypedElementsHeader &other) MOZ_DELETE;
};

template<typename T> inline void
TypedElementsHeader<T>::assign(uint32_t index, double d)
{
    MOZ_ASSUME_UNREACHABLE("didn't specialize for this element type");
}

template<> inline void
TypedElementsHeader<uint8_clamped>::assign(uint32_t index, double d)
{
    double i = ToInteger(d);
    uint8_t u = (i <= 0)
                ? 0
                : (i >= 255)
                ? 255
                : uint8_t(i);
    setElement(index, uint8_clamped(u));
}

template<> inline void
TypedElementsHeader<uint8_t>::assign(uint32_t index, double d)
{
    setElement(index, uint8_t(ToUint32(d)));
}

template<> inline void
TypedElementsHeader<int8_t>::assign(uint32_t index, double d)
{
    
    setElement(index, int8_t(ToInt32(d)));
}

template<> inline void
TypedElementsHeader<uint16_t>::assign(uint32_t index, double d)
{
    setElement(index, uint16_t(ToUint32(d)));
}

template<> inline void
TypedElementsHeader<int16_t>::assign(uint32_t index, double d)
{
    
    setElement(index, int16_t(ToInt32(d)));
}

template<> inline void
TypedElementsHeader<uint32_t>::assign(uint32_t index, double d)
{
    setElement(index, ToUint32(d));
}

template<> inline void
TypedElementsHeader<int32_t>::assign(uint32_t index, double d)
{
    
    setElement(index, int32_t(ToInt32(d)));
}

template<> inline void
TypedElementsHeader<float>::assign(uint32_t index, double d)
{
    setElement(index, float(d));
}

template<> inline void
TypedElementsHeader<double>::assign(uint32_t index, double d)
{
    setElement(index, d);
}

class Uint8ElementsHeader : public TypedElementsHeader<uint8_t>
{
  private:
    inline bool isUint8Elements() const MOZ_DELETE;
    inline Uint8ElementsHeader & asUint8Elements() MOZ_DELETE;
    Uint8ElementsHeader(const Uint8ElementsHeader &other) MOZ_DELETE;
    void operator=(const Uint8ElementsHeader &other) MOZ_DELETE;
};
class Int8ElementsHeader : public TypedElementsHeader<int8_t>
{
  private:
    bool isInt8Elements() const MOZ_DELETE;
    Int8ElementsHeader & asInt8Elements() MOZ_DELETE;
    Int8ElementsHeader(const Int8ElementsHeader &other) MOZ_DELETE;
    void operator=(const Int8ElementsHeader &other) MOZ_DELETE;
};
class Uint16ElementsHeader : public TypedElementsHeader<uint16_t>
{
  private:
    bool isUint16Elements() const MOZ_DELETE;
    Uint16ElementsHeader & asUint16Elements() MOZ_DELETE;
    Uint16ElementsHeader(const Uint16ElementsHeader &other) MOZ_DELETE;
    void operator=(const Uint16ElementsHeader &other) MOZ_DELETE;
};
class Int16ElementsHeader : public TypedElementsHeader<int16_t>
{
  private:
    bool isInt16Elements() const MOZ_DELETE;
    Int16ElementsHeader & asInt16Elements() MOZ_DELETE;
    Int16ElementsHeader(const Int16ElementsHeader &other) MOZ_DELETE;
    void operator=(const Int16ElementsHeader &other) MOZ_DELETE;
};
class Uint32ElementsHeader : public TypedElementsHeader<uint32_t>
{
  private:
    bool isUint32Elements() const MOZ_DELETE;
    Uint32ElementsHeader & asUint32Elements() MOZ_DELETE;
    Uint32ElementsHeader(const Uint32ElementsHeader &other) MOZ_DELETE;
    void operator=(const Uint32ElementsHeader &other) MOZ_DELETE;
};
class Int32ElementsHeader : public TypedElementsHeader<int32_t>
{
  private:
    bool isInt32Elements() const MOZ_DELETE;
    Int32ElementsHeader & asInt32Elements() MOZ_DELETE;
    Int32ElementsHeader(const Int32ElementsHeader &other) MOZ_DELETE;
    void operator=(const Int32ElementsHeader &other) MOZ_DELETE;
};
class Float32ElementsHeader : public TypedElementsHeader<float>
{
  private:
    bool isFloat32Elements() const MOZ_DELETE;
    Float32ElementsHeader & asFloat32Elements() MOZ_DELETE;
    Float32ElementsHeader(const Float32ElementsHeader &other) MOZ_DELETE;
    void operator=(const Float32ElementsHeader &other) MOZ_DELETE;
};
class Float64ElementsHeader : public TypedElementsHeader<double>
{
  private:
    bool isFloat64Elements() const MOZ_DELETE;
    Float64ElementsHeader & asFloat64Elements() MOZ_DELETE;
    Float64ElementsHeader(const Float64ElementsHeader &other) MOZ_DELETE;
    void operator=(const Float64ElementsHeader &other) MOZ_DELETE;
};

class Uint8ClampedElementsHeader : public TypedElementsHeader<uint8_clamped>
{
  private:
    inline bool isUint8Clamped() const MOZ_DELETE;
    inline Uint8ClampedElementsHeader & asUint8ClampedElements() MOZ_DELETE;
    Uint8ClampedElementsHeader(const Uint8ClampedElementsHeader &other) MOZ_DELETE;
    void operator=(const Uint8ClampedElementsHeader &other) MOZ_DELETE;
};

class ArrayBufferElementsHeader : public ElementsHeader
{
  public:
    bool getOwnElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       unsigned resolveFlags, PropDesc *desc);

    bool defineElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index,
                       const PropDesc &desc, bool shouldThrow, unsigned resolveFlags,
                       bool *succeeded);

    bool setElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
                    uint32_t index, const Value &v, unsigned resolveFlags, bool *succeeded);

    JSObject **viewList() { return &buffer.views; }

  private:
    inline bool isArrayBufferElements() const MOZ_DELETE;
    inline ArrayBufferElementsHeader & asArrayBufferElements() MOZ_DELETE;

    ArrayBufferElementsHeader(const ArrayBufferElementsHeader &other) MOZ_DELETE;
    void operator=(const ArrayBufferElementsHeader &other) MOZ_DELETE;
};

inline DenseElementsHeader &
ElementsHeader::asDenseElements()
{
    MOZ_ASSERT(isDenseElements());
    return *static_cast<DenseElementsHeader *>(this);
}

inline SparseElementsHeader &
ElementsHeader::asSparseElements()
{
    MOZ_ASSERT(isSparseElements());
    return *static_cast<SparseElementsHeader *>(this);
}

inline Uint8ElementsHeader &
ElementsHeader::asUint8Elements()
{
    MOZ_ASSERT(isUint8Elements());
    return *static_cast<Uint8ElementsHeader *>(this);
}

inline Int8ElementsHeader &
ElementsHeader::asInt8Elements()
{
    MOZ_ASSERT(isInt8Elements());
    return *static_cast<Int8ElementsHeader *>(this);
}

inline Uint16ElementsHeader &
ElementsHeader::asUint16Elements()
{
    MOZ_ASSERT(isUint16Elements());
    return *static_cast<Uint16ElementsHeader *>(this);
}

inline Int16ElementsHeader &
ElementsHeader::asInt16Elements()
{
    MOZ_ASSERT(isInt16Elements());
    return *static_cast<Int16ElementsHeader *>(this);
}

inline Uint32ElementsHeader &
ElementsHeader::asUint32Elements()
{
    MOZ_ASSERT(isUint32Elements());
    return *static_cast<Uint32ElementsHeader *>(this);
}

inline Int32ElementsHeader &
ElementsHeader::asInt32Elements()
{
    MOZ_ASSERT(isInt32Elements());
    return *static_cast<Int32ElementsHeader *>(this);
}

inline Uint8ClampedElementsHeader &
ElementsHeader::asUint8ClampedElements()
{
    MOZ_ASSERT(isUint8ClampedElements());
    return *static_cast<Uint8ClampedElementsHeader *>(this);
}

inline Float32ElementsHeader &
ElementsHeader::asFloat32Elements()
{
    MOZ_ASSERT(isFloat32Elements());
    return *static_cast<Float32ElementsHeader *>(this);
}

inline Float64ElementsHeader &
ElementsHeader::asFloat64Elements()
{
    MOZ_ASSERT(isFloat64Elements());
    return *static_cast<Float64ElementsHeader *>(this);
}

inline ArrayBufferElementsHeader &
ElementsHeader::asArrayBufferElements()
{
    MOZ_ASSERT(isArrayBufferElements());
    return *static_cast<ArrayBufferElementsHeader *>(this);
}

class ArrayObject;
class ArrayBufferObject;









extern bool
ArraySetLength(JSContext *cx, Handle<ArrayObject*> obj, HandleId id, unsigned attrs,
               HandleValue value, bool setterIsStrict);






































































class ObjectElements
{
  public:
    enum Flags {
        CONVERT_DOUBLE_ELEMENTS = 0x1,
        ASMJS_ARRAY_BUFFER = 0x2,

        
        
        NONWRITABLE_ARRAY_LENGTH = 0x4
    };

  private:
    friend class ::JSObject;
    friend class ObjectImpl;
    friend class ArrayObject;
    friend class ArrayBufferObject;
    friend class Nursery;

    friend bool
    ArraySetLength(JSContext *cx, Handle<ArrayObject*> obj, HandleId id, unsigned attrs,
                   HandleValue value, bool setterIsStrict);

    
    uint32_t flags;

    







    uint32_t initializedLength;

    




    
    uint32_t capacity;

    
    uint32_t length;

    void staticAsserts() {
        static_assert(sizeof(ObjectElements) == VALUES_PER_HEADER * sizeof(Value),
                      "Elements size and values-per-Elements mismatch");
    }

    bool shouldConvertDoubleElements() const {
        return flags & CONVERT_DOUBLE_ELEMENTS;
    }
    void setShouldConvertDoubleElements() {
        flags |= CONVERT_DOUBLE_ELEMENTS;
    }
    bool isAsmJSArrayBuffer() const {
        return flags & ASMJS_ARRAY_BUFFER;
    }
    void setIsAsmJSArrayBuffer() {
        flags |= ASMJS_ARRAY_BUFFER;
    }
    bool hasNonwritableArrayLength() const {
        return flags & NONWRITABLE_ARRAY_LENGTH;
    }
    void setNonwritableArrayLength() {
        flags |= NONWRITABLE_ARRAY_LENGTH;
    }

  public:
    ObjectElements(uint32_t capacity, uint32_t length)
      : flags(0), initializedLength(0), capacity(capacity), length(length)
    {}

    HeapSlot *elements() {
        return reinterpret_cast<HeapSlot*>(uintptr_t(this) + sizeof(ObjectElements));
    }
    static ObjectElements * fromElements(HeapSlot *elems) {
        return reinterpret_cast<ObjectElements*>(uintptr_t(elems) - sizeof(ObjectElements));
    }

    static int offsetOfFlags() {
        return int(offsetof(ObjectElements, flags)) - int(sizeof(ObjectElements));
    }
    static int offsetOfInitializedLength() {
        return int(offsetof(ObjectElements, initializedLength)) - int(sizeof(ObjectElements));
    }
    static int offsetOfCapacity() {
        return int(offsetof(ObjectElements, capacity)) - int(sizeof(ObjectElements));
    }
    static int offsetOfLength() {
        return int(offsetof(ObjectElements, length)) - int(sizeof(ObjectElements));
    }

    static bool ConvertElementsToDoubles(JSContext *cx, uintptr_t elements);

    static const size_t VALUES_PER_HEADER = 2;
};


extern HeapSlot *const emptyObjectElements;

struct Class;
struct GCMarker;
struct ObjectOps;
class Shape;

class NewObjectCache;
class TaggedProto;

inline Value
ObjectValue(ObjectImpl &obj);

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(js::Value v, JSCompartment *comp);
#endif
















































class ObjectImpl : public gc::BarrieredCell<ObjectImpl>
{
    friend Zone *js::gc::BarrieredCell<ObjectImpl>::zone() const;
    friend Zone *js::gc::BarrieredCell<ObjectImpl>::zoneFromAnyThread() const;

  protected:
    



    HeapPtrShape shape_;

    




    HeapPtrTypeObject type_;

    HeapSlot *slots;     
    HeapSlot *elements;  

    friend bool
    ArraySetLength(JSContext *cx, Handle<ArrayObject*> obj, HandleId id, unsigned attrs,
                   HandleValue value, bool setterIsStrict);

  private:
    static void staticAsserts() {
        static_assert(sizeof(ObjectImpl) == sizeof(shadow::Object),
                      "shadow interface must match actual implementation");
        static_assert(sizeof(ObjectImpl) % sizeof(Value) == 0,
                      "fixed slots after an object must be aligned");

        static_assert(offsetof(ObjectImpl, shape_) == offsetof(shadow::Object, shape),
                      "shadow shape must match actual shape");
        static_assert(offsetof(ObjectImpl, type_) == offsetof(shadow::Object, type),
                      "shadow type must match actual type");
        static_assert(offsetof(ObjectImpl, slots) == offsetof(shadow::Object, slots),
                      "shadow slots must match actual slots");
        static_assert(offsetof(ObjectImpl, elements) == offsetof(shadow::Object, _1),
                      "shadow placeholder must match actual elements");
    }

    JSObject * asObjectPtr() { return reinterpret_cast<JSObject *>(this); }
    const JSObject * asObjectPtr() const { return reinterpret_cast<const JSObject *>(this); }

    friend inline Value ObjectValue(ObjectImpl &obj);

    

  public:
    JSObject * getProto() const {
        return type_->proto;
    }

    const Class *getClass() const {
        return type_->clasp;
    }

    static inline bool
    isExtensible(ExclusiveContext *cx, Handle<ObjectImpl*> obj, bool *extensible);

    
    
    
    
    bool nonProxyIsExtensible() const {
        MOZ_ASSERT(!isProxy());

        
        return !lastProperty()->hasObjectFlag(BaseShape::NOT_EXTENSIBLE);
    }

#ifdef DEBUG
    bool isProxy() const;
#endif

    
    
    static bool
    preventExtensions(JSContext *cx, Handle<ObjectImpl*> obj);

    HeapSlotArray getDenseElements() {
        JS_ASSERT(isNative());
        return HeapSlotArray(elements);
    }
    const Value &getDenseElement(uint32_t idx) {
        JS_ASSERT(isNative());
        MOZ_ASSERT(idx < getDenseInitializedLength());
        return elements[idx];
    }
    bool containsDenseElement(uint32_t idx) {
        JS_ASSERT(isNative());
        return idx < getDenseInitializedLength() && !elements[idx].isMagic(JS_ELEMENTS_HOLE);
    }
    uint32_t getDenseInitializedLength() {
        JS_ASSERT(isNative());
        return getElementsHeader()->initializedLength;
    }
    uint32_t getDenseCapacity() {
        JS_ASSERT(isNative());
        return getElementsHeader()->capacity;
    }

    bool makeElementsSparse(JSContext *cx) {
        JS_NEW_OBJECT_REPRESENTATION_ONLY();
        MOZ_ASSUME_UNREACHABLE("NYI");
    }

  protected:
#ifdef DEBUG
    void checkShapeConsistency();
#else
    void checkShapeConsistency() { }
#endif

    Shape *
    replaceWithNewEquivalentShape(ExclusiveContext *cx,
                                  Shape *existingShape, Shape *newShape = nullptr);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(ExclusiveContext *cx,  uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);
    bool clearFlag(ExclusiveContext *cx,  uint32_t flag);

    bool toDictionaryMode(ThreadSafeContext *cx);

  private:
    friend class Nursery;

    



    void getSlotRangeUnchecked(uint32_t start, uint32_t length,
                               HeapSlot **fixedStart, HeapSlot **fixedEnd,
                               HeapSlot **slotsStart, HeapSlot **slotsEnd)
    {
        MOZ_ASSERT(start + length >= start);

        uint32_t fixed = numFixedSlots();
        if (start < fixed) {
            if (start + length < fixed) {
                *fixedStart = &fixedSlots()[start];
                *fixedEnd = &fixedSlots()[start + length];
                *slotsStart = *slotsEnd = nullptr;
            } else {
                uint32_t localCopy = fixed - start;
                *fixedStart = &fixedSlots()[start];
                *fixedEnd = &fixedSlots()[start + localCopy];
                *slotsStart = &slots[0];
                *slotsEnd = &slots[length - localCopy];
            }
        } else {
            *fixedStart = *fixedEnd = nullptr;
            *slotsStart = &slots[start - fixed];
            *slotsEnd = &slots[start - fixed + length];
        }
    }

    void getSlotRange(uint32_t start, uint32_t length,
                      HeapSlot **fixedStart, HeapSlot **fixedEnd,
                      HeapSlot **slotsStart, HeapSlot **slotsEnd)
    {
        MOZ_ASSERT(slotInRange(start + length, SENTINEL_ALLOWED));
        getSlotRangeUnchecked(start, length, fixedStart, fixedEnd, slotsStart, slotsEnd);
    }

  protected:
    friend struct GCMarker;
    friend class Shape;
    friend class NewObjectCache;

    void invalidateSlotRange(uint32_t start, uint32_t length) {
#ifdef DEBUG
        HeapSlot *fixedStart, *fixedEnd, *slotsStart, *slotsEnd;
        getSlotRange(start, length, &fixedStart, &fixedEnd, &slotsStart, &slotsEnd);
        Debug_SetSlotRangeToCrashOnTouch(fixedStart, fixedEnd);
        Debug_SetSlotRangeToCrashOnTouch(slotsStart, slotsEnd);
#endif 
    }

    void initializeSlotRange(uint32_t start, uint32_t count);

    



    void initSlotRange(uint32_t start, const Value *vector, uint32_t length);

    



    void copySlotRange(uint32_t start, const Value *vector, uint32_t length);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    



    bool slotInRange(uint32_t slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
#endif

    
    static const uint32_t SLOT_CAPACITY_MIN = 8;

    HeapSlot *fixedSlots() const {
        return reinterpret_cast<HeapSlot *>(uintptr_t(this) + sizeof(ObjectImpl));
    }

    friend class ElementsHeader;
    friend class DenseElementsHeader;
    friend class SparseElementsHeader;

    enum DenseElementsResult {
        Failure,
        ConvertToSparse,
        Succeeded
    };

    DenseElementsResult ensureDenseElementsInitialized(JSContext *cx, uint32_t index,
                                                       uint32_t extra)
    {
        JS_NEW_OBJECT_REPRESENTATION_ONLY();
        MOZ_ASSUME_UNREACHABLE("NYI");
    }

    




  public:
    js::TaggedProto getTaggedProto() const {
        return TaggedProto(getProto());
    }

    Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    bool generateOwnShape(ExclusiveContext *cx, js::Shape *newShape = nullptr) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

    JSCompartment *compartment() const {
        return lastProperty()->base()->compartment();
    }

    bool isNative() const {
        return lastProperty()->isNative();
    }

    types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return type_;
    }

    uint32_t numFixedSlots() const {
        return reinterpret_cast<const shadow::Object *>(this)->numFixedSlots();
    }

    



    bool hasSingletonType() const { return !!type_->singleton; }

    



    bool hasLazyType() const { return type_->lazy(); }

    uint32_t slotSpan() const {
        if (inDictionaryMode())
            return lastProperty()->base()->slotSpan();
        return lastProperty()->slotSpan();
    }

    
    uint32_t numDynamicSlots() const {
        return dynamicSlotsCount(numFixedSlots(), slotSpan());
    }

    Shape *nativeLookup(ExclusiveContext *cx, jsid id);
    Shape *nativeLookup(ExclusiveContext *cx, PropertyId pid) {
        return nativeLookup(cx, pid.asId());
    }
    Shape *nativeLookup(ExclusiveContext *cx, PropertyName *name) {
        return nativeLookup(cx, NameToId(name));
    }

    bool nativeContains(ExclusiveContext *cx, jsid id) {
        return nativeLookup(cx, id) != nullptr;
    }
    bool nativeContains(ExclusiveContext *cx, PropertyName* name) {
        return nativeLookup(cx, name) != nullptr;
    }
    bool nativeContains(ExclusiveContext *cx, Shape* shape) {
        return nativeLookup(cx, shape->propid()) == shape;
    }

    



    Shape *nativeLookupPure(jsid id);
    Shape *nativeLookupPure(PropertyId pid) {
        return nativeLookupPure(pid.asId());
    }
    Shape *nativeLookupPure(PropertyName *name) {
        return nativeLookupPure(NameToId(name));
    }

    bool nativeContainsPure(jsid id) {
        return nativeLookupPure(id) != nullptr;
    }
    bool nativeContainsPure(PropertyName* name) {
        return nativeContainsPure(NameToId(name));
    }
    bool nativeContainsPure(Shape* shape) {
        return nativeLookupPure(shape->propid()) == shape;
    }

    const JSClass *getJSClass() const {
        return Jsvalify(getClass());
    }
    bool hasClass(const Class *c) const {
        return getClass() == c;
    }
    const ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    








    bool isDelegate() const {
        return lastProperty()->hasObjectFlag(BaseShape::DELEGATE);
    }

    




    bool inDictionaryMode() const {
        return lastProperty()->inDictionary();
    }

    const Value &getSlot(uint32_t slot) const {
        MOZ_ASSERT(slotInRange(slot));
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    HeapSlot *getSlotAddressUnchecked(uint32_t slot) {
        uint32_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    HeapSlot *getSlotAddress(uint32_t slot) {
        




        MOZ_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    HeapSlot &getSlotRef(uint32_t slot) {
        MOZ_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    HeapSlot &nativeGetSlotRef(uint32_t slot) {
        JS_ASSERT(isNative() && slot < slotSpan());
        return getSlotRef(slot);
    }
    const Value &nativeGetSlot(uint32_t slot) const {
        JS_ASSERT(isNative() && slot < slotSpan());
        return getSlot(slot);
    }

    void setSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    inline void setCrossCompartmentSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slotInRange(slot));
        getSlotRef(slot).set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    void initSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(getSlot(slot).isUndefined());
        MOZ_ASSERT(slotInRange(slot));
        MOZ_ASSERT(IsObjectValueInCompartment(value, compartment()));
        initSlotUnchecked(slot, value);
    }

    void initCrossCompartmentSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(getSlot(slot).isUndefined());
        MOZ_ASSERT(slotInRange(slot));
        initSlotUnchecked(slot, value);
    }

    void initSlotUnchecked(uint32_t slot, const Value &value) {
        getSlotAddressUnchecked(slot)->init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    

    HeapSlot &getFixedSlotRef(uint32_t slot) {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const Value &getFixedSlot(uint32_t slot) const {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    void setFixedSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].set(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    void initFixedSlot(uint32_t slot, const Value &value) {
        MOZ_ASSERT(slot < numFixedSlots());
        fixedSlots()[slot].init(this->asObjectPtr(), HeapSlot::Slot, slot, value);
    }

    





    static uint32_t dynamicSlotsCount(uint32_t nfixed, uint32_t span) {
        if (span <= nfixed)
            return 0;
        span -= nfixed;
        if (span <= SLOT_CAPACITY_MIN)
            return SLOT_CAPACITY_MIN;

        uint32_t slots = mozilla::RoundUpPow2(span);
        MOZ_ASSERT(slots >= span);
        return slots;
    }

    
    size_t tenuredSizeOfThis() const {
        return js::gc::Arena::thingSize(tenuredGetAllocKind());
    }

    

    ObjectElements * getElementsHeader() const {
        return ObjectElements::fromElements(elements);
    }

    ElementsHeader & elementsHeader() const {
        JS_NEW_OBJECT_REPRESENTATION_ONLY();
        return *ElementsHeader::fromElements(elements);
    }

    inline HeapSlot *fixedElements() const {
        static_assert(2 * sizeof(Value) == sizeof(ObjectElements),
                      "when elements are stored inline, the first two "
                      "slots will hold the ObjectElements header");
        return &fixedSlots()[2];
    }

    void setFixedElements() { this->elements = fixedElements(); }

    inline bool hasDynamicElements() const {
        






        return !hasEmptyElements() && elements != fixedElements();
    }

    inline bool hasFixedElements() const {
        return elements == fixedElements();
    }

    inline bool hasEmptyElements() const {
        return elements == emptyObjectElements;
    }

    
    static ThingRootKind rootKind() { return THING_ROOT_OBJECT; }

    inline void privateWriteBarrierPre(void **oldval);

    void privateWriteBarrierPost(void **pprivate) {
#ifdef JSGC_GENERATIONAL
        shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->putCell(reinterpret_cast<js::gc::Cell **>(pprivate));
#endif
    }

    void markChildren(JSTracer *trc);

    

    inline void *&privateRef(uint32_t nfixed) const { 
        




        MOZ_ASSERT(nfixed == numFixedSlots());
        MOZ_ASSERT(hasPrivate());
        HeapSlot *end = &fixedSlots()[nfixed];
        return *reinterpret_cast<void**>(end);
    }

    bool hasPrivate() const {
        return getClass()->hasPrivate();
    }
    void *getPrivate() const {
        return privateRef(numFixedSlots());
    }
    void setPrivate(void *data) {
        void **pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = data;
    }

    void setPrivateGCThing(gc::Cell *cell) {
        void **pprivate = &privateRef(numFixedSlots());
        privateWriteBarrierPre(pprivate);
        *pprivate = reinterpret_cast<void *>(cell);
        privateWriteBarrierPost(pprivate);
    }

    void setPrivateUnbarriered(void *data) {
        void **pprivate = &privateRef(numFixedSlots());
        *pprivate = data;
    }
    void initPrivate(void *data) {
        privateRef(numFixedSlots()) = data;
    }

    
    inline void *getPrivate(uint32_t nfixed) const {
        return privateRef(nfixed);
    }

    
    static size_t offsetOfShape() { return offsetof(ObjectImpl, shape_); }
    HeapPtrShape *addressOfShape() { return &shape_; }

    static size_t offsetOfType() { return offsetof(ObjectImpl, type_); }
    HeapPtrTypeObject *addressOfType() { return &type_; }

    static size_t offsetOfElements() { return offsetof(ObjectImpl, elements); }
    static size_t offsetOfFixedElements() {
        return sizeof(ObjectImpl) + sizeof(ObjectElements);
    }

    static size_t getFixedSlotOffset(size_t slot) {
        return sizeof(ObjectImpl) + slot * sizeof(Value);
    }
    static size_t getPrivateDataOffset(size_t nfixed) { return getFixedSlotOffset(nfixed); }
    static size_t offsetOfSlots() { return offsetof(ObjectImpl, slots); }
};

namespace gc {

template <>
JS_ALWAYS_INLINE Zone *
BarrieredCell<ObjectImpl>::zone() const
{
    const ObjectImpl* obj = static_cast<const ObjectImpl*>(this);
    JS::Zone *zone = obj->shape_->zone();
    JS_ASSERT(CurrentThreadCanAccessZone(zone));
    return zone;
}

template <>
JS_ALWAYS_INLINE Zone *
BarrieredCell<ObjectImpl>::zoneFromAnyThread() const
{
    const ObjectImpl* obj = static_cast<const ObjectImpl*>(this);
    return obj->shape_->zoneFromAnyThread();
}


template<>
 inline bool
BarrieredCell<ObjectImpl>::isNullLike(ObjectImpl *obj)
{
    return IsNullTaggedPointer(obj);
}

template<>
 inline void
BarrieredCell<ObjectImpl>::writeBarrierPost(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    if (IsNullTaggedPointer(obj))
        return;
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->putCell((Cell **)addr);
#endif
}

template<>
 inline void
BarrieredCell<ObjectImpl>::writeBarrierPostRelocate(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->putRelocatableCell((Cell **)addr);
#endif
}

template<>
 inline void
BarrieredCell<ObjectImpl>::writeBarrierPostRemove(ObjectImpl *obj, void *addr)
{
#ifdef JSGC_GENERATIONAL
    obj->shadowRuntimeFromAnyThread()->gcStoreBufferPtr()->removeRelocatableCell((Cell **)addr);
#endif
}

} 

inline void
ObjectImpl::privateWriteBarrierPre(void **oldval)
{
#ifdef JSGC_INCREMENTAL
    JS::shadow::Zone *shadowZone = this->shadowZone();
    if (shadowZone->needsBarrier()) {
        if (*oldval && getClass()->trace)
            getClass()->trace(shadowZone->barrierTracer(), this->asObjectPtr());
    }
#endif
}

inline Value
ObjectValue(ObjectImpl &obj)
{
    Value v;
    v.setObject(*obj.asObjectPtr());
    return v;
}

inline Handle<JSObject*>
Downcast(Handle<ObjectImpl*> obj)
{
    return Handle<JSObject*>::fromMarkedLocation(reinterpret_cast<JSObject* const*>(obj.address()));
}

#ifdef DEBUG
static inline bool
IsObjectValueInCompartment(js::Value v, JSCompartment *comp)
{
    if (!v.isObject())
        return true;
    return reinterpret_cast<ObjectImpl*>(&v.toObject())->compartment() == comp;
}
#endif

extern JSObject *
ArrayBufferDelegate(JSContext *cx, Handle<ObjectImpl*> obj);


bool
GetOwnElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index, unsigned resolveFlags,
              PropDesc *desc);
extern bool
GetOwnProperty(JSContext *cx, Handle<ObjectImpl*> obj, PropertyId pid, unsigned resolveFlags,
               PropDesc *desc);
inline bool
GetOwnProperty(JSContext *cx, Handle<ObjectImpl*> obj, Handle<PropertyName*> name,
               unsigned resolveFlags, PropDesc *desc)
{
    return GetOwnProperty(cx, obj, PropertyId(name), resolveFlags, desc);
}
inline bool
GetOwnProperty(JSContext *cx, Handle<ObjectImpl*> obj, Handle<SpecialId> sid, unsigned resolveFlags,
               PropDesc *desc)
{
    return GetOwnProperty(cx, obj, PropertyId(sid), resolveFlags, desc);
}


extern bool
GetElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver, uint32_t index,
           unsigned resolveFlags, Value *vp);
extern bool
GetProperty(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
            Handle<PropertyId> pid, unsigned resolveFlags, MutableHandle<Value> vp);
inline bool
GetProperty(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
            Handle<PropertyName*> name, unsigned resolveFlags, MutableHandle<Value> vp)
{
    Rooted<PropertyId> pid(cx, PropertyId(name));
    return GetProperty(cx, obj, receiver, pid, resolveFlags, vp);
}
inline bool
GetProperty(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver,
            Handle<SpecialId> sid, unsigned resolveFlags, MutableHandle<Value> vp)
{
    Rooted<PropertyId> pid(cx, PropertyId(sid));
    return GetProperty(cx, obj, receiver, pid, resolveFlags, vp);
}

extern bool
DefineElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index, const PropDesc &desc,
              bool shouldThrow, unsigned resolveFlags, bool *succeeded);


extern bool
SetElement(JSContext *cx, Handle<ObjectImpl*> obj, Handle<ObjectImpl*> receiver, uint32_t index,
           const Value &v, unsigned resolveFlags, bool *succeeded);

extern bool
HasElement(JSContext *cx, Handle<ObjectImpl*> obj, uint32_t index, unsigned resolveFlags,
           bool *found);

template <> struct GCMethods<PropertyId>
{
    static PropertyId initial() { return PropertyId(); }
    static ThingRootKind kind() { return THING_ROOT_PROPERTY_ID; }
    static bool poisoned(PropertyId propid) { return IsPoisonedId(propid.asId()); }
};

} 

#endif 

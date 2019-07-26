






#ifndef ObjectImpl_h___
#define ObjectImpl_h___

#include "mozilla/Assertions.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/StandardInteger.h"

#include "jsfriendapi.h"
#include "jsinfer.h"

#include "gc/Barrier.h"
#include "gc/Heap.h"
#include "js/Value.h"
#include "vm/NumericConversions.h"
#include "vm/String.h"

namespace js {

class Debugger;
class ObjectImpl;
ForwardDeclare(Shape);

class AutoPropDescArrayRooter;

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline StrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
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

    PropDesc(const Value &v)
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
    friend class AutoPropDescArrayRooter;
    friend void JS::AutoGCRooter::trace(JSTracer *trc);

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

    








    void initFromPropertyDescriptor(const PropertyDescriptor &desc);
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
        return get_.isUndefined() ? NULL : &get_.toObject();
    }
    JSObject * setterObject() const {
        MOZ_ASSERT(!isUndefined());
        MOZ_ASSERT(hasSet());
        return set_.isUndefined() ? NULL : &set_.toObject();
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

    



    PropertyOp getter() const {
        return CastAsPropertyOp(get_.isUndefined() ? NULL : &get_.toObject());
    }
    StrictPropertyOp setter() const {
        return CastAsStrictPropertyOp(set_.isUndefined() ? NULL : &set_.toObject());
    }

    




    bool checkGetter(JSContext *cx);
    bool checkSetter(JSContext *cx);

    bool unwrapDebuggerObjectsInto(JSContext *cx, Debugger *dbg, HandleObject obj,
                                   PropDesc *unwrapped) const;

    bool wrapInto(JSContext *cx, HandleObject obj, const jsid &id, jsid *wrappedId,
                  PropDesc *wrappedDesc) const;

    class AutoRooter : private AutoGCRooter
    {
      public:
        explicit AutoRooter(JSContext *cx, PropDesc *pd_
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
          : AutoGCRooter(cx, PROPDESC), pd(pd_), skip(cx, pd_)
        {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        }

        friend void AutoGCRooter::trace(JSTracer *trc);

      private:
        PropDesc *pd;
        SkipRoot skip;
        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
     };
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
            RawShape shape;
        } sparse;
        class {
            friend class ArrayBufferElementsHeader;
            JSObject * views;
        } buffer;
    };

    void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(ElementsHeader) == ValuesPerHeader * sizeof(Value),
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
    RawShape shape() {
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
        MOZ_STATIC_ASSERT(sizeof(uint8_clamped) == 1,
                          "uint8_clamped must be layout-compatible with uint8_t");
    }
};


template<typename T> static inline const bool TypeIsFloatingPoint() { return false; }
template<> inline const bool TypeIsFloatingPoint<float>() { return true; }
template<> inline const bool TypeIsFloatingPoint<double>() { return true; }

template<typename T> static inline const bool TypeIsUnsigned() { return false; }
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
    MOZ_NOT_REACHED("didn't specialize for this element type");
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

class ArrayBufferObject;






























































class ObjectElements
{
  public:
    enum Flags {
        CONVERT_DOUBLE_ELEMENTS = 0x1,
        ASMJS_ARRAY_BUFFER = 0x2
    };

  private:
    friend class ::JSObject;
    friend class ObjectImpl;
    friend class ArrayBufferObject;

    
    uint32_t flags;

    







    uint32_t initializedLength;

    




    
    uint32_t capacity;

    
    uint32_t length;

    void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(ObjectElements) == VALUES_PER_HEADER * sizeof(Value),
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

  public:
    ObjectElements(uint32_t capacity, uint32_t length)
      : flags(0), initializedLength(0), capacity(capacity), length(length)
    {}

    HeapSlot *elements() { return (HeapSlot *)(uintptr_t(this) + sizeof(ObjectElements)); }
    static ObjectElements * fromElements(HeapSlot *elems) {
        return (ObjectElements *)(uintptr_t(elems) - sizeof(ObjectElements));
    }

    static int offsetOfFlags() {
        return (int)offsetof(ObjectElements, flags) - (int)sizeof(ObjectElements);
    }
    static int offsetOfInitializedLength() {
        return (int)offsetof(ObjectElements, initializedLength) - (int)sizeof(ObjectElements);
    }
    static int offsetOfCapacity() {
        return (int)offsetof(ObjectElements, capacity) - (int)sizeof(ObjectElements);
    }
    static int offsetOfLength() {
        return (int)offsetof(ObjectElements, length) - (int)sizeof(ObjectElements);
    }

    static bool ConvertElementsToDoubles(JSContext *cx, uintptr_t elements);

    static const size_t VALUES_PER_HEADER = 2;
};


extern HeapSlot *emptyObjectElements;

struct Class;
struct GCMarker;
struct ObjectOps;
class Shape;

class NewObjectCache;

inline Value
ObjectValue(ObjectImpl &obj);
















































class ObjectImpl : public gc::Cell
{
  protected:
    



    HeapPtrShape shape_;

    




    HeapPtrTypeObject type_;

    HeapSlot *slots;     
    HeapSlot *elements;  

  private:
    static void staticAsserts() {
        MOZ_STATIC_ASSERT(sizeof(ObjectImpl) == sizeof(shadow::Object),
                          "shadow interface must match actual implementation");
        MOZ_STATIC_ASSERT(sizeof(ObjectImpl) % sizeof(Value) == 0,
                          "fixed slots after an object must be aligned");

        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, shape_) == offsetof(shadow::Object, shape),
                          "shadow shape must match actual shape");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, type_) == offsetof(shadow::Object, type),
                          "shadow type must match actual type");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, slots) == offsetof(shadow::Object, slots),
                          "shadow slots must match actual slots");
        MOZ_STATIC_ASSERT(offsetof(ObjectImpl, elements) == offsetof(shadow::Object, _1),
                          "shadow placeholder must match actual elements");
    }

    JSObject * asObjectPtr() { return reinterpret_cast<JSObject *>(this); }
    const JSObject * asObjectPtr() const { return reinterpret_cast<const JSObject *>(this); }

    friend inline Value ObjectValue(ObjectImpl &obj);

    

  public:
    JSObject * getProto() const {
        return type_->proto;
    }

    Class *getClass() const {
        return type_->clasp;
    }

    inline bool isExtensible() const;

    inline HeapSlotArray getDenseElements();
    inline const Value & getDenseElement(uint32_t idx);
    inline bool containsDenseElement(uint32_t idx);
    inline uint32_t getDenseInitializedLength();

    bool makeElementsSparse(JSContext *cx) {
        NEW_OBJECT_REPRESENTATION_ONLY();

        MOZ_NOT_REACHED("NYI");
        return false;
    }

  protected:
#ifdef DEBUG
    void checkShapeConsistency();
#else
    void checkShapeConsistency() { }
#endif

  private:
    



    inline void getSlotRangeUnchecked(uint32_t start, uint32_t length,
                                      HeapSlot **fixedStart, HeapSlot **fixedEnd,
                                      HeapSlot **slotsStart, HeapSlot **slotsEnd);
    inline void getSlotRange(uint32_t start, uint32_t length,
                             HeapSlot **fixedStart, HeapSlot **fixedEnd,
                             HeapSlot **slotsStart, HeapSlot **slotsEnd);

  protected:
    friend struct GCMarker;
    friend class Shape;
    friend class NewObjectCache;

    inline void invalidateSlotRange(uint32_t start, uint32_t count);
    inline void initializeSlotRange(uint32_t start, uint32_t count);

    



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
        NEW_OBJECT_REPRESENTATION_ONLY();

        MOZ_NOT_REACHED("NYI");
        return Failure;
    }

    




  public:
    Shape * lastProperty() const {
        MOZ_ASSERT(shape_);
        return shape_;
    }

    inline bool isNative() const;

    types::TypeObject *type() const {
        MOZ_ASSERT(!hasLazyType());
        return type_;
    }

    uint32_t numFixedSlots() const {
        return reinterpret_cast<const shadow::Object *>(this)->numFixedSlots();
    }

    



    bool hasSingletonType() const { return !!type_->singleton; }

    



    bool hasLazyType() const { return type_->lazy(); }

    inline uint32_t slotSpan() const;

    
    inline uint32_t numDynamicSlots() const;

    RawShape nativeLookup(JSContext *cx, jsid id);
    inline RawShape nativeLookup(JSContext *cx, PropertyId pid);
    inline RawShape nativeLookup(JSContext *cx, PropertyName *name);

    inline bool nativeContains(JSContext *cx, jsid id);
    inline bool nativeContains(JSContext *cx, PropertyName* name);
    inline bool nativeContains(JSContext *cx, Shape* shape);

    inline JSClass *getJSClass() const;
    inline bool hasClass(const Class *c) const;
    inline const ObjectOps *getOps() const;

    








    inline bool isDelegate() const;

    




    inline bool inDictionaryMode() const;

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

    inline HeapSlot &nativeGetSlotRef(uint32_t slot);
    inline const Value &nativeGetSlot(uint32_t slot) const;

    inline void setSlot(uint32_t slot, const Value &value);
    inline void setCrossCompartmentSlot(uint32_t slot, const Value &value);
    inline void initSlot(uint32_t slot, const Value &value);
    inline void initCrossCompartmentSlot(uint32_t slot, const Value &value);
    inline void initSlotUnchecked(uint32_t slot, const Value &value);

    

    HeapSlot &getFixedSlotRef(uint32_t slot) {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const Value &getFixedSlot(uint32_t slot) const {
        MOZ_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    inline void setFixedSlot(uint32_t slot, const Value &value);
    inline void initFixedSlot(uint32_t slot, const Value &value);

    





    static inline uint32_t dynamicSlotsCount(uint32_t nfixed, uint32_t span);

    
    inline size_t sizeOfThis() const;

    

    ObjectElements * getElementsHeader() const {
        return ObjectElements::fromElements(elements);
    }

    ElementsHeader & elementsHeader() const {
        NEW_OBJECT_REPRESENTATION_ONLY();
        return *ElementsHeader::fromElements(elements);
    }

    inline HeapSlot *fixedElements() const {
        MOZ_STATIC_ASSERT(2 * sizeof(Value) == sizeof(ObjectElements),
                          "when elements are stored inline, the first two "
                          "slots will hold the ObjectElements header");
        return &fixedSlots()[2];
    }

    void setFixedElements() { this->elements = fixedElements(); }

    inline bool hasDynamicElements() const {
        






        return elements != emptyObjectElements && elements != fixedElements();
    }

    inline bool hasEmptyElements() const {
        return elements == emptyObjectElements;
    }

    
    JS_ALWAYS_INLINE Zone *zone() const;
    static inline ThingRootKind rootKind() { return THING_ROOT_OBJECT; }
    static inline void readBarrier(ObjectImpl *obj);
    static inline void writeBarrierPre(ObjectImpl *obj);
    static inline void writeBarrierPost(ObjectImpl *obj, void *addr);
    inline void privateWriteBarrierPre(void **oldval);
    inline void privateWriteBarrierPost(void **pprivate);
    void markChildren(JSTracer *trc);

    

    inline void *&privateRef(uint32_t nfixed) const; 

    inline bool hasPrivate() const;
    inline void *getPrivate() const;
    inline void setPrivate(void *data);
    inline void setPrivateGCThing(gc::Cell *cell);
    inline void setPrivateUnbarriered(void *data);
    inline void initPrivate(void *data);

    
    inline void *getPrivate(uint32_t nfixed) const;

    
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

template <> struct RootMethods<PropertyId>
{
    static PropertyId initial() { return PropertyId(); }
    static ThingRootKind kind() { return THING_ROOT_PROPERTY_ID; }
    static bool poisoned(PropertyId propid) { return IsPoisonedId(propid.asId()); }
};

} 

#endif 

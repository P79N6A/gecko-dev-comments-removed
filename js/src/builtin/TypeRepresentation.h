





#ifndef builtin_TypeRepresentation_h
#define builtin_TypeRepresentation_h

















































#include "jsalloc.h"
#include "jscntxt.h"
#include "jspubtd.h"

#include "builtin/TypedObjectConstants.h"
#include "gc/Barrier.h"
#include "js/HashTable.h"

namespace js {

class TypeRepresentation;
class SizedTypeRepresentation;
class ScalarTypeRepresentation;
class ReferenceTypeRepresentation;
class X4TypeRepresentation;
class SizedArrayTypeRepresentation;
class UnsizedArrayTypeRepresentation;
class StructTypeRepresentation;

struct Class;
class StringBuffer;

struct TypeRepresentationHasher
{
    typedef TypeRepresentation *Lookup;
    static HashNumber hash(TypeRepresentation *key);
    static bool match(TypeRepresentation *key1, TypeRepresentation *key2);

  private:
    static HashNumber hashScalar(ScalarTypeRepresentation *key);
    static HashNumber hashReference(ReferenceTypeRepresentation *key);
    static HashNumber hashX4(X4TypeRepresentation *key);
    static HashNumber hashStruct(StructTypeRepresentation *key);
    static HashNumber hashUnsizedArray(UnsizedArrayTypeRepresentation *key);
    static HashNumber hashSizedArray(SizedArrayTypeRepresentation *key);

    static bool matchScalars(ScalarTypeRepresentation *key1,
                             ScalarTypeRepresentation *key2);
    static bool matchReferences(ReferenceTypeRepresentation *key1,
                                ReferenceTypeRepresentation *key2);
    static bool matchX4s(X4TypeRepresentation *key1,
                         X4TypeRepresentation *key2);
    static bool matchStructs(StructTypeRepresentation *key1,
                             StructTypeRepresentation *key2);
    static bool matchSizedArrays(SizedArrayTypeRepresentation *key1,
                                 SizedArrayTypeRepresentation *key2);
    static bool matchUnsizedArrays(UnsizedArrayTypeRepresentation *key1,
                                   UnsizedArrayTypeRepresentation *key2);
};

typedef js::HashSet<TypeRepresentation *,
                    TypeRepresentationHasher,
                    RuntimeAllocPolicy> TypeRepresentationHash;

class TypeRepresentationHelper;

class TypeRepresentation {
  public:
    enum Kind {
        Scalar = JS_TYPEREPR_SCALAR_KIND,
        Reference = JS_TYPEREPR_REFERENCE_KIND,
        X4 = JS_TYPEREPR_X4_KIND,
        Struct = JS_TYPEREPR_STRUCT_KIND,
        SizedArray = JS_TYPEREPR_SIZED_ARRAY_KIND,
        UnsizedArray = JS_TYPEREPR_UNSIZED_ARRAY_KIND,
    };

  protected:
    TypeRepresentation(Kind kind, bool opaque);

    
    friend class TypeRepresentationHelper;

    Kind kind_;
    bool opaque_;

    JSObject *addToTableOrFree(JSContext *cx, TypeRepresentationHash::AddPtr &p);

  private:
    static const Class class_;
    static void obj_trace(JSTracer *trace, JSObject *object);
    static void obj_finalize(js::FreeOp *fop, JSObject *object);

    HeapPtrObject ownerObject_;
    void traceFields(JSTracer *tracer);

  public:
    Kind kind() const { return kind_; }
    bool opaque() const { return opaque_; }
    bool transparent() const { return !opaque_; }
    JSObject *ownerObject() const { return ownerObject_.get(); }

    
    
    bool appendString(JSContext *cx, StringBuffer &buffer);

    static bool isOwnerObject(JSObject &obj);
    static TypeRepresentation *fromOwnerObject(JSObject &obj);

    static bool isSized(Kind kind) {
        return kind > JS_TYPEREPR_MAX_UNSIZED_KIND;
    }

    bool isSized() const {
        return isSized(kind());
    }

    inline SizedTypeRepresentation *asSized();

    bool isScalar() const {
        return kind() == Scalar;
    }

    inline ScalarTypeRepresentation *asScalar();

    bool isReference() const {
        return kind() == Reference;
    }

    inline ReferenceTypeRepresentation *asReference();

    bool isX4() const {
        return kind() == X4;
    }

    inline X4TypeRepresentation *asX4();

    bool isSizedArray() const {
        return kind() == SizedArray;
    }

    inline SizedArrayTypeRepresentation *asSizedArray();

    bool isUnsizedArray() const {
        return kind() == UnsizedArray;
    }

    inline UnsizedArrayTypeRepresentation *asUnsizedArray();

    bool isAnyArray() const {
        return isSizedArray() || isUnsizedArray();
    }

    bool isStruct() const {
        return kind() == Struct;
    }

    inline StructTypeRepresentation *asStruct();

    void mark(JSTracer *tracer);
};

class SizedTypeRepresentation : public TypeRepresentation {
  protected:
    SizedTypeRepresentation(Kind kind, bool opaque, size_t size, size_t align);

    size_t size_;
    size_t alignment_;

  public:
    size_t size() const { return size_; }
    size_t alignment() const { return alignment_; }

    
    void initInstance(const JSRuntime *rt, uint8_t *mem, size_t count);

    
    void traceInstance(JSTracer *trace, uint8_t *mem, size_t count);
};

class ScalarTypeRepresentation : public SizedTypeRepresentation {
  public:
    
    enum Type {
        TYPE_INT8 = JS_SCALARTYPEREPR_INT8,
        TYPE_UINT8 = JS_SCALARTYPEREPR_UINT8,
        TYPE_INT16 = JS_SCALARTYPEREPR_INT16,
        TYPE_UINT16 = JS_SCALARTYPEREPR_UINT16,
        TYPE_INT32 = JS_SCALARTYPEREPR_INT32,
        TYPE_UINT32 = JS_SCALARTYPEREPR_UINT32,
        TYPE_FLOAT32 = JS_SCALARTYPEREPR_FLOAT32,
        TYPE_FLOAT64 = JS_SCALARTYPEREPR_FLOAT64,

        



        TYPE_UINT8_CLAMPED = JS_SCALARTYPEREPR_UINT8_CLAMPED,
    };
    static const int32_t TYPE_MAX = TYPE_UINT8_CLAMPED + 1;

  private:
    
    friend class TypeRepresentation;

    
    friend class TypeRepresentationHelper;

    const Type type_;

    explicit ScalarTypeRepresentation(Type type);

    
    bool appendStringScalar(JSContext *cx, StringBuffer &buffer);

  public:
    Type type() const {
        return type_;
    }

    const char *typeName() const {
        return typeName(type());
    }

    static const char *typeName(Type type);
    static JSObject *Create(JSContext *cx, Type type);
};




#define JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)                     \
    macro_(ScalarTypeRepresentation::TYPE_INT8,    int8_t,   int8)            \
    macro_(ScalarTypeRepresentation::TYPE_UINT8,   uint8_t,  uint8)           \
    macro_(ScalarTypeRepresentation::TYPE_INT16,   int16_t,  int16)           \
    macro_(ScalarTypeRepresentation::TYPE_UINT16,  uint16_t, uint16)          \
    macro_(ScalarTypeRepresentation::TYPE_INT32,   int32_t,  int32)           \
    macro_(ScalarTypeRepresentation::TYPE_UINT32,  uint32_t, uint32)          \
    macro_(ScalarTypeRepresentation::TYPE_FLOAT32, float,    float32)         \
    macro_(ScalarTypeRepresentation::TYPE_FLOAT64, double,   float64)


#define JS_FOR_EACH_SCALAR_TYPE_REPR(macro_)                                    \
    JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)                           \
    macro_(ScalarTypeRepresentation::TYPE_UINT8_CLAMPED, uint8_t, uint8Clamped)

class ReferenceTypeRepresentation : public SizedTypeRepresentation {
  public:
    
    enum Type {
        TYPE_ANY = JS_REFERENCETYPEREPR_ANY,
        TYPE_OBJECT = JS_REFERENCETYPEREPR_OBJECT,
        TYPE_STRING = JS_REFERENCETYPEREPR_STRING,
    };
    static const int32_t TYPE_MAX = TYPE_STRING + 1;

  private:
    
    friend class TypeRepresentation;

    Type type_;

    explicit ReferenceTypeRepresentation(Type type);

    
    bool appendStringReference(JSContext *cx, StringBuffer &buffer);

  public:
    Type type() const {
        return type_;
    }

    const char *typeName() const {
        return typeName(type());
    }

    static const char *typeName(Type type);
    static JSObject *Create(JSContext *cx, Type type);
};

#define JS_FOR_EACH_REFERENCE_TYPE_REPR(macro_)                             \
    macro_(ReferenceTypeRepresentation::TYPE_ANY,    HeapValue, Any)        \
    macro_(ReferenceTypeRepresentation::TYPE_OBJECT, HeapPtrObject, Object) \
    macro_(ReferenceTypeRepresentation::TYPE_STRING, HeapPtrString, string)

class X4TypeRepresentation : public SizedTypeRepresentation {
  public:
    enum Type {
        TYPE_INT32 = JS_X4TYPEREPR_INT32,
        TYPE_FLOAT32 = JS_X4TYPEREPR_FLOAT32,
    };

  private:
    
    friend class TypeRepresentation;

    
    friend class TypeRepresentationHelper;

    const Type type_;

    explicit X4TypeRepresentation(Type type);

    
    bool appendStringX4(JSContext *cx, StringBuffer &buffer);

  public:
    Type type() const {
        return type_;
    }

    static JSObject *Create(JSContext *cx, Type type);
};


#define JS_FOR_EACH_X4_TYPE_REPR(macro_)                                      \
    macro_(X4TypeRepresentation::TYPE_INT32, int32_t, int32)                  \
    macro_(X4TypeRepresentation::TYPE_FLOAT32, float, float32)

class UnsizedArrayTypeRepresentation : public TypeRepresentation {
  private:
    
    friend class TypeRepresentation;

    SizedTypeRepresentation *element_;

    UnsizedArrayTypeRepresentation(SizedTypeRepresentation *element);

    
    void traceUnsizedArrayFields(JSTracer *trace);

    
    bool appendStringUnsizedArray(JSContext *cx, StringBuffer &buffer);

  public:
    SizedTypeRepresentation *element() {
        return element_;
    }

    static JSObject *Create(JSContext *cx,
                            SizedTypeRepresentation *elementTypeRepr);
};

class SizedArrayTypeRepresentation : public SizedTypeRepresentation {
  private:
    
    friend class TypeRepresentation;

    SizedTypeRepresentation *element_;
    size_t length_;

    SizedArrayTypeRepresentation(SizedTypeRepresentation *element,
                                 size_t length);

    
    void traceSizedArrayFields(JSTracer *trace);

    
    bool appendStringSizedArray(JSContext *cx, StringBuffer &buffer);

  public:
    SizedTypeRepresentation *element() {
        return element_;
    }

    size_t length() {
        return length_;
    }

    static JSObject *Create(JSContext *cx,
                            SizedTypeRepresentation *elementTypeRepr,
                            size_t length);
};

struct StructField {
    size_t index;
    HeapId id;
    SizedTypeRepresentation *typeRepr;
    size_t offset;

    explicit StructField(size_t index,
                         jsid &id,
                         SizedTypeRepresentation *typeRepr,
                         size_t offset);
};

class StructTypeRepresentation : public SizedTypeRepresentation {
  private:
    
    friend class TypeRepresentation;

    size_t fieldCount_;

    
    
    StructField* fields() {
        return (StructField*) (this+1);
    }
    const StructField* fields() const {
        return (StructField*) (this+1);
    }

    StructTypeRepresentation();
    bool init(JSContext *cx,
              AutoIdVector &ids,
              AutoObjectVector &typeReprOwners);

    
    void traceStructFields(JSTracer *trace);

    
    bool appendStringStruct(JSContext *cx, StringBuffer &buffer);

  public:
    size_t fieldCount() const {
        return fieldCount_;
    }

    const StructField &field(size_t i) const {
        JS_ASSERT(i < fieldCount());
        return fields()[i];
    }

    const StructField *fieldNamed(jsid id) const;

    
    
    
    static JSObject *Create(JSContext *cx,
                            AutoIdVector &ids,
                            AutoObjectVector &typeReprOwners);
};





SizedTypeRepresentation *
TypeRepresentation::asSized() {
    JS_ASSERT(isSized());
    return static_cast<SizedTypeRepresentation*>(this);
}

ScalarTypeRepresentation *
TypeRepresentation::asScalar() {
    JS_ASSERT(isScalar());
    return static_cast<ScalarTypeRepresentation*>(this);
}

ReferenceTypeRepresentation *
TypeRepresentation::asReference() {
    JS_ASSERT(isReference());
    return static_cast<ReferenceTypeRepresentation*>(this);
}

X4TypeRepresentation *
TypeRepresentation::asX4() {
    JS_ASSERT(isX4());
    return static_cast<X4TypeRepresentation*>(this);
}

SizedArrayTypeRepresentation *
TypeRepresentation::asSizedArray() {
    JS_ASSERT(isSizedArray());
    return static_cast<SizedArrayTypeRepresentation*>(this);
}

UnsizedArrayTypeRepresentation *
TypeRepresentation::asUnsizedArray() {
    JS_ASSERT(isUnsizedArray());
    return static_cast<UnsizedArrayTypeRepresentation*>(this);
}

StructTypeRepresentation *
TypeRepresentation::asStruct() {
    JS_ASSERT(isStruct());
    return static_cast<StructTypeRepresentation*>(this);
}

} 

#endif

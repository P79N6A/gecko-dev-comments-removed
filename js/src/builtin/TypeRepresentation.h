





#ifndef builtin_TypeRepresentation_h
#define builtin_TypeRepresentation_h




























































#include "jsalloc.h"
#include "jscntxt.h"
#include "jspubtd.h"

#include "builtin/TypedObject.h"
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
  protected:
    TypeRepresentation(TypeDescr::Kind kind, bool opaque);

    
    friend class TypeRepresentationHelper;

    TypeDescr::Kind kind_;
    bool opaque_;

    JSObject *addToTableOrFree(JSContext *cx, TypeRepresentationHash::AddPtr &p);

  private:
    static const Class class_;
    static void obj_trace(JSTracer *trace, JSObject *object);
    static void obj_finalize(js::FreeOp *fop, JSObject *object);

    HeapPtrObject ownerObject_;
    HeapPtrTypeObject typeObject_;
    void traceFields(JSTracer *tracer);

  public:
    TypeDescr::Kind kind() const { return kind_; }
    bool opaque() const { return opaque_; }
    bool transparent() const { return !opaque_; }
    JSObject *ownerObject() const { return ownerObject_.get(); }
    types::TypeObject *typeObject() const { return typeObject_.get(); }

    static bool isOwnerObject(JSObject &obj);
    static TypeRepresentation *fromOwnerObject(JSObject &obj);

    bool isSized() const {
        return TypeDescr::isSized(kind());
    }

    inline SizedTypeRepresentation *asSized();

    bool isScalar() const {
        return kind() == TypeDescr::Scalar;
    }

    inline ScalarTypeRepresentation *asScalar();

    bool isReference() const {
        return kind() == TypeDescr::Reference;
    }

    inline ReferenceTypeRepresentation *asReference();

    bool isX4() const {
        return kind() == TypeDescr::X4;
    }

    inline X4TypeRepresentation *asX4();

    bool isSizedArray() const {
        return kind() == TypeDescr::SizedArray;
    }

    inline SizedArrayTypeRepresentation *asSizedArray();

    bool isUnsizedArray() const {
        return kind() == TypeDescr::UnsizedArray;
    }

    inline UnsizedArrayTypeRepresentation *asUnsizedArray();

    bool isAnyArray() const {
        return isSizedArray() || isUnsizedArray();
    }

    bool isStruct() const {
        return kind() == TypeDescr::Struct;
    }

    inline StructTypeRepresentation *asStruct();

    void mark(JSTracer *tracer);
};

class SizedTypeRepresentation : public TypeRepresentation {
  protected:
    SizedTypeRepresentation(TypeDescr::Kind kind, bool opaque, size_t size, size_t align);

    size_t size_;
    size_t alignment_;

  public:
    size_t size() const { return size_; }
    size_t alignment() const { return alignment_; }

    
    
    void initInstance(const JSRuntime *rt, uint8_t *mem, size_t count);

    
    void traceInstance(JSTracer *trace, uint8_t *mem, size_t count);
};

class ScalarTypeRepresentation : public SizedTypeRepresentation {
  private:
    
    friend class TypeRepresentationHelper;

    const ScalarTypeDescr::Type type_;

    explicit ScalarTypeRepresentation(ScalarTypeDescr::Type type);

  public:
    ScalarTypeDescr::Type type() const {
        return type_;
    }

    const char *typeName() const {
        return ScalarTypeDescr::typeName(type());
    }

    static JSObject *Create(JSContext *cx, ScalarTypeDescr::Type type);
};

class ReferenceTypeRepresentation : public SizedTypeRepresentation {
  private:
    ReferenceTypeDescr::Type type_;

    explicit ReferenceTypeRepresentation(ReferenceTypeDescr::Type type);

  public:
    ReferenceTypeDescr::Type type() const {
        return type_;
    }

    const char *typeName() const {
        return ReferenceTypeDescr::typeName(type());
    }

    static JSObject *Create(JSContext *cx, ReferenceTypeDescr::Type type);
};

class X4TypeRepresentation : public SizedTypeRepresentation {
  private:
    
    friend class TypeRepresentationHelper;

    const X4TypeDescr::Type type_;

    explicit X4TypeRepresentation(X4TypeDescr::Type type);

  public:
    X4TypeDescr::Type type() const {
        return type_;
    }

    static JSObject *Create(JSContext *cx, X4TypeDescr::Type type);
};

class UnsizedArrayTypeRepresentation : public TypeRepresentation {
  private:
    
    friend class TypeRepresentation;

    SizedTypeRepresentation *element_;

    UnsizedArrayTypeRepresentation(SizedTypeRepresentation *element);

    
    void traceUnsizedArrayFields(JSTracer *trace);

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
    HeapPtrPropertyName propertyName;
    SizedTypeRepresentation *typeRepr;
    size_t offset;

    explicit StructField(size_t index,
                         PropertyName *propertyName,
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
              AutoPropertyNameVector &names,
              AutoObjectVector &typeReprOwners);

    
    void traceStructFields(JSTracer *trace);

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
                            AutoPropertyNameVector &names,
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

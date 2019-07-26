





#ifndef builtin_TypeRepresentation_h
#define builtin_TypeRepresentation_h









































#include "jsalloc.h"
#include "jscntxt.h"
#include "jspubtd.h"

#include "gc/Barrier.h"
#include "js/HashTable.h"

namespace js {

class TypeRepresentation;
class ScalarTypeRepresentation;
class ArrayTypeRepresentation;
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
    static HashNumber hashStruct(StructTypeRepresentation *key);
    static HashNumber hashArray(ArrayTypeRepresentation *key);

    static bool matchScalars(ScalarTypeRepresentation *key1,
                             ScalarTypeRepresentation *key2);
    static bool matchStructs(StructTypeRepresentation *key1,
                             StructTypeRepresentation *key2);
    static bool matchArrays(ArrayTypeRepresentation *key1,
                            ArrayTypeRepresentation *key2);
};

typedef js::HashSet<TypeRepresentation *,
                    TypeRepresentationHasher,
                    RuntimeAllocPolicy> TypeRepresentationSet;

class TypeRepresentation {
  public:
    enum Kind {
        Scalar,
        Struct,
        Array
    };

  protected:
    TypeRepresentation(Kind kind, size_t size, size_t align);

    size_t size_;
    size_t alignment_;
    Kind kind_;

    JSObject *addToTableOrFree(JSContext *cx, TypeRepresentationSet::AddPtr &p);

  private:
    static Class class_;
    static void obj_trace(JSTracer *trace, JSObject *object);
    static void obj_finalize(js::FreeOp *fop, JSObject *object);

    js::HeapPtrObject ownerObject_;
    void traceFields(JSTracer *tracer);

  public:
    size_t size() const { return size_; }
    size_t alignment() const { return alignment_; }
    Kind kind() const { return kind_; }
    JSObject *ownerObject() const { return ownerObject_.get(); }

    
    
    bool appendString(JSContext *cx, StringBuffer &buffer);

    static bool isTypeRepresentationOwnerObject(JSObject *obj);
    static TypeRepresentation *fromOwnerObject(JSObject *obj);

    bool isScalar() const {
        return kind() == Scalar;
    }

    ScalarTypeRepresentation *asScalar() {
        JS_ASSERT(isScalar());
        return (ScalarTypeRepresentation*) this;
    }

    bool isArray() const {
        return kind() == Array;
    }

    ArrayTypeRepresentation *asArray() {
        JS_ASSERT(isArray());
        return (ArrayTypeRepresentation*) this;
    }

    bool isStruct() const {
        return kind() == Struct;
    }

    StructTypeRepresentation *asStruct() {
        JS_ASSERT(isStruct());
        return (StructTypeRepresentation*) this;
    }

    void mark(JSTracer *tracer);
};

class ScalarTypeRepresentation : public TypeRepresentation {
  public:
    
    enum Type {
        TYPE_INT8 = 0,
        TYPE_UINT8,
        TYPE_INT16,
        TYPE_UINT16,
        TYPE_INT32,
        TYPE_UINT32,
        TYPE_FLOAT32,
        TYPE_FLOAT64,

        



        TYPE_UINT8_CLAMPED,
    };
    static const int32_t TYPE_MAX = TYPE_UINT8_CLAMPED + 1;

  private:
    
    friend class TypeRepresentation;

    Type type_;

    explicit ScalarTypeRepresentation(Type type);

    
    bool appendStringScalar(JSContext *cx, StringBuffer &buffer);

  public:
    Type type() const {
        return type_;
    }

    bool convertValue(JSContext *cx, HandleValue value, MutableHandleValue vp);

    static const char *typeName(Type type);
    static JSObject *Create(JSContext *cx, Type type);
};


#define JS_FOR_EACH_SCALAR_TYPE_REPR(macro_)                                    \
    macro_(ScalarTypeRepresentation::TYPE_INT8,    int8_t,   int8)              \
    macro_(ScalarTypeRepresentation::TYPE_UINT8,   uint8_t,  uint8)             \
    macro_(ScalarTypeRepresentation::TYPE_INT16,   int16_t,  int16)             \
    macro_(ScalarTypeRepresentation::TYPE_UINT16,  uint16_t, uint16)            \
    macro_(ScalarTypeRepresentation::TYPE_INT32,   int32_t,  int32)             \
    macro_(ScalarTypeRepresentation::TYPE_UINT32,  uint32_t, uint32)            \
    macro_(ScalarTypeRepresentation::TYPE_FLOAT32, float,    float32)           \
    macro_(ScalarTypeRepresentation::TYPE_FLOAT64, double,   float64)           \
    macro_(ScalarTypeRepresentation::TYPE_UINT8_CLAMPED, uint8_t, uint8Clamped)

class ArrayTypeRepresentation : public TypeRepresentation {
  private:
    
    friend class TypeRepresentation;

    TypeRepresentation *element_;
    size_t length_;

    ArrayTypeRepresentation(TypeRepresentation *element,
                            size_t length);

    
    void traceArrayFields(JSTracer *trace);

    
    bool appendStringArray(JSContext *cx, StringBuffer &buffer);

  public:
    TypeRepresentation *element() {
        return element_;
    }

    size_t length() {
        return length_;
    }

    static JSObject *Create(JSContext *cx,
                            TypeRepresentation *elementTypeRepr,
                            size_t length);
};

struct StructField {
    size_t index;
    HeapId id;
    TypeRepresentation *typeRepr;
    size_t offset;

    explicit StructField(size_t index,
                         jsid &id,
                         TypeRepresentation *typeRepr,
                         size_t offset)
      : index(index),
        id(id),
        typeRepr(typeRepr),
        offset(offset)
    {}
};

class StructTypeRepresentation : public TypeRepresentation {
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

    const StructField *fieldNamed(HandleId id) const;

    static JSObject *Create(JSContext *cx,
                            AutoIdVector &ids,
                            AutoObjectVector &typeReprOwners);
};

} 

#endif

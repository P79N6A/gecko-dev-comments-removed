





#ifndef jit_TypedObjectPrediction_h
#define jit_TypedObjectPrediction_h

#include "builtin/TypedObject.h"
#include "jit/IonAllocPolicy.h"

namespace js {
namespace jit {























class TypedObjectPrediction {
  public:
    enum PredictionKind {
        
        Empty,

        
        Inconsistent,

        
        
        
        
        Prefix,

        
        
        Descr
    };

    struct PrefixData {
        const StructTypeDescr *descr;
        size_t fields;
    };

    union Data {
        const TypeDescr *descr;
        PrefixData prefix;
    };

  private:
    PredictionKind kind_;
    Data data_;

    PredictionKind predictionKind() const {
        return kind_;
    }

    void markInconsistent() {
        kind_ = Inconsistent;
    }

    const TypeDescr &descr() const {
        MOZ_ASSERT(predictionKind() == Descr);
        return *data_.descr;
    }

    const PrefixData &prefix() const {
        MOZ_ASSERT(predictionKind() == Prefix);
        return data_.prefix;
    }

    void setDescr(const TypeDescr &descr) {
        kind_ = Descr;
        data_.descr = &descr;
    }

    void setPrefix(const StructTypeDescr &descr, size_t fields) {
        kind_ = Prefix;
        data_.prefix.descr = &descr;
        data_.prefix.fields = fields;
    }

    void markAsCommonPrefix(const StructTypeDescr &descrA,
                            const StructTypeDescr &descrB,
                            size_t max);

    template<typename T>
    typename T::Type extractType() const;

    bool hasFieldNamedPrefix(const StructTypeDescr &descr,
                             size_t fieldCount,
                             jsid id,
                             size_t *fieldOffset,
                             TypedObjectPrediction *out,
                             size_t *index) const;

  public:

    
    
    

    TypedObjectPrediction() {
        kind_ = Empty;
    }

    explicit TypedObjectPrediction(const TypeDescr &descr) {
        setDescr(descr);
    }

    TypedObjectPrediction(const StructTypeDescr &descr, size_t fields) {
        setPrefix(descr, fields);
    }

    void addDescr(const TypeDescr &descr);

    
    

    bool isUseless() const {
        return predictionKind() == Empty || predictionKind() == Inconsistent;
    }

    
    
    
    
    
    const TypedProto *getKnownPrototype() const;

    
    

    type::Kind kind() const;

    bool ofArrayKind() const;

    
    
    
    
    
    
    bool hasKnownSize(int32_t *out) const;

    
    
    
    

    ScalarTypeDescr::Type scalarType() const;
    ReferenceTypeDescr::Type referenceType() const;
    SimdTypeDescr::Type simdType() const;

    
    

    
    
    bool hasKnownArrayLength(int32_t *length) const;

    
    TypedObjectPrediction arrayElementType() const;

    
    
    
    

    
    
    
    
    bool hasFieldNamed(jsid id,
                       size_t *fieldOffset,
                       TypedObjectPrediction *fieldType,
                       size_t *fieldIndex) const;
};

} 
} 

#endif







#ifndef jit_TypeRepresentationSet_h
#define jit_TypeRepresentationSet_h

#include "builtin/TypedObject.h"
#include "jit/IonAllocPolicy.h"
#include "js/HashTable.h"























namespace js {
namespace jit {

class IonBuilder;
class TypeDescrSet;

class TypeDescrSetBuilder {
  private:
    Vector<TypeDescr *, 4, SystemAllocPolicy> entries_;
    bool invalid_;

  public:
    TypeDescrSetBuilder();

    bool insert(TypeDescr *typeRepr);
    bool build(IonBuilder &builder, TypeDescrSet *out);
};

class TypeDescrSet {
  private:
    friend struct TypeDescrSetHasher;
    friend class TypeDescrSetBuilder;

    size_t length_;
    TypeDescr **entries_; 

    TypeDescrSet(size_t length, TypeDescr **entries);

    size_t length() const {
        return length_;
    }

    TypeDescr *get(uint32_t i) const {
        return entries_[i];
    }

    template<typename T>
    bool genericType(typename T::Type *out);

  public:
    
    
    
    
    

    TypeDescrSet(const TypeDescrSet &c);
    TypeDescrSet(); 

    
    

    bool empty() const;
    bool allOfKind(type::Kind kind);

    
    
    bool allOfArrayKind();

    
    
    
    
    
    
    
    
    bool allHaveSameSize(int32_t *out);

    types::TemporaryTypeSet *suitableTypeSet(IonBuilder &builder,
                                             const Class *knownClass);

    
    

    type::Kind kind();

    
    
    
    JSObject *knownPrototype() const;

    
    
    
    

    
    
    bool scalarType(ScalarTypeDescr::Type *out);

    
    
    
    

    
    
    bool referenceType(ReferenceTypeDescr::Type *out);

    
    
    
    

    
    
    bool x4Type(X4TypeDescr::Type *out);

    
    
    
    

    
    
    
    bool hasKnownArrayLength(int32_t *length);

    
    
    
    bool arrayElementType(IonBuilder &builder, TypeDescrSet *out);

    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool fieldNamed(IonBuilder &builder,
                    jsid id,
                    int32_t *offset,
                    TypeDescrSet *out,
                    size_t *index);
};

struct TypeDescrSetHasher
{
    typedef TypeDescrSet Lookup;
    static HashNumber hash(TypeDescrSet key);
    static bool match(TypeDescrSet key1,
                      TypeDescrSet key2);
};

typedef js::HashSet<TypeDescrSet,
                    TypeDescrSetHasher,
                    IonAllocPolicy> TypeDescrSetHash;

} 
} 

#endif

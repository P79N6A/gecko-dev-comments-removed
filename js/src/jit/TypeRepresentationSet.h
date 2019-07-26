





#ifndef jit_TypeRepresentationSet_h
#define jit_TypeRepresentationSet_h

#include "builtin/TypeRepresentation.h"
#include "jit/IonAllocPolicy.h"
#include "js/HashTable.h"























namespace js {
namespace jit {

class IonBuilder;
class TypeRepresentationSet;

class TypeRepresentationSetBuilder {
  private:
    Vector<TypeRepresentation *, 4, SystemAllocPolicy> entries_;
    bool invalid_;

    bool overlaps(TypeRepresentation *a, TypeRepresentation *b);

  public:
    TypeRepresentationSetBuilder();

    bool insert(TypeRepresentation *typeRepr);
    bool build(IonBuilder &builder, TypeRepresentationSet *out);
};

class TypeRepresentationSet {
  private:
    friend struct TypeRepresentationSetHasher;
    friend class TypeRepresentationSetBuilder;

    size_t length_;
    TypeRepresentation **entries_; 

    TypeRepresentationSet(size_t length, TypeRepresentation **entries);

    size_t length() const {
        return length_;
    }

    TypeRepresentation *get(uint32_t i) const {
        return entries_[i];
    }

  public:
    
    
    
    
    

    TypeRepresentationSet(const TypeRepresentationSet &c);
    TypeRepresentationSet(); 

    
    

    bool empty();
    bool singleton();
    bool allOfKind(TypeRepresentation::Kind kind);

    
    
    bool allOfArrayKind();

    
    
    
    
    
    
    
    
    bool allHaveSameSize(size_t *out);

    
    

    TypeRepresentation::Kind kind();

    
    

    TypeRepresentation *getTypeRepresentation();

    
    
    
    

    
    
    
    bool hasKnownArrayLength(size_t *length);

    
    
    
    bool arrayElementType(IonBuilder &builder, TypeRepresentationSet *out);

    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool fieldNamed(IonBuilder &builder,
                    jsid id,
                    size_t *offset,
                    TypeRepresentationSet *out,
                    size_t *index);
};

struct TypeRepresentationSetHasher
{
    typedef TypeRepresentationSet Lookup;
    static HashNumber hash(TypeRepresentationSet key);
    static bool match(TypeRepresentationSet key1,
                      TypeRepresentationSet key2);
};

typedef js::HashSet<TypeRepresentationSet,
                    TypeRepresentationSetHasher,
                    IonAllocPolicy> TypeRepresentationSetHash;

} 
} 

#endif

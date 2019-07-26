





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
    friend class TypeRepresentationSetBuilder;

    size_t length_;
    TypeRepresentation **entries_; 

    TypeRepresentationSet(size_t length, TypeRepresentation **entries);

  public:
    
    
    
    
    

    TypeRepresentationSet(const TypeRepresentationSet &c);
    TypeRepresentationSet(); 

    
    

    bool empty();
    size_t length();
    TypeRepresentation *get(size_t i);
    bool allOfKind(TypeRepresentation::Kind kind);

    
    

    TypeRepresentation::Kind kind();

    
    
    
    

    
    
    size_t arrayLength();

    
    
    
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

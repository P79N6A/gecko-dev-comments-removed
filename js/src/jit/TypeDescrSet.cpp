





#include "jit/TypeDescrSet.h"

#include "mozilla/HashFunctions.h"

#include "builtin/TypedObject.h"
#include "jit/IonBuilder.h"

using namespace js;
using namespace jit;




HashNumber
TypeDescrSetHasher::hash(TypeDescrSet key)
{
    HashNumber hn = mozilla::HashGeneric(key.length());
    for (size_t i = 0; i < key.length(); i++)
        hn = mozilla::AddToHash(hn, uintptr_t(key.get(i)));
    return hn;
}

bool
TypeDescrSetHasher::match(TypeDescrSet key1, TypeDescrSet key2)
{
    if (key1.length() != key2.length())
        return false;

    
    for (size_t i = 0; i < key1.length(); i++) {
        if (key1.get(i) != key2.get(i))
            return false;
    }

    return true;
}




TypeDescrSetBuilder::TypeDescrSetBuilder()
  : invalid_(false)
{}

bool
TypeDescrSetBuilder::insert(TypeDescr *descr)
{
    if (invalid_)
        return true;

    if (entries_.empty())
        return entries_.append(descr);

    
    
    
    TypeDescr *entry0 = entries_[0];
    if (descr->kind() != entry0->kind()) {
        invalid_ = true;
        entries_.clear();
        return true;
    }

    
    
    
    uintptr_t descrAddr = (uintptr_t) descr;
    size_t min = 0;
    size_t max = entries_.length();
    while (min != max) {
        size_t i = min + ((max - min) >> 1); 

        uintptr_t entryiaddr = (uintptr_t) entries_[i];
        if (entryiaddr == descrAddr)
            return true; 

        if (entryiaddr < descrAddr) {
            
            min = i + 1;
        } else {
            
            max = i;
        }
    }

    
    if (entries_.length() >= 512) {
        invalid_ = true;
        entries_.clear();
        return true;
    }

    
    if (min == entries_.length())
        return entries_.append(descr);
    TypeDescr **insertLoc = &entries_[min];
    return entries_.insert(insertLoc, descr) != nullptr;
}

bool
TypeDescrSetBuilder::build(IonBuilder &builder, TypeDescrSet *out)
{
    if (invalid_) {
        *out = TypeDescrSet();
        return true;
    }

    TypeDescrSetHash *table = builder.getOrCreateDescrSetHash();
    if (!table)
        return false;

    
    size_t length = entries_.length();
    TypeDescrSet tempSet(length, entries_.begin());
    TypeDescrSetHash::AddPtr p = table->lookupForAdd(tempSet);
    if (p) {
        *out = *p;
        return true;
    }

    
    size_t space = sizeof(TypeDescr*) * length;
    TypeDescr **array = (TypeDescr**)
        GetIonContext()->temp->allocate(space);
    if (!array)
        return false;
    memcpy(array, entries_.begin(), space);
    TypeDescrSet permSet(length, array);
    if (!table->add(p, permSet))
        return false;

    *out = permSet;
    return true;
}




TypeDescrSet::TypeDescrSet(const TypeDescrSet &c)
  : length_(c.length_),
    entries_(c.entries_)
{}

TypeDescrSet::TypeDescrSet(size_t length,
                           TypeDescr **entries)
  : length_(length),
    entries_(entries)
{}

TypeDescrSet::TypeDescrSet()
  : length_(0),
    entries_(nullptr)
{}

bool
TypeDescrSet::empty()
{
    return length_ == 0;
}

bool
TypeDescrSet::allOfArrayKind()
{
    if (empty())
        return false;

    switch (kind()) {
      case TypeRepresentation::SizedArray:
      case TypeRepresentation::UnsizedArray:
        return true;

      case TypeRepresentation::X4:
      case TypeRepresentation::Reference:
      case TypeRepresentation::Scalar:
      case TypeRepresentation::Struct:
        return false;
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind() in TypeDescrSet");
}

bool
TypeDescrSet::allOfKind(TypeRepresentation::Kind aKind)
{
    if (empty())
        return false;

    return kind() == aKind;
}

bool
TypeDescrSet::allHaveSameSize(size_t *out)
{
    if (empty())
        return false;

    JS_ASSERT(TypeRepresentation::isSized(kind()));

    size_t size = get(0)->as<SizedTypeDescr>().size();
    for (size_t i = 1; i < length(); i++) {
        if (get(i)->as<SizedTypeDescr>().size() != size)
            return false;
    }

    *out = size;
    return true;
}

TypeRepresentation::Kind
TypeDescrSet::kind()
{
    JS_ASSERT(!empty());
    return get(0)->kind();
}

template<typename T>
bool
TypeDescrSet::genericType(typename T::TypeRepr::Type *out)
{
    JS_ASSERT(allOfKind(TypeRepresentation::Scalar));

    typename T::TypeRepr::Type type = get(0)->as<T>().type();
    for (size_t i = 1; i < length(); i++) {
        if (get(i)->as<T>().type() != type)
            return false;
    }

    *out = type;
    return true;
}

bool
TypeDescrSet::scalarType(ScalarTypeRepresentation::Type *out)
{
    return genericType<ScalarTypeDescr>(out);
}

bool
TypeDescrSet::referenceType(ReferenceTypeRepresentation::Type *out)
{
    return genericType<ReferenceTypeDescr>(out);
}

bool
TypeDescrSet::x4Type(X4TypeRepresentation::Type *out)
{
    return genericType<X4TypeDescr>(out);
}

bool
TypeDescrSet::hasKnownArrayLength(size_t *l)
{
    switch (kind()) {
      case TypeRepresentation::UnsizedArray:
        return false;

      case TypeRepresentation::SizedArray:
      {
        const size_t result = get(0)->as<SizedArrayTypeDescr>().length();
        for (size_t i = 1; i < length(); i++) {
            size_t l = get(i)->as<SizedArrayTypeDescr>().length();
            if (l != result)
                return false;
        }
        *l = result;
        return true;
      }

      default:
        MOZ_ASSUME_UNREACHABLE("Invalid array size for call to arrayLength()");
    }
}

bool
TypeDescrSet::arrayElementType(IonBuilder &builder, TypeDescrSet *out)
{
    TypeDescrSetBuilder elementTypes;
    for (size_t i = 0; i < length(); i++) {
        switch (kind()) {
          case TypeRepresentation::UnsizedArray:
            if (!elementTypes.insert(&get(i)->as<UnsizedArrayTypeDescr>().elementType()))
                return false;
            break;

          case TypeRepresentation::SizedArray:
            if (!elementTypes.insert(&get(i)->as<SizedArrayTypeDescr>().elementType()))
                return false;
            break;

          default:
            MOZ_ASSUME_UNREACHABLE("Invalid kind for arrayElementType()");
        }
    }
    return elementTypes.build(builder, out);
}

bool
TypeDescrSet::fieldNamed(IonBuilder &builder,
                         jsid id,
                         size_t *offset,
                         TypeDescrSet *out,
                         size_t *index)
{
    JS_ASSERT(kind() == TypeRepresentation::Struct);

    
    
    *offset = SIZE_MAX;
    *index = SIZE_MAX;
    *out = TypeDescrSet();

    
    size_t offset0;
    size_t index0;
    TypeDescrSetBuilder fieldTypes;
    {
        StructTypeDescr &descr0 = get(0)->as<StructTypeDescr>();
        if (!descr0.fieldIndex(id, &index0))
            return true;

        offset0 = descr0.fieldOffset(index0);
        if (!fieldTypes.insert(&descr0.fieldDescr(index0)))
            return false;
    }

    
    
    for (size_t i = 1; i < length(); i++) {
        StructTypeDescr &descri = get(0)->as<StructTypeDescr>();

        size_t indexi;
        if (!descri.fieldIndex(id, &indexi))
            return true;

        
        if (indexi != index0)
            index0 = SIZE_MAX;

        
        if (descri.fieldOffset(indexi) != offset0)
            return true;

        if (!fieldTypes.insert(&descri.fieldDescr(indexi)))
            return false;
    }

    
    
    
    *offset = offset0;
    *index = index0;
    return fieldTypes.build(builder, out);
}

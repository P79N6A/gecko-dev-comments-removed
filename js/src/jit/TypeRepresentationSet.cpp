





#include "jit/TypeRepresentationSet.h"

#include "mozilla/HashFunctions.h"

#include "jit/IonBuilder.h"

using namespace js;
using namespace jit;




HashNumber
TypeRepresentationSetHasher::hash(TypeRepresentationSet key)
{
    HashNumber hn = mozilla::HashGeneric(key.length());
    for (size_t i = 0; i < key.length(); i++)
        hn = mozilla::AddToHash(hn, uintptr_t(key.get(i)));
    return hn;
}

bool
TypeRepresentationSetHasher::match(TypeRepresentationSet key1,
                                   TypeRepresentationSet key2)
{
    if (key1.length() != key2.length())
        return false;

    
    for (size_t i = 0; i < key1.length(); i++) {
        if (key1.get(i) != key2.get(i))
            return false;
    }

    return true;
}




TypeRepresentationSetBuilder::TypeRepresentationSetBuilder()
  : invalid_(false)
{}

bool
TypeRepresentationSetBuilder::insert(TypeRepresentation *typeRepr)
{
    if (invalid_)
        return true;

    if (entries_.length() == 0)
        return entries_.append(typeRepr);

    
    
    
    TypeRepresentation *entry0 = entries_[0];
    if (typeRepr->kind() != entry0->kind()) {
        invalid_ = true;
        entries_.clear();
        return true;
    }

    
    
    
    uintptr_t typeReprAddr = (uintptr_t) typeRepr;
    size_t min = 0;
    size_t max = entries_.length();
    while (min != max) {
        size_t i = min + ((max - min) >> 1); 

        uintptr_t entryiaddr = (uintptr_t) entries_[i];
        if (entryiaddr == typeReprAddr)
            return true; 

        if (entryiaddr < typeReprAddr) {
            
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
        return entries_.append(typeRepr);
    TypeRepresentation **insertLoc = &entries_[min];
    return entries_.insert(insertLoc, typeRepr) != nullptr;
}

bool
TypeRepresentationSetBuilder::build(IonBuilder &builder,
                                    TypeRepresentationSet *out)
{
    if (invalid_) {
        *out = TypeRepresentationSet();
        return true;
    }

    TypeRepresentationSetHash *table = builder.getOrCreateReprSetHash();
    if (!table)
        return false;

    
    size_t length = entries_.length();
    TypeRepresentationSet tempSet(length, entries_.begin());
    TypeRepresentationSetHash::AddPtr p = table->lookupForAdd(tempSet);
    if (p) {
        *out = *p;
        return true;
    }

    
    size_t space = sizeof(TypeRepresentation*) * length;
    TypeRepresentation **array = (TypeRepresentation**)
        GetIonContext()->temp->allocate(space);
    if (!array)
        return false;
    memcpy(array, entries_.begin(), space);
    TypeRepresentationSet permSet(length, array);
    if (!table->add(p, permSet))
        return false;

    *out = permSet;
    return true;
}




TypeRepresentationSet::TypeRepresentationSet(const TypeRepresentationSet &c)
  : length_(c.length_),
    entries_(c.entries_)
{}

TypeRepresentationSet::TypeRepresentationSet(size_t length,
                                             TypeRepresentation **entries)
  : length_(length),
    entries_(entries)
{}

TypeRepresentationSet::TypeRepresentationSet()
  : length_(0),
    entries_(nullptr)
{}

bool
TypeRepresentationSet::empty()
{
    return length_ == 0;
}

bool
TypeRepresentationSet::singleton()
{
    return length_ == 1;
}

TypeRepresentation *
TypeRepresentationSet::getTypeRepresentation()
{
    JS_ASSERT(singleton());
    return get(0);
}

bool
TypeRepresentationSet::allOfArrayKind()
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

    MOZ_ASSUME_UNREACHABLE("Invalid kind() in TypeRepresentationSet");
}

bool
TypeRepresentationSet::allOfKind(TypeRepresentation::Kind aKind)
{
    if (empty())
        return false;

    return kind() == aKind;
}

bool
TypeRepresentationSet::allHaveSameSize(size_t *out)
{
    if (empty())
        return false;

    JS_ASSERT(TypeRepresentation::isSized(kind()));

    size_t size = get(0)->asSized()->size();
    for (size_t i = 1; i < length(); i++) {
        if (get(i)->asSized()->size() != size)
            return false;
    }

    *out = size;
    return true;
}

TypeRepresentation::Kind
TypeRepresentationSet::kind()
{
    JS_ASSERT(!empty());
    return get(0)->kind();
}

bool
TypeRepresentationSet::hasKnownArrayLength(size_t *l)
{
    switch (kind()) {
      case TypeRepresentation::UnsizedArray:
        return false;

      case TypeRepresentation::SizedArray:
      {
        const size_t result = get(0)->asSizedArray()->length();
        for (size_t i = 1; i < length(); i++) {
            if (get(i)->asSizedArray()->length() != result)
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
TypeRepresentationSet::arrayElementType(IonBuilder &builder,
                                        TypeRepresentationSet *out)
{
    TypeRepresentationSetBuilder elementTypes;
    for (size_t i = 0; i < length(); i++) {
        switch (kind()) {
          case TypeRepresentation::UnsizedArray:
            if (!elementTypes.insert(get(i)->asUnsizedArray()->element()))
                return false;
            break;

          case TypeRepresentation::SizedArray:
            if (!elementTypes.insert(get(i)->asSizedArray()->element()))
                return false;
            break;

          default:
            MOZ_ASSUME_UNREACHABLE("Invalid kind for arrayElementType()");
        }
    }
    return elementTypes.build(builder, out);
}

bool
TypeRepresentationSet::fieldNamed(IonBuilder &builder,
                                  jsid id,
                                  size_t *offset,
                                  TypeRepresentationSet *out,
                                  size_t *index)
{
    JS_ASSERT(kind() == TypeRepresentation::Struct);

    
    
    *offset = SIZE_MAX;
    *index = SIZE_MAX;
    *out = TypeRepresentationSet();

    
    size_t offset0;
    size_t index0;
    TypeRepresentationSetBuilder fieldTypes;
    {
        const StructField *field = get(0)->asStruct()->fieldNamed(id);
        if (!field)
            return true;

        offset0 = field->offset;
        index0 = field->index;
        if (!fieldTypes.insert(field->typeRepr))
            return false;
    }

    
    
    for (size_t i = 1; i < length(); i++) {
        const StructField *field = get(i)->asStruct()->fieldNamed(id);
        if (!field)
            return true;

        if (field->offset != offset0)
            return true;

        if (field->index != index0)
            index0 = SIZE_MAX;

        if (!fieldTypes.insert(field->typeRepr))
            return false;
    }

    
    
    
    *offset = offset0;
    *index = index0;
    return fieldTypes.build(builder, out);
}







#include "jit/TypedObjectPrediction.h"

using namespace js;
using namespace jit;

static const size_t ALL_FIELDS = SIZE_MAX;
















void
TypedObjectPrediction::markAsCommonPrefix(const StructTypeDescr &descrA,
                                          const StructTypeDescr &descrB,
                                          size_t max)
{
    
    
    
    if (max > descrA.fieldCount())
        max = descrA.fieldCount();
    if (max > descrB.fieldCount())
        max = descrB.fieldCount();

    size_t i = 0;
    for (; i < max; i++) {
        if (&descrA.fieldName(i) != &descrB.fieldName(i))
            break;
        if (&descrA.fieldDescr(i) != &descrB.fieldDescr(i))
            break;
        MOZ_ASSERT(descrA.fieldOffset(i) == descrB.fieldOffset(i));
    }

    if (i == 0) {
        
        markInconsistent();
    } else {
        setPrefix(descrA, i);
    }
}

void
TypedObjectPrediction::addDescr(const TypeDescr &descr)
{
    switch (predictionKind()) {
      case Empty:
        return setDescr(descr);

      case Inconsistent:
        return; 

      case Descr: {
        if (&descr == data_.descr)
            return; 

        if (descr.kind() != data_.descr->kind())
            return markInconsistent();

        if (descr.kind() != type::Struct)
            return markInconsistent();

        const StructTypeDescr &structDescr = descr.as<StructTypeDescr>();
        const StructTypeDescr &currentDescr = data_.descr->as<StructTypeDescr>();
        markAsCommonPrefix(structDescr, currentDescr, ALL_FIELDS);
        return;
      }

      case Prefix:
        if (descr.kind() != type::Struct)
            return markInconsistent();

        markAsCommonPrefix(*data_.prefix.descr,
                           descr.as<StructTypeDescr>(),
                           data_.prefix.fields);
        return;
    }

    MOZ_CRASH("Bad predictionKind");
}

type::Kind
TypedObjectPrediction::kind() const
{
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        return descr().kind();

      case TypedObjectPrediction::Prefix:
        return prefix().descr->kind();
    }

    MOZ_CRASH("Bad prediction kind");
}

bool
TypedObjectPrediction::ofArrayKind() const
{
    switch (kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
      case type::Struct:
        return false;

      case type::Array:
        return true;
    }

    MOZ_CRASH("Bad kind");
}

bool
TypedObjectPrediction::hasKnownSize(int32_t *out) const
{
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        *out = descr().size();
        return true;

      case TypedObjectPrediction::Prefix:
        
        
        return false;
    }

    MOZ_CRASH("Bad prediction kind");
}

const TypedProto *
TypedObjectPrediction::getKnownPrototype() const
{
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        return nullptr;

      case TypedObjectPrediction::Descr:
        if (descr().is<ComplexTypeDescr>())
            return &descr().as<ComplexTypeDescr>().instancePrototype();
        return nullptr;

      case TypedObjectPrediction::Prefix:
        
        
        return nullptr;
    }

    MOZ_CRASH("Bad prediction kind");
}

template<typename T>
typename T::Type
TypedObjectPrediction::extractType() const
{
    MOZ_ASSERT(kind() == T::Kind);
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        return descr().as<T>().type();

      case TypedObjectPrediction::Prefix:
        break; 
    }

    MOZ_CRASH("Bad prediction kind");
}

ScalarTypeDescr::Type
TypedObjectPrediction::scalarType() const
{
    return extractType<ScalarTypeDescr>();
}

ReferenceTypeDescr::Type
TypedObjectPrediction::referenceType() const
{
    return extractType<ReferenceTypeDescr>();
}

SimdTypeDescr::Type
TypedObjectPrediction::simdType() const
{
    return extractType<SimdTypeDescr>();
}

bool
TypedObjectPrediction::hasKnownArrayLength(int32_t *length) const
{
    MOZ_ASSERT(ofArrayKind());
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        
        
        if (descr().is<ArrayTypeDescr>()) {
            *length = descr().as<ArrayTypeDescr>().length();
            return true;
        }
        return false;

      case TypedObjectPrediction::Prefix:
        break; 
    }
    MOZ_CRASH("Bad prediction kind");
}

TypedObjectPrediction
TypedObjectPrediction::arrayElementType() const
{
    MOZ_ASSERT(ofArrayKind());
    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        return TypedObjectPrediction(descr().as<ArrayTypeDescr>().elementType());

      case TypedObjectPrediction::Prefix:
        break; 
    }
    MOZ_CRASH("Bad prediction kind");
}

bool
TypedObjectPrediction::hasFieldNamedPrefix(const StructTypeDescr &descr,
                                           size_t fieldCount,
                                           jsid id,
                                           size_t *fieldOffset,
                                           TypedObjectPrediction *out,
                                           size_t *index) const
{
    
    if (!descr.fieldIndex(id, index))
        return false;

    
    if (*index >= fieldCount)
        return false;

    
    *fieldOffset = descr.fieldOffset(*index);
    *out = TypedObjectPrediction(descr.fieldDescr(*index));
    return true;
}

bool
TypedObjectPrediction::hasFieldNamed(jsid id,
                                     size_t *fieldOffset,
                                     TypedObjectPrediction *fieldType,
                                     size_t *fieldIndex) const
{
    MOZ_ASSERT(kind() == type::Struct);

    switch (predictionKind()) {
      case TypedObjectPrediction::Empty:
      case TypedObjectPrediction::Inconsistent:
        break;

      case TypedObjectPrediction::Descr:
        return hasFieldNamedPrefix(
            descr().as<StructTypeDescr>(), ALL_FIELDS,
            id, fieldOffset, fieldType, fieldIndex);

      case TypedObjectPrediction::Prefix:
        return hasFieldNamedPrefix(
            *prefix().descr, prefix().fields,
            id, fieldOffset, fieldType, fieldIndex);
    }
    MOZ_CRASH("Bad prediction kind");
}

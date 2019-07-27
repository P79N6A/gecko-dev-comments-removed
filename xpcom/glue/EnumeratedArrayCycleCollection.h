





#ifndef EnumeratedArrayCycleCollection_h_
#define EnumeratedArrayCycleCollection_h_

#include "mozilla/EnumeratedArray.h"
#include "nsCycleCollectionTraversalCallback.h"

template<typename IndexType,
         MOZ_TEMPLATE_ENUM_CLASS_ENUM_TYPE(IndexType) SizeAsEnumValue,
         typename ValueType>
inline void
ImplCycleCollectionUnlink(mozilla::EnumeratedArray<IndexType,
                                                   SizeAsEnumValue,
                                                   ValueType>& aField)
{
  for (size_t i = 0; i < size_t(SizeAsEnumValue); ++i) {
    aField[IndexType(i)] = nullptr;
  }
}

template<typename IndexType,
         MOZ_TEMPLATE_ENUM_CLASS_ENUM_TYPE(IndexType) SizeAsEnumValue,
         typename ValueType>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            mozilla::EnumeratedArray<IndexType,
                                                     SizeAsEnumValue,
                                                     ValueType>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aFlags |= CycleCollectionEdgeNameArrayFlag;
  for (size_t i = 0; i < size_t(SizeAsEnumValue); ++i) {
    ImplCycleCollectionTraverse(aCallback, aField[IndexType(i)], aName, aFlags);
  }
}

#endif 









#ifndef mozilla_EnumeratedArray_h
#define mozilla_EnumeratedArray_h

#include "mozilla/Array.h"
#include "mozilla/TypedEnum.h"

namespace mozilla {


























template<typename IndexType,
         MOZ_TEMPLATE_ENUM_CLASS_ENUM_TYPE(IndexType) SizeAsEnumValue,
         typename ValueType>
class EnumeratedArray
{
  public:
    static const size_t Size = size_t(SizeAsEnumValue);

  private:
    Array<ValueType, Size> mArray;

  public:
    EnumeratedArray() {}

    explicit EnumeratedArray(const EnumeratedArray& aOther)
    {
      for (size_t i = 0; i < Size; i++)
        mArray[i] = aOther.mArray[i];
    }

    ValueType& operator[](IndexType aIndex)
    {
      return mArray[size_t(aIndex)];
    }

    const ValueType& operator[](IndexType aIndex) const
    {
      return mArray[size_t(aIndex)];
    }
};

} 

#endif 

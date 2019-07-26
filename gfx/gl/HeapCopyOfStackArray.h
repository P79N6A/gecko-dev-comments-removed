





#ifndef HEAPCOPYOFSTACKARRAY_H_
#define HEAPCOPYOFSTACKARRAY_H_

#include "mozilla/Attributes.h"
#include "mozilla/Scoped.h"

#include <string.h>

namespace mozilla {





template <typename ElemType>
class HeapCopyOfStackArray
{
public:
  template<size_t N>
  HeapCopyOfStackArray(ElemType (&array)[N])
    : mArrayLength(N)
    , mArrayData(new ElemType[N])
  {
    memcpy(mArrayData, &array[0], N * sizeof(ElemType));
  }

  ElemType* Data() const { return mArrayData; }
  size_t ArrayLength() const { return mArrayLength; }
  size_t ByteLength() const { return mArrayLength * sizeof(ElemType); }

private:
  HeapCopyOfStackArray() MOZ_DELETE;
  HeapCopyOfStackArray(const HeapCopyOfStackArray&) MOZ_DELETE;

  const size_t mArrayLength;
  ScopedDeletePtr<ElemType> const mArrayData;
};

}

#endif 
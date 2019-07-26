










#ifndef mozilla_Poison_h_
#define mozilla_Poison_h_

#include "mozilla/Assertions.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Types.h"

MOZ_BEGIN_EXTERN_C

extern MFBT_DATA uintptr_t gMozillaPoisonValue;




inline uintptr_t mozPoisonValue()
{
  return gMozillaPoisonValue;
}







inline void mozWritePoison(void* aPtr, size_t aSize)
{
  MOZ_ASSERT((uintptr_t)aPtr % sizeof(uintptr_t) == 0, "bad alignment");
  MOZ_ASSERT(aSize >= sizeof(uintptr_t), "poisoning this object has no effect");
  const uintptr_t POISON = mozPoisonValue();
  char* p = (char*)aPtr;
  char* limit = p + aSize;
  for (; p < limit; p += sizeof(uintptr_t)) {
    *((uintptr_t*)p) = POISON;
  }
}





extern MFBT_API void mozPoisonValueInit();


extern MFBT_DATA uintptr_t gMozillaPoisonBase;
extern MFBT_DATA uintptr_t gMozillaPoisonSize;

MOZ_END_EXTERN_C

#endif 

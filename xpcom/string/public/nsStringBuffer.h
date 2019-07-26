





#ifndef nsStringBuffer_h__
#define nsStringBuffer_h__

#include "mozilla/Atomics.h"
#include "mozilla/MemoryReporting.h"

template<class T> struct already_AddRefed;










class nsStringBuffer
{
private:
  friend class CheckStaticAtomSizes;

  mozilla::Atomic<int32_t> mRefCount;
  uint32_t mStorageSize;

public:

  













  static already_AddRefed<nsStringBuffer> Alloc(size_t aStorageSize);

  










  static nsStringBuffer* Realloc(nsStringBuffer* aBuf, size_t aStorageSize);

  


  void NS_FASTCALL AddRef();

  



  void NS_FASTCALL Release();

  




  static nsStringBuffer* FromData(void* aData)
  {
    return reinterpret_cast<nsStringBuffer*>(aData) - 1;
  }

  


  void* Data() const
  {
    return const_cast<char*>(reinterpret_cast<const char*>(this + 1));
  }

  




  uint32_t StorageSize() const
  {
    return mStorageSize;
  }

  







  bool IsReadonly() const
  {
    return mRefCount > 1;
  }

  







  static nsStringBuffer* FromString(const nsAString& aStr);
  static nsStringBuffer* FromString(const nsACString& aStr);

  













  void ToString(uint32_t aLen, nsAString& aStr, bool aMoveOwnership = false);
  void ToString(uint32_t aLen, nsACString& aStr, bool aMoveOwnership = false);

  



  size_t SizeOfIncludingThisMustBeUnshared(mozilla::MallocSizeOf aMallocSizeOf) const;

  


  size_t SizeOfIncludingThisIfUnshared(mozilla::MallocSizeOf aMallocSizeOf) const;

  








  size_t SizeOfIncludingThisEvenIfShared(mozilla::MallocSizeOf aMallocSizeOf) const;
};

#endif 

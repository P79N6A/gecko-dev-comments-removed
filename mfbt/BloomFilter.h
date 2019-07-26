












#ifndef mozilla_BloomFilter_h
#define mozilla_BloomFilter_h

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"

#include <stdint.h>
#include <string.h>

namespace mozilla {































template<unsigned KeySize, class T>
class BloomFilter
{
  















































public:
  BloomFilter()
  {
    static_assert(KeySize <= kKeyShift, "KeySize too big");

    
    
    clear();
  }

  




  void clear();

  


  void add(const T* aValue);

  


  void remove(const T* aValue);

  





  bool mightContain(const T* aValue) const;

  


  void add(uint32_t aHash);
  void remove(uint32_t aHash);
  bool mightContain(uint32_t aHash) const;

private:
  static const size_t kArraySize = (1 << KeySize);
  static const uint32_t kKeyMask = (1 << KeySize) - 1;
  static const uint32_t kKeyShift = 16;

  static uint32_t hash1(uint32_t aHash)
  {
    return aHash & kKeyMask;
  }
  static uint32_t hash2(uint32_t aHash)
  {
    return (aHash >> kKeyShift) & kKeyMask;
  }

  uint8_t& firstSlot(uint32_t aHash)
  {
    return mCounters[hash1(aHash)];
  }
  uint8_t& secondSlot(uint32_t aHash)
  {
    return mCounters[hash2(aHash)];
  }

  const uint8_t& firstSlot(uint32_t aHash) const
  {
    return mCounters[hash1(aHash)];
  }
  const uint8_t& secondSlot(uint32_t aHash) const
  {
    return mCounters[hash2(aHash)];
  }

  static bool full(const uint8_t& aSlot) { return aSlot == UINT8_MAX; }

  uint8_t mCounters[kArraySize];
};

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::clear()
{
  memset(mCounters, 0, kArraySize);
}

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::add(uint32_t aHash)
{
  uint8_t& slot1 = firstSlot(aHash);
  if (MOZ_LIKELY(!full(slot1))) {
    ++slot1;
  }
  uint8_t& slot2 = secondSlot(aHash); {
  if (MOZ_LIKELY(!full(slot2)))
    ++slot2;
  }
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE void
BloomFilter<KeySize, T>::add(const T* aValue)
{
  uint32_t hash = aValue->hash();
  return add(hash);
}

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::remove(uint32_t aHash)
{
  
  
  uint8_t& slot1 = firstSlot(aHash);
  if (MOZ_LIKELY(!full(slot1))) {
    --slot1;
  }
  uint8_t& slot2 = secondSlot(aHash);
  if (MOZ_LIKELY(!full(slot2))) {
    --slot2;
  }
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE void
BloomFilter<KeySize, T>::remove(const T* aValue)
{
  uint32_t hash = aValue->hash();
  remove(hash);
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE bool
BloomFilter<KeySize, T>::mightContain(uint32_t aHash) const
{
  
  return firstSlot(aHash) && secondSlot(aHash);
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE bool
BloomFilter<KeySize, T>::mightContain(const T* aValue) const
{
  uint32_t hash = aValue->hash();
  return mightContain(hash);
}

} 

#endif 

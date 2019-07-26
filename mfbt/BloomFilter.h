











#ifndef mozilla_BloomFilter_h_
#define mozilla_BloomFilter_h_

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"

#include <string.h>

namespace mozilla {































template<unsigned KeySize, class T>
class BloomFilter
{
    















































  public:
    BloomFilter() {
        MOZ_STATIC_ASSERT(KeySize <= keyShift, "KeySize too big");

        
        
        clear();
    }

    




    void clear();

    


    void add(const T* t);

    


    void remove(const T* t);

    





    bool mightContain(const T* t) const;

    


    void add(uint32_t hash);
    void remove(uint32_t hash);
    bool mightContain(uint32_t hash) const;

  private:
    static const size_t arraySize = (1 << KeySize);
    static const uint32_t keyMask = (1 << KeySize) - 1;
    static const uint32_t keyShift = 16;

    static uint32_t hash1(uint32_t hash) { return hash & keyMask; }
    static uint32_t hash2(uint32_t hash) { return (hash >> keyShift) & keyMask; }

    uint8_t& firstSlot(uint32_t hash) { return counters[hash1(hash)]; }
    uint8_t& secondSlot(uint32_t hash) { return counters[hash2(hash)]; }
    const uint8_t& firstSlot(uint32_t hash) const { return counters[hash1(hash)]; }
    const uint8_t& secondSlot(uint32_t hash) const { return counters[hash2(hash)]; }

    static bool full(const uint8_t& slot) { return slot == UINT8_MAX; }

    uint8_t counters[arraySize];
};

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::clear()
{
  memset(counters, 0, arraySize);
}

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::add(uint32_t hash)
{
  uint8_t& slot1 = firstSlot(hash);
  if (MOZ_LIKELY(!full(slot1)))
    ++slot1;

  uint8_t& slot2 = secondSlot(hash);
  if (MOZ_LIKELY(!full(slot2)))
    ++slot2;
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE void
BloomFilter<KeySize, T>::add(const T* t)
{
  uint32_t hash = t->hash();
  return add(hash);
}

template<unsigned KeySize, class T>
inline void
BloomFilter<KeySize, T>::remove(uint32_t hash)
{
  
  
  uint8_t& slot1 = firstSlot(hash);
  if (MOZ_LIKELY(!full(slot1)))
    --slot1;

  uint8_t& slot2 = secondSlot(hash);
  if (MOZ_LIKELY(!full(slot2)))
    --slot2;
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE void
BloomFilter<KeySize, T>::remove(const T* t)
{
  uint32_t hash = t->hash();
  remove(hash);
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE bool
BloomFilter<KeySize, T>::mightContain(uint32_t hash) const
{
  
  return firstSlot(hash) && secondSlot(hash);
}

template<unsigned KeySize, class T>
MOZ_ALWAYS_INLINE bool
BloomFilter<KeySize, T>::mightContain(const T* t) const
{
  uint32_t hash = t->hash();
  return mightContain(hash);
}

} 

#endif 

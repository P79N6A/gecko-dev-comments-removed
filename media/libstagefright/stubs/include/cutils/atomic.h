


#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <stdint.h>





static inline int32_t
android_atomic_dec(volatile int32_t* aValue)
{
  return (*aValue)--;
}

static inline int32_t
android_atomic_inc(volatile int32_t* aValue)
{
  return (*aValue)++;
}

static inline int32_t
android_atomic_or(int32_t aModifier, volatile int32_t* aValue)
{
  int32_t ret = *aValue;
  *aValue |= aModifier;
  return ret;
}

static inline int32_t
android_atomic_add(int32_t aModifier, volatile int32_t* aValue)
{
  int32_t ret = *aValue;
  *aValue += aModifier;
  return ret;
}

static inline int32_t
android_atomic_cmpxchg(int32_t aOld, int32_t aNew, volatile int32_t* aValue)
{
  if (*aValue == aOld)
  {
    return *aValue = aNew;
  }
  return aOld;
}

#endif

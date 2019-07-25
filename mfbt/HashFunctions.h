








#ifndef mozilla_HashFunctions_h_
#define mozilla_HashFunctions_h_

#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"

#ifdef __cplusplus
namespace mozilla {




static const uint32_t GoldenRatioU32 = 0x9E3779B9U;

inline uint32_t
RotateLeft32(uint32_t value, uint8_t bits)
{
  MOZ_ASSERT(bits < 32);
  return (value << bits) | (value >> (32 - bits));
}






MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uint32_t value)
{
  





































  return GoldenRatioU32 * (RotateLeft32(hash, 5) ^ value);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uint32_t v1, uint32_t v2)
{
  return AddToHash(AddToHash(hash, v1), v2);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uint32_t v1, uint32_t v2, uint32_t v3)
{
  return AddToHash(AddToHash(hash, v1, v2), v3);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4)
{
  return AddToHash(AddToHash(hash, v1, v2, v3), v4);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5)
{
  return AddToHash(AddToHash(hash, v1, v2, v3, v4), v5);
}

} 
#endif 
#endif 

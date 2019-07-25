









































#ifndef CITY_HASH_H_
#define CITY_HASH_H_

#include "../CityHash.h" 

#include <stdlib.h>  
#include "stdint.h"
#include <utility>

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef std::pair<uint64, uint64> uint128;

inline uint64 Uint128Low64(const uint128& x) { return x.first; }
inline uint64 Uint128High64(const uint128& x) { return x.second; }


uint64 CityHash64(const char *buf, size_t len);



uint64 CityHash64WithSeed(const char *buf, size_t len, uint64 seed);



uint64 CityHash64WithSeeds(const char *buf, size_t len,
                           uint64 seed0, uint64 seed1);


uint128 CityHash128(const char *s, size_t len);



uint128 CityHash128WithSeed(const char *s, size_t len, uint128 seed);



inline uint64 Hash128to64(const uint128& x) {
  
  const uint64 kMul = 0x9ddfea08eb382d69ULL;
  uint64 a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
  a ^= (a >> 47);
  uint64 b = (Uint128High64(x) ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}

#endif  

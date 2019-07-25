




























#include "city.h"

#include <algorithm>

using namespace std;

#define UNALIGNED_LOAD64(p) (*(const uint64*)(p))
#define UNALIGNED_LOAD32(p) (*(const uint32*)(p))

#if !defined(LIKELY)
#if defined(__GNUC__)
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define LIKELY(x) (x)
#endif
#endif


static const uint64 k0 = 0xc3a5c85c97cb3127ULL;
static const uint64 k1 = 0xb492b66fbe98f273ULL;
static const uint64 k2 = 0x9ae16a3b2f90404fULL;
static const uint64 k3 = 0xc949d7c7509e6557ULL;



static uint64 Rotate(uint64 val, int shift) {
  
  return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}




static uint64 RotateByAtLeast1(uint64 val, int shift) {
  return (val >> shift) | (val << (64 - shift));
}

static uint64 ShiftMix(uint64 val) {
  return val ^ (val >> 47);
}

static uint64 HashLen16(uint64 u, uint64 v) {
  return Hash128to64(uint128(u, v));
}

static uint64 HashLen0to16(const char *s, size_t len) {
  if (len > 8) {
    uint64 a = UNALIGNED_LOAD64(s);
    uint64 b = UNALIGNED_LOAD64(s + len - 8);
    return HashLen16(a, RotateByAtLeast1(b + len, len)) ^ b;
  }
  if (len >= 4) {
    uint64 a = UNALIGNED_LOAD32(s);
    return HashLen16(len + (a << 3), UNALIGNED_LOAD32(s + len - 4));
  }
  if (len > 0) {
    uint8 a = s[0];
    uint8 b = s[len >> 1];
    uint8 c = s[len - 1];
    uint32 y = static_cast<uint32>(a) + (static_cast<uint32>(b) << 8);
    uint32 z = len + (static_cast<uint32>(c) << 2);
    return ShiftMix(y * k2 ^ z * k3) * k2;
  }
  return k2;
}



static uint64 HashLen17to32(const char *s, size_t len) {
  uint64 a = UNALIGNED_LOAD64(s) * k1;
  uint64 b = UNALIGNED_LOAD64(s + 8);
  uint64 c = UNALIGNED_LOAD64(s + len - 8) * k2;
  uint64 d = UNALIGNED_LOAD64(s + len - 16) * k0;
  return HashLen16(Rotate(a - b, 43) + Rotate(c, 30) + d,
                   a + Rotate(b ^ k3, 20) - c + len);
}



static pair<uint64, uint64> WeakHashLen32WithSeeds(
    uint64 w, uint64 x, uint64 y, uint64 z, uint64 a, uint64 b) {
  a += w;
  b = Rotate(b + a + z, 21);
  uint64 c = a;
  a += x;
  a += y;
  b += Rotate(a, 44);
  return make_pair(a + z, b + c);
}


static pair<uint64, uint64> WeakHashLen32WithSeeds(
    const char* s, uint64 a, uint64 b) {
  return WeakHashLen32WithSeeds(UNALIGNED_LOAD64(s),
                                UNALIGNED_LOAD64(s + 8),
                                UNALIGNED_LOAD64(s + 16),
                                UNALIGNED_LOAD64(s + 24),
                                a,
                                b);
}


static uint64 HashLen33to64(const char *s, size_t len) {
  uint64 z = UNALIGNED_LOAD64(s + 24);
  uint64 a = UNALIGNED_LOAD64(s) + (len + UNALIGNED_LOAD64(s + len - 16)) * k0;
  uint64 b = Rotate(a + z, 52);
  uint64 c = Rotate(a, 37);
  a += UNALIGNED_LOAD64(s + 8);
  c += Rotate(a, 7);
  a += UNALIGNED_LOAD64(s + 16);
  uint64 vf = a + z;
  uint64 vs = b + Rotate(a, 31) + c;
  a = UNALIGNED_LOAD64(s + 16) + UNALIGNED_LOAD64(s + len - 32);
  z = UNALIGNED_LOAD64(s + len - 8);
  b = Rotate(a + z, 52);
  c = Rotate(a, 37);
  a += UNALIGNED_LOAD64(s + len - 24);
  c += Rotate(a, 7);
  a += UNALIGNED_LOAD64(s + len - 16);
  uint64 wf = a + z;
  uint64 ws = b + Rotate(a, 31) + c;
  uint64 r = ShiftMix((vf + ws) * k2 + (wf + vs) * k0);
  return ShiftMix(r * k0 + vs) * k2;
}

uint64 CityHash64(const char *s, size_t len) {
  if (len <= 32) {
    if (len <= 16) {
      return HashLen0to16(s, len);
    } else {
      return HashLen17to32(s, len);
    }
  } else if (len <= 64) {
    return HashLen33to64(s, len);
  }

  
  
  uint64 x = UNALIGNED_LOAD64(s);
  uint64 y = UNALIGNED_LOAD64(s + len - 16) ^ k1;
  uint64 z = UNALIGNED_LOAD64(s + len - 56) ^ k0;
  pair<uint64, uint64> v = WeakHashLen32WithSeeds(s + len - 64, len, y);
  pair<uint64, uint64> w = WeakHashLen32WithSeeds(s + len - 32, len * k1, k0);
  z += ShiftMix(v.second) * k1;
  x = Rotate(z + x, 39) * k1;
  y = Rotate(y, 33) * k1;

  
  len = (len - 1) & ~static_cast<size_t>(63);
  do {
    x = Rotate(x + y + v.first + UNALIGNED_LOAD64(s + 16), 37) * k1;
    y = Rotate(y + v.second + UNALIGNED_LOAD64(s + 48), 42) * k1;
    x ^= w.second;
    y ^= v.first;
    z = Rotate(z ^ w.first, 33);
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y);
    std::swap(z, x);
    s += 64;
    len -= 64;
  } while (len != 0);
  return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z,
                   HashLen16(v.second, w.second) + x);
}

uint64 CityHash64WithSeed(const char *s, size_t len, uint64 seed) {
  return CityHash64WithSeeds(s, len, k2, seed);
}

uint64 CityHash64WithSeeds(const char *s, size_t len,
                           uint64 seed0, uint64 seed1) {
  return HashLen16(CityHash64(s, len) - seed0, seed1);
}



static uint128 CityMurmur(const char *s, size_t len, uint128 seed) {
  uint64 a = Uint128Low64(seed);
  uint64 b = Uint128High64(seed);
  uint64 c = 0;
  uint64 d = 0;
  ssize_t l = len - 16;
  if (l <= 0) {  
    c = b * k1 + HashLen0to16(s, len);
    d = Rotate(a + (len >= 8 ? UNALIGNED_LOAD64(s) : c), 32);
  } else {  
    c = HashLen16(UNALIGNED_LOAD64(s + len - 8) + k1, a);
    d = HashLen16(b + len, c + UNALIGNED_LOAD64(s + len - 16));
    a += d;
    do {
      a ^= ShiftMix(UNALIGNED_LOAD64(s) * k1) * k1;
      a *= k1;
      b ^= a;
      c ^= ShiftMix(UNALIGNED_LOAD64(s + 8) * k1) * k1;
      c *= k1;
      d ^= c;
      s += 16;
      l -= 16;
    } while (l > 0);
  }
  a = HashLen16(a, c);
  b = HashLen16(d, b);
  return uint128(a ^ b, HashLen16(b, a));
}

uint128 CityHash128WithSeed(const char *s, size_t len, uint128 seed) {
  if (len < 128) {
    return CityMurmur(s, len, seed);
  }

  
  
  pair<uint64, uint64> v, w;
  uint64 x = Uint128Low64(seed);
  uint64 y = Uint128High64(seed);
  uint64 z = len * k1;
  v.first = Rotate(y ^ k1, 49) * k1 + UNALIGNED_LOAD64(s);
  v.second = Rotate(v.first, 42) * k1 + UNALIGNED_LOAD64(s + 8);
  w.first = Rotate(y + z, 35) * k1 + x;
  w.second = Rotate(x + UNALIGNED_LOAD64(s + 88), 53) * k1;

  
  do {
    x = Rotate(x + y + v.first + UNALIGNED_LOAD64(s + 16), 37) * k1;
    y = Rotate(y + v.second + UNALIGNED_LOAD64(s + 48), 42) * k1;
    x ^= w.second;
    y ^= v.first;
    z = Rotate(z ^ w.first, 33);
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y);
    std::swap(z, x);
    s += 64;
    x = Rotate(x + y + v.first + UNALIGNED_LOAD64(s + 16), 37) * k1;
    y = Rotate(y + v.second + UNALIGNED_LOAD64(s + 48), 42) * k1;
    x ^= w.second;
    y ^= v.first;
    z = Rotate(z ^ w.first, 33);
    v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
    w = WeakHashLen32WithSeeds(s + 32, z + w.second, y);
    std::swap(z, x);
    s += 64;
    len -= 128;
  } while (LIKELY(len >= 128));
  y += Rotate(w.first, 37) * k0 + z;
  x += Rotate(v.first + z, 49) * k0;
  
  for (size_t tail_done = 0; tail_done < len; ) {
    tail_done += 32;
    y = Rotate(y - x, 42) * k0 + v.second;
    w.first += UNALIGNED_LOAD64(s + len - tail_done + 16);
    x = Rotate(x, 49) * k0 + w.first;
    w.first += v.first;
    v = WeakHashLen32WithSeeds(s + len - tail_done, v.first, v.second);
  }
  
  
  
  x = HashLen16(x, v.first);
  y = HashLen16(y, w.first);
  return uint128(HashLen16(x + v.second, w.second) + y,
                 HashLen16(x + w.second, y + v.second));
}

uint128 CityHash128(const char *s, size_t len) {
  if (len >= 16) {
    return CityHash128WithSeed(s + 16,
                               len - 16,
                               uint128(UNALIGNED_LOAD64(s) ^ k3,
                                       UNALIGNED_LOAD64(s + 8)));
  } else if (len >= 8) {
    return CityHash128WithSeed(NULL,
                               0,
                               uint128(UNALIGNED_LOAD64(s) ^ (len * k0),
                                       UNALIGNED_LOAD64(s + len - 8) ^ k1));
  } else {
    return CityHash128WithSeed(s, len, uint128(k0, k1));
  }
}

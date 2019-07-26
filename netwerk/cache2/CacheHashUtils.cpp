



#include "CacheHashUtils.h"

#include "plstr.h"

namespace mozilla {
namespace net {










static inline void hashmix(uint32_t& a, uint32_t& b, uint32_t& c)
{
  a -= b; a -= c; a ^= (c>>13);
  b -= c; b -= a; b ^= (a<<8);
  c -= a; c -= b; c ^= (b>>13);
  a -= b; a -= c; a ^= (c>>12);
  b -= c; b -= a; b ^= (a<<16);
  c -= a; c -= b; c ^= (b>>5);
  a -= b; a -= c; a ^= (c>>3);
  b -= c; b -= a; b ^= (a<<10);
  c -= a; c -= b; c ^= (b>>15);
}

CacheHashUtils::Hash32_t
CacheHashUtils::Hash(const char *aData, uint32_t aSize, uint32_t aInitval)
{
  const uint8_t *k = reinterpret_cast<const uint8_t*>(aData);
  uint32_t a, b, c, len;


  
  len = aSize;
  a = b = 0x9e3779b9;  
  c = aInitval;        

  
  while (len >= 12)
  {
    a += k[0] + (uint32_t(k[1])<<8) + (uint32_t(k[2])<<16) + (uint32_t(k[3])<<24);
    b += k[4] + (uint32_t(k[5])<<8) + (uint32_t(k[6])<<16) + (uint32_t(k[7])<<24);
    c += k[8] + (uint32_t(k[9])<<8) + (uint32_t(k[10])<<16) + (uint32_t(k[11])<<24);
    hashmix(a, b, c);
    k += 12; len -= 12;
  }

  
  c += aSize;
  switch(len) {              
    case 11: c += (uint32_t(k[10])<<24);
    case 10: c += (uint32_t(k[9])<<16);
    case 9 : c += (uint32_t(k[8])<<8);
    
    case 8 : b += (uint32_t(k[7])<<24);
    case 7 : b += (uint32_t(k[6])<<16);
    case 6 : b += (uint32_t(k[5])<<8);
    case 5 : b += k[4];
    case 4 : a += (uint32_t(k[3])<<24);
    case 3 : a += (uint32_t(k[2])<<16);
    case 2 : a += (uint32_t(k[1])<<8);
    case 1 : a += k[0];
    
  }
  hashmix(a, b, c);

  return c;
}

CacheHashUtils::Hash16_t
CacheHashUtils::Hash16(const char *aData, uint32_t aSize, uint32_t aInitval)
{
  Hash32_t hash = Hash(aData, aSize, aInitval);
  return (hash & 0xFFFF);
}

} 
} 


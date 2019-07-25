






















#ifndef mozilla_SHA1_h_
#define mozilla_SHA1_h_

#include <stdint.h>
namespace mozilla {
class SHA1Sum {
  union {
    uint32_t w[16];         
    uint8_t  b[64];
  } u;
  uint64_t size;            
  unsigned H[22];           
  bool mDone;

public:
  static const unsigned int HashSize = 20;
  SHA1Sum();
  void update(const uint8_t *dataIn, uint32_t len);
  void finish(uint8_t hashout[20]);
};
}

#endif 

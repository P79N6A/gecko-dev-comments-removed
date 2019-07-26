









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_RANDOM_VECTOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_RANDOM_VECTOR_H_

#include <string.h>  

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class RandomVector {
 public:
  static const int kRandomTableSize = 256;
  static const int16_t kRandomTable[kRandomTableSize];

  RandomVector()
      : seed_(777),
        seed_increment_(1) {
  }

  void Reset();

  void Generate(size_t length, int16_t* output);

  void IncreaseSeedIncrement(int16_t increase_by);

  
  int16_t seed_increment() { return seed_increment_; }
  void set_seed_increment(int16_t value) { seed_increment_ = value; }

 private:
  uint32_t seed_;
  int16_t seed_increment_;

  DISALLOW_COPY_AND_ASSIGN(RandomVector);
};

}  
#endif  

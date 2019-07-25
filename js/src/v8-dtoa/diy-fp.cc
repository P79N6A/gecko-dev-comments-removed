


























#include "v8.h"

#include "diy-fp.h"

namespace v8 {
namespace internal {

void DiyFp::Multiply(const DiyFp& other) {
  
  
  
  
  const uint64_t kM32 = 0xFFFFFFFFu;
  uint64_t a = f_ >> 32;
  uint64_t b = f_ & kM32;
  uint64_t c = other.f_ >> 32;
  uint64_t d = other.f_ & kM32;
  uint64_t ac = a * c;
  uint64_t bc = b * c;
  uint64_t ad = a * d;
  uint64_t bd = b * d;
  uint64_t tmp = (bd >> 32) + (ad & kM32) + (bc & kM32);
  
  
  tmp += 1U << 31;
  uint64_t result_f = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
  e_ += other.e_ + 64;
  f_ = result_f;
}

} }  

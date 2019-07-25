


























#ifndef DOUBLE_CONVERSION_DIY_FP_H_
#define DOUBLE_CONVERSION_DIY_FP_H_

#include "utils.h"

namespace double_conversion {






class DiyFp {
 public:
  static const int kSignificandSize = 64;

  DiyFp() : f_(0), e_(0) {}
  DiyFp(uint64_t f, int e) : f_(f), e_(e) {}

  
  
  
  
  void Subtract(const DiyFp& other) {
    ASSERT(e_ == other.e_);
    ASSERT(f_ >= other.f_);
    f_ -= other.f_;
  }

  
  
  
  static DiyFp Minus(const DiyFp& a, const DiyFp& b) {
    DiyFp result = a;
    result.Subtract(b);
    return result;
  }


  
  void Multiply(const DiyFp& other);

  
  static DiyFp Times(const DiyFp& a, const DiyFp& b) {
    DiyFp result = a;
    result.Multiply(b);
    return result;
  }

  void Normalize() {
    ASSERT(f_ != 0);
    uint64_t f = f_;
    int e = e_;

    
    
    const uint64_t k10MSBits = UINT64_2PART_C(0xFFC00000, 00000000);
    while ((f & k10MSBits) == 0) {
      f <<= 10;
      e -= 10;
    }
    while ((f & kUint64MSB) == 0) {
      f <<= 1;
      e--;
    }
    f_ = f;
    e_ = e;
  }

  static DiyFp Normalize(const DiyFp& a) {
    DiyFp result = a;
    result.Normalize();
    return result;
  }

  uint64_t f() const { return f_; }
  int e() const { return e_; }

  void set_f(uint64_t new_value) { f_ = new_value; }
  void set_e(int new_value) { e_ = new_value; }

 private:
  static const uint64_t kUint64MSB = UINT64_2PART_C(0x80000000, 00000000);

  uint64_t f_;
  int e_;
};

}  

#endif  

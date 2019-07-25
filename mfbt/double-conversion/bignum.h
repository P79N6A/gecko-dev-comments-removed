


























#ifndef DOUBLE_CONVERSION_BIGNUM_H_
#define DOUBLE_CONVERSION_BIGNUM_H_

#include "utils.h"

namespace double_conversion {

class Bignum {
 public:
  
  
  
  static const int kMaxSignificantBits = 3584;

  Bignum();
  void AssignUInt16(uint16_t value);
  void AssignUInt64(uint64_t value);
  void AssignBignum(const Bignum& other);

  void AssignDecimalString(Vector<const char> value);
  void AssignHexString(Vector<const char> value);

  void AssignPowerUInt16(uint16_t base, int exponent);

  void AddUInt16(uint16_t operand);
  void AddUInt64(uint64_t operand);
  void AddBignum(const Bignum& other);
  
  void SubtractBignum(const Bignum& other);

  void Square();
  void ShiftLeft(int shift_amount);
  void MultiplyByUInt32(uint32_t factor);
  void MultiplyByUInt64(uint64_t factor);
  void MultiplyByPowerOfTen(int exponent);
  void Times10() { return MultiplyByUInt32(10); }
  
  
  
  
  uint16_t DivideModuloIntBignum(const Bignum& other);

  bool ToHexString(char* buffer, int buffer_size) const;

  
  
  
  
  static int Compare(const Bignum& a, const Bignum& b);
  static bool Equal(const Bignum& a, const Bignum& b) {
    return Compare(a, b) == 0;
  }
  static bool LessEqual(const Bignum& a, const Bignum& b) {
    return Compare(a, b) <= 0;
  }
  static bool Less(const Bignum& a, const Bignum& b) {
    return Compare(a, b) < 0;
  }
  
  static int PlusCompare(const Bignum& a, const Bignum& b, const Bignum& c);
  
  static bool PlusEqual(const Bignum& a, const Bignum& b, const Bignum& c) {
    return PlusCompare(a, b, c) == 0;
  }
  
  static bool PlusLessEqual(const Bignum& a, const Bignum& b, const Bignum& c) {
    return PlusCompare(a, b, c) <= 0;
  }
  
  static bool PlusLess(const Bignum& a, const Bignum& b, const Bignum& c) {
    return PlusCompare(a, b, c) < 0;
  }
 private:
  typedef uint32_t Chunk;
  typedef uint64_t DoubleChunk;

  static const int kChunkSize = sizeof(Chunk) * 8;
  static const int kDoubleChunkSize = sizeof(DoubleChunk) * 8;
  
  
  static const int kBigitSize = 28;
  static const Chunk kBigitMask = (1 << kBigitSize) - 1;
  
  
  static const int kBigitCapacity = kMaxSignificantBits / kBigitSize;

  void EnsureCapacity(int size) {
    if (size > kBigitCapacity) {
      UNREACHABLE();
    }
  }
  void Align(const Bignum& other);
  void Clamp();
  bool IsClamped() const;
  void Zero();
  
  
  
  void BigitsShiftLeft(int shift_amount);
  
  int BigitLength() const { return used_digits_ + exponent_; }
  Chunk BigitAt(int index) const;
  void SubtractTimes(const Bignum& other, int factor);

  Chunk bigits_buffer_[kBigitCapacity];
  
  
  Vector<Chunk> bigits_;
  int used_digits_;
  
  int exponent_;

  DISALLOW_COPY_AND_ASSIGN(Bignum);
};

}  

#endif  




























#ifndef V8_DOUBLE_H_
#define V8_DOUBLE_H_

#include "diy-fp.h"

namespace v8 {
namespace internal {


static uint64_t double_to_uint64(double d) { return BitCast<uint64_t>(d); }
static double uint64_to_double(uint64_t d64) { return BitCast<double>(d64); }


class Double {
 public:
  static const uint64_t kSignMask = V8_2PART_UINT64_C(0x80000000, 00000000);
  static const uint64_t kExponentMask = V8_2PART_UINT64_C(0x7FF00000, 00000000);
  static const uint64_t kSignificandMask =
      V8_2PART_UINT64_C(0x000FFFFF, FFFFFFFF);
  static const uint64_t kHiddenBit = V8_2PART_UINT64_C(0x00100000, 00000000);

  Double() : d64_(0) {}
  explicit Double(double d) : d64_(double_to_uint64(d)) {}
  explicit Double(uint64_t d64) : d64_(d64) {}

  DiyFp AsDiyFp() const {
    ASSERT(!IsSpecial());
    return DiyFp(Significand(), Exponent());
  }

  
  DiyFp AsNormalizedDiyFp() const {
    uint64_t f = Significand();
    int e = Exponent();

    ASSERT(f != 0);

    
    while ((f & kHiddenBit) == 0) {
      f <<= 1;
      e--;
    }
    
    f <<= DiyFp::kSignificandSize - kSignificandSize - 1;
    e -= DiyFp::kSignificandSize - kSignificandSize - 1;
    return DiyFp(f, e);
  }

  
  uint64_t AsUint64() const {
    return d64_;
  }

  int Exponent() const {
    if (IsDenormal()) return kDenormalExponent;

    uint64_t d64 = AsUint64();
    int biased_e = static_cast<int>((d64 & kExponentMask) >> kSignificandSize);
    return biased_e - kExponentBias;
  }

  uint64_t Significand() const {
    uint64_t d64 = AsUint64();
    uint64_t significand = d64 & kSignificandMask;
    if (!IsDenormal()) {
      return significand + kHiddenBit;
    } else {
      return significand;
    }
  }

  
  bool IsDenormal() const {
    uint64_t d64 = AsUint64();
    return (d64 & kExponentMask) == 0;
  }

  
  
  bool IsSpecial() const {
    uint64_t d64 = AsUint64();
    return (d64 & kExponentMask) == kExponentMask;
  }

  bool IsNan() const {
    uint64_t d64 = AsUint64();
    return ((d64 & kExponentMask) == kExponentMask) &&
        ((d64 & kSignificandMask) != 0);
  }


  bool IsInfinite() const {
    uint64_t d64 = AsUint64();
    return ((d64 & kExponentMask) == kExponentMask) &&
        ((d64 & kSignificandMask) == 0);
  }


  int Sign() const {
    uint64_t d64 = AsUint64();
    return (d64 & kSignMask) == 0? 1: -1;
  }


  
  
  
  void NormalizedBoundaries(DiyFp* out_m_minus, DiyFp* out_m_plus) const {
    DiyFp v = this->AsDiyFp();
    bool significand_is_zero = (v.f() == kHiddenBit);
    DiyFp m_plus = DiyFp::Normalize(DiyFp((v.f() << 1) + 1, v.e() - 1));
    DiyFp m_minus;
    if (significand_is_zero && v.e() != kDenormalExponent) {
      
      
      
      
      
      
      m_minus = DiyFp((v.f() << 2) - 1, v.e() - 2);
    } else {
      m_minus = DiyFp((v.f() << 1) - 1, v.e() - 1);
    }
    m_minus.set_f(m_minus.f() << (m_minus.e() - m_plus.e()));
    m_minus.set_e(m_plus.e());
    *out_m_plus = m_plus;
    *out_m_minus = m_minus;
  }

  double value() const { return uint64_to_double(d64_); }

 private:
  static const int kSignificandSize = 52;  
  static const int kExponentBias = 0x3FF + kSignificandSize;
  static const int kDenormalExponent = -kExponentBias + 1;

  uint64_t d64_;
};

} }  

#endif  

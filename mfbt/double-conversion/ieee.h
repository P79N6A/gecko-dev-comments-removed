


























#ifndef DOUBLE_CONVERSION_DOUBLE_H_
#define DOUBLE_CONVERSION_DOUBLE_H_

#include "diy-fp.h"

namespace double_conversion {


static uint64_t double_to_uint64(double d) { return BitCast<uint64_t>(d); }
static double uint64_to_double(uint64_t d64) { return BitCast<double>(d64); }
static uint32_t float_to_uint32(float f) { return BitCast<uint32_t>(f); }
static float uint32_to_float(uint32_t d32) { return BitCast<float>(d32); }


class Double {
 public:
  static const uint64_t kSignMask = UINT64_2PART_C(0x80000000, 00000000);
  static const uint64_t kExponentMask = UINT64_2PART_C(0x7FF00000, 00000000);
  static const uint64_t kSignificandMask = UINT64_2PART_C(0x000FFFFF, FFFFFFFF);
  static const uint64_t kHiddenBit = UINT64_2PART_C(0x00100000, 00000000);
  static const int kPhysicalSignificandSize = 52;  
  static const int kSignificandSize = 53;

  Double() : d64_(0) {}
  explicit Double(double d) : d64_(double_to_uint64(d)) {}
  explicit Double(uint64_t d64) : d64_(d64) {}
  explicit Double(DiyFp diy_fp)
    : d64_(DiyFpToUint64(diy_fp)) {}

  
  
  DiyFp AsDiyFp() const {
    ASSERT(Sign() > 0);
    ASSERT(!IsSpecial());
    return DiyFp(Significand(), Exponent());
  }

  
  DiyFp AsNormalizedDiyFp() const {
    ASSERT(value() > 0.0);
    uint64_t f = Significand();
    int e = Exponent();

    
    while ((f & kHiddenBit) == 0) {
      f <<= 1;
      e--;
    }
    
    f <<= DiyFp::kSignificandSize - kSignificandSize;
    e -= DiyFp::kSignificandSize - kSignificandSize;
    return DiyFp(f, e);
  }

  
  uint64_t AsUint64() const {
    return d64_;
  }

  
  double NextDouble() const {
    if (d64_ == kInfinity) return Double(kInfinity).value();
    if (Sign() < 0 && Significand() == 0) {
      
      return 0.0;
    }
    if (Sign() < 0) {
      return Double(d64_ - 1).value();
    } else {
      return Double(d64_ + 1).value();
    }
  }

  double PreviousDouble() const {
    if (d64_ == (kInfinity | kSignMask)) return -Double::Infinity();
    if (Sign() < 0) {
      return Double(d64_ + 1).value();
    } else {
      if (Significand() == 0) return -0.0;
      return Double(d64_ - 1).value();
    }
  }

  int Exponent() const {
    if (IsDenormal()) return kDenormalExponent;

    uint64_t d64 = AsUint64();
    int biased_e =
        static_cast<int>((d64 & kExponentMask) >> kPhysicalSignificandSize);
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

  
  
  DiyFp UpperBoundary() const {
    ASSERT(Sign() > 0);
    return DiyFp(Significand() * 2 + 1, Exponent() - 1);
  }

  
  
  
  
  void NormalizedBoundaries(DiyFp* out_m_minus, DiyFp* out_m_plus) const {
    ASSERT(value() > 0.0);
    DiyFp v = this->AsDiyFp();
    DiyFp m_plus = DiyFp::Normalize(DiyFp((v.f() << 1) + 1, v.e() - 1));
    DiyFp m_minus;
    if (LowerBoundaryIsCloser()) {
      m_minus = DiyFp((v.f() << 2) - 1, v.e() - 2);
    } else {
      m_minus = DiyFp((v.f() << 1) - 1, v.e() - 1);
    }
    m_minus.set_f(m_minus.f() << (m_minus.e() - m_plus.e()));
    m_minus.set_e(m_plus.e());
    *out_m_plus = m_plus;
    *out_m_minus = m_minus;
  }

  bool LowerBoundaryIsCloser() const {
    
    
    
    
    
    
    
    
    bool physical_significand_is_zero = ((AsUint64() & kSignificandMask) == 0);
    return physical_significand_is_zero && (Exponent() != kDenormalExponent);
  }

  double value() const { return uint64_to_double(d64_); }

  
  
  
  
  
  
  static int SignificandSizeForOrderOfMagnitude(int order) {
    if (order >= (kDenormalExponent + kSignificandSize)) {
      return kSignificandSize;
    }
    if (order <= kDenormalExponent) return 0;
    return order - kDenormalExponent;
  }

  static double Infinity() {
    return Double(kInfinity).value();
  }

  static double NaN() {
    return Double(kNaN).value();
  }

 private:
  static const int kExponentBias = 0x3FF + kPhysicalSignificandSize;
  static const int kDenormalExponent = -kExponentBias + 1;
  static const int kMaxExponent = 0x7FF - kExponentBias;
  static const uint64_t kInfinity = UINT64_2PART_C(0x7FF00000, 00000000);
  static const uint64_t kNaN = UINT64_2PART_C(0x7FF80000, 00000000);

  const uint64_t d64_;

  static uint64_t DiyFpToUint64(DiyFp diy_fp) {
    uint64_t significand = diy_fp.f();
    int exponent = diy_fp.e();
    while (significand > kHiddenBit + kSignificandMask) {
      significand >>= 1;
      exponent++;
    }
    if (exponent >= kMaxExponent) {
      return kInfinity;
    }
    if (exponent < kDenormalExponent) {
      return 0;
    }
    while (exponent > kDenormalExponent && (significand & kHiddenBit) == 0) {
      significand <<= 1;
      exponent--;
    }
    uint64_t biased_exponent;
    if (exponent == kDenormalExponent && (significand & kHiddenBit) == 0) {
      biased_exponent = 0;
    } else {
      biased_exponent = static_cast<uint64_t>(exponent + kExponentBias);
    }
    return (significand & kSignificandMask) |
        (biased_exponent << kPhysicalSignificandSize);
  }
};

class Single {
 public:
  static const uint32_t kSignMask = 0x80000000;
  static const uint32_t kExponentMask = 0x7F800000;
  static const uint32_t kSignificandMask = 0x007FFFFF;
  static const uint32_t kHiddenBit = 0x00800000;
  static const int kPhysicalSignificandSize = 23;  
  static const int kSignificandSize = 24;

  Single() : d32_(0) {}
  explicit Single(float f) : d32_(float_to_uint32(f)) {}
  explicit Single(uint32_t d32) : d32_(d32) {}

  
  
  DiyFp AsDiyFp() const {
    ASSERT(Sign() > 0);
    ASSERT(!IsSpecial());
    return DiyFp(Significand(), Exponent());
  }

  
  uint32_t AsUint32() const {
    return d32_;
  }

  int Exponent() const {
    if (IsDenormal()) return kDenormalExponent;

    uint32_t d32 = AsUint32();
    int biased_e =
        static_cast<int>((d32 & kExponentMask) >> kPhysicalSignificandSize);
    return biased_e - kExponentBias;
  }

  uint32_t Significand() const {
    uint32_t d32 = AsUint32();
    uint32_t significand = d32 & kSignificandMask;
    if (!IsDenormal()) {
      return significand + kHiddenBit;
    } else {
      return significand;
    }
  }

  
  bool IsDenormal() const {
    uint32_t d32 = AsUint32();
    return (d32 & kExponentMask) == 0;
  }

  
  
  bool IsSpecial() const {
    uint32_t d32 = AsUint32();
    return (d32 & kExponentMask) == kExponentMask;
  }

  bool IsNan() const {
    uint32_t d32 = AsUint32();
    return ((d32 & kExponentMask) == kExponentMask) &&
        ((d32 & kSignificandMask) != 0);
  }

  bool IsInfinite() const {
    uint32_t d32 = AsUint32();
    return ((d32 & kExponentMask) == kExponentMask) &&
        ((d32 & kSignificandMask) == 0);
  }

  int Sign() const {
    uint32_t d32 = AsUint32();
    return (d32 & kSignMask) == 0? 1: -1;
  }

  
  
  
  
  void NormalizedBoundaries(DiyFp* out_m_minus, DiyFp* out_m_plus) const {
    ASSERT(value() > 0.0);
    DiyFp v = this->AsDiyFp();
    DiyFp m_plus = DiyFp::Normalize(DiyFp((v.f() << 1) + 1, v.e() - 1));
    DiyFp m_minus;
    if (LowerBoundaryIsCloser()) {
      m_minus = DiyFp((v.f() << 2) - 1, v.e() - 2);
    } else {
      m_minus = DiyFp((v.f() << 1) - 1, v.e() - 1);
    }
    m_minus.set_f(m_minus.f() << (m_minus.e() - m_plus.e()));
    m_minus.set_e(m_plus.e());
    *out_m_plus = m_plus;
    *out_m_minus = m_minus;
  }

  
  
  DiyFp UpperBoundary() const {
    ASSERT(Sign() > 0);
    return DiyFp(Significand() * 2 + 1, Exponent() - 1);
  }

  bool LowerBoundaryIsCloser() const {
    
    
    
    
    
    
    
    
    bool physical_significand_is_zero = ((AsUint32() & kSignificandMask) == 0);
    return physical_significand_is_zero && (Exponent() != kDenormalExponent);
  }

  float value() const { return uint32_to_float(d32_); }

  static float Infinity() {
    return Single(kInfinity).value();
  }

  static float NaN() {
    return Single(kNaN).value();
  }

 private:
  static const int kExponentBias = 0x7F + kPhysicalSignificandSize;
  static const int kDenormalExponent = -kExponentBias + 1;
  static const int kMaxExponent = 0xFF - kExponentBias;
  static const uint32_t kInfinity = 0x7F800000;
  static const uint32_t kNaN = 0x7FC00000;

  const uint32_t d32_;
};

}  

#endif  

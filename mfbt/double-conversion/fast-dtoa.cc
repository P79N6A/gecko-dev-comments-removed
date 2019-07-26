


























#include "fast-dtoa.h"

#include "cached-powers.h"
#include "diy-fp.h"
#include "ieee.h"

namespace double_conversion {







static const int kMinimalTargetExponent = -60;
static const int kMaximalTargetExponent = -32;

















static bool RoundWeed(Vector<char> buffer,
                      int length,
                      uint64_t distance_too_high_w,
                      uint64_t unsafe_interval,
                      uint64_t rest,
                      uint64_t ten_kappa,
                      uint64_t unit) {
  uint64_t small_distance = distance_too_high_w - unit;
  uint64_t big_distance = distance_too_high_w + unit;
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ASSERT(rest <= unsafe_interval);
  while (rest < small_distance &&  
         unsafe_interval - rest >= ten_kappa &&  
         (rest + ten_kappa < small_distance ||  
          small_distance - rest >= rest + ten_kappa - small_distance)) {
    buffer[length - 1]--;
    rest += ten_kappa;
  }

  
  
  
  if (rest < big_distance &&
      unsafe_interval - rest >= ten_kappa &&
      (rest + ten_kappa < big_distance ||
       big_distance - rest > rest + ten_kappa - big_distance)) {
    return false;
  }

  
  
  
  
  
  return (2 * unit <= rest) && (rest <= unsafe_interval - 4 * unit);
}














static bool RoundWeedCounted(Vector<char> buffer,
                             int length,
                             uint64_t rest,
                             uint64_t ten_kappa,
                             uint64_t unit,
                             int* kappa) {
  ASSERT(rest < ten_kappa);
  
  
  
  
  
  
  if (unit >= ten_kappa) return false;
  
  
  
  if (ten_kappa - unit <= unit) return false;
  
  if ((ten_kappa - rest > rest) && (ten_kappa - 2 * rest >= 2 * unit)) {
    return true;
  }
  
  if ((rest > unit) && (ten_kappa - (rest - unit) <= (rest - unit))) {
    
    buffer[length - 1]++;
    for (int i = length - 1; i > 0; --i) {
      if (buffer[i] != '0' + 10) break;
      buffer[i] = '0';
      buffer[i - 1]++;
    }
    
    
    
    
    if (buffer[0] == '0' + 10) {
      buffer[0] = '1';
      (*kappa) += 1;
    }
    return true;
  }
  return false;
}












static unsigned int const kSmallPowersOfTen[] =
    {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
     1000000000};

static void BiggestPowerTen(uint32_t number,
                            int number_bits,
                            uint32_t* power,
                            int* exponent_plus_one) {
  ASSERT(number < (1u << (number_bits + 1)));
  
  int exponent_plus_one_guess = ((number_bits + 1) * 1233 >> 12);
  
  
  exponent_plus_one_guess++;
  
  
  
  
  while (number < kSmallPowersOfTen[exponent_plus_one_guess]) {
    exponent_plus_one_guess--;
  }
  *power = kSmallPowersOfTen[exponent_plus_one_guess];
  *exponent_plus_one = exponent_plus_one_guess;
}











































static bool DigitGen(DiyFp low,
                     DiyFp w,
                     DiyFp high,
                     Vector<char> buffer,
                     int* length,
                     int* kappa) {
  ASSERT(low.e() == w.e() && w.e() == high.e());
  ASSERT(low.f() + 1 <= high.f() - 1);
  ASSERT(kMinimalTargetExponent <= w.e() && w.e() <= kMaximalTargetExponent);
  
  
  
  
  
  
  
  
  
  
  
  uint64_t unit = 1;
  DiyFp too_low = DiyFp(low.f() - unit, low.e());
  DiyFp too_high = DiyFp(high.f() + unit, high.e());
  
  
  DiyFp unsafe_interval = DiyFp::Minus(too_high, too_low);
  
  
  
  
  
  
  
  DiyFp one = DiyFp(static_cast<uint64_t>(1) << -w.e(), w.e());
  
  uint32_t integrals = static_cast<uint32_t>(too_high.f() >> -one.e());
  
  uint64_t fractionals = too_high.f() & (one.f() - 1);
  uint32_t divisor;
  int divisor_exponent_plus_one;
  BiggestPowerTen(integrals, DiyFp::kSignificandSize - (-one.e()),
                  &divisor, &divisor_exponent_plus_one);
  *kappa = divisor_exponent_plus_one;
  *length = 0;
  
  
  
  
  while (*kappa > 0) {
    int digit = integrals / divisor;
    buffer[*length] = '0' + digit;
    (*length)++;
    integrals %= divisor;
    (*kappa)--;
    
    
    uint64_t rest =
        (static_cast<uint64_t>(integrals) << -one.e()) + fractionals;
    
    
    if (rest < unsafe_interval.f()) {
      
      
      return RoundWeed(buffer, *length, DiyFp::Minus(too_high, w).f(),
                       unsafe_interval.f(), rest,
                       static_cast<uint64_t>(divisor) << -one.e(), unit);
    }
    divisor /= 10;
  }

  
  
  
  
  
  
  ASSERT(one.e() >= -60);
  ASSERT(fractionals < one.f());
  ASSERT(UINT64_2PART_C(0xFFFFFFFF, FFFFFFFF) / 10 >= one.f());
  while (true) {
    fractionals *= 10;
    unit *= 10;
    unsafe_interval.set_f(unsafe_interval.f() * 10);
    
    int digit = static_cast<int>(fractionals >> -one.e());
    buffer[*length] = '0' + digit;
    (*length)++;
    fractionals &= one.f() - 1;  
    (*kappa)--;
    if (fractionals < unsafe_interval.f()) {
      return RoundWeed(buffer, *length, DiyFp::Minus(too_high, w).f() * unit,
                       unsafe_interval.f(), fractionals, one.f(), unit);
    }
  }
}































static bool DigitGenCounted(DiyFp w,
                            int requested_digits,
                            Vector<char> buffer,
                            int* length,
                            int* kappa) {
  ASSERT(kMinimalTargetExponent <= w.e() && w.e() <= kMaximalTargetExponent);
  ASSERT(kMinimalTargetExponent >= -60);
  ASSERT(kMaximalTargetExponent <= -32);
  
  
  uint64_t w_error = 1;
  
  
  
  
  DiyFp one = DiyFp(static_cast<uint64_t>(1) << -w.e(), w.e());
  
  uint32_t integrals = static_cast<uint32_t>(w.f() >> -one.e());
  
  uint64_t fractionals = w.f() & (one.f() - 1);
  uint32_t divisor;
  int divisor_exponent_plus_one;
  BiggestPowerTen(integrals, DiyFp::kSignificandSize - (-one.e()),
                  &divisor, &divisor_exponent_plus_one);
  *kappa = divisor_exponent_plus_one;
  *length = 0;

  
  
  
  
  while (*kappa > 0) {
    int digit = integrals / divisor;
    buffer[*length] = '0' + digit;
    (*length)++;
    requested_digits--;
    integrals %= divisor;
    (*kappa)--;
    
    
    if (requested_digits == 0) break;
    divisor /= 10;
  }

  if (requested_digits == 0) {
    uint64_t rest =
        (static_cast<uint64_t>(integrals) << -one.e()) + fractionals;
    return RoundWeedCounted(buffer, *length, rest,
                            static_cast<uint64_t>(divisor) << -one.e(), w_error,
                            kappa);
  }

  
  
  
  
  
  
  ASSERT(one.e() >= -60);
  ASSERT(fractionals < one.f());
  ASSERT(UINT64_2PART_C(0xFFFFFFFF, FFFFFFFF) / 10 >= one.f());
  while (requested_digits > 0 && fractionals > w_error) {
    fractionals *= 10;
    w_error *= 10;
    
    int digit = static_cast<int>(fractionals >> -one.e());
    buffer[*length] = '0' + digit;
    (*length)++;
    requested_digits--;
    fractionals &= one.f() - 1;  
    (*kappa)--;
  }
  if (requested_digits != 0) return false;
  return RoundWeedCounted(buffer, *length, fractionals, one.f(), w_error,
                          kappa);
}













static bool Grisu3(double v,
                   FastDtoaMode mode,
                   Vector<char> buffer,
                   int* length,
                   int* decimal_exponent) {
  DiyFp w = Double(v).AsNormalizedDiyFp();
  
  
  
  
  DiyFp boundary_minus, boundary_plus;
  if (mode == FAST_DTOA_SHORTEST) {
    Double(v).NormalizedBoundaries(&boundary_minus, &boundary_plus);
  } else {
    ASSERT(mode == FAST_DTOA_SHORTEST_SINGLE);
    float single_v = static_cast<float>(v);
    Single(single_v).NormalizedBoundaries(&boundary_minus, &boundary_plus);
  }
  ASSERT(boundary_plus.e() == w.e());
  DiyFp ten_mk;  
  int mk;        
  int ten_mk_minimal_binary_exponent =
     kMinimalTargetExponent - (w.e() + DiyFp::kSignificandSize);
  int ten_mk_maximal_binary_exponent =
     kMaximalTargetExponent - (w.e() + DiyFp::kSignificandSize);
  PowersOfTenCache::GetCachedPowerForBinaryExponentRange(
      ten_mk_minimal_binary_exponent,
      ten_mk_maximal_binary_exponent,
      &ten_mk, &mk);
  ASSERT((kMinimalTargetExponent <= w.e() + ten_mk.e() +
          DiyFp::kSignificandSize) &&
         (kMaximalTargetExponent >= w.e() + ten_mk.e() +
          DiyFp::kSignificandSize));
  
  

  
  
  
  
  
  
  DiyFp scaled_w = DiyFp::Times(w, ten_mk);
  ASSERT(scaled_w.e() ==
         boundary_plus.e() + ten_mk.e() + DiyFp::kSignificandSize);
  
  
  
  
  
  DiyFp scaled_boundary_minus = DiyFp::Times(boundary_minus, ten_mk);
  DiyFp scaled_boundary_plus  = DiyFp::Times(boundary_plus,  ten_mk);

  
  
  
  
  
  
  int kappa;
  bool result = DigitGen(scaled_boundary_minus, scaled_w, scaled_boundary_plus,
                         buffer, length, &kappa);
  *decimal_exponent = -mk + kappa;
  return result;
}







static bool Grisu3Counted(double v,
                          int requested_digits,
                          Vector<char> buffer,
                          int* length,
                          int* decimal_exponent) {
  DiyFp w = Double(v).AsNormalizedDiyFp();
  DiyFp ten_mk;  
  int mk;        
  int ten_mk_minimal_binary_exponent =
     kMinimalTargetExponent - (w.e() + DiyFp::kSignificandSize);
  int ten_mk_maximal_binary_exponent =
     kMaximalTargetExponent - (w.e() + DiyFp::kSignificandSize);
  PowersOfTenCache::GetCachedPowerForBinaryExponentRange(
      ten_mk_minimal_binary_exponent,
      ten_mk_maximal_binary_exponent,
      &ten_mk, &mk);
  ASSERT((kMinimalTargetExponent <= w.e() + ten_mk.e() +
          DiyFp::kSignificandSize) &&
         (kMaximalTargetExponent >= w.e() + ten_mk.e() +
          DiyFp::kSignificandSize));
  
  

  
  
  
  
  
  
  DiyFp scaled_w = DiyFp::Times(w, ten_mk);

  
  
  
  
  
  int kappa;
  bool result = DigitGenCounted(scaled_w, requested_digits,
                                buffer, length, &kappa);
  *decimal_exponent = -mk + kappa;
  return result;
}


bool FastDtoa(double v,
              FastDtoaMode mode,
              int requested_digits,
              Vector<char> buffer,
              int* length,
              int* decimal_point) {
  ASSERT(v > 0);
  ASSERT(!Double(v).IsSpecial());

  bool result = false;
  int decimal_exponent = 0;
  switch (mode) {
    case FAST_DTOA_SHORTEST:
    case FAST_DTOA_SHORTEST_SINGLE:
      result = Grisu3(v, mode, buffer, length, &decimal_exponent);
      break;
    case FAST_DTOA_PRECISION:
      result = Grisu3Counted(v, requested_digits,
                             buffer, length, &decimal_exponent);
      break;
    default:
      UNREACHABLE();
  }
  if (result) {
    *decimal_point = *length + decimal_exponent;
    buffer[*length] = '\0';
  }
  return result;
}

}  

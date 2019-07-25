


























#include "v8.h"

#include "fast-dtoa.h"

#include "cached-powers.h"
#include "diy-fp.h"
#include "double.h"

namespace v8 {
namespace internal {







static const int minimal_target_exponent = -60;
static const int maximal_target_exponent = -32;

















bool RoundWeed(Vector<char> buffer,
               int length,
               uint64_t distance_too_high_w,
               uint64_t unsafe_interval,
               uint64_t rest,
               uint64_t ten_kappa,
               uint64_t unit) {
  uint64_t small_distance = distance_too_high_w - unit;
  uint64_t big_distance = distance_too_high_w + unit;
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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



static const uint32_t kTen4 = 10000;
static const uint32_t kTen5 = 100000;
static const uint32_t kTen6 = 1000000;
static const uint32_t kTen7 = 10000000;
static const uint32_t kTen8 = 100000000;
static const uint32_t kTen9 = 1000000000;






static void BiggestPowerTen(uint32_t number,
                            int number_bits,
                            uint32_t* power,
                            int* exponent) {
  switch (number_bits) {
    case 32:
    case 31:
    case 30:
      if (kTen9 <= number) {
        *power = kTen9;
        *exponent = 9;
        break;
      }  
    case 29:
    case 28:
    case 27:
      if (kTen8 <= number) {
        *power = kTen8;
        *exponent = 8;
        break;
      }  
    case 26:
    case 25:
    case 24:
      if (kTen7 <= number) {
        *power = kTen7;
        *exponent = 7;
        break;
      }  
    case 23:
    case 22:
    case 21:
    case 20:
      if (kTen6 <= number) {
        *power = kTen6;
        *exponent = 6;
        break;
      }  
    case 19:
    case 18:
    case 17:
      if (kTen5 <= number) {
        *power = kTen5;
        *exponent = 5;
        break;
      }  
    case 16:
    case 15:
    case 14:
      if (kTen4 <= number) {
        *power = kTen4;
        *exponent = 4;
        break;
      }  
    case 13:
    case 12:
    case 11:
    case 10:
      if (1000 <= number) {
        *power = 1000;
        *exponent = 3;
        break;
      }  
    case 9:
    case 8:
    case 7:
      if (100 <= number) {
        *power = 100;
        *exponent = 2;
        break;
      }  
    case 6:
    case 5:
    case 4:
      if (10 <= number) {
        *power = 10;
        *exponent = 1;
        break;
      }  
    case 3:
    case 2:
    case 1:
      if (1 <= number) {
        *power = 1;
        *exponent = 0;
        break;
      }  
    case 0:
      *power = 0;
      *exponent = -1;
      break;
    default:
      
      *power = 0;
      *exponent = 0;
      UNREACHABLE();
  }
}












































bool DigitGen(DiyFp low,
              DiyFp w,
              DiyFp high,
              Vector<char> buffer,
              int* length,
              int* kappa) {
  ASSERT(low.e() == w.e() && w.e() == high.e());
  ASSERT(low.f() + 1 <= high.f() - 1);
  ASSERT(minimal_target_exponent <= w.e() && w.e() <= maximal_target_exponent);
  
  
  
  
  
  
  
  
  
  
  
  uint64_t unit = 1;
  DiyFp too_low = DiyFp(low.f() - unit, low.e());
  DiyFp too_high = DiyFp(high.f() + unit, high.e());
  
  
  DiyFp unsafe_interval = DiyFp::Minus(too_high, too_low);
  
  
  
  
  
  
  
  DiyFp one = DiyFp(static_cast<uint64_t>(1) << -w.e(), w.e());
  
  uint32_t integrals = static_cast<uint32_t>(too_high.f() >> -one.e());
  
  uint64_t fractionals = too_high.f() & (one.f() - 1);
  uint32_t divider;
  int divider_exponent;
  BiggestPowerTen(integrals, DiyFp::kSignificandSize - (-one.e()),
                  &divider, &divider_exponent);
  *kappa = divider_exponent + 1;
  *length = 0;
  
  
  
  
  while (*kappa > 0) {
    int digit = integrals / divider;
    buffer[*length] = '0' + digit;
    (*length)++;
    integrals %= divider;
    (*kappa)--;
    
    
    uint64_t rest =
        (static_cast<uint64_t>(integrals) << -one.e()) + fractionals;
    
    
    if (rest < unsafe_interval.f()) {
      
      
      return RoundWeed(buffer, *length, DiyFp::Minus(too_high, w).f(),
                       unsafe_interval.f(), rest,
                       static_cast<uint64_t>(divider) << -one.e(), unit);
    }
    divider /= 10;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  while (true) {
    fractionals *= 5;
    unit *= 5;
    unsafe_interval.set_f(unsafe_interval.f() * 5);
    unsafe_interval.set_e(unsafe_interval.e() + 1);  
    one.set_f(one.f() >> 1);
    one.set_e(one.e() + 1);
    
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













bool grisu3(double v, Vector<char> buffer, int* length, int* decimal_exponent) {
  DiyFp w = Double(v).AsNormalizedDiyFp();
  
  
  
  
  DiyFp boundary_minus, boundary_plus;
  Double(v).NormalizedBoundaries(&boundary_minus, &boundary_plus);
  ASSERT(boundary_plus.e() == w.e());
  DiyFp ten_mk;  
  int mk;        
  GetCachedPower(w.e() + DiyFp::kSignificandSize, minimal_target_exponent,
                 maximal_target_exponent, &mk, &ten_mk);
  ASSERT(minimal_target_exponent <= w.e() + ten_mk.e() +
         DiyFp::kSignificandSize &&
         maximal_target_exponent >= w.e() + ten_mk.e() +
         DiyFp::kSignificandSize);
  
  

  
  
  
  
  
  
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


bool FastDtoa(double v,
              Vector<char> buffer,
              int* length,
              int* point) {
  ASSERT(v > 0);
  ASSERT(!Double(v).IsSpecial());

  int decimal_exponent;
  bool result = grisu3(v, buffer, length, &decimal_exponent);
  *point = *length + decimal_exponent;
  buffer[*length] = '\0';
  return result;
}

} }  

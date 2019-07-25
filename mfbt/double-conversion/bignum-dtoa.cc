


























#include <math.h>

#include "bignum-dtoa.h"

#include "bignum.h"
#include "ieee.h"

namespace double_conversion {

static int NormalizedExponent(uint64_t significand, int exponent) {
  ASSERT(significand != 0);
  while ((significand & Double::kHiddenBit) == 0) {
    significand = significand << 1;
    exponent = exponent - 1;
  }
  return exponent;
}




static int EstimatePower(int exponent);


static void InitialScaledStartValues(uint64_t significand,
                                     int exponent,
                                     bool lower_boundary_is_closer,
                                     int estimated_power,
                                     bool need_boundary_deltas,
                                     Bignum* numerator,
                                     Bignum* denominator,
                                     Bignum* delta_minus,
                                     Bignum* delta_plus);





static void FixupMultiply10(int estimated_power, bool is_even,
                            int* decimal_point,
                            Bignum* numerator, Bignum* denominator,
                            Bignum* delta_minus, Bignum* delta_plus);


static void GenerateShortestDigits(Bignum* numerator, Bignum* denominator,
                                   Bignum* delta_minus, Bignum* delta_plus,
                                   bool is_even,
                                   Vector<char> buffer, int* length);

static void BignumToFixed(int requested_digits, int* decimal_point,
                          Bignum* numerator, Bignum* denominator,
                          Vector<char>(buffer), int* length);




static void GenerateCountedDigits(int count, int* decimal_point,
                                  Bignum* numerator, Bignum* denominator,
                                  Vector<char>(buffer), int* length);


void BignumDtoa(double v, BignumDtoaMode mode, int requested_digits,
                Vector<char> buffer, int* length, int* decimal_point) {
  ASSERT(v > 0);
  ASSERT(!Double(v).IsSpecial());
  uint64_t significand;
  int exponent;
  bool lower_boundary_is_closer;
  if (mode == BIGNUM_DTOA_SHORTEST_SINGLE) {
    float f = static_cast<float>(v);
    ASSERT(f == v);
    significand = Single(f).Significand();
    exponent = Single(f).Exponent();
    lower_boundary_is_closer = Single(f).LowerBoundaryIsCloser();
  } else {
    significand = Double(v).Significand();
    exponent = Double(v).Exponent();
    lower_boundary_is_closer = Double(v).LowerBoundaryIsCloser();
  }
  bool need_boundary_deltas =
      (mode == BIGNUM_DTOA_SHORTEST || mode == BIGNUM_DTOA_SHORTEST_SINGLE);

  bool is_even = (significand & 1) == 0;
  int normalized_exponent = NormalizedExponent(significand, exponent);
  
  int estimated_power = EstimatePower(normalized_exponent);

  
  
  
  
  if (mode == BIGNUM_DTOA_FIXED && -estimated_power - 1 > requested_digits) {
    buffer[0] = '\0';
    *length = 0;
    
    
    
    *decimal_point = -requested_digits;
    return;
  }

  Bignum numerator;
  Bignum denominator;
  Bignum delta_minus;
  Bignum delta_plus;
  
  
  
  
  ASSERT(Bignum::kMaxSignificantBits >= 324*4);
  InitialScaledStartValues(significand, exponent, lower_boundary_is_closer,
                           estimated_power, need_boundary_deltas,
                           &numerator, &denominator,
                           &delta_minus, &delta_plus);
  
  FixupMultiply10(estimated_power, is_even, decimal_point,
                  &numerator, &denominator,
                  &delta_minus, &delta_plus);
  
  
  switch (mode) {
    case BIGNUM_DTOA_SHORTEST:
    case BIGNUM_DTOA_SHORTEST_SINGLE:
      GenerateShortestDigits(&numerator, &denominator,
                             &delta_minus, &delta_plus,
                             is_even, buffer, length);
      break;
    case BIGNUM_DTOA_FIXED:
      BignumToFixed(requested_digits, decimal_point,
                    &numerator, &denominator,
                    buffer, length);
      break;
    case BIGNUM_DTOA_PRECISION:
      GenerateCountedDigits(requested_digits, decimal_point,
                            &numerator, &denominator,
                            buffer, length);
      break;
    default:
      UNREACHABLE();
  }
  buffer[*length] = '\0';
}















static void GenerateShortestDigits(Bignum* numerator, Bignum* denominator,
                                   Bignum* delta_minus, Bignum* delta_plus,
                                   bool is_even,
                                   Vector<char> buffer, int* length) {
  
  
  if (Bignum::Equal(*delta_minus, *delta_plus)) {
    delta_plus = delta_minus;
  }
  *length = 0;
  while (true) {
    uint16_t digit;
    digit = numerator->DivideModuloIntBignum(*denominator);
    ASSERT(digit <= 9);  
    
    
    buffer[(*length)++] = digit + '0';

    
    
    
    
    
    bool in_delta_room_minus;
    bool in_delta_room_plus;
    if (is_even) {
      in_delta_room_minus = Bignum::LessEqual(*numerator, *delta_minus);
    } else {
      in_delta_room_minus = Bignum::Less(*numerator, *delta_minus);
    }
    if (is_even) {
      in_delta_room_plus =
          Bignum::PlusCompare(*numerator, *delta_plus, *denominator) >= 0;
    } else {
      in_delta_room_plus =
          Bignum::PlusCompare(*numerator, *delta_plus, *denominator) > 0;
    }
    if (!in_delta_room_minus && !in_delta_room_plus) {
      
      numerator->Times10();
      delta_minus->Times10();
      
      
      
      if (delta_minus != delta_plus) {
        delta_plus->Times10();
      }
    } else if (in_delta_room_minus && in_delta_room_plus) {
      
      
      int compare = Bignum::PlusCompare(*numerator, *numerator, *denominator);
      if (compare < 0) {
        
      } else if (compare > 0) {
        
        
        
        
        
        ASSERT(buffer[(*length) - 1] != '9');
        buffer[(*length) - 1]++;
      } else {
        
        
        
        

        if ((buffer[(*length) - 1] - '0') % 2 == 0) {
          
        } else {
          ASSERT(buffer[(*length) - 1] != '9');
          buffer[(*length) - 1]++;
        }
      }
      return;
    } else if (in_delta_room_minus) {
      
      return;
    } else {  
      
      
      
      
      
      ASSERT(buffer[(*length) -1] != '9');
      buffer[(*length) - 1]++;
      return;
    }
  }
}








static void GenerateCountedDigits(int count, int* decimal_point,
                                  Bignum* numerator, Bignum* denominator,
                                  Vector<char>(buffer), int* length) {
  ASSERT(count >= 0);
  for (int i = 0; i < count - 1; ++i) {
    uint16_t digit;
    digit = numerator->DivideModuloIntBignum(*denominator);
    ASSERT(digit <= 9);  
    
    
    buffer[i] = digit + '0';
    
    numerator->Times10();
  }
  
  uint16_t digit;
  digit = numerator->DivideModuloIntBignum(*denominator);
  if (Bignum::PlusCompare(*numerator, *numerator, *denominator) >= 0) {
    digit++;
  }
  buffer[count - 1] = digit + '0';
  
  
  for (int i = count - 1; i > 0; --i) {
    if (buffer[i] != '0' + 10) break;
    buffer[i] = '0';
    buffer[i - 1]++;
  }
  if (buffer[0] == '0' + 10) {
    
    buffer[0] = '1';
    (*decimal_point)++;
  }
  *length = count;
}







static void BignumToFixed(int requested_digits, int* decimal_point,
                          Bignum* numerator, Bignum* denominator,
                          Vector<char>(buffer), int* length) {
  
  
  
  if (-(*decimal_point) > requested_digits) {
    
    
    
    
    
    *decimal_point = -requested_digits;
    *length = 0;
    return;
  } else if (-(*decimal_point) == requested_digits) {
    
    
    ASSERT(*decimal_point == -requested_digits);
    
    
    denominator->Times10();
    if (Bignum::PlusCompare(*numerator, *numerator, *denominator) >= 0) {
      
      
      buffer[0] = '1';
      *length = 1;
      (*decimal_point)++;
    } else {
      
      *length = 0;
    }
    return;
  } else {
    
    
    int needed_digits = (*decimal_point) + requested_digits;
    GenerateCountedDigits(needed_digits, decimal_point,
                          numerator, denominator,
                          buffer, length);
  }
}

















static int EstimatePower(int exponent) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  const double k1Log10 = 0.30102999566398114;  

  
  const int kSignificandSize = Double::kSignificandSize;
  double estimate = ceil((exponent + kSignificandSize - 1) * k1Log10 - 1e-10);
  return static_cast<int>(estimate);
}



static void InitialScaledStartValuesPositiveExponent(
    uint64_t significand, int exponent,
    int estimated_power, bool need_boundary_deltas,
    Bignum* numerator, Bignum* denominator,
    Bignum* delta_minus, Bignum* delta_plus) {
  
  ASSERT(estimated_power >= 0);
  
  

  
  numerator->AssignUInt64(significand);
  numerator->ShiftLeft(exponent);
  
  denominator->AssignPowerUInt16(10, estimated_power);

  if (need_boundary_deltas) {
    
    
    denominator->ShiftLeft(1);
    numerator->ShiftLeft(1);
    
    
    delta_plus->AssignUInt16(1);
    delta_plus->ShiftLeft(exponent);
    
    delta_minus->AssignUInt16(1);
    delta_minus->ShiftLeft(exponent);
  }
}



static void InitialScaledStartValuesNegativeExponentPositivePower(
    uint64_t significand, int exponent,
    int estimated_power, bool need_boundary_deltas,
    Bignum* numerator, Bignum* denominator,
    Bignum* delta_minus, Bignum* delta_plus) {
  
  
  

  
  
  
  numerator->AssignUInt64(significand);
  
  denominator->AssignPowerUInt16(10, estimated_power);
  denominator->ShiftLeft(-exponent);

  if (need_boundary_deltas) {
    
    
    denominator->ShiftLeft(1);
    numerator->ShiftLeft(1);
    
    
    
    
    delta_plus->AssignUInt16(1);
    
    delta_minus->AssignUInt16(1);
  }
}



static void InitialScaledStartValuesNegativeExponentNegativePower(
    uint64_t significand, int exponent,
    int estimated_power, bool need_boundary_deltas,
    Bignum* numerator, Bignum* denominator,
    Bignum* delta_minus, Bignum* delta_plus) {
  
  

  
  Bignum* power_ten = numerator;
  power_ten->AssignPowerUInt16(10, -estimated_power);

  if (need_boundary_deltas) {
    
    
    
    delta_plus->AssignBignum(*power_ten);
    delta_minus->AssignBignum(*power_ten);
  }

  
  
  
  
  
  ASSERT(numerator == power_ten);
  numerator->MultiplyByUInt64(significand);

  
  denominator->AssignUInt16(1);
  denominator->ShiftLeft(-exponent);

  if (need_boundary_deltas) {
    
    
    numerator->ShiftLeft(1);
    denominator->ShiftLeft(1);
    
    
    
    
    
  }
}








































static void InitialScaledStartValues(uint64_t significand,
                                     int exponent,
                                     bool lower_boundary_is_closer,
                                     int estimated_power,
                                     bool need_boundary_deltas,
                                     Bignum* numerator,
                                     Bignum* denominator,
                                     Bignum* delta_minus,
                                     Bignum* delta_plus) {
  if (exponent >= 0) {
    InitialScaledStartValuesPositiveExponent(
        significand, exponent, estimated_power, need_boundary_deltas,
        numerator, denominator, delta_minus, delta_plus);
  } else if (estimated_power >= 0) {
    InitialScaledStartValuesNegativeExponentPositivePower(
        significand, exponent, estimated_power, need_boundary_deltas,
        numerator, denominator, delta_minus, delta_plus);
  } else {
    InitialScaledStartValuesNegativeExponentNegativePower(
        significand, exponent, estimated_power, need_boundary_deltas,
        numerator, denominator, delta_minus, delta_plus);
  }

  if (need_boundary_deltas && lower_boundary_is_closer) {
    
    
    denominator->ShiftLeft(1);  
    numerator->ShiftLeft(1);    
    delta_plus->ShiftLeft(1);   
  }
}













static void FixupMultiply10(int estimated_power, bool is_even,
                            int* decimal_point,
                            Bignum* numerator, Bignum* denominator,
                            Bignum* delta_minus, Bignum* delta_plus) {
  bool in_range;
  if (is_even) {
    
    
    in_range = Bignum::PlusCompare(*numerator, *delta_plus, *denominator) >= 0;
  } else {
    in_range = Bignum::PlusCompare(*numerator, *delta_plus, *denominator) > 0;
  }
  if (in_range) {
    
    
    *decimal_point = estimated_power + 1;
  } else {
    *decimal_point = estimated_power;
    numerator->Times10();
    if (Bignum::Equal(*delta_minus, *delta_plus)) {
      delta_minus->Times10();
      delta_plus->AssignBignum(*delta_minus);
    } else {
      delta_minus->Times10();
      delta_plus->Times10();
    }
  }
}

}  

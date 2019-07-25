


























#include <math.h>

#include "fixed-dtoa.h"
#include "ieee.h"

namespace double_conversion {



class UInt128 {
 public:
  UInt128() : high_bits_(0), low_bits_(0) { }
  UInt128(uint64_t high, uint64_t low) : high_bits_(high), low_bits_(low) { }

  void Multiply(uint32_t multiplicand) {
    uint64_t accumulator;

    accumulator = (low_bits_ & kMask32) * multiplicand;
    uint32_t part = static_cast<uint32_t>(accumulator & kMask32);
    accumulator >>= 32;
    accumulator = accumulator + (low_bits_ >> 32) * multiplicand;
    low_bits_ = (accumulator << 32) + part;
    accumulator >>= 32;
    accumulator = accumulator + (high_bits_ & kMask32) * multiplicand;
    part = static_cast<uint32_t>(accumulator & kMask32);
    accumulator >>= 32;
    accumulator = accumulator + (high_bits_ >> 32) * multiplicand;
    high_bits_ = (accumulator << 32) + part;
    ASSERT((accumulator >> 32) == 0);
  }

  void Shift(int shift_amount) {
    ASSERT(-64 <= shift_amount && shift_amount <= 64);
    if (shift_amount == 0) {
      return;
    } else if (shift_amount == -64) {
      high_bits_ = low_bits_;
      low_bits_ = 0;
    } else if (shift_amount == 64) {
      low_bits_ = high_bits_;
      high_bits_ = 0;
    } else if (shift_amount <= 0) {
      high_bits_ <<= -shift_amount;
      high_bits_ += low_bits_ >> (64 + shift_amount);
      low_bits_ <<= -shift_amount;
    } else {
      low_bits_ >>= shift_amount;
      low_bits_ += high_bits_ << (64 - shift_amount);
      high_bits_ >>= shift_amount;
    }
  }

  
  
  int DivModPowerOf2(int power) {
    if (power >= 64) {
      int result = static_cast<int>(high_bits_ >> (power - 64));
      high_bits_ -= static_cast<uint64_t>(result) << (power - 64);
      return result;
    } else {
      uint64_t part_low = low_bits_ >> power;
      uint64_t part_high = high_bits_ << (64 - power);
      int result = static_cast<int>(part_low + part_high);
      high_bits_ = 0;
      low_bits_ -= part_low << power;
      return result;
    }
  }

  bool IsZero() const {
    return high_bits_ == 0 && low_bits_ == 0;
  }

  int BitAt(int position) {
    if (position >= 64) {
      return static_cast<int>(high_bits_ >> (position - 64)) & 1;
    } else {
      return static_cast<int>(low_bits_ >> position) & 1;
    }
  }

 private:
  static const uint64_t kMask32 = 0xFFFFFFFF;
  
  uint64_t high_bits_;
  uint64_t low_bits_;
};


static const int kDoubleSignificandSize = 53;  


static void FillDigits32FixedLength(uint32_t number, int requested_length,
                                    Vector<char> buffer, int* length) {
  for (int i = requested_length - 1; i >= 0; --i) {
    buffer[(*length) + i] = '0' + number % 10;
    number /= 10;
  }
  *length += requested_length;
}


static void FillDigits32(uint32_t number, Vector<char> buffer, int* length) {
  int number_length = 0;
  
  while (number != 0) {
    int digit = number % 10;
    number /= 10;
    buffer[(*length) + number_length] = '0' + digit;
    number_length++;
  }
  
  int i = *length;
  int j = *length + number_length - 1;
  while (i < j) {
    char tmp = buffer[i];
    buffer[i] = buffer[j];
    buffer[j] = tmp;
    i++;
    j--;
  }
  *length += number_length;
}


static void FillDigits64FixedLength(uint64_t number, int requested_length,
                                    Vector<char> buffer, int* length) {
  const uint32_t kTen7 = 10000000;
  
  uint32_t part2 = static_cast<uint32_t>(number % kTen7);
  number /= kTen7;
  uint32_t part1 = static_cast<uint32_t>(number % kTen7);
  uint32_t part0 = static_cast<uint32_t>(number / kTen7);

  FillDigits32FixedLength(part0, 3, buffer, length);
  FillDigits32FixedLength(part1, 7, buffer, length);
  FillDigits32FixedLength(part2, 7, buffer, length);
}


static void FillDigits64(uint64_t number, Vector<char> buffer, int* length) {
  const uint32_t kTen7 = 10000000;
  
  uint32_t part2 = static_cast<uint32_t>(number % kTen7);
  number /= kTen7;
  uint32_t part1 = static_cast<uint32_t>(number % kTen7);
  uint32_t part0 = static_cast<uint32_t>(number / kTen7);

  if (part0 != 0) {
    FillDigits32(part0, buffer, length);
    FillDigits32FixedLength(part1, 7, buffer, length);
    FillDigits32FixedLength(part2, 7, buffer, length);
  } else if (part1 != 0) {
    FillDigits32(part1, buffer, length);
    FillDigits32FixedLength(part2, 7, buffer, length);
  } else {
    FillDigits32(part2, buffer, length);
  }
}


static void RoundUp(Vector<char> buffer, int* length, int* decimal_point) {
  
  if (*length == 0) {
    buffer[0] = '1';
    *decimal_point = 1;
    *length = 1;
    return;
  }
  
  
  buffer[(*length) - 1]++;
  for (int i = (*length) - 1; i > 0; --i) {
    if (buffer[i] != '0' + 10) {
      return;
    }
    buffer[i] = '0';
    buffer[i - 1]++;
  }
  
  
  
  
  
  if (buffer[0] == '0' + 10) {
    buffer[0] = '1';
    (*decimal_point)++;
  }
}













static void FillFractionals(uint64_t fractionals, int exponent,
                            int fractional_count, Vector<char> buffer,
                            int* length, int* decimal_point) {
  ASSERT(-128 <= exponent && exponent <= 0);
  
  
  
  if (-exponent <= 64) {
    
    ASSERT(fractionals >> 56 == 0);
    int point = -exponent;
    for (int i = 0; i < fractional_count; ++i) {
      if (fractionals == 0) break;
      
      
      
      
      
      
      
      
      
      
      fractionals *= 5;
      point--;
      int digit = static_cast<int>(fractionals >> point);
      buffer[*length] = '0' + digit;
      (*length)++;
      fractionals -= static_cast<uint64_t>(digit) << point;
    }
    
    if (((fractionals >> (point - 1)) & 1) == 1) {
      RoundUp(buffer, length, decimal_point);
    }
  } else {  
    ASSERT(64 < -exponent && -exponent <= 128);
    UInt128 fractionals128 = UInt128(fractionals, 0);
    fractionals128.Shift(-exponent - 64);
    int point = 128;
    for (int i = 0; i < fractional_count; ++i) {
      if (fractionals128.IsZero()) break;
      
      
      
      fractionals128.Multiply(5);
      point--;
      int digit = fractionals128.DivModPowerOf2(point);
      buffer[*length] = '0' + digit;
      (*length)++;
    }
    if (fractionals128.BitAt(point - 1) == 1) {
      RoundUp(buffer, length, decimal_point);
    }
  }
}




static void TrimZeros(Vector<char> buffer, int* length, int* decimal_point) {
  while (*length > 0 && buffer[(*length) - 1] == '0') {
    (*length)--;
  }
  int first_non_zero = 0;
  while (first_non_zero < *length && buffer[first_non_zero] == '0') {
    first_non_zero++;
  }
  if (first_non_zero != 0) {
    for (int i = first_non_zero; i < *length; ++i) {
      buffer[i - first_non_zero] = buffer[i];
    }
    *length -= first_non_zero;
    *decimal_point -= first_non_zero;
  }
}


bool FastFixedDtoa(double v,
                   int fractional_count,
                   Vector<char> buffer,
                   int* length,
                   int* decimal_point) {
  const uint32_t kMaxUInt32 = 0xFFFFFFFF;
  uint64_t significand = Double(v).Significand();
  int exponent = Double(v).Exponent();
  
  
  
  
  
  if (exponent > 20) return false;
  if (fractional_count > 20) return false;
  *length = 0;
  
  
  
  if (exponent + kDoubleSignificandSize > 64) {
    
    
    
    
    
    
    
    
    const uint64_t kFive17 = UINT64_2PART_C(0xB1, A2BC2EC5);  
    uint64_t divisor = kFive17;
    int divisor_power = 17;
    uint64_t dividend = significand;
    uint32_t quotient;
    uint64_t remainder;
    
    
    
    
    
    
    
    
    
    if (exponent > divisor_power) {
      
      dividend <<= exponent - divisor_power;
      quotient = static_cast<uint32_t>(dividend / divisor);
      remainder = (dividend % divisor) << divisor_power;
    } else {
      divisor <<= divisor_power - exponent;
      quotient = static_cast<uint32_t>(dividend / divisor);
      remainder = (dividend % divisor) << exponent;
    }
    FillDigits32(quotient, buffer, length);
    FillDigits64FixedLength(remainder, divisor_power, buffer, length);
    *decimal_point = *length;
  } else if (exponent >= 0) {
    
    significand <<= exponent;
    FillDigits64(significand, buffer, length);
    *decimal_point = *length;
  } else if (exponent > -kDoubleSignificandSize) {
    
    uint64_t integrals = significand >> -exponent;
    uint64_t fractionals = significand - (integrals << -exponent);
    if (integrals > kMaxUInt32) {
      FillDigits64(integrals, buffer, length);
    } else {
      FillDigits32(static_cast<uint32_t>(integrals), buffer, length);
    }
    *decimal_point = *length;
    FillFractionals(fractionals, exponent, fractional_count,
                    buffer, length, decimal_point);
  } else if (exponent < -128) {
    
    
    ASSERT(fractional_count <= 20);
    buffer[0] = '\0';
    *length = 0;
    *decimal_point = -fractional_count;
  } else {
    *decimal_point = 0;
    FillFractionals(significand, exponent, fractional_count,
                    buffer, length, decimal_point);
  }
  TrimZeros(buffer, length, decimal_point);
  buffer[*length] = '\0';
  if ((*length) == 0) {
    
    
    *decimal_point = -fractional_count;
  }
  return true;
}

}  

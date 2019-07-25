


























#include <limits.h>
#include <math.h>

#include "double-conversion.h"

#include "bignum-dtoa.h"
#include "fast-dtoa.h"
#include "fixed-dtoa.h"
#include "ieee.h"
#include "strtod.h"
#include "utils.h"

namespace double_conversion {

const DoubleToStringConverter& DoubleToStringConverter::EcmaScriptConverter() {
  int flags = UNIQUE_ZERO | EMIT_POSITIVE_EXPONENT_SIGN;
  static DoubleToStringConverter converter(flags,
                                           "Infinity",
                                           "NaN",
                                           'e',
                                           -6, 21,
                                           6, 0);
  return converter;
}


bool DoubleToStringConverter::HandleSpecialValues(
    double value,
    StringBuilder* result_builder) const {
  Double double_inspect(value);
  if (double_inspect.IsInfinite()) {
    if (infinity_symbol_ == NULL) return false;
    if (value < 0) {
      result_builder->AddCharacter('-');
    }
    result_builder->AddString(infinity_symbol_);
    return true;
  }
  if (double_inspect.IsNan()) {
    if (nan_symbol_ == NULL) return false;
    result_builder->AddString(nan_symbol_);
    return true;
  }
  return false;
}


void DoubleToStringConverter::CreateExponentialRepresentation(
    const char* decimal_digits,
    int length,
    int exponent,
    StringBuilder* result_builder) const {
  ASSERT(length != 0);
  result_builder->AddCharacter(decimal_digits[0]);
  if (length != 1) {
    result_builder->AddCharacter('.');
    result_builder->AddSubstring(&decimal_digits[1], length-1);
  }
  result_builder->AddCharacter(exponent_character_);
  if (exponent < 0) {
    result_builder->AddCharacter('-');
    exponent = -exponent;
  } else {
    if ((flags_ & EMIT_POSITIVE_EXPONENT_SIGN) != 0) {
      result_builder->AddCharacter('+');
    }
  }
  if (exponent == 0) {
    result_builder->AddCharacter('0');
    return;
  }
  ASSERT(exponent < 1e4);
  const int kMaxExponentLength = 5;
  char buffer[kMaxExponentLength + 1];
  buffer[kMaxExponentLength] = '\0';
  int first_char_pos = kMaxExponentLength;
  while (exponent > 0) {
    buffer[--first_char_pos] = '0' + (exponent % 10);
    exponent /= 10;
  }
  result_builder->AddSubstring(&buffer[first_char_pos],
                               kMaxExponentLength - first_char_pos);
}


void DoubleToStringConverter::CreateDecimalRepresentation(
    const char* decimal_digits,
    int length,
    int decimal_point,
    int digits_after_point,
    StringBuilder* result_builder) const {
  
  if (decimal_point <= 0) {
      
    result_builder->AddCharacter('0');
    if (digits_after_point > 0) {
      result_builder->AddCharacter('.');
      result_builder->AddPadding('0', -decimal_point);
      ASSERT(length <= digits_after_point - (-decimal_point));
      result_builder->AddSubstring(decimal_digits, length);
      int remaining_digits = digits_after_point - (-decimal_point) - length;
      result_builder->AddPadding('0', remaining_digits);
    }
  } else if (decimal_point >= length) {
    
    result_builder->AddSubstring(decimal_digits, length);
    result_builder->AddPadding('0', decimal_point - length);
    if (digits_after_point > 0) {
      result_builder->AddCharacter('.');
      result_builder->AddPadding('0', digits_after_point);
    }
  } else {
    
    ASSERT(digits_after_point > 0);
    result_builder->AddSubstring(decimal_digits, decimal_point);
    result_builder->AddCharacter('.');
    ASSERT(length - decimal_point <= digits_after_point);
    result_builder->AddSubstring(&decimal_digits[decimal_point],
                                 length - decimal_point);
    int remaining_digits = digits_after_point - (length - decimal_point);
    result_builder->AddPadding('0', remaining_digits);
  }
  if (digits_after_point == 0) {
    if ((flags_ & EMIT_TRAILING_DECIMAL_POINT) != 0) {
      result_builder->AddCharacter('.');
    }
    if ((flags_ & EMIT_TRAILING_ZERO_AFTER_POINT) != 0) {
      result_builder->AddCharacter('0');
    }
  }
}


bool DoubleToStringConverter::ToShortestIeeeNumber(
    double value,
    StringBuilder* result_builder,
    DoubleToStringConverter::DtoaMode mode) const {
  assert(mode == SHORTEST || mode == SHORTEST_SINGLE);
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  int decimal_point;
  bool sign;
  const int kDecimalRepCapacity = kBase10MaximalLength + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  DoubleToAscii(value, mode, 0, decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);

  bool unique_zero = (flags_ & UNIQUE_ZERO) != 0;
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  int exponent = decimal_point - 1;
  if ((decimal_in_shortest_low_ <= exponent) &&
      (exponent < decimal_in_shortest_high_)) {
    CreateDecimalRepresentation(decimal_rep, decimal_rep_length,
                                decimal_point,
                                Max(0, decimal_rep_length - decimal_point),
                                result_builder);
  } else {
    CreateExponentialRepresentation(decimal_rep, decimal_rep_length, exponent,
                                    result_builder);
  }
  return true;
}


bool DoubleToStringConverter::ToFixed(double value,
                                      int requested_digits,
                                      StringBuilder* result_builder) const {
  ASSERT(kMaxFixedDigitsBeforePoint == 60);
  const double kFirstNonFixed = 1e60;

  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (requested_digits > kMaxFixedDigitsAfterPoint) return false;
  if (value >= kFirstNonFixed || value <= -kFirstNonFixed) return false;

  
  int decimal_point;
  bool sign;
  
  const int kDecimalRepCapacity =
      kMaxFixedDigitsBeforePoint + kMaxFixedDigitsAfterPoint + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;
  DoubleToAscii(value, FIXED, requested_digits,
                decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  CreateDecimalRepresentation(decimal_rep, decimal_rep_length, decimal_point,
                              requested_digits, result_builder);
  return true;
}


bool DoubleToStringConverter::ToExponential(
    double value,
    int requested_digits,
    StringBuilder* result_builder) const {
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (requested_digits < -1) return false;
  if (requested_digits > kMaxExponentialDigits) return false;

  int decimal_point;
  bool sign;
  
  const int kDecimalRepCapacity = kMaxExponentialDigits + 2;
  ASSERT(kDecimalRepCapacity > kBase10MaximalLength);
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  if (requested_digits == -1) {
    DoubleToAscii(value, SHORTEST, 0,
                  decimal_rep, kDecimalRepCapacity,
                  &sign, &decimal_rep_length, &decimal_point);
  } else {
    DoubleToAscii(value, PRECISION, requested_digits + 1,
                  decimal_rep, kDecimalRepCapacity,
                  &sign, &decimal_rep_length, &decimal_point);
    ASSERT(decimal_rep_length <= requested_digits + 1);

    for (int i = decimal_rep_length; i < requested_digits + 1; ++i) {
      decimal_rep[i] = '0';
    }
    decimal_rep_length = requested_digits + 1;
  }

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  int exponent = decimal_point - 1;
  CreateExponentialRepresentation(decimal_rep,
                                  decimal_rep_length,
                                  exponent,
                                  result_builder);
  return true;
}


bool DoubleToStringConverter::ToPrecision(double value,
                                          int precision,
                                          StringBuilder* result_builder) const {
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (precision < kMinPrecisionDigits || precision > kMaxPrecisionDigits) {
    return false;
  }

  
  int decimal_point;
  bool sign;
  
  const int kDecimalRepCapacity = kMaxPrecisionDigits + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  DoubleToAscii(value, PRECISION, precision,
                decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);
  ASSERT(decimal_rep_length <= precision);

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  
  
  int exponent = decimal_point - 1;

  int extra_zero = ((flags_ & EMIT_TRAILING_ZERO_AFTER_POINT) != 0) ? 1 : 0;
  if ((-decimal_point + 1 > max_leading_padding_zeroes_in_precision_mode_) ||
      (decimal_point - precision + extra_zero >
       max_trailing_padding_zeroes_in_precision_mode_)) {
    
    
    
    for (int i = decimal_rep_length; i < precision; ++i) {
      decimal_rep[i] = '0';
    }

    CreateExponentialRepresentation(decimal_rep,
                                    precision,
                                    exponent,
                                    result_builder);
  } else {
    CreateDecimalRepresentation(decimal_rep, decimal_rep_length, decimal_point,
                                Max(0, precision - decimal_point),
                                result_builder);
  }
  return true;
}


static BignumDtoaMode DtoaToBignumDtoaMode(
    DoubleToStringConverter::DtoaMode dtoa_mode) {
  switch (dtoa_mode) {
    case DoubleToStringConverter::SHORTEST:  return BIGNUM_DTOA_SHORTEST;
    case DoubleToStringConverter::SHORTEST_SINGLE:
        return BIGNUM_DTOA_SHORTEST_SINGLE;
    case DoubleToStringConverter::FIXED:     return BIGNUM_DTOA_FIXED;
    case DoubleToStringConverter::PRECISION: return BIGNUM_DTOA_PRECISION;
    default:
      UNREACHABLE();
      return BIGNUM_DTOA_SHORTEST;  
  }
}


void DoubleToStringConverter::DoubleToAscii(double v,
                                            DtoaMode mode,
                                            int requested_digits,
                                            char* buffer,
                                            int buffer_length,
                                            bool* sign,
                                            int* length,
                                            int* point) {
  Vector<char> vector(buffer, buffer_length);
  ASSERT(!Double(v).IsSpecial());
  ASSERT(mode == SHORTEST || mode == SHORTEST_SINGLE || requested_digits >= 0);

  if (Double(v).Sign() < 0) {
    *sign = true;
    v = -v;
  } else {
    *sign = false;
  }

  if (mode == PRECISION && requested_digits == 0) {
    vector[0] = '\0';
    *length = 0;
    return;
  }

  if (v == 0) {
    vector[0] = '0';
    vector[1] = '\0';
    *length = 1;
    *point = 1;
    return;
  }

  bool fast_worked;
  switch (mode) {
    case SHORTEST:
      fast_worked = FastDtoa(v, FAST_DTOA_SHORTEST, 0, vector, length, point);
      break;
    case SHORTEST_SINGLE:
      fast_worked = FastDtoa(v, FAST_DTOA_SHORTEST_SINGLE, 0,
                             vector, length, point);
      break;
    case FIXED:
      fast_worked = FastFixedDtoa(v, requested_digits, vector, length, point);
      break;
    case PRECISION:
      fast_worked = FastDtoa(v, FAST_DTOA_PRECISION, requested_digits,
                             vector, length, point);
      break;
    default:
      UNREACHABLE();
      fast_worked = false;
  }
  if (fast_worked) return;

  
  BignumDtoaMode bignum_mode = DtoaToBignumDtoaMode(mode);
  BignumDtoa(v, bignum_mode, requested_digits, vector, length, point);
  vector[*length] = '\0';
}




static bool ConsumeSubString(const char** current,
                             const char* end,
                             const char* substring) {
  ASSERT(**current == *substring);
  for (substring++; *substring != '\0'; substring++) {
    ++*current;
    if (*current == end || **current != *substring) return false;
  }
  ++*current;
  return true;
}









const int kMaxSignificantDigits = 772;



static inline bool AdvanceToNonspace(const char** current, const char* end) {
  while (*current != end) {
    if (**current != ' ') return true;
    ++*current;
  }
  return false;
}


static bool isDigit(int x, int radix) {
  return (x >= '0' && x <= '9' && x < '0' + radix)
      || (radix > 10 && x >= 'a' && x < 'a' + radix - 10)
      || (radix > 10 && x >= 'A' && x < 'A' + radix - 10);
}


static double SignedZero(bool sign) {
  return sign ? -0.0 : 0.0;
}



template <int radix_log_2>
static double RadixStringToIeee(const char* current,
                                const char* end,
                                bool sign,
                                bool allow_trailing_junk,
                                double junk_string_value,
                                bool read_as_double,
                                const char** trailing_pointer) {
  ASSERT(current != end);

  const int kDoubleSize = Double::kSignificandSize;
  const int kSingleSize = Single::kSignificandSize;
  const int kSignificandSize = read_as_double? kDoubleSize: kSingleSize;

  
  while (*current == '0') {
    ++current;
    if (current == end) {
      *trailing_pointer = end;
      return SignedZero(sign);
    }
  }

  int64_t number = 0;
  int exponent = 0;
  const int radix = (1 << radix_log_2);

  do {
    int digit;
    if (*current >= '0' && *current <= '9' && *current < '0' + radix) {
      digit = static_cast<char>(*current) - '0';
    } else if (radix > 10 && *current >= 'a' && *current < 'a' + radix - 10) {
      digit = static_cast<char>(*current) - 'a' + 10;
    } else if (radix > 10 && *current >= 'A' && *current < 'A' + radix - 10) {
      digit = static_cast<char>(*current) - 'A' + 10;
    } else {
      if (allow_trailing_junk || !AdvanceToNonspace(&current, end)) {
        break;
      } else {
        return junk_string_value;
      }
    }

    number = number * radix + digit;
    int overflow = static_cast<int>(number >> kSignificandSize);
    if (overflow != 0) {
      
      
      int overflow_bits_count = 1;
      while (overflow > 1) {
        overflow_bits_count++;
        overflow >>= 1;
      }

      int dropped_bits_mask = ((1 << overflow_bits_count) - 1);
      int dropped_bits = static_cast<int>(number) & dropped_bits_mask;
      number >>= overflow_bits_count;
      exponent = overflow_bits_count;

      bool zero_tail = true;
      while (true) {
        ++current;
        if (current == end || !isDigit(*current, radix)) break;
        zero_tail = zero_tail && *current == '0';
        exponent += radix_log_2;
      }

      if (!allow_trailing_junk && AdvanceToNonspace(&current, end)) {
        return junk_string_value;
      }

      int middle_value = (1 << (overflow_bits_count - 1));
      if (dropped_bits > middle_value) {
        number++;  
      } else if (dropped_bits == middle_value) {
        
        
        if ((number & 1) != 0 || !zero_tail) {
          number++;  
        }
      }

      
      if ((number & ((int64_t)1 << kSignificandSize)) != 0) {
        exponent++;
        number >>= 1;
      }
      break;
    }
    ++current;
  } while (current != end);

  ASSERT(number < ((int64_t)1 << kSignificandSize));
  ASSERT(static_cast<int64_t>(static_cast<double>(number)) == number);

  *trailing_pointer = current;

  if (exponent == 0) {
    if (sign) {
      if (number == 0) return -0.0;
      number = -number;
    }
    return static_cast<double>(number);
  }

  ASSERT(number != 0);
  return Double(DiyFp(number, exponent)).value();
}


double StringToDoubleConverter::StringToIeee(
    const char* input,
    int length,
    int* processed_characters_count,
    bool read_as_double) {
  const char* current = input;
  const char* end = input + length;

  *processed_characters_count = 0;

  const bool allow_trailing_junk = (flags_ & ALLOW_TRAILING_JUNK) != 0;
  const bool allow_leading_spaces = (flags_ & ALLOW_LEADING_SPACES) != 0;
  const bool allow_trailing_spaces = (flags_ & ALLOW_TRAILING_SPACES) != 0;
  const bool allow_spaces_after_sign = (flags_ & ALLOW_SPACES_AFTER_SIGN) != 0;

  
  
  
  
  
  
  
  
  if (current == end) return empty_string_value_;

  if (allow_leading_spaces || allow_trailing_spaces) {
    if (!AdvanceToNonspace(&current, end)) {
      *processed_characters_count = current - input;
      return empty_string_value_;
    }
    if (!allow_leading_spaces && (input != current)) {
      
      return junk_string_value_;
    }
  }

  
  const int kBufferSize = kMaxSignificantDigits + 10;
  char buffer[kBufferSize];  
  int buffer_pos = 0;

  
  
  int exponent = 0;
  int significant_digits = 0;
  int insignificant_digits = 0;
  bool nonzero_digit_dropped = false;

  bool sign = false;

  if (*current == '+' || *current == '-') {
    sign = (*current == '-');
    ++current;
    const char* next_non_space = current;
    
    if (!AdvanceToNonspace(&next_non_space, end)) return junk_string_value_;
    if (!allow_spaces_after_sign && (current != next_non_space)) {
      return junk_string_value_;
    }
    current = next_non_space;
  }

  if (infinity_symbol_ != NULL) {
    if (*current == infinity_symbol_[0]) {
      if (!ConsumeSubString(&current, end, infinity_symbol_)) {
        return junk_string_value_;
      }

      if (!(allow_trailing_spaces || allow_trailing_junk) && (current != end)) {
        return junk_string_value_;
      }
      if (!allow_trailing_junk && AdvanceToNonspace(&current, end)) {
        return junk_string_value_;
      }

      ASSERT(buffer_pos == 0);
      *processed_characters_count = current - input;
      return sign ? -Double::Infinity() : Double::Infinity();
    }
  }

  if (nan_symbol_ != NULL) {
    if (*current == nan_symbol_[0]) {
      if (!ConsumeSubString(&current, end, nan_symbol_)) {
        return junk_string_value_;
      }

      if (!(allow_trailing_spaces || allow_trailing_junk) && (current != end)) {
        return junk_string_value_;
      }
      if (!allow_trailing_junk && AdvanceToNonspace(&current, end)) {
        return junk_string_value_;
      }

      ASSERT(buffer_pos == 0);
      *processed_characters_count = current - input;
      return sign ? -Double::NaN() : Double::NaN();
    }
  }

  bool leading_zero = false;
  if (*current == '0') {
    ++current;
    if (current == end) {
      *processed_characters_count = current - input;
      return SignedZero(sign);
    }

    leading_zero = true;

    
    if ((flags_ & ALLOW_HEX) && (*current == 'x' || *current == 'X')) {
      ++current;
      if (current == end || !isDigit(*current, 16)) {
        return junk_string_value_;  
      }

      const char* tail_pointer = NULL;
      double result = RadixStringToIeee<4>(current,
                                           end,
                                           sign,
                                           allow_trailing_junk,
                                           junk_string_value_,
                                           read_as_double,
                                           &tail_pointer);
      if (tail_pointer != NULL) {
        if (allow_trailing_spaces) AdvanceToNonspace(&tail_pointer, end);
        *processed_characters_count = tail_pointer - input;
      }
      return result;
    }

    
    while (*current == '0') {
      ++current;
      if (current == end) {
        *processed_characters_count = current - input;
        return SignedZero(sign);
      }
    }
  }

  bool octal = leading_zero && (flags_ & ALLOW_OCTALS) != 0;

  
  while (*current >= '0' && *current <= '9') {
    if (significant_digits < kMaxSignificantDigits) {
      ASSERT(buffer_pos < kBufferSize);
      buffer[buffer_pos++] = static_cast<char>(*current);
      significant_digits++;
      
    } else {
      insignificant_digits++;  
      nonzero_digit_dropped = nonzero_digit_dropped || *current != '0';
    }
    octal = octal && *current < '8';
    ++current;
    if (current == end) goto parsing_done;
  }

  if (significant_digits == 0) {
    octal = false;
  }

  if (*current == '.') {
    if (octal && !allow_trailing_junk) return junk_string_value_;
    if (octal) goto parsing_done;

    ++current;
    if (current == end) {
      if (significant_digits == 0 && !leading_zero) {
        return junk_string_value_;
      } else {
        goto parsing_done;
      }
    }

    if (significant_digits == 0) {
      
      
      
      while (*current == '0') {
        ++current;
        if (current == end) {
          *processed_characters_count = current - input;
          return SignedZero(sign);
        }
        exponent--;  
      }
    }

    
    
    while (*current >= '0' && *current <= '9') {
      if (significant_digits < kMaxSignificantDigits) {
        ASSERT(buffer_pos < kBufferSize);
        buffer[buffer_pos++] = static_cast<char>(*current);
        significant_digits++;
        exponent--;
      } else {
        
        nonzero_digit_dropped = nonzero_digit_dropped || *current != '0';
      }
      ++current;
      if (current == end) goto parsing_done;
    }
  }

  if (!leading_zero && exponent == 0 && significant_digits == 0) {
    
    
    
    
    return junk_string_value_;
  }

  
  if (*current == 'e' || *current == 'E') {
    if (octal && !allow_trailing_junk) return junk_string_value_;
    if (octal) goto parsing_done;
    ++current;
    if (current == end) {
      if (allow_trailing_junk) {
        goto parsing_done;
      } else {
        return junk_string_value_;
      }
    }
    char sign = '+';
    if (*current == '+' || *current == '-') {
      sign = static_cast<char>(*current);
      ++current;
      if (current == end) {
        if (allow_trailing_junk) {
          goto parsing_done;
        } else {
          return junk_string_value_;
        }
      }
    }

    if (current == end || *current < '0' || *current > '9') {
      if (allow_trailing_junk) {
        goto parsing_done;
      } else {
        return junk_string_value_;
      }
    }

    const int max_exponent = INT_MAX / 2;
    ASSERT(-max_exponent / 2 <= exponent && exponent <= max_exponent / 2);
    int num = 0;
    do {
      
      int digit = *current - '0';
      if (num >= max_exponent / 10
          && !(num == max_exponent / 10 && digit <= max_exponent % 10)) {
        num = max_exponent;
      } else {
        num = num * 10 + digit;
      }
      ++current;
    } while (current != end && *current >= '0' && *current <= '9');

    exponent += (sign == '-' ? -num : num);
  }

  if (!(allow_trailing_spaces || allow_trailing_junk) && (current != end)) {
    return junk_string_value_;
  }
  if (!allow_trailing_junk && AdvanceToNonspace(&current, end)) {
    return junk_string_value_;
  }
  if (allow_trailing_spaces) {
    AdvanceToNonspace(&current, end);
  }

  parsing_done:
  exponent += insignificant_digits;

  if (octal) {
    double result;
    const char* tail_pointer = NULL;
    result = RadixStringToIeee<3>(buffer,
                                  buffer + buffer_pos,
                                  sign,
                                  allow_trailing_junk,
                                  junk_string_value_,
                                  read_as_double,
                                  &tail_pointer);
    ASSERT(tail_pointer != NULL);
    *processed_characters_count = current - input;
    return result;
  }

  if (nonzero_digit_dropped) {
    buffer[buffer_pos++] = '1';
    exponent--;
  }

  ASSERT(buffer_pos < kBufferSize);
  buffer[buffer_pos] = '\0';

  double converted;
  if (read_as_double) {
    converted = Strtod(Vector<const char>(buffer, buffer_pos), exponent);
  } else {
    converted = Strtof(Vector<const char>(buffer, buffer_pos), exponent);
  }
  *processed_characters_count = current - input;
  return sign? -converted: converted;
}

}  

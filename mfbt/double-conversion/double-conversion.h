


























#ifndef DOUBLE_CONVERSION_DOUBLE_CONVERSION_H_
#define DOUBLE_CONVERSION_DOUBLE_CONVERSION_H_

#include "mozilla/Types.h"
#include "utils.h"

namespace double_conversion {

class DoubleToStringConverter {
 public:
  
  
  
  static const int kMaxFixedDigitsBeforePoint = 60;
  static const int kMaxFixedDigitsAfterPoint = 60;

  
  
  static const int kMaxExponentialDigits = 120;

  
  
  
  static const int kMinPrecisionDigits = 1;
  static const int kMaxPrecisionDigits = 120;

  enum Flags {
    NO_FLAGS = 0,
    EMIT_POSITIVE_EXPONENT_SIGN = 1,
    EMIT_TRAILING_DECIMAL_POINT = 2,
    EMIT_TRAILING_ZERO_AFTER_POINT = 4,
    UNIQUE_ZERO = 8
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  DoubleToStringConverter(int flags,
                          const char* infinity_symbol,
                          const char* nan_symbol,
                          char exponent_character,
                          int decimal_in_shortest_low,
                          int decimal_in_shortest_high,
                          int max_leading_padding_zeroes_in_precision_mode,
                          int max_trailing_padding_zeroes_in_precision_mode)
      : flags_(flags),
        infinity_symbol_(infinity_symbol),
        nan_symbol_(nan_symbol),
        exponent_character_(exponent_character),
        decimal_in_shortest_low_(decimal_in_shortest_low),
        decimal_in_shortest_high_(decimal_in_shortest_high),
        max_leading_padding_zeroes_in_precision_mode_(
            max_leading_padding_zeroes_in_precision_mode),
        max_trailing_padding_zeroes_in_precision_mode_(
            max_trailing_padding_zeroes_in_precision_mode) {
    
    
    ASSERT(((flags & EMIT_TRAILING_DECIMAL_POINT) != 0) ||
        !((flags & EMIT_TRAILING_ZERO_AFTER_POINT) != 0));
  }

  
  static MFBT_API const DoubleToStringConverter& EcmaScriptConverter();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool ToShortest(double value, StringBuilder* result_builder) const {
    return ToShortestIeeeNumber(value, result_builder, SHORTEST);
  }

  
  bool ToShortestSingle(float value, StringBuilder* result_builder) const {
    return ToShortestIeeeNumber(value, result_builder, SHORTEST_SINGLE);
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MFBT_API bool ToFixed(double value,
               int requested_digits,
               StringBuilder* result_builder) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MFBT_API bool ToExponential(double value,
                     int requested_digits,
                     StringBuilder* result_builder) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MFBT_API bool ToPrecision(double value,
                   int precision,
                   StringBuilder* result_builder) const;

  enum DtoaMode {
    
    
    
    SHORTEST,
    
    SHORTEST_SINGLE,
    
    
    
    FIXED,
    
    PRECISION
  };

  
  
  
  
  
  
  static const MFBT_DATA int kBase10MaximalLength = 17;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static MFBT_API void DoubleToAscii(double v,
                            DtoaMode mode,
                            int requested_digits,
                            char* buffer,
                            int buffer_length,
                            bool* sign,
                            int* length,
                            int* point);

 private:
  
  MFBT_API bool ToShortestIeeeNumber(double value,
                            StringBuilder* result_builder,
                            DtoaMode mode) const;

  
  
  
  
  MFBT_API bool HandleSpecialValues(double value, StringBuilder* result_builder) const;
  
  
  MFBT_API void CreateExponentialRepresentation(const char* decimal_digits,
                                       int length,
                                       int exponent,
                                       StringBuilder* result_builder) const;
  
  MFBT_API void CreateDecimalRepresentation(const char* decimal_digits,
                                   int length,
                                   int decimal_point,
                                   int digits_after_point,
                                   StringBuilder* result_builder) const;

  const int flags_;
  const char* const infinity_symbol_;
  const char* const nan_symbol_;
  const char exponent_character_;
  const int decimal_in_shortest_low_;
  const int decimal_in_shortest_high_;
  const int max_leading_padding_zeroes_in_precision_mode_;
  const int max_trailing_padding_zeroes_in_precision_mode_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DoubleToStringConverter);
};


class StringToDoubleConverter {
 public:
  
  
  enum Flags {
    NO_FLAGS = 0,
    ALLOW_HEX = 1,
    ALLOW_OCTALS = 2,
    ALLOW_TRAILING_JUNK = 4,
    ALLOW_LEADING_SPACES = 8,
    ALLOW_TRAILING_SPACES = 16,
    ALLOW_SPACES_AFTER_SIGN = 32
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  StringToDoubleConverter(int flags,
                          double empty_string_value,
                          double junk_string_value,
                          const char* infinity_symbol,
                          const char* nan_symbol)
      : flags_(flags),
        empty_string_value_(empty_string_value),
        junk_string_value_(junk_string_value),
        infinity_symbol_(infinity_symbol),
        nan_symbol_(nan_symbol) {
  }

  
  
  
  
  
  double StringToDouble(const char* buffer,
                        int length,
                        int* processed_characters_count) {
    return StringToIeee(buffer, length, processed_characters_count, true);
  }

  
  
  
  float StringToFloat(const char* buffer,
                      int length,
                      int* processed_characters_count) {
    return static_cast<float>(StringToIeee(buffer, length,
                                           processed_characters_count, false));
  }

 private:
  const int flags_;
  const double empty_string_value_;
  const double junk_string_value_;
  const char* const infinity_symbol_;
  const char* const nan_symbol_;

  double StringToIeee(const char* buffer,
                      int length,
                      int* processed_characters_count,
                      bool read_as_double);

  DISALLOW_IMPLICIT_CONSTRUCTORS(StringToDoubleConverter);
};

}  

#endif  

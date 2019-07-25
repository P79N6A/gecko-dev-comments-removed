


























#include <math.h>

#include "v8.h"
#include "dtoa.h"

namespace v8 {
namespace internal {












char* DoubleToCString(double v, char* buffer, int buflen) {
  StringBuilder builder(buffer, buflen);

  switch (fpclassify(v)) {
    case FP_NAN:
      builder.AddString("NaN");
      break;

    case FP_INFINITE:
      if (v < 0.0) {
        builder.AddString("-Infinity");
      } else {
        builder.AddString("Infinity");
      }
      break;

    case FP_ZERO:
      builder.AddCharacter('0');
      break;

    default: {
      int decimal_point;
      int sign;
      char* decimal_rep;
      
      const int kV8DtoaBufferCapacity = kBase10MaximalLength + 1;
      char v8_dtoa_buffer[kV8DtoaBufferCapacity];
      int length;

      if (DoubleToAscii(v, DTOA_SHORTEST, 0,
                        Vector<char>(v8_dtoa_buffer, kV8DtoaBufferCapacity),
                        &sign, &length, &decimal_point)) {
        decimal_rep = v8_dtoa_buffer;
      } else {
        return NULL;    
        
        
        
      }

      if (sign) builder.AddCharacter('-');

      if (length <= decimal_point && decimal_point <= 21) {
        
        builder.AddString(decimal_rep);
        builder.AddPadding('0', decimal_point - length);

      } else if (0 < decimal_point && decimal_point <= 21) {
        
        builder.AddSubstring(decimal_rep, decimal_point);
        builder.AddCharacter('.');
        builder.AddString(decimal_rep + decimal_point);

      } else if (decimal_point <= 0 && decimal_point > -6) {
        
        builder.AddString("0.");
        builder.AddPadding('0', -decimal_point);
        builder.AddString(decimal_rep);

      } else {
        
        builder.AddCharacter(decimal_rep[0]);
        if (length != 1) {
          builder.AddCharacter('.');
          builder.AddString(decimal_rep + 1);
        }
        builder.AddCharacter('e');
        builder.AddCharacter((decimal_point >= 0) ? '+' : '-');
        int exponent = decimal_point - 1;
        if (exponent < 0) exponent = -exponent;
        
        
        
        
        
        builder.AddInteger(exponent);
      }

      
    }
  }
  return builder.Finalize();
}

} }  

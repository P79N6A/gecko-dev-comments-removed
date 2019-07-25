


























#ifndef V8_DTOA_H_
#define V8_DTOA_H_

namespace v8 {
namespace internal {

enum DtoaMode {
  
  DTOA_SHORTEST,
  
  
  
  DTOA_FIXED,
  
  DTOA_PRECISION
};




static const int kBase10MaximalLength = 17;



























bool DoubleToAscii(double v, DtoaMode mode, int requested_digits,
                   Vector<char> buffer, int* sign, int* length, int* point);

} }  

#endif  

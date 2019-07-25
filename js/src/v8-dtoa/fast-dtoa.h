


























#ifndef V8_FAST_DTOA_H_
#define V8_FAST_DTOA_H_

namespace v8 {
namespace internal {



static const int kFastDtoaMaximalLength = 17;














bool FastDtoa(double d,
              Vector<char> buffer,
              int* length,
              int* point);

} }  

#endif  

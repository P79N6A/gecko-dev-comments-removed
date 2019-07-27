



#ifndef BASE_RAND_UTIL_H_
#define BASE_RAND_UTIL_H_

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {


BASE_EXPORT uint64 RandUint64();


BASE_EXPORT int RandInt(int min, int max);






BASE_EXPORT uint64 RandGenerator(uint64 range);


BASE_EXPORT double RandDouble();



BASE_EXPORT double BitsToOpenEndedUnitInterval(uint64 bits);






BASE_EXPORT void RandBytes(void* output, size_t output_length);









BASE_EXPORT std::string RandBytesAsString(size_t length);

#if defined(OS_POSIX)
BASE_EXPORT int GetUrandomFD();
#endif

}  

#endif  











#ifndef WEBRTC_BASE_HELPERS_H_
#define WEBRTC_BASE_HELPERS_H_

#include <string>
#include "webrtc/base/basictypes.h"

namespace rtc {


void SetRandomTestMode(bool test);


bool InitRandom(int seed);
bool InitRandom(const char* seed, size_t len);




std::string CreateRandomString(size_t length);




bool CreateRandomString(size_t length, std::string* str);




bool CreateRandomString(size_t length, const std::string& table,
                        std::string* str);


uint32 CreateRandomId();


uint64 CreateRandomId64();


uint32 CreateRandomNonZeroId();


double CreateRandomDouble();

}  

#endif  

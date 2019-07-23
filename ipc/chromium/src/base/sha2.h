



#ifndef BASE_SHA2_H__
#define BASE_SHA2_H__

#include <string>

namespace base {





enum {
  SHA256_LENGTH = 32  
};




void SHA256HashString(const std::string& str, void* output, size_t len);

}  

#endif 







#ifndef RLZ_LIB_STRING_UTILS_H_
#define RLZ_LIB_STRING_UTILS_H_

#include <string>

namespace rlz_lib {

bool BytesToString(const unsigned char* data,
                   int data_len,
                   std::string* string);

};  

#endif  

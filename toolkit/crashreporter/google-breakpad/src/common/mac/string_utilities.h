






























#ifndef COMMON_MAC_STRING_UTILITIES_H__
#define COMMON_MAC_STRING_UTILITIES_H__

#include <CoreFoundation/CoreFoundation.h>

#include <string>

namespace MacStringUtils {

using std::string;


string ConvertToString(CFStringRef str);



unsigned int IntegerValueAtIndex(string &str, unsigned int idx);

}  

#endif  

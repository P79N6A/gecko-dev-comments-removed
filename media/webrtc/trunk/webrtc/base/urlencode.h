









#ifndef _URLENCODE_H_
#define _URLENCODE_H_ 

#include <string>

namespace rtc {


int UrlDecode(const char *source, char *dest);


int UrlDecodeWithoutEncodingSpaceAsPlus(const char *source, char *dest);



int UrlEncode(const char *source, char *dest, unsigned max);


int UrlEncodeWithoutEncodingSpaceAsPlus(const char *source, char *dest,
                                        unsigned max);



int UrlEncodeOnlyUnsafeChars(const char *source, char *dest, unsigned max);

std::string UrlDecodeString(const std::string & encoded);
std::string UrlDecodeStringWithoutEncodingSpaceAsPlus(
    const std::string & encoded);
std::string UrlEncodeString(const std::string & decoded);
std::string UrlEncodeStringWithoutEncodingSpaceAsPlus(
    const std::string & decoded);
std::string UrlEncodeStringForOnlyUnsafeChars(const std::string & decoded);

#endif

}  

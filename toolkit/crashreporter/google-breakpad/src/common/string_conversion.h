






























#ifndef COMMON_STRING_CONVERSION_H__
#define COMMON_STRING_CONVERSION_H__

#include <string>
#include <vector>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {
  
using std::vector;



void UTF8ToUTF16(const char *in, vector<u_int16_t> *out);





int UTF8ToUTF16Char(const char *in, int in_length, u_int16_t out[2]);



void UTF32ToUTF16(const wchar_t *in, vector<u_int16_t> *out);



void UTF32ToUTF16Char(wchar_t in, u_int16_t out[2]);


std::string UTF16ToUTF8(const vector<u_int16_t> &in, bool swap);

}  

#endif  
































#ifndef COMMON_STRING_CONVERSION_H__
#define COMMON_STRING_CONVERSION_H__

#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {
  
using std::vector;



void UTF8ToUTF16(const char *in, vector<uint16_t> *out);





int UTF8ToUTF16Char(const char *in, int in_length, uint16_t out[2]);



void UTF32ToUTF16(const wchar_t *in, vector<uint16_t> *out);



void UTF32ToUTF16Char(wchar_t in, uint16_t out[2]);


string UTF16ToUTF8(const vector<uint16_t> &in, bool swap);

}  

#endif  

































#include <google/protobuf/stubs/stringprintf.h>

#include <errno.h>
#include <stdarg.h> 
#include <stdio.h> 
#include <vector>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

#ifdef _MSC_VER
enum { IS_COMPILER_MSVC = 1 };
#ifndef va_copy


#define va_copy(dest, src) ((dest) = (src))
#endif
#else
enum { IS_COMPILER_MSVC = 0 };
#endif

void StringAppendV(string* dst, const char* format, va_list ap) {
  
  static const int kSpaceLength = 1024;
  char space[kSpaceLength];

  
  
  
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = vsnprintf(space, kSpaceLength, format, backup_ap);
  va_end(backup_ap);

  if (result < kSpaceLength) {
    if (result >= 0) {
      
      dst->append(space, result);
      return;
    }

    if (IS_COMPILER_MSVC) {
      
      
      va_copy(backup_ap, ap);
      result = vsnprintf(NULL, 0, format, backup_ap);
      va_end(backup_ap);
    }

    if (result < 0) {
      
      return;
    }
  }

  
  
  int length = result+1;
  char* buf = new char[length];

  
  va_copy(backup_ap, ap);
  result = vsnprintf(buf, length, format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < length) {
    
    dst->append(buf, result);
  }
  delete[] buf;
}


string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  string result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

const string& SStringPrintf(string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  StringAppendV(dst, format, ap);
  va_end(ap);
  return *dst;
}

void StringAppendF(string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringAppendV(dst, format, ap);
  va_end(ap);
}


const int kStringPrintfVectorMaxArgs = 32;




static const char string_printf_empty_block[256] = { '\0' };

string StringPrintfVector(const char* format, const vector<string>& v) {
  GOOGLE_CHECK_LE(v.size(), kStringPrintfVectorMaxArgs)
      << "StringPrintfVector currently only supports up to "
      << kStringPrintfVectorMaxArgs << " arguments. "
      << "Feel free to add support for more if you need it.";

  
  
  

  const char* cstr[kStringPrintfVectorMaxArgs];
  for (int i = 0; i < v.size(); ++i) {
    cstr[i] = v[i].c_str();
  }
  for (int i = v.size(); i < GOOGLE_ARRAYSIZE(cstr); ++i) {
    cstr[i] = &string_printf_empty_block[0];
  }

  
  
  
  

  GOOGLE_COMPILE_ASSERT(kStringPrintfVectorMaxArgs == 32, arg_count_mismatch);
  return StringPrintf(format,
                      cstr[0], cstr[1], cstr[2], cstr[3], cstr[4],
                      cstr[5], cstr[6], cstr[7], cstr[8], cstr[9],
                      cstr[10], cstr[11], cstr[12], cstr[13], cstr[14],
                      cstr[15], cstr[16], cstr[17], cstr[18], cstr[19],
                      cstr[20], cstr[21], cstr[22], cstr[23], cstr[24],
                      cstr[25], cstr[26], cstr[27], cstr[28], cstr[29],
                      cstr[30], cstr[31]);
}
}  
}  

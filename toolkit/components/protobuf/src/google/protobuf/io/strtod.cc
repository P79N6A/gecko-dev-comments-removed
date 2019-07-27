





























#include <google/protobuf/io/strtod.h>

#include <cstdio>
#include <cstring>
#include <string>

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {






namespace {




string LocalizeRadix(const char* input, const char* radix_pos) {
  
  
  
  
  
  char temp[16];
  int size = sprintf(temp, "%.1f", 1.5);
  GOOGLE_CHECK_EQ(temp[0], '1');
  GOOGLE_CHECK_EQ(temp[size-1], '5');
  GOOGLE_CHECK_LE(size, 6);

  
  string result;
  result.reserve(strlen(input) + size - 3);
  result.append(input, radix_pos);
  result.append(temp + 1, size - 2);
  result.append(radix_pos + 1);
  return result;
}

}  

double NoLocaleStrtod(const char* text, char** original_endptr) {
  
  
  
  
  

  char* temp_endptr;
  double result = strtod(text, &temp_endptr);
  if (original_endptr != NULL) *original_endptr = temp_endptr;
  if (*temp_endptr != '.') return result;

  
  
  
  string localized = LocalizeRadix(text, temp_endptr);
  const char* localized_cstr = localized.c_str();
  char* localized_endptr;
  result = strtod(localized_cstr, &localized_endptr);
  if ((localized_endptr - localized_cstr) >
      (temp_endptr - text)) {
    
    
    if (original_endptr != NULL) {
      
      int size_diff = localized.size() - strlen(text);
      
      *original_endptr = const_cast<char*>(
        text + (localized_endptr - localized_cstr - size_diff));
    }
  }

  return result;
}

}  
}  
}  

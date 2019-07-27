





#include "rlz/lib/string_utils.h"

namespace rlz_lib {

bool BytesToString(const unsigned char* data,
                   int data_len,
                   std::string* string) {
  if (!string)
    return false;

  string->clear();
  if (data_len < 1 || !data)
    return false;

  static const char kHex[] = "0123456789ABCDEF";

  
  string->resize(data_len * 2);
  int index = data_len;
  while (index--) {
    string->at(2 * index) = kHex[data[index] >> 4];  
    string->at(2 * index + 1) = kHex[data[index] & 0x0F];  
  }

  return true;
}

}  

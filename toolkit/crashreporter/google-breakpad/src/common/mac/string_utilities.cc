




























#include "processor/scoped_ptr.h"
#include "common/mac/string_utilities.h"

namespace MacStringUtils {

using google_breakpad::scoped_array;

std::string ConvertToString(CFStringRef str) {
  CFIndex length = CFStringGetLength(str);
  std::string result;

  if (!length)
    return result;

  CFIndex maxUTF8Length =
    CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
  scoped_array<UInt8> buffer(new UInt8[maxUTF8Length + 1]);
  CFIndex actualUTF8Length;
  CFStringGetBytes(str, CFRangeMake(0, length), kCFStringEncodingUTF8, 0,
                   false, buffer.get(), maxUTF8Length, &actualUTF8Length);
  buffer[actualUTF8Length] = 0;
  result.assign((const char *)buffer.get());

  return result;
}

unsigned int IntegerValueAtIndex(string &str, unsigned int idx) {
  string digits("0123456789"), temp;
  unsigned int start = 0;
  unsigned int end;
  unsigned int found = 0;
  unsigned int result = 0;

  for (; found <= idx; ++found) {
    end = str.find_first_not_of(digits, start);

    if (end == string::npos)
      end = str.size();

    temp = str.substr(start, end - start);
    result = atoi(temp.c_str());
    start = str.find_first_of(digits, end + 1);

    if (start == string::npos)
      break;
  }

  return result;
}

}  

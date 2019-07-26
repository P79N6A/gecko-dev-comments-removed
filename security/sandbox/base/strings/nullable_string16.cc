



#include "base/strings/nullable_string16.h"

#include <ostream>

#include "base/strings/utf_string_conversions.h"

namespace base {

std::ostream& operator<<(std::ostream& out, const NullableString16& value) {
  return value.is_null() ? out << "(null)" : out << UTF16ToUTF8(value.string());
}

}  

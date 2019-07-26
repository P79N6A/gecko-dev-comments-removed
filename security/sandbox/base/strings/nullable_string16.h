



#ifndef BASE_STRINGS_NULLABLE_STRING16_H_
#define BASE_STRINGS_NULLABLE_STRING16_H_

#include <iosfwd>

#include "base/base_export.h"
#include "base/strings/string16.h"

namespace base {




class NullableString16 {
 public:
  NullableString16() : is_null_(true) { }
  NullableString16(const string16& string, bool is_null)
      : string_(string), is_null_(is_null) {
  }

  const string16& string() const { return string_; }
  bool is_null() const { return is_null_; }

 private:
  string16 string_;
  bool is_null_;
};

inline bool operator==(const NullableString16& a, const NullableString16& b) {
  return a.is_null() == b.is_null() && a.string() == b.string();
}

inline bool operator!=(const NullableString16& a, const NullableString16& b) {
  return !(a == b);
}

BASE_EXPORT std::ostream& operator<<(std::ostream& out,
                                     const NullableString16& value);

}  

#endif  

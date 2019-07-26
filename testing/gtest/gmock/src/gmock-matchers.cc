



































#include "gmock/gmock-matchers.h"
#include "gmock/gmock-generated-matchers.h"

#include <string.h>
#include <sstream>
#include <string>

namespace testing {



Matcher<const internal::string&>::Matcher(const internal::string& s) {
  *this = Eq(s);
}



Matcher<const internal::string&>::Matcher(const char* s) {
  *this = Eq(internal::string(s));
}


Matcher<internal::string>::Matcher(const internal::string& s) { *this = Eq(s); }


Matcher<internal::string>::Matcher(const char* s) {
  *this = Eq(internal::string(s));
}

namespace internal {



string JoinAsTuple(const Strings& fields) {
  switch (fields.size()) {
    case 0:
      return "";
    case 1:
      return fields[0];
    default:
      string result = "(" + fields[0];
      for (size_t i = 1; i < fields.size(); i++) {
        result += ", ";
        result += fields[i];
      }
      result += ")";
      return result;
  }
}






string FormatMatcherDescription(bool negation, const char* matcher_name,
                                const Strings& param_values) {
  string result = ConvertIdentifierNameToWords(matcher_name);
  if (param_values.size() >= 1)
    result += " " + JoinAsTuple(param_values);
  return negation ? "not (" + result + ")" : result;
}

}  
}  

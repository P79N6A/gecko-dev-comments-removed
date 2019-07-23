





#ifndef BASE_STRING_ESCAPE_H__
#define BASE_STRING_ESCAPE_H__

#include "base/string16.h"

namespace string_escape {







void JavascriptDoubleQuote(const string16& str,
                           bool put_in_quotes,
                           std::string* dst);






void JavascriptDoubleQuote(const std::string& str,
                           bool put_in_quotes,
                           std::string* dst);

}  

#endif  















#ifndef BASE_HASH_TABLES_H_
#define BASE_HASH_TABLES_H_

#include "build/build_config.h"

#include "base/string16.h"

#if defined(COMPILER_MSVC)
#include <hash_map>
#include <hash_set>
namespace base {
using stdext::hash_map;
using stdext::hash_set;
}
#elif defined(COMPILER_GCC)



#ifdef __DEPRECATED
#define CHROME_OLD__DEPRECATED __DEPRECATED
#undef __DEPRECATED
#endif

#include <ext/hash_map>
#include <ext/hash_set>
#include <string>

#ifdef CHROME_OLD__DEPRECATED
#define __DEPRECATED CHROME_OLD__DEPRECATED
#undef CHROME_OLD__DEPRECATED
#endif

namespace base {
using __gnu_cxx::hash_map;
using __gnu_cxx::hash_set;
}  

namespace __gnu_cxx {






#define DEFINE_TRIVIAL_HASH(integral_type) \
    template<> \
    struct hash<integral_type> { \
      std::size_t operator()(integral_type value) const { \
        return static_cast<std::size_t>(value); \
      } \
    }

DEFINE_TRIVIAL_HASH(long long);
DEFINE_TRIVIAL_HASH(unsigned long long);

#undef DEFINE_TRIVIAL_HASH







#define DEFINE_STRING_HASH(string_type) \
    template<> \
    struct hash<string_type> { \
      std::size_t operator()(const string_type& s) const { \
        std::size_t result = 0; \
        for (string_type::const_iterator i = s.begin(); i != s.end(); ++i) \
          result = (result * 131) + *i; \
        return result; \
      } \
    }

DEFINE_STRING_HASH(std::string);
DEFINE_STRING_HASH(std::wstring);

#if defined(WCHAR_T_IS_UTF32)


DEFINE_STRING_HASH(string16);
#endif  

#undef DEFINE_STRING_HASH

}  

#endif  

#endif  

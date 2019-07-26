







#ifndef mozilla_Char16_h
#define mozilla_Char16_h











#ifdef _MSC_VER
   






#  define MOZ_UTF16_HELPER(s) L##s
#  include <yvals.h>
#  define char16_t wchar_t
#elif defined(__cplusplus) && \
      (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
   
#  define MOZ_UTF16_HELPER(s) u##s
   



#  define MOZ_CHAR16_IS_NOT_WCHAR
#else
#  error "Char16.h requires C++11 (or something like it) for UTF-16 support."
#endif









#define MOZ_UTF16(s) MOZ_UTF16_HELPER(s)

static_assert(sizeof(char16_t) == 2, "Is char16_t type 16 bits?");
static_assert(char16_t(-1) > char16_t(0), "Is char16_t type unsigned?");
static_assert(sizeof(MOZ_UTF16('A')) == 2, "Is char literal 16 bits?");
static_assert(sizeof(MOZ_UTF16("")[0]) == 2, "Is string char 16 bits?");

#endif 









#ifndef mozilla_TypedEnum_h
#define mozilla_TypedEnum_h

#include "mozilla/Attributes.h"
#include "mozilla/MacroArgs.h"

#if defined(__cplusplus)

#if defined(__clang__)
   




#  ifndef __has_extension
#    define __has_extension __has_feature /* compatibility, for older versions of clang */
#  endif
#  if __has_extension(cxx_strong_enums)
#    define MOZ_HAVE_CXX11_ENUM_TYPE
#    define MOZ_HAVE_CXX11_STRONG_ENUMS
#  endif
#elif defined(__GNUC__)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if MOZ_GCC_VERSION_AT_LEAST(4, 6, 3)
#      define MOZ_HAVE_CXX11_ENUM_TYPE
#      define MOZ_HAVE_CXX11_STRONG_ENUMS
#    endif
#  endif
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1400
#    define MOZ_HAVE_CXX11_ENUM_TYPE
#  endif
#  if _MSC_VER >= 1700
#    define MOZ_HAVE_CXX11_STRONG_ENUMS
#  endif
#endif




















#ifdef MOZ_HAVE_CXX11_ENUM_TYPE
#  define MOZ_ENUM_TYPE(type)   : type
#else
#  define MOZ_ENUM_TYPE(type)
#endif










































#if defined(MOZ_HAVE_CXX11_STRONG_ENUMS)
  




   
#  define MOZ_BEGIN_NESTED_ENUM_CLASS_HELPER1(Name) \
     enum class Name {
   
#  define MOZ_BEGIN_NESTED_ENUM_CLASS_HELPER2(Name, type) \
     enum class Name : type {
#  define MOZ_END_NESTED_ENUM_CLASS(Name) \
     };
#  define MOZ_FINISH_NESTED_ENUM_CLASS(Name)







#  define MOZ_ENUM_CLASS_ENUM_TYPE(Name) Name
  


#  define MOZ_TEMPLATE_ENUM_CLASS_ENUM_TYPE(Name) Name
#else
   



































   
#  define MOZ_BEGIN_NESTED_ENUM_CLASS_HELPER1(Name) \
     class Name \
     { \
       public: \
         enum Enum \
         {
   
#  define MOZ_BEGIN_NESTED_ENUM_CLASS_HELPER2(Name, type) \
     class Name \
     { \
       public: \
         enum Enum MOZ_ENUM_TYPE(type) \
         {
#  define MOZ_END_NESTED_ENUM_CLASS(Name) \
         }; \
         Name() {} \
         Name(Enum aEnum) : mEnum(aEnum) {} \
         explicit Name(int num) : mEnum((Enum)num) {} \
         operator Enum() const { return mEnum; } \
       private: \
         Enum mEnum; \
     };
#  define MOZ_FINISH_NESTED_ENUM_CLASS(Name) \
     inline int operator+(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator+(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator-(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator-(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator*(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator*(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator/(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator/(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator%(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator%(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator+(const Name::Enum&) MOZ_DELETE; \
     inline int operator-(const Name::Enum&) MOZ_DELETE; \
     inline int& operator++(Name::Enum&) MOZ_DELETE; \
     inline int operator++(Name::Enum&, int) MOZ_DELETE; \
     inline int& operator--(Name::Enum&) MOZ_DELETE; \
     inline int operator--(Name::Enum&, int) MOZ_DELETE; \
     inline bool operator==(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator==(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator!=(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator!=(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator>(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator>(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator<(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator<(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator>=(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator>=(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator<=(const int&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator<=(const Name::Enum&, const int&) MOZ_DELETE; \
     inline bool operator!(const Name::Enum&) MOZ_DELETE; \
     inline bool operator&&(const bool&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator&&(const Name::Enum&, const bool&) MOZ_DELETE; \
     inline bool operator||(const bool&, const Name::Enum&) MOZ_DELETE; \
     inline bool operator||(const Name::Enum&, const bool&) MOZ_DELETE; \
     inline int operator~(const Name::Enum&) MOZ_DELETE; \
     inline int operator&(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator&(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator|(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator|(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator^(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator^(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator<<(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator<<(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int operator>>(const int&, const Name::Enum&) MOZ_DELETE; \
     inline int operator>>(const Name::Enum&, const int&) MOZ_DELETE; \
     inline int& operator+=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator-=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator*=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator/=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator%=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator&=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator|=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator^=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator<<=(int&, const Name::Enum&) MOZ_DELETE; \
     inline int& operator>>=(int&, const Name::Enum&) MOZ_DELETE;

  




#  define MOZ_ENUM_CLASS_ENUM_TYPE(Name) Name::Enum
  





























#  define MOZ_TEMPLATE_ENUM_CLASS_ENUM_TYPE(Name) typename Name::Enum
#endif

#  define MOZ_BEGIN_NESTED_ENUM_CLASS_GLUE(a, b) a b
#  define MOZ_BEGIN_NESTED_ENUM_CLASS(...) \
     MOZ_BEGIN_NESTED_ENUM_CLASS_GLUE( \
       MOZ_PASTE_PREFIX_AND_ARG_COUNT(MOZ_BEGIN_NESTED_ENUM_CLASS_HELPER, \
                                      __VA_ARGS__), \
       (__VA_ARGS__))

#  define MOZ_BEGIN_ENUM_CLASS(...) MOZ_BEGIN_NESTED_ENUM_CLASS(__VA_ARGS__)
#  define MOZ_END_ENUM_CLASS(Name) \
     MOZ_END_NESTED_ENUM_CLASS(Name) \
     MOZ_FINISH_NESTED_ENUM_CLASS(Name)

#endif 

#endif 

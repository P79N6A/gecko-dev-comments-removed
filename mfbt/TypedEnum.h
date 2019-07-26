






#ifndef mozilla_TypedEnum_h_
#define mozilla_TypedEnum_h_

#include "mozilla/Attributes.h"

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
#    if MOZ_GCC_VERSION_AT_LEAST(4, 5, 1)
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
  



#  define MOZ_BEGIN_ENUM_CLASS(Name, type) enum class Name : type {
#  define MOZ_END_ENUM_CLASS(Name)         };
#else
   


































#  define MOZ_BEGIN_ENUM_CLASS(Name, type) \
     class Name \
     { \
       public: \
         enum Enum MOZ_ENUM_TYPE(type) \
         {
#  define MOZ_END_ENUM_CLASS(Name) \
         }; \
         Name() {} \
         Name(Enum aEnum) : mEnum(aEnum) {} \
         explicit Name(int num) : mEnum((Enum)num) {} \
         operator Enum() const { return mEnum; } \
       private: \
         Enum mEnum; \
     }; \
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
#endif

#endif 

#endif  

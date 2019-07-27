







#ifndef mozilla_TypedEnum_h
#define mozilla_TypedEnum_h

#include "mozilla/TypedEnumInternal.h"
#include "mozilla/MacroArgs.h"

#if defined(__cplusplus)









































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
       enum Enum : type \
       {
#  define MOZ_END_NESTED_ENUM_CLASS(Name) \
       }; \
       Name() {} \
       MOZ_CONSTEXPR Name(Enum aEnum) : mEnum(aEnum) {} \
       template<typename Other> \
       explicit MOZ_CONSTEXPR Name(Other num) : mEnum((Enum)num) {} \
       MOZ_CONSTEXPR operator Enum() const { return mEnum; } \
       explicit MOZ_CONSTEXPR Name(const mozilla::CastableTypedEnumResult<Name>& aOther) \
         : mEnum(aOther.get()) \
       {} \
     private: \
       Enum mEnum; \
     };
#  define MOZ_FINISH_NESTED_ENUM_CLASS(Name) \
     inline int operator+(const int&, const Name::Enum&) = delete; \
     inline int operator+(const Name::Enum&, const int&) = delete; \
     inline int operator-(const int&, const Name::Enum&) = delete; \
     inline int operator-(const Name::Enum&, const int&) = delete; \
     inline int operator*(const int&, const Name::Enum&) = delete; \
     inline int operator*(const Name::Enum&, const int&) = delete; \
     inline int operator/(const int&, const Name::Enum&) = delete; \
     inline int operator/(const Name::Enum&, const int&) = delete; \
     inline int operator%(const int&, const Name::Enum&) = delete; \
     inline int operator%(const Name::Enum&, const int&) = delete; \
     inline int operator+(const Name::Enum&) = delete; \
     inline int operator-(const Name::Enum&) = delete; \
     inline int& operator++(Name::Enum&) = delete; \
     inline int operator++(Name::Enum&, int) = delete; \
     inline int& operator--(Name::Enum&) = delete; \
     inline int operator--(Name::Enum&, int) = delete; \
     inline bool operator==(const int&, const Name::Enum&) = delete; \
     inline bool operator==(const Name::Enum&, const int&) = delete; \
     inline bool operator!=(const int&, const Name::Enum&) = delete; \
     inline bool operator!=(const Name::Enum&, const int&) = delete; \
     inline bool operator>(const int&, const Name::Enum&) = delete; \
     inline bool operator>(const Name::Enum&, const int&) = delete; \
     inline bool operator<(const int&, const Name::Enum&) = delete; \
     inline bool operator<(const Name::Enum&, const int&) = delete; \
     inline bool operator>=(const int&, const Name::Enum&) = delete; \
     inline bool operator>=(const Name::Enum&, const int&) = delete; \
     inline bool operator<=(const int&, const Name::Enum&) = delete; \
     inline bool operator<=(const Name::Enum&, const int&) = delete; \
     inline bool operator!(const Name::Enum&) = delete; \
     inline bool operator&&(const bool&, const Name::Enum&) = delete; \
     inline bool operator&&(const Name::Enum&, const bool&) = delete; \
     inline bool operator||(const bool&, const Name::Enum&) = delete; \
     inline bool operator||(const Name::Enum&, const bool&) = delete; \
     inline int operator&(const int&, const Name::Enum&) = delete; \
     inline int operator&(const Name::Enum&, const int&) = delete; \
     inline int operator|(const int&, const Name::Enum&) = delete; \
     inline int operator|(const Name::Enum&, const int&) = delete; \
     inline int operator^(const int&, const Name::Enum&) = delete; \
     inline int operator^(const Name::Enum&, const int&) = delete; \
     inline int operator<<(const int&, const Name::Enum&) = delete; \
     inline int operator<<(const Name::Enum&, const int&) = delete; \
     inline int operator>>(const int&, const Name::Enum&) = delete; \
     inline int operator>>(const Name::Enum&, const int&) = delete; \
     inline int& operator+=(int&, const Name::Enum&) = delete; \
     inline int& operator-=(int&, const Name::Enum&) = delete; \
     inline int& operator*=(int&, const Name::Enum&) = delete; \
     inline int& operator/=(int&, const Name::Enum&) = delete; \
     inline int& operator%=(int&, const Name::Enum&) = delete; \
     inline int& operator&=(int&, const Name::Enum&) = delete; \
     inline int& operator|=(int&, const Name::Enum&) = delete; \
     inline int& operator^=(int&, const Name::Enum&) = delete; \
     inline int& operator<<=(int&, const Name::Enum&) = delete; \
     inline int& operator>>=(int&, const Name::Enum&) = delete;

  




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

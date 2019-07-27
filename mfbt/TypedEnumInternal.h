










#ifndef mozilla_TypedEnumInternal_h
#define mozilla_TypedEnumInternal_h

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

namespace mozilla {

























template<typename E>
class CastableTypedEnumResult
{
private:
  const E mValue;

public:
  explicit MOZ_CONSTEXPR CastableTypedEnumResult(E aValue)
    : mValue(aValue)
  {}

  MOZ_CONSTEXPR operator E() const { return mValue; }

  template<typename DestinationType>
  MOZ_EXPLICIT_CONVERSION MOZ_CONSTEXPR
  operator DestinationType() const { return DestinationType(mValue); }

  MOZ_CONSTEXPR bool operator !() const { return !bool(mValue); }

#ifndef MOZ_HAVE_CXX11_STRONG_ENUMS
  
  
  
  
  
  
  
  
  MOZ_CONSTEXPR E get() const { return mValue; }
#endif
};

} 

#endif 

#endif 






#ifndef nsLiteralString_h___
#define nsLiteralString_h___

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsDependentString_h___
#include "nsDependentString.h"
#endif


#if 0
inline
const nsDependentString
literal_string( const nsAString::char_type* aPtr )
  {
    return nsDependentString(aPtr);
  }

inline
const nsDependentString
literal_string( const nsAString::char_type* aPtr, uint32_t aLength )
  {
    return nsDependentString(aPtr, aLength);
  }

inline
const nsDependentCString
literal_string( const nsACString::char_type* aPtr )
  {
    return nsDependentCString(aPtr);
  }

inline
const nsDependentCString
literal_string( const nsACString::char_type* aPtr, uint32_t aLength )
  {
    return nsDependentCString(aPtr, aLength);
  }
#endif

namespace mozilla {
namespace internal {




template<int n>
inline uint32_t LiteralStringLength(const char (&c)[n])
{
  return n - 1;
}

#if defined(HAVE_CPP_CHAR16_T)
template<int n>
inline uint32_t LiteralWStringLength(const char16_t (&c)[n])
{
  return n - 1;
}
#elif defined(HAVE_CPP_2BYTE_WCHAR_T)
template<int n>
inline uint32_t LiteralWStringLength(const wchar_t (&c)[n])
{
  return n - 1;
}
#endif

} 
} 

#if defined(HAVE_CPP_CHAR16_T) || defined(HAVE_CPP_2BYTE_WCHAR_T)
#if defined(HAVE_CPP_CHAR16_T)
  
  #define NS_LL(s)                                u##s
#else
  
  #define NS_LL(s)                                L##s
#endif
  #define NS_MULTILINE_LITERAL_STRING(s)          nsDependentString(reinterpret_cast<const nsAString::char_type*>(s), mozilla::internal::LiteralWStringLength(s))
  #define NS_MULTILINE_LITERAL_STRING_INIT(n,s)   n(reinterpret_cast<const nsAString::char_type*>(s), mozilla::internal::LiteralWStringLength(s))
  #define NS_NAMED_MULTILINE_LITERAL_STRING(n,s)  const nsDependentString n(reinterpret_cast<const nsAString::char_type*>(s), mozilla::internal::LiteralWStringLength(s))
  typedef nsDependentString nsLiteralString;
#else
  #define NS_LL(s)                                s
  #define NS_MULTILINE_LITERAL_STRING(s)          NS_ConvertASCIItoUTF16(s, mozilla::internal::LiteralStringLength(s))
  #define NS_MULTILINE_LITERAL_STRING_INIT(n,s)   n(s, mozilla::internal::LiteralStringLength(s))
  #define NS_NAMED_MULTILINE_LITERAL_STRING(n,s)  const NS_ConvertASCIItoUTF16 n(s, mozilla::internal::LiteralStringLength(s))
  typedef NS_ConvertASCIItoUTF16 nsLiteralString;
#endif










#define NS_L(s)                                   NS_LL(s)

#define NS_LITERAL_STRING(s)                      static_cast<const nsAFlatString&>(NS_MULTILINE_LITERAL_STRING(NS_LL(s)))
#define NS_LITERAL_STRING_INIT(n,s)               NS_MULTILINE_LITERAL_STRING_INIT(n, NS_LL(s))
#define NS_NAMED_LITERAL_STRING(n,s)              NS_NAMED_MULTILINE_LITERAL_STRING(n, NS_LL(s))

#define NS_LITERAL_CSTRING(s)                     static_cast<const nsDependentCString&>(nsDependentCString(s, mozilla::internal::LiteralStringLength(s)))
#define NS_LITERAL_CSTRING_INIT(n,s)              n(s, mozilla::internal::LiteralStringLength(s))
#define NS_NAMED_LITERAL_CSTRING(n,s)             const nsDependentCString n(s, mozilla::internal::LiteralStringLength(s))

typedef nsDependentCString nsLiteralCString;

#endif 






#ifndef nsCRT_h___
#define nsCRT_h___

#include <stdlib.h>
#include <ctype.h>
#include "plstr.h"
#include "nscore.h"
#include "nsCRTGlue.h"

#if defined(XP_WIN)
#  define NS_LINEBREAK           "\015\012"
#  define NS_LINEBREAK_LEN       2
#else
#  ifdef XP_UNIX
#    define NS_LINEBREAK         "\012"
#    define NS_LINEBREAK_LEN     1
#  endif 
#endif 

extern const char16_t kIsoLatin1ToUCS2[256];



#define NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW   \
  void* operator new(size_t sz) CPP_THROW_NEW { \
    void* rv = ::operator new(sz);              \
    if (rv) {                                   \
      memset(rv, 0, sz);                        \
    }                                           \
    return rv;                                  \
  }                                             \
  void operator delete(void* ptr) {             \
    ::operator delete(ptr);                     \
  }



#define NS_DECL_ZEROING_OPERATOR_NEW           \
  void* operator new(size_t sz) CPP_THROW_NEW; \
  void operator delete(void* ptr);

#define NS_IMPL_ZEROING_OPERATOR_NEW(_class)            \
  void* _class::operator new(size_t sz) CPP_THROW_NEW { \
    void* rv = ::operator new(sz);                      \
    if (rv) {                                           \
      memset(rv, 0, sz);                                \
    }                                                   \
    return rv;                                          \
  }                                                     \
  void _class::operator delete(void* ptr) {             \
    ::operator delete(ptr);                             \
  }


#define CRTFREEIF(x) if (x) { nsCRT::free(x); x = 0; }



class nsCRT
{
public:
  enum
  {
    LF = '\n'   ,
    VTAB = '\v' ,
    CR = '\r'   
  };

  
  static int32_t strcmp(const char* aStr1, const char* aStr2)
  {
    return int32_t(PL_strcmp(aStr1, aStr2));
  }

  static int32_t strncmp(const char* aStr1, const char* aStr2,
                         uint32_t aMaxLen)
  {
    return int32_t(PL_strncmp(aStr1, aStr2, aMaxLen));
  }

  
  static int32_t strcasecmp(const char* aStr1, const char* aStr2)
  {
    return int32_t(PL_strcasecmp(aStr1, aStr2));
  }

  
  static int32_t strncasecmp(const char* aStr1, const char* aStr2,
                             uint32_t aMaxLen)
  {
    int32_t result = int32_t(PL_strncasecmp(aStr1, aStr2, aMaxLen));
    
    
    if (result < 0) {
      result = -1;
    }
    return result;
  }

  static int32_t strncmp(const char* aStr1, const char* aStr2, int32_t aMaxLen)
  {
    
    int32_t diff =
      ((const unsigned char*)aStr1)[0] - ((const unsigned char*)aStr2)[0];
    if (diff != 0) {
      return diff;
    }
    return int32_t(PL_strncmp(aStr1, aStr2, unsigned(aMaxLen)));
  }

  


















  static char* strtok(char* aStr, const char* aDelims, char** aNewStr);

  
  static int32_t strcmp(const char16_t* aStr1, const char16_t* aStr2);
  
  static int32_t strncmp(const char16_t* aStr1, const char16_t* aStr2,
                         uint32_t aMaxLen);

  
  
  
  static const char* memmem(const char* aHaystack, uint32_t aHaystackLen,
                            const char* aNeedle, uint32_t aNeedleLen);

  
  static int64_t atoll(const char* aStr);

  static char ToUpper(char aChar) { return NS_ToUpper(aChar); }
  static char ToLower(char aChar) { return NS_ToLower(aChar); }

  static bool IsUpper(char aChar) { return NS_IsUpper(aChar); }
  static bool IsLower(char aChar) { return NS_IsLower(aChar); }

  static bool IsAscii(char16_t aChar) { return NS_IsAscii(aChar); }
  static bool IsAscii(const char16_t* aString) { return NS_IsAscii(aString); }
  static bool IsAsciiAlpha(char16_t aChar) { return NS_IsAsciiAlpha(aChar); }
  static bool IsAsciiDigit(char16_t aChar) { return NS_IsAsciiDigit(aChar); }
  static bool IsAsciiSpace(char16_t aChar) { return NS_IsAsciiWhitespace(aChar); }
  static bool IsAscii(const char* aString) { return NS_IsAscii(aString); }
  static bool IsAscii(const char* aString, uint32_t aLength)
  {
    return NS_IsAscii(aString, aLength);
  }
};


inline bool
NS_IS_SPACE(char16_t aChar)
{
  return ((int(aChar) & 0x7f) == int(aChar)) && isspace(int(aChar));
}

#define NS_IS_CNTRL(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : iscntrl(i))
#define NS_IS_DIGIT(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : isdigit(i))
#if defined(XP_WIN)
#define NS_IS_ALPHA(VAL) (isascii((int)(VAL)) && isalpha((int)(VAL)))
#else
#define NS_IS_ALPHA(VAL) ((((unsigned int) (VAL)) > 0x7f) ? (int) 0 : isalpha((int)(VAL)))
#endif


#endif

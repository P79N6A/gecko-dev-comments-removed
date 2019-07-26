



#ifndef nsCRT_h___
#define nsCRT_h___

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "plstr.h"
#include "nscore.h"
#include "prtypes.h"
#include "nsCppSharedAllocator.h"
#include "nsCRTGlue.h"

#if defined(XP_WIN) || defined(XP_OS2)
#  define NS_LINEBREAK           "\015\012"
#  define NS_LINEBREAK_LEN       2
#else
#  ifdef XP_UNIX
#    define NS_LINEBREAK         "\012"
#    define NS_LINEBREAK_LEN     1
#  endif 
#endif 

extern const PRUnichar kIsoLatin1ToUCS2[256];



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



class nsCRT {
public:
  enum {
    LF='\n'   ,
    VTAB='\v' ,
    CR='\r'   
  };

  
  static int32_t strcmp(const char* s1, const char* s2) {
    return int32_t(PL_strcmp(s1, s2));
  }

  static int32_t strncmp(const char* s1, const char* s2,
                         uint32_t aMaxLen) {
    return int32_t(PL_strncmp(s1, s2, aMaxLen));
  }

  
  static int32_t strcasecmp(const char* s1, const char* s2) {
    return int32_t(PL_strcasecmp(s1, s2));
  }

  
  static int32_t strncasecmp(const char* s1, const char* s2, uint32_t aMaxLen) {
    int32_t result=int32_t(PL_strncasecmp(s1, s2, aMaxLen));
    
    
    if (result<0) 
      result=-1;
    return result;
  }

  static int32_t strncmp(const char* s1, const char* s2, int32_t aMaxLen) {
    
    int32_t diff = ((const unsigned char*)s1)[0] - ((const unsigned char*)s2)[0];
    if (diff != 0) return diff;
    return int32_t(PL_strncmp(s1,s2,unsigned(aMaxLen)));
  }
  
  static char* strdup(const char* str) {
    return PL_strdup(str);
  }

  static char* strndup(const char* str, uint32_t len) {
    return PL_strndup(str, len);
  }

  static void free(char* str) {
    PL_strfree(str);
  }

  


















  static char* strtok(char* str, const char* delims, char* *newStr); 

  
  static int32_t strcmp(const PRUnichar* s1, const PRUnichar* s2);
  
  static int32_t strncmp(const PRUnichar* s1, const PRUnichar* s2,
                         uint32_t aMaxLen);

  
  
  
  static const char* memmem(const char* haystack, uint32_t haystackLen,
                            const char* needle, uint32_t needleLen);

  
  
  static PRUnichar* strdup(const PRUnichar* str);

  
  
  static PRUnichar* strndup(const PRUnichar* str, uint32_t len);

  static void free(PRUnichar* str) {
  	nsCppSharedAllocator<PRUnichar> shared_allocator;
  	shared_allocator.deallocate(str, 0 );
  }

  
  static int64_t atoll(const char *str);
  
  static char ToUpper(char aChar) { return NS_ToUpper(aChar); }
  static char ToLower(char aChar) { return NS_ToLower(aChar); }
  
  static bool IsUpper(char aChar) { return NS_IsUpper(aChar); }
  static bool IsLower(char aChar) { return NS_IsLower(aChar); }

  static bool IsAscii(PRUnichar aChar) { return NS_IsAscii(aChar); }
  static bool IsAscii(const PRUnichar* aString) { return NS_IsAscii(aString); }
  static bool IsAsciiAlpha(PRUnichar aChar) { return NS_IsAsciiAlpha(aChar); }
  static bool IsAsciiDigit(PRUnichar aChar) { return NS_IsAsciiDigit(aChar); }
  static bool IsAsciiSpace(PRUnichar aChar) { return NS_IsAsciiWhitespace(aChar); }
  static bool IsAscii(const char* aString) { return NS_IsAscii(aString); }
  static bool IsAscii(const char* aString, uint32_t aLength) { return NS_IsAscii(aString, aLength); }
};


inline bool
NS_IS_SPACE(PRUnichar c)
{
  return ((int(c) & 0x7f) == int(c)) && isspace(int(c));
}

#define NS_IS_CNTRL(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : iscntrl(i))
#define NS_IS_DIGIT(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : isdigit(i))
#if defined(XP_WIN) || defined(XP_OS2)
#define NS_IS_ALPHA(VAL) (isascii((int)(VAL)) && isalpha((int)(VAL)))
#else
#define NS_IS_ALPHA(VAL) ((((unsigned int) (VAL)) > 0x7f) ? (int) 0 : isalpha((int)(VAL)))
#endif


#endif

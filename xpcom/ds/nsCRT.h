



































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
#  if defined(XP_UNIX) || defined(XP_BEOS)
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



class NS_COM nsCRT {
public:
  enum {
    LF='\n'   ,
    VTAB='\v' ,
    CR='\r'   
  };

  


















  



  static PRUint32 strlen(const char* s) {                                       
    return PRUint32(::strlen(s));                                               
  }                                                                             

  
  static PRInt32 strcmp(const char* s1, const char* s2) {
    return PRInt32(PL_strcmp(s1, s2));
  }

  static PRInt32 strncmp(const char* s1, const char* s2,
                         PRUint32 aMaxLen) {
    return PRInt32(PL_strncmp(s1, s2, aMaxLen));
  }

  
  static PRInt32 strcasecmp(const char* s1, const char* s2) {
    return PRInt32(PL_strcasecmp(s1, s2));
  }

  
  static PRInt32 strncasecmp(const char* s1, const char* s2, PRUint32 aMaxLen) {
    PRInt32 result=PRInt32(PL_strncasecmp(s1, s2, aMaxLen));
    
    
    if (result<0) 
      result=-1;
    return result;
  }

  static PRInt32 strncmp(const char* s1, const char* s2, PRInt32 aMaxLen) {
    
    PRInt32 diff = ((const unsigned char*)s1)[0] - ((const unsigned char*)s2)[0];
    if (diff != 0) return diff;
    return PRInt32(PL_strncmp(s1,s2,unsigned(aMaxLen)));
  }
  
  static char* strdup(const char* str) {
    return PL_strdup(str);
  }

  static char* strndup(const char* str, PRUint32 len) {
    return PL_strndup(str, len);
  }

  static void free(char* str) {
    PL_strfree(str);
  }

  


















  static char* strtok(char* str, const char* delims, char* *newStr); 

  static PRUint32 strlen(const PRUnichar* s) {
    
    if (!s) {
      NS_ERROR("Passing null to nsCRT::strlen");
      return 0;
    }
    return NS_strlen(s);
  }

  
  static PRInt32 strcmp(const PRUnichar* s1, const PRUnichar* s2);
  
  static PRInt32 strncmp(const PRUnichar* s1, const PRUnichar* s2,
                         PRUint32 aMaxLen);

  
  
  static PRUnichar* strdup(const PRUnichar* str);

  
  
  static PRUnichar* strndup(const PRUnichar* str, PRUint32 len);

  static void free(PRUnichar* str) {
  	nsCppSharedAllocator<PRUnichar> shared_allocator;
  	shared_allocator.deallocate(str, 0 );
  }

  
  
  static PRUint32 HashCode(const char* str,
                           PRUint32* resultingStrLen = nsnull);

  
  
  static PRUint32 HashCode(const PRUnichar* str,
                           PRUint32* resultingStrLen = nsnull);

  
  
  
  static PRUint32 HashCodeAsUTF8(const PRUnichar* str,
                                 PRUint32* resultingStrLen = nsnull);

  
  static PRUint32 BufferHashCode(const PRUnichar* str, PRUint32 strLen);

  
  static PRInt64 atoll(const char *str);
  
  static char ToUpper(char aChar) { return NS_ToUpper(aChar); }
  static char ToLower(char aChar) { return NS_ToLower(aChar); }
  
  static PRBool IsUpper(char aChar) { return NS_IsUpper(aChar); }
  static PRBool IsLower(char aChar) { return NS_IsLower(aChar); }

  static PRBool IsAscii(PRUnichar aChar) { return NS_IsAscii(aChar); }
  static PRBool IsAscii(const PRUnichar* aString) { return NS_IsAscii(aString); }
  static PRBool IsAsciiAlpha(PRUnichar aChar) { return NS_IsAsciiAlpha(aChar); }
  static PRBool IsAsciiDigit(PRUnichar aChar) { return NS_IsAsciiDigit(aChar); }
  static PRBool IsAsciiSpace(PRUnichar aChar) { return NS_IsAsciiWhitespace(aChar); }
  static PRBool IsAscii(const char* aString) { return NS_IsAscii(aString); }
  static PRBool IsAscii(const char* aString, PRUint32 aLength) { return NS_IsAscii(aString, aLength); }
};

#define FF '\f'
#define TAB '\t'

#define CRSTR "\015"
#define LFSTR "\012"
#define CRLF "\015\012"     /* A CR LF equivalent string */


#if defined(XP_MACOSX)
  #define FILE_PATH_SEPARATOR       "/"
  #define FILE_ILLEGAL_CHARACTERS   ":"
#elif defined(XP_WIN) || defined(XP_OS2)
  #define FILE_PATH_SEPARATOR       "\\"
  #define FILE_ILLEGAL_CHARACTERS   "/:*?\"<>|"
#elif defined(XP_UNIX) || defined(XP_BEOS)
  #define FILE_PATH_SEPARATOR       "/"
  #define FILE_ILLEGAL_CHARACTERS   ""
#else
  #error need_to_define_your_file_path_separator_and_illegal_characters
#endif

#define NS_IS_SPACE(VAL) \
  (((((intn)(VAL)) & 0x7f) == ((intn)(VAL))) && isspace((intn)(VAL)) )

#define NS_IS_CNTRL(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : iscntrl(i))
#define NS_IS_DIGIT(i)   ((((unsigned int) (i)) > 0x7f) ? (int) 0 : isdigit(i))
#if defined(XP_WIN) || defined(XP_OS2)
#define NS_IS_ALPHA(VAL) (isascii((int)(VAL)) && isalpha((int)(VAL)))
#else
#define NS_IS_ALPHA(VAL) ((((unsigned int) (VAL)) > 0x7f) ? (int) 0 : isalpha((int)(VAL)))
#endif


#endif

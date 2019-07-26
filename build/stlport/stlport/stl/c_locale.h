

















#ifndef _STLP_C_LOCALE_H
#define _STLP_C_LOCALE_H







#if defined (__sgi)
#  if defined (ROOT_65) 
#    include <sgidefs.h>
#    include <standards.h>
#    include <wchar.h>
#    include <ctype.h>
#  else 
#    include <sgidefs.h>
#    include <standards.h>
#    if !defined(_SIZE_T) && !defined(_SIZE_T_)
#      define _SIZE_T
#      if (_MIPS_SZLONG == 32)
typedef unsigned int size_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef unsigned long size_t;
#      endif
#    endif
#    if !defined (_WCHAR_T)
#      define _WCHAR_T
#      if (_MIPS_SZLONG == 32)
typedef long wchar_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef __int32_t wchar_t;
#      endif
#    endif 
#    if !defined (_WINT_T)
#      define _WINT_T
#      if (_MIPS_SZLONG == 32)
typedef long wint_t;
#      endif
#      if (_MIPS_SZLONG == 64)
typedef __int32_t wint_t;
#      endif
#    endif 
#    if !defined (_MBSTATE_T)
#      define _MBSTATE_T



#      if defined (_MSC_VER)
typedef int mbstate_t;
#      else
typedef char mbstate_t;
#      endif
#    endif 
#  endif 
#elif defined (_STLP_USE_GLIBC)
#  include <ctype.h>
#endif









struct _Locale_ctype;
struct _Locale_codecvt;
struct _Locale_numeric;
struct _Locale_time;
struct _Locale_collate;
struct _Locale_monetary;
struct _Locale_messages;









#if defined (_STLP_USE_GLIBC)

#  define _Locale_CNTRL  _IScntrl
#  define _Locale_UPPER  _ISupper
#  define _Locale_LOWER  _ISlower
#  define _Locale_DIGIT  _ISdigit
#  define _Locale_XDIGIT _ISxdigit
#  define _Locale_PUNCT  _ISpunct
#  define _Locale_SPACE  _ISspace
#  define _Locale_PRINT  _ISprint
#  define _Locale_ALPHA  _ISalpha
#else




#  define _Locale_SPACE  0x0001
#  define _Locale_PRINT  0x0002
#  define _Locale_CNTRL  0x0004
#  define _Locale_UPPER  0x0008
#  define _Locale_LOWER  0x0010
#  define _Locale_ALPHA  0x0020
#  define _Locale_DIGIT  0x0040
#  define _Locale_PUNCT  0x0080
#  define _Locale_XDIGIT 0x0100
#endif

#endif 

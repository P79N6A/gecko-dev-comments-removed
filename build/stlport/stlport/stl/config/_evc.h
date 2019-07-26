






#ifndef _STLP_EVC_H
#define _STLP_EVC_H

#define _STLP_COMPILER "eMbedded Visual C++"




#ifdef _STLP_WINCE
#  undef _STLP_WINCE
#endif






#undef _STLP_WCE_EVC3
#undef _STLP_WCE_NET

#if (_WIN32_WCE > 300)
#  define _STLP_WCE_NET UNDER_CE
#elif (_WIN32_WCE == 300)
#  define _STLP_WCE_EVC3 UNDER_CE
#else
#  error No support for Windows CE below 3.0!
#endif


#define _STLP_WCE



#define _STLP_LITTLE_ENDIAN


#if defined (DEBUG) && !defined (_DEBUG)
#  define _DEBUG
#endif


#include <stl/config/_msvc.h>


#define _STLP_NO_LOCALE_SUPPORT

#if _WIN32_WCE >= 420
   
#  define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_STD
#  define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_STD
   
#  define _STLP_GLOBAL_NEW_HANDLER 1
#endif


#ifndef _MT
#  define _MT
#endif


#undef _STLP_USING_CROSS_NATIVE_RUNTIME_LIB

#if _WIN32_WCE < 400

#  define _STLP_NO_LONG_DOUBLE
#endif


#define _STLP_NO_VENDOR_MATH_F
#define _STLP_NO_VENDOR_MATH_L



















#if defined (UNICODE)
#  define _STLP_USE_WIDE_INTERFACE
#endif


#if defined (__cplusplus) && !defined (_STLP_HAS_NO_NAMESPACES)
#  ifdef _STLP_VENDOR_EXCEPT_STD
#    undef _STLP_VENDOR_EXCEPT_STD
#  endif
#  define _STLP_VENDOR_EXCEPT_STD std
#endif


#if _MSC_VER < 1400 && (defined (ARM) || defined (_ARM_))
#  define _STLP_DONT_USE_SHORT_STRING_OPTIM
#endif


#if !defined (__BUILDING_STLPORT) && defined (_MFC_VER)
#  define __PLACEMENT_NEW_INLINE
#endif


#undef _REENTRANT
#define _REENTRANT
#undef _NOTHREADS


#undef _STLP_NO_NEW_C_HEADERS
#define _STLP_NO_NEW_C_HEADERS


#if defined (_STLP_WCE_EVC3) || !defined (_CPPUNWIND)
#  define _STLP_NO_EXCEPTION_HEADER
#  define _STLP_NO_EXCEPTIONS
#  undef _STLP_USE_EXCEPTIONS
#  ifndef _STLP_THROW_BAD_ALLOC
#    define _STLP_THROW_BAD_ALLOC { _STLP_WINCE_TRACE(L"out of memory"); ExitThread(1); }
#  endif
#endif

#define _STLP_WINCE_TRACE(msg) OutputDebugString(msg)




#if defined (_STLP_WCE_NET)


#  define _STLP_NO_LOCALE_SUPPORT
#  define _STLP_NO_TIME_SUPPORT


#  ifndef _PTRDIFF_T_DEFINED
   typedef int ptrdiff_t;
#    define _PTRDIFF_T_DEFINED
#  endif








#  if !defined (_STLP_NATIVE_INCLUDE_PATH)
#    if defined (_X86_)
#      if defined (_STLP_WCE_TARGET_PROC_SUBTYPE_EMULATOR)
#        define _STLP_NATIVE_INCLUDE_PATH ../Emulator
#      else
#        define _STLP_NATIVE_INCLUDE_PATH ../X86
#      endif
#    elif defined (_ARM_)
#      if _MSC_VER < 1400
         
#        if defined (ARMV4)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4
#        elif defined (ARMV4I)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4i
#        elif defined (ARMV4T)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4t
#        else
#          error Unknown ARM SDK.
#        endif
#      else
         
#        if defined (ARMV4)
           
#          if defined(WIN32_PLATFORM_PSPC)
#            define _STLP_NATIVE_INCLUDE_PATH ../Include
#          else
#            define _STLP_NATIVE_INCLUDE_PATH ../Armv4
#          endif
#        elif defined(ARMV4I) || defined(ARMV4T)
#          define _STLP_NATIVE_INCLUDE_PATH ../Armv4i
#        else
#          error Unknown ARM SDK.
#        endif
#      endif
#    elif defined (_MIPS_)
#      if defined (MIPS16)
#        define _STLP_NATIVE_INCLUDE_PATH ../mips16
#      elif defined (MIPSII)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsII
#      elif defined (MIPSII_FP)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsII_fp
#      elif defined (MIPSIV)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsIV
#      elif defined (MIPSIV_FP)
#        define _STLP_NATIVE_INCLUDE_PATH ../mipsIV_fp
#      else
#        error Unknown MIPS SDK.
#      endif
#    elif defined (SHx)
#      if defined (SH3)
#        define _STLP_NATIVE_INCLUDE_PATH ../sh3
#      elif defined (SH4)
#        define _STLP_NATIVE_INCLUDE_PATH ../sh4
#      else
#        error Unknown SHx SDK.
#      endif
#    else
#      error Unknown SDK.
#    endif
#  endif 






#  ifdef _STLP_USE_MFC
#    define __PLACEMENT_NEW_INLINE
#  endif

#endif 




#if defined (_STLP_WCE_EVC3)

#  define _STLP_NO_NATIVE_MBSTATE_T


#  define _STLP_NO_LOCALE_SUPPORT
#  define _STLP_NO_TIME_SUPPORT


#  define _STLP_NO_NEW_HEADER
#  define _STLP_NO_NEW_NEW_HEADER


#  undef _STLP_NO_BAD_ALLOC
#  define _STLP_NO_BAD_ALLOC

#  undef _STLP_NO_TYPEINFO
#  define _STLP_NO_TYPEINFO


#  ifndef _SIZE_T_DEFINED
   typedef unsigned int size_t;
#    define _SIZE_T_DEFINED
#  endif

#  ifndef _WCHAR_T_DEFINED
   typedef unsigned short wchar_t;
#    define _WCHAR_T_DEFINED
#  endif


#  ifndef _PTRDIFF_T_DEFINED
   typedef int ptrdiff_t;
#    define _PTRDIFF_T_DEFINED
#  endif


#  ifndef _CLOCK_T_DEFINED
   typedef long clock_t;
#    define _CLOCK_T_DEFINED
#  endif


#  ifndef _TM_DEFINED
struct tm {
   int tm_sec;     
   int tm_min;     
   int tm_hour;    
   int tm_mday;    
   int tm_mon;     
   int tm_year;    
   int tm_wday;    
   int tm_yday;    
   int tm_isdst;   
};
#    define _TM_DEFINED
#  endif




#  ifdef __cplusplus
#    ifndef __PLACEMENT_NEW_INLINE
#      ifndef _MFC_VER
inline void *__cdecl operator new(size_t, void *_P) { return (_P); }
#      endif 
inline void __cdecl operator delete(void *, void *) { return; }
#      define __PLACEMENT_NEW_INLINE
#    endif
#  endif 


#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS


#  ifndef _ASSERT_DEFINED
#    define assert(expr) _STLP_ASSERT(expr)
#    define _ASSERT_DEFINED
#  endif

#endif 







#ifndef _ABORT_DEFINED
#  define _STLP_ABORT() TerminateProcess(reinterpret_cast<HANDLE>(66), 0)
#  define _ABORT_DEFINED
#endif




#endif 

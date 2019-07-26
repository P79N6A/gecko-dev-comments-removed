



















#ifndef _STLP_INTERNAL_WINDOWS_H
#define _STLP_INTERNAL_WINDOWS_H

#if !defined (_STLP_PLATFORM)
#  define _STLP_PLATFORM "Windows"
#endif

#if !defined (_STLP_BIG_ENDIAN) && !defined (_STLP_LITTLE_ENDIAN)
#  if defined (_MIPSEB)
#    define _STLP_BIG_ENDIAN 1
#  endif
#  if defined (__i386) || defined (_M_IX86) || defined (_M_ARM) || \
      defined (__amd64__) || defined (_M_AMD64) || defined (__x86_64__) || \
      defined (__alpha__)
#    define _STLP_LITTLE_ENDIAN 1
#  endif
#  if defined (__ia64__)
    
#    if defined (__BIG_ENDIAN__)
#      define _STLP_BIG_ENDIAN 1
#    else
#      define _STLP_LITTLE_ENDIAN 1
#    endif
#  endif
#endif 

#if !defined (_STLP_WINDOWS_H_INCLUDED)
#  define _STLP_WINDOWS_H_INCLUDED
#  if defined (__BUILDING_STLPORT)
#    include <stl/config/_native_headers.h>



#    if !defined (_STLP_OUTERMOST_HEADER_ID)
#      define _STLP_OUTERMOST_HEADER_ID 0x100
#    endif
#    if !defined (WIN32_LEAN_AND_MEAN)
#      define WIN32_LEAN_AND_MEAN
#    endif
#    if !defined (VC_EXTRALEAN)
#      define VC_EXTRALEAN
#    endif

#    if !defined (NOMINMAX)
#      define NOMINMAX
#    endif
#    if !defined (STRICT)
#      define STRICT
#    endif
#    if defined (_STLP_USE_MFC)
#      include <afx.h>
#    else
#      include <windows.h>
#    endif
#    if (_STLP_OUTERMOST_HEADER_ID == 0x100)
#      undef _STLP_OUTERMOST_HEADER_ID
#    endif
#  else

#    if defined (__cplusplus)
extern "C" {
#    endif
#    if (defined (_M_AMD64) || defined (_M_IA64) || (!defined (_STLP_WCE) && defined (_M_MRX000)) || defined (_M_ALPHA) || \
        (defined (_M_PPC) && (_STLP_MSVC_LIB >= 1000))) && !defined (RC_INVOKED)
#      define InterlockedIncrement       _InterlockedIncrement
#      define InterlockedDecrement       _InterlockedDecrement
#      define InterlockedExchange        _InterlockedExchange
#      define _STLP_STDCALL
#    else
#      if defined (_MAC)
#        define _STLP_STDCALL _cdecl
#      else
#        define _STLP_STDCALL __stdcall
#      endif
#    endif

#    if defined (_STLP_NEW_PLATFORM_SDK)
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedIncrement(long volatile *);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedDecrement(long volatile *);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedExchange(long volatile *, long);
#      if defined (_WIN64)
_STLP_IMPORT_DECLSPEC void* _STLP_STDCALL _InterlockedExchangePointer(void* volatile *, void*);
#      endif
#    elif !defined (_STLP_WCE)



_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedIncrement(long*);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedDecrement(long*);
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL InterlockedExchange(long*, long);
#    else

#      include <stl/config/_native_headers.h>


#      if !defined (NOMINMAX)
#        define NOMINMAX
#      endif
#      include <windef.h> 

       

#      if (_WIN32_WCE >= 0x500)
#        define _STLP_NATIVE_SETJMP_H_INCLUDED
#      endif

#      ifndef _WINBASE_ 
long WINAPI InterlockedIncrement(long*);
long WINAPI InterlockedDecrement(long*);
long WINAPI InterlockedExchange(long*, long);
#      endif

#      ifndef __WINDOWS__ 

#        if defined (x86)
#          include <winbase.h> 
#        endif

#        ifndef _MFC_VER

#          define MessageBox MessageBoxW
int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

#          define wvsprintf wvsprintfW
int WINAPI wvsprintfW(LPWSTR, LPCWSTR, va_list ArgList);

void WINAPI ExitThread(DWORD dwExitCode);

#          if !defined (COREDLL)
#            define _STLP_WCE_WINBASEAPI DECLSPEC_IMPORT
#          else
#            define _STLP_WCE_WINBASEAPI
#          endif

_STLP_WCE_WINBASEAPI int WINAPI
MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                    int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

_STLP_WCE_WINBASEAPI UINT WINAPI GetACP();

_STLP_WCE_WINBASEAPI BOOL WINAPI TerminateProcess(HANDLE hProcess, DWORD uExitCode);

#          define OutputDebugString OutputDebugStringW
void WINAPI OutputDebugStringW(LPCWSTR);

_STLP_WCE_WINBASEAPI void WINAPI Sleep(DWORD);

#          undef _STLP_WCE_WINBASEAPI

#        endif 

#      endif 


#    endif

#    if !defined (_STLP_WCE)
_STLP_IMPORT_DECLSPEC void _STLP_STDCALL Sleep(unsigned long);
_STLP_IMPORT_DECLSPEC void _STLP_STDCALL OutputDebugStringA(const char* lpOutputString);
#    endif

#    if defined (InterlockedIncrement)
#      pragma intrinsic(_InterlockedIncrement)
#      pragma intrinsic(_InterlockedDecrement)
#      pragma intrinsic(_InterlockedExchange)
#      if defined (_WIN64)
#        pragma intrinsic(_InterlockedExchangePointer)
#      endif
#    endif
#    if defined (__cplusplus)
} 
#    endif

#  endif



#  if !defined (_WIN64)

#    if defined (__cplusplus)




inline
void* _STLP_CALL STLPInterlockedExchangePointer(void* volatile* __a, void* __b) {
#      if defined (_STLP_MSVC)




#        pragma warning (push)
#        pragma warning (disable : 4311) // pointer truncation from void* to long
#        pragma warning (disable : 4312) // conversion from long to void* of greater size
#      endif
#      if !defined (_STLP_NO_NEW_STYLE_CASTS)
  return reinterpret_cast<void*>(InterlockedExchange(reinterpret_cast<long*>(const_cast<void**>(__a)),
                                                     reinterpret_cast<long>(__b)));
#      else
  return (void*)InterlockedExchange((long*)__a, (long)__b);
#      endif
#      if defined (_STLP_MSVC)
#        pragma warning (pop)
#      endif
}
#    endif
#  else
#    define STLPInterlockedExchangePointer _InterlockedExchangePointer
#  endif

#endif





#if (defined (WINVER) && (WINVER < 0x0410) && (!defined (_WIN32_WINNT) || (_WIN32_WINNT < 0x400))) || \
    (!defined (WINVER) && (defined (_WIN32_WINDOWS) && (_WIN32_WINDOWS < 0x0410) || \
                          (defined (_WIN32_WINNT) && (_WIN32_WINNT < 0x400))))
#  define _STLP_WIN95_LIKE
#endif







#if (defined (_DEBUG) || defined (_STLP_DEBUG)) && \
    (defined (_STLP_MSVC) && (_STLP_MSVC < 1310) || \
     defined (__GNUC__) && (__GNUC__ < 3))





#  if defined (__BUILDING_STLPORT)
#    if defined (_STLP_WIN95_LIKE)
#      define _STLP_SIGNAL_RUNTIME_COMPATIBILITY building_for_windows95_but_library_built_for_at_least_windows98
#    else
#      define _STLP_SIGNAL_RUNTIME_COMPATIBILITY building_for_at_least_windows98_but_library_built_for_windows95
#    endif
#  else
#    if defined (_STLP_WIN95_LIKE)
#      define _STLP_CHECK_RUNTIME_COMPATIBILITY building_for_windows95_but_library_built_for_at_least_windows98
#    else
#      define _STLP_CHECK_RUNTIME_COMPATIBILITY building_for_at_least_windows98_but_library_built_for_windows95
#    endif
#  endif
#endif

#if defined (__WIN16) || defined (WIN16) || defined (_WIN16)
#  define _STLP_WIN16
#else
#  define _STLP_WIN32
#endif

#if defined(_STLP_WIN32)
#  define _STLP_USE_WIN32_IO
#endif

#if defined(__MINGW32__) && !defined(_STLP_USE_STDIO_IO)
#  define _STLP_USE_WIN32_IO
#endif 

#ifdef _STLP_WIN16
#  define _STLP_USE_UNIX_EMULATION_IO
#  define _STLP_LDOUBLE_80
#endif

#endif

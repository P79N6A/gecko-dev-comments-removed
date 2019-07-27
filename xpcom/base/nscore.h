





#ifndef nscore_h___
#define nscore_h___





#ifndef _XPCOM_CONFIG_H_
#include "xpcom-config.h"
#endif


#if !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  include "mozilla/mozalloc.h"
#endif




#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#  include "mozilla/NullPtr.h"
#endif

#include "mozilla/RefCountType.h"






#ifdef HAVE_VISIBILITY_HIDDEN_ATTRIBUTE
#define NS_VISIBILITY_HIDDEN   __attribute__ ((visibility ("hidden")))
#else
#define NS_VISIBILITY_HIDDEN
#endif

#if defined(HAVE_VISIBILITY_ATTRIBUTE)
#define NS_VISIBILITY_DEFAULT __attribute__ ((visibility ("default")))
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define NS_VISIBILITY_DEFAULT __global
#else
#define NS_VISIBILITY_DEFAULT
#endif

#define NS_HIDDEN_(type)   NS_VISIBILITY_HIDDEN type
#define NS_EXTERNAL_VIS_(type) NS_VISIBILITY_DEFAULT type

#define NS_HIDDEN           NS_VISIBILITY_HIDDEN
#define NS_EXTERNAL_VIS     NS_VISIBILITY_DEFAULT
























#if defined(__i386__) && defined(__GNUC__)
#define NS_FASTCALL __attribute__ ((regparm (3), stdcall))
#define NS_CONSTRUCTOR_FASTCALL __attribute__ ((regparm (3), stdcall))
#elif defined(XP_WIN) && !defined(_WIN64)
#define NS_FASTCALL __fastcall
#define NS_CONSTRUCTOR_FASTCALL
#else
#define NS_FASTCALL
#define NS_CONSTRUCTOR_FASTCALL
#endif

#ifdef XP_WIN

#define NS_IMPORT __declspec(dllimport)
#define NS_IMPORT_(type) __declspec(dllimport) type __stdcall
#define NS_EXPORT __declspec(dllexport)
#define NS_EXPORT_(type) __declspec(dllexport) type __stdcall
#define NS_IMETHOD_(type) virtual type __stdcall
#define NS_IMETHODIMP_(type) type __stdcall
#define NS_METHOD_(type) type __stdcall
#define NS_CALLBACK_(_type, _name) _type (__stdcall * _name)
#ifndef _WIN64

#define NS_STDCALL __stdcall
#define NS_HAVE_STDCALL
#else
#define NS_STDCALL
#endif
#define NS_FROZENCALL __cdecl






#define NS_EXPORT_STATIC_MEMBER_(type) type
#define NS_IMPORT_STATIC_MEMBER_(type) type

#else

#define NS_IMPORT NS_EXTERNAL_VIS
#define NS_IMPORT_(type) NS_EXTERNAL_VIS_(type)
#define NS_EXPORT NS_EXTERNAL_VIS
#define NS_EXPORT_(type) NS_EXTERNAL_VIS_(type)
#define NS_IMETHOD_(type) virtual type
#define NS_IMETHODIMP_(type) type
#define NS_METHOD_(type) type
#define NS_CALLBACK_(_type, _name) _type (* _name)
#define NS_STDCALL
#define NS_FROZENCALL
#define NS_EXPORT_STATIC_MEMBER_(type) NS_EXTERNAL_VIS_(type)
#define NS_IMPORT_STATIC_MEMBER_(type) NS_EXTERNAL_VIS_(type)

#endif




















#ifdef __GNUC__
#define NS_STDCALL_FUNCPROTO(ret, name, class, func, args) \
  typeof(&class::func) name
#else
#define NS_STDCALL_FUNCPROTO(ret, name, class, func, args) \
  ret (NS_STDCALL class::*name) args
#endif




#ifdef __GNUC__
# define MOZ_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
# define MOZ_DEPRECATED __declspec(deprecated)
#else
# define MOZ_DEPRECATED
#endif




#ifdef __GNUC__
#define MOZ_FORMAT_PRINTF(stringIndex, firstToCheck)  \
    __attribute__ ((format (printf, stringIndex, firstToCheck)))
#else
#define MOZ_FORMAT_PRINTF(stringIndex, firstToCheck)
#endif




#define NS_IMETHOD          NS_IMETHOD_(nsresult)
#define NS_IMETHODIMP       NS_IMETHODIMP_(nsresult)
#define NS_METHOD           NS_METHOD_(nsresult)
#define NS_CALLBACK(_name)  NS_CALLBACK_(nsresult, _name)





#ifdef __cplusplus
#define NS_EXTERN_C extern "C"
#else
#define NS_EXTERN_C
#endif

#define EXPORT_XPCOM_API(type) NS_EXTERN_C NS_EXPORT type NS_FROZENCALL
#define IMPORT_XPCOM_API(type) NS_EXTERN_C NS_IMPORT type NS_FROZENCALL
#define GLUE_XPCOM_API(type) NS_EXTERN_C NS_HIDDEN_(type) NS_FROZENCALL

#ifdef IMPL_LIBXUL
#define XPCOM_API(type) EXPORT_XPCOM_API(type)
#elif defined(XPCOM_GLUE)
#define XPCOM_API(type) GLUE_XPCOM_API(type)
#else
#define XPCOM_API(type) IMPORT_XPCOM_API(type)
#endif

#ifdef MOZILLA_INTERNAL_API
   






#  define nsAString nsAString_internal
#  define nsACString nsACString_internal
#endif

#if (defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING))



#define NS_BUILD_REFCNT_LOGGING
#endif



#if defined(NO_BUILD_REFCNT_LOGGING)
#undef NS_BUILD_REFCNT_LOGGING
#endif





#if defined(NS_TRACE_MALLOC) || defined(NS_BUILD_REFCNT_LOGGING) || defined(MOZ_VALGRIND)
#define NS_FREE_PERMANENT_DATA
#endif








#ifdef NS_NO_VTABLE
#undef NS_NO_VTABLE
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define NS_NO_VTABLE __declspec(novtable)
#else
#define NS_NO_VTABLE
#endif





#include "nsError.h"

typedef MozRefCountType nsrefcnt;





#define NS_PTR_TO_INT32(x) ((int32_t)(intptr_t)(x))
#define NS_PTR_TO_UINT32(x) ((uint32_t)(intptr_t)(x))
#define NS_INT32_TO_PTR(x) ((void*)(intptr_t)(x))




#define NS_STRINGIFY_HELPER(x_) #x_
#define NS_STRINGIFY(x_) NS_STRINGIFY_HELPER(x_)






#if defined(XPCOM_GLUE) && !defined(XPCOM_GLUE_USE_NSPR)
#define XPCOM_GLUE_AVOID_NSPR
#endif

#if defined(HAVE_THREAD_TLS_KEYWORD)
#define NS_TLS __thread
#endif




#ifdef HAVE_SEH_EXCEPTIONS
#define MOZ_SEH_TRY           __try
#define MOZ_SEH_EXCEPT(expr)  __except(expr)
#else
#define MOZ_SEH_TRY           if(true)
#define MOZ_SEH_EXCEPT(expr)  else
#endif

#endif 

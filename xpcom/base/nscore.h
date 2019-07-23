



































#ifndef nscore_h___
#define nscore_h___





#ifndef _XPCOM_CONFIG_H_
#include "xpcom-config.h"
#endif


#if !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  if defined(__cplusplus)
#    include <new>              
#  endif
#  include <stdlib.h>         
#  include "mozilla/mozalloc.h"
#  include "mozilla/mozalloc_macro_wrappers.h"
#endif




#include "prtypes.h"






#ifdef _WIN32
#define NS_WIN32 1

#elif defined(__unix)
#define NS_UNIX 1

#elif defined(XP_OS2)
#define NS_OS2 1
#endif



































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

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY  NS_VISIBILITY_HIDDEN
























#if defined(__i386__) && defined(__GNUC__) && \
    (__GNUC__ >= 3) && !defined(XP_OS2)
#define NS_FASTCALL __attribute__ ((regparm (3), stdcall))
#define NS_CONSTRUCTOR_FASTCALL __attribute__ ((regparm (3), stdcall))
#elif defined(XP_WIN)
#define NS_FASTCALL __fastcall
#define NS_CONSTRUCTOR_FASTCALL
#else
#define NS_FASTCALL
#define NS_CONSTRUCTOR_FASTCALL
#endif





#if defined(__i386__) && defined(__GNUC__) && \
    (__GNUC__ >= 3) && !defined(XP_OS2)
#define NS_DEFCALL __attribute__ ((regparm (0), cdecl))
#else
#define NS_DEFCALL
#endif

#ifdef NS_WIN32

#define NS_IMPORT __declspec(dllimport)
#define NS_IMPORT_(type) __declspec(dllimport) type __stdcall
#define NS_EXPORT __declspec(dllexport)
#define NS_EXPORT_(type) __declspec(dllexport) type __stdcall
#define NS_IMETHOD_(type) virtual type __stdcall
#define NS_IMETHODIMP_(type) type __stdcall
#define NS_METHOD_(type) type __stdcall
#define NS_CALLBACK_(_type, _name) _type (__stdcall * _name)
#define NS_STDCALL __stdcall
#define NS_FROZENCALL __cdecl






#define NS_EXPORT_STATIC_MEMBER_(type) type
#define NS_IMPORT_STATIC_MEMBER_(type) type

#elif defined(XP_OS2) && defined(__declspec)

#define NS_IMPORT __declspec(dllimport)
#define NS_IMPORT_(type) type __declspec(dllimport)
#define NS_EXPORT __declspec(dllexport)
#define NS_EXPORT_(type) type __declspec(dllexport)
#define NS_IMETHOD_(type) virtual type
#define NS_IMETHODIMP_(type) type
#define NS_METHOD_(type) type
#define NS_CALLBACK_(_type, _name) _type (* _name)
#define NS_STDCALL
#define NS_FROZENCALL
#define NS_EXPORT_STATIC_MEMBER_(type) NS_EXTERNAL_VIS_(type)
#define NS_IMPORT_STATIC_MEMBER_(type) NS_EXTERNAL_VIS_(type)

#else

#define NS_IMPORT NS_EXTERNAL_VIS
#define NS_IMPORT_(type) NS_EXTERNAL_VIS_(type)
#define NS_EXPORT NS_EXTERNAL_VIS
#define NS_EXPORT_(type) NS_EXTERNAL_VIS_(type)
#define NS_IMETHOD_(type) virtual IMETHOD_VISIBILITY type NS_DEFCALL
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




#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
# define NS_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
# define NS_DEPRECATED __declspec(deprecated)
#else
# define NS_DEPRECATED
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

#ifdef _IMPL_NS_COM
#define XPCOM_API(type) EXPORT_XPCOM_API(type)
#elif defined(XPCOM_GLUE)
#define XPCOM_API(type) GLUE_XPCOM_API(type)
#else
#define XPCOM_API(type) IMPORT_XPCOM_API(type)
#endif

#ifdef MOZ_ENABLE_LIBXUL
#define NS_COM
#elif defined(_IMPL_NS_COM)
#define NS_COM NS_EXPORT
#elif defined(XPCOM_GLUE)
#define NS_COM
#else
#define NS_COM NS_IMPORT
#endif

#ifdef MOZILLA_INTERNAL_API
#  define NS_COM_GLUE NS_COM
   






#  define nsAString nsAString_internal
#  define nsACString nsACString_internal
#else
#  ifdef HAVE_VISIBILITY_ATTRIBUTE
#    define NS_COM_GLUE NS_VISIBILITY_HIDDEN
#  else
#    define NS_COM_GLUE
#  endif
#endif









#ifdef NS_NO_VTABLE
#undef NS_NO_VTABLE
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1100
#define NS_NO_VTABLE __declspec(novtable)
#else
#define NS_NO_VTABLE
#endif





typedef PRUint32 nsresult;









#if defined(XP_WIN) && PR_BYTES_PER_LONG == 4
typedef unsigned long nsrefcnt;
#else
typedef PRUint32 nsrefcnt;
#endif




#define nsnull 0

#include "nsError.h"




  







  
#if defined(_MSC_VER) && (_MSC_VER>=1100)
  #define HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX

  #define HAVE_CPP_EXPLICIT
  #define HAVE_CPP_TYPENAME
  #define HAVE_CPP_ACCESS_CHANGING_USING

  #define HAVE_CPP_NAMESPACE_STD
  #define HAVE_CPP_UNAMBIGUOUS_STD_NOTEQUAL
  #define HAVE_CPP_2BYTE_WCHAR_T
#endif

#ifndef __PRUNICHAR__
#define __PRUNICHAR__




  #if defined(HAVE_CPP_2BYTE_WCHAR_T) && defined(NS_WIN32)
    typedef wchar_t PRUnichar;
  #else
    typedef PRUint16 PRUnichar;
  #endif
#endif

  




#ifndef HAVE_CPP_EXPLICIT
  #define explicit
#endif

#ifndef HAVE_CPP_TYPENAME
  #define typename
#endif

#ifdef HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX
  #define NS_SPECIALIZE_TEMPLATE  template <>
#else
  #define NS_SPECIALIZE_TEMPLATE
#endif





#define NS_PTR_TO_INT32(x)  ((PRInt32)  (PRWord) (x))
#define NS_PTR_TO_UINT32(x) ((PRUint32) (PRWord) (x))
#define NS_INT32_TO_PTR(x)  ((void *)   (PRWord) (x))




#define NS_STRINGIFY_HELPER(x_) #x_
#define NS_STRINGIFY(x_) NS_STRINGIFY_HELPER(x_)


















#if defined(__GNUC__) && (__GNUC__ > 2)
#define NS_LIKELY(x)    (__builtin_expect(!!(x), 1))
#define NS_UNLIKELY(x)  (__builtin_expect(!!(x), 0))
#else
#define NS_LIKELY(x)    (!!(x))
#define NS_UNLIKELY(x)  (!!(x))
#endif

 




#if defined(XPCOM_GLUE) && !defined(XPCOM_GLUE_USE_NSPR)
#define XPCOM_GLUE_AVOID_NSPR
#endif


















#ifdef NS_STATIC_CHECKING
#define NS_STACK_CLASS __attribute__((user("NS_stack")))
#define NS_OKONHEAP    __attribute__((user("NS_okonheap")))
#define NS_SUPPRESS_STACK_CHECK __attribute__((user("NS_suppress_stackcheck")))
#define NS_FINAL_CLASS __attribute__((user("NS_final")))
#define NS_MUST_OVERRIDE __attribute__((user("NS_must_override")))
#else
#define NS_STACK_CLASS
#define NS_OKONHEAP
#define NS_SUPPRESS_STACK_CHECK
#define NS_FINAL_CLASS
#define NS_MUST_OVERRIDE
#endif




#ifdef NS_STATIC_CHECKING
# define NS_SCRIPTABLE __attribute__((user("NS_script")))
# define NS_INPARAM __attribute__((user("NS_inparam")))
# define NS_OUTPARAM  __attribute__((user("NS_outparam")))
# define NS_INOUTPARAM __attribute__((user("NS_inoutparam")))
# define NS_OVERRIDE __attribute__((user("NS_override")))
#else
# define NS_SCRIPTABLE
# define NS_INPARAM
# define NS_OUTPARAM
# define NS_INOUTPARAM
# define NS_OVERRIDE
#endif

#endif










































#ifndef mozilla_Types_h_
#define mozilla_Types_h_












#include "mozilla/StdInt.h"


#include <stddef.h>





















#if defined(WIN32) || defined(XP_OS2)
#  define MOZ_EXPORT_API(type)    __declspec(dllexport) type
#  define MOZ_EXPORT_DATA(type)   __declspec(dllexport) type
#else 
#  ifdef HAVE_VISIBILITY_ATTRIBUTE
#    define MOZ_EXTERNAL_VIS       __attribute__((visibility("default")))
#  elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    define MOZ_EXTERNAL_VIS      __global
#  else
#    define MOZ_EXTERNAL_VIS
#  endif
#  define MOZ_EXPORT_API(type)    MOZ_EXTERNAL_VIS type
#  define MOZ_EXPORT_DATA(type)   MOZ_EXTERNAL_VIS type
#endif








#ifdef _WIN32
#  if defined(__MWERKS__) || defined(__GNUC__)
#    define MOZ_IMPORT_API(x)    x
#  else
#    define MOZ_IMPORT_API(x)    __declspec(dllimport) x
#  endif
#elif defined(XP_OS2)
#  define MOZ_IMPORT_API(x)     __declspec(dllimport) x
#else
#  define MOZ_IMPORT_API(x)     MOZ_EXPORT_API(x)
#endif

#if defined(_WIN32) && !defined(__MWERKS__)
#  define MOZ_IMPORT_DATA(x)     __declspec(dllimport) x
#elif defined(XP_OS2)
#  define MOZ_IMPORT_DATA(x)     __declspec(dllimport) x
#else
#  define MOZ_IMPORT_DATA(x)     MOZ_EXPORT_DATA(x)
#endif






#if defined(IMPL_MFBT)
#  define MFBT_API(type)        MOZ_EXPORT_API(type)
#  define MFBT_DATA(type)       MOZ_EXPORT_DATA(type)
#else
#  define MFBT_API(type)        MOZ_IMPORT_API(type)
#  define MFBT_DATA(type)       MOZ_IMPORT_DATA(type)
#endif


















#ifdef __cplusplus
#  define MOZ_BEGIN_EXTERN_C    extern "C" {
#  define MOZ_END_EXTERN_C      }
#else
#  define MOZ_BEGIN_EXTERN_C
#  define MOZ_END_EXTERN_C
#endif

#endif  

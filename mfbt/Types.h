






#ifndef mozilla_Types_h_
#define mozilla_Types_h_












#include "mozilla/StandardInteger.h"


#include <stddef.h>





















#if defined(WIN32) || defined(XP_OS2)
#  define MOZ_EXPORT_DIRECTIVE  __declspec(dllexport)
#else 
#  ifdef HAVE_VISIBILITY_ATTRIBUTE
#    define MOZ_EXTERNAL_VIS       __attribute__((visibility("default")))
#  elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    define MOZ_EXTERNAL_VIS      __global
#  else
#    define MOZ_EXTERNAL_VIS
#  endif
#  define MOZ_EXPORT_DIRECTIVE   MOZ_EXTERNAL_VIS
#endif

#define MOZ_EXPORT_API(type)    MOZ_EXPORT_DIRECTIVE type
#define MOZ_EXPORT_DATA(type)   MOZ_EXPORT_DIRECTIVE type








#ifdef _WIN32
#  if defined(__MWERKS__)
#    define MOZ_IMPORT_API_DIRECTIVE
#  else
#    define MOZ_IMPORT_API_DIRECTIVE __declspec(dllimport)
#  endif
#elif defined(XP_OS2)
#  define MOZ_IMPORT_API_DIRECTIVE  __declspec(dllimport)
#else
#  define MOZ_IMPORT_API_DIRECTIVE MOZ_EXPORT_DIRECTIVE
#endif

#define MOZ_IMPORT_API(x)    MOZ_IMPORT_API_DIRECTIVE x

#if defined(_WIN32) && !defined(__MWERKS__)
#  define MOZ_IMPORT_DATA_DIRECTIVE __declspec(dllimport)
#elif defined(XP_OS2)
#  define MOZ_IMPORT_DATA_DIRECTIVE __declspec(dllimport)
#else
#  define MOZ_IMPORT_DATA_DIRECTIVE MOZ_EXPORT_DIRECTIVE
#endif

#define MOZ_IMPORT_DATA(x)    MOZ_IMPORT_DATA_DIRECTIVE x






#if defined(IMPL_MFBT)
#  define MFBT_API     MOZ_EXPORT_DIRECTIVE
#  define MFBT_DATA    MOZ_EXPORT_DIRECTIVE
#else
  






#  if defined(MOZ_GLUE_IN_PROGRAM)
#    define MFBT_API   __attribute__((weak)) MOZ_IMPORT_API_DIRECTIVE
#    define MFBT_DATA  __attribute__((weak)) MOZ_IMPORT_DATA_DIRECTIVE
#  else
#    define MFBT_API   MOZ_IMPORT_API_DIRECTIVE
#    define MFBT_DATA  MOZ_IMPORT_DATA_DIRECTIVE
#  endif
#endif


















#ifdef __cplusplus
#  define MOZ_BEGIN_EXTERN_C    extern "C" {
#  define MOZ_END_EXTERN_C      }
#else
#  define MOZ_BEGIN_EXTERN_C
#  define MOZ_END_EXTERN_C
#endif

#endif  

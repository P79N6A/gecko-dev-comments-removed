







#ifndef mozilla_Types_h
#define mozilla_Types_h







#include <stddef.h>
#include <stdint.h>




















#if defined(WIN32)
#  define MOZ_EXPORT   __declspec(dllexport)
#else 
#  ifdef HAVE_VISIBILITY_ATTRIBUTE
#    define MOZ_EXPORT       __attribute__((visibility("default")))
#  elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#    define MOZ_EXPORT      __global
#  else
#    define MOZ_EXPORT
#  endif
#endif









#ifdef _WIN32
#  if defined(__MWERKS__)
#    define MOZ_IMPORT_API
#  else
#    define MOZ_IMPORT_API __declspec(dllimport)
#  endif
#else
#  define MOZ_IMPORT_API MOZ_EXPORT
#endif

#if defined(_WIN32) && !defined(__MWERKS__)
#  define MOZ_IMPORT_DATA  __declspec(dllimport)
#else
#  define MOZ_IMPORT_DATA  MOZ_EXPORT
#endif






#if defined(IMPL_MFBT)
#  define MFBT_API     MOZ_EXPORT
#  define MFBT_DATA    MOZ_EXPORT
#else
  






#  if defined(MOZ_GLUE_IN_PROGRAM)
#    define MFBT_API   __attribute__((weak)) MOZ_IMPORT_API
#    define MFBT_DATA  __attribute__((weak)) MOZ_IMPORT_DATA
#  else
#    define MFBT_API   MOZ_IMPORT_API
#    define MFBT_DATA  MOZ_IMPORT_DATA
#  endif
#endif


















#ifdef __cplusplus
#  define MOZ_BEGIN_EXTERN_C    extern "C" {
#  define MOZ_END_EXTERN_C      }
#else
#  define MOZ_BEGIN_EXTERN_C
#  define MOZ_END_EXTERN_C
#endif




#if defined(__GNUC__) && defined(__cplusplus) && \
  !defined(__GXX_EXPERIMENTAL_CXX0X__) && __cplusplus < 201103L
#  define decltype __typeof__
#endif

#endif 

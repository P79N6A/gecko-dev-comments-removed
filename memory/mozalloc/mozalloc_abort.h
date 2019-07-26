






#ifndef mozilla_mozalloc_abort_h
#define mozilla_mozalloc_abort_h

#include "mozilla/Attributes.h"

#if defined(MOZALLOC_EXPORT)


#elif defined(XP_WIN)
#  define MOZALLOC_EXPORT __declspec(dllimport)
#elif defined(HAVE_VISIBILITY_ATTRIBUTE)


#  define MOZALLOC_EXPORT __attribute__ ((visibility ("default")))
#else
#  define MOZALLOC_EXPORT
#endif








MOZALLOC_EXPORT
#if !defined(__arm__)
  MOZ_NORETURN
#endif
  void mozalloc_abort(const char* const msg);


#endif  

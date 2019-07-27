






#ifndef mozilla_mozalloc_abort_h
#define mozilla_mozalloc_abort_h

#include "mozilla/Attributes.h"
#include "mozilla/Types.h"








MFBT_API
#if !defined(__arm__)
  MOZ_NORETURN
#endif
  void mozalloc_abort(const char* const msg);


#endif  








#ifndef mozilla_throw_msvc_h
#define mozilla_throw_msvc_h

#if defined(MOZ_MSVC_STL_WRAP_RAISE)
#  include "msvc_raise_wrappers.h"
#elif defined(MOZ_MSVC_STL_WRAP_Throw)
#  include "msvc_throw_wrapper.h"
#else
#  error "Unknown STL wrapper tactic"
#endif

#endif  

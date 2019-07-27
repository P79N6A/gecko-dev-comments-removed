

























#include "hb-atomic-private.hh"
#include "hb-mutex-private.hh"


#if defined(HB_ATOMIC_INT_NIL)
#ifdef _MSC_VER
#pragma error("Could not find any system to define atomic_int macros, library WILL NOT be thread-safe")
#pragma error("Check hb-atomic-private.hh for possible resolutions.")
#else
#error "Could not find any system to define atomic_int macros, library WILL NOT be thread-safe"
#error "Check hb-atomic-private.hh for possible resolutions."
#endif
#endif

#if defined(HB_MUTEX_IMPL_NIL)
#ifdef _MSC_VER
#pragma error("Could not find any system to define mutex macros, library WILL NOT be thread-safe")
#pragma error("Check hb-mutex-private.hh for possible resolutions.")
#else
#error "Could not find any system to define mutex macros, library WILL NOT be thread-safe"
#error "Check hb-mutex-private.hh for possible resolutions."
#endif
#endif



























#include "hb-atomic-private.hh"
#include "hb-mutex-private.hh"


#if defined(HB_ATOMIC_INT_NIL)
#ifdef _MSC_VER
#pragma message("Could not find any system to define atomic_int macros, library may NOT be thread-safe")
#else
#warning "Could not find any system to define atomic_int macros, library may NOT be thread-safe"
#endif
#endif

#if defined(HB_MUTEX_IMPL_NIL)
#ifdef _MSC_VER
#pragma message("Could not find any system to define mutex macros, library may NOT be thread-safe")
#else
#warning "Could not find any system to define mutex macros, library may NOT be thread-safe"
#endif
#endif

#if defined(HB_ATOMIC_INT_NIL) || defined(HB_MUTEX_IMPL_NIL)
#ifdef _MSC_VER
#pragma message("To suppress these warnings, define HB_NO_MT")
#else
#warning "To suppress these warnings, define HB_NO_MT"
#endif
#endif


#include "hb-unicode-private.hh"

#if !defined(HB_NO_UNICODE_FUNCS) && defined(HB_UNICODE_FUNCS_NIL)
#ifdef _MSC_VER
#pragma message("Could not find any Unicode functions implementation, you have to provide your own")
#pragma message("To suppress this warnings, define HB_NO_UNICODE_FUNCS")
#else
#warning "Could not find any Unicode functions implementation, you have to provide your own"
#warning "To suppress this warning, define HB_NO_UNICODE_FUNCS"
#endif
#endif

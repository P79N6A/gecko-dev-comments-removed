









#ifndef mozilla_Likely_h_
#define mozilla_Likely_h_

#if defined(__clang__) || defined(__GNUC__)
#  define MOZ_LIKELY(x)   (__builtin_expect(!!(x), 1))
#  define MOZ_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#  define MOZ_LIKELY(x)   (!!(x))
#  define MOZ_UNLIKELY(x) (!!(x))
#endif

#endif 

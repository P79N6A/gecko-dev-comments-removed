






#ifndef mozilla_Compiler_h_
#define mozilla_Compiler_h_

#if !defined(__clang__) && defined(__GNUC__)
   



#  define MOZ_GCC_VERSION_AT_LEAST(major, minor, patchlevel)          \
     ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) \
      >= ((major) * 10000 + (minor) * 100 + (patchlevel)))
#if !MOZ_GCC_VERSION_AT_LEAST(4, 4, 0)
#  error "mfbt (and Gecko) require at least gcc 4.4 to build."
#endif
#endif

#endif  

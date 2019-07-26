







#ifndef mozilla_Compiler_h
#define mozilla_Compiler_h

#if !defined(__clang__) && defined(__GNUC__)

#define MOZ_IS_GCC 1
   



#  define MOZ_GCC_VERSION_AT_LEAST(major, minor, patchlevel)          \
     ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) \
      >= ((major) * 10000 + (minor) * 100 + (patchlevel)))
#if !MOZ_GCC_VERSION_AT_LEAST(4, 4, 0)
#  error "mfbt (and Gecko) require at least gcc 4.4 to build."
#endif

#else

#define MOZ_IS_GCC 0

#endif









#ifdef __cplusplus
#  include <cstddef>
#  ifdef _STLPORT_MAJOR
#    define MOZ_USING_STLPORT 1
#    define MOZ_STLPORT_VERSION_AT_LEAST(major, minor, patch) \
       (_STLPORT_VERSION >= ((major) << 8 | (minor) << 4 | (patch)))
#  elif defined(__LIBCPP_VERSION)
   





#    define MOZ_USING_LIBCXX 1
#  elif defined(__GLIBCXX__)
#    define MOZ_USING_LIBSTDCXX 1
   





#    if MOZ_IS_GCC
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          MOZ_GCC_VERSION_AT_LEAST(major, minor, patch)
#    elif defined(_GLIBCXX_THROW_OR_ABORT)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 8))
#    elif defined(_GLIBCXX_NOEXCEPT)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 7))
#    elif defined(_GLIBCXX_USE_DEPRECATED)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 6))
#    elif defined(_GLIBCXX_PSEUDO_VISIBILITY)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 5))
#    elif defined(_GLIBCXX_BEGIN_EXTERN_C)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 4))
#    elif defined(_GLIBCXX_VISIBILITY_ATTR)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 3))
#    elif defined(_GLIBCXX_VISIBILITY)
#      define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) \
          ((major) < 4 || ((major) == 4 && (minor) <= 2))
#    else
#      error "Your version of libstdc++ is unknown to us and is likely too old."
#    endif
#  endif

   
#  ifndef MOZ_USING_STLPORT
#    define MOZ_USING_STLPORT 0
#    define MOZ_STLPORT_VERSION_AT_LEAST(major, minor, patch) 0
#  endif
#  ifndef MOZ_USING_LIBCXX
#    define MOZ_USING_LIBCXX 0
#  endif
#  ifndef MOZ_USING_LIBSTDCXX
#    define MOZ_USING_LIBSTDCXX 0
#    define MOZ_LIBSTDCXX_VERSION_AT_LEAST(major, minor, patch) 0
#  endif
#endif 

#endif 

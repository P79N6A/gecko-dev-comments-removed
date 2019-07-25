









































#ifndef mozilla_Attributes_h_
#define mozilla_Attributes_h_






#ifndef __cplusplus
#error "mozilla/Attributes.h is only relevant to C++ code."
#endif









#if defined(__clang__)
   




#  ifndef __has_extension
#    define __has_extension __has_feature /* compatibility, for older versions of clang */
#  endif
#  if __has_extension(cxx_deleted_functions)
#    define MOZ_HAVE_CXX11_DELETE
#  endif
#  if __has_extension(cxx_override_control)
#    define MOZ_HAVE_CXX11_OVERRIDE
#    define MOZ_HAVE_CXX11_FINAL         final
#  endif
#elif defined(__GNUC__)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if __GNUC__ > 4
#      define MOZ_HAVE_CXX11_DELETE
#      define MOZ_HAVE_CXX11_OVERRIDE
#      define MOZ_HAVE_CXX11_FINAL       final
#    elif __GNUC__ == 4
#      if __GNUC_MINOR__ >= 7
#        define MOZ_HAVE_CXX11_OVERRIDE
#        define MOZ_HAVE_CXX11_FINAL     final
#      endif
#      if __GNUC_MINOR__ >= 4
#        define MOZ_HAVE_CXX11_DELETE
#      endif
#    endif
#  else
     
#    if __GNUC__ > 4
#      define MOZ_HAVE_CXX11_FINAL       __final
#    elif __GNUC__ == 4
#      if __GNUC_MINOR__ >= 7
#        define MOZ_HAVE_CXX11_FINAL     __final
#      endif
#    endif
#  endif
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1400
#    define MOZ_HAVE_CXX11_OVERRIDE

#    define MOZ_HAVE_CXX11_FINAL         sealed
#  endif
#endif























#if defined(MOZ_HAVE_CXX11_DELETE)
#  define MOZ_DELETE            = delete
#else
#  define MOZ_DELETE
#endif




































#if defined(MOZ_HAVE_CXX11_OVERRIDE)
#  define MOZ_OVERRIDE          override
#else
#  define MOZ_OVERRIDE
#endif
































































#if defined(MOZ_HAVE_CXX11_FINAL)
#  define MOZ_FINAL             MOZ_HAVE_CXX11_FINAL
#else
#  define MOZ_FINAL
#endif

#endif  

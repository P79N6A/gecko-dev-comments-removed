














#if defined (__BUILDING_STLPORT)
#  undef _STLP_USE_DYNAMIC_LIB
#  undef _STLP_USE_STATIC_LIB
#  if defined (_STLP_DLL)

#    define _STLP_USE_DYNAMIC_LIB
#    if !defined (_STLP_RUNTIME_DLL)
#      define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#    endif
#  else
#    define _STLP_USE_STATIC_LIB
#    if defined (_STLP_RUNTIME_DLL)
#      define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#    endif
#  endif
#else
#  if !defined (_STLP_NO_IOSTREAMS)



#    if defined (_STLP_RUNTIME_DLL)
#      if !defined (_STLP_USE_STATIC_LIB)
#        if !defined (_STLP_USE_DYNAMIC_LIB)
#          define _STLP_USE_DYNAMIC_LIB
#        endif
#      else




#        define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#      endif
#    else
#      if !defined(_STLP_USE_DYNAMIC_LIB)
#        if !defined (_STLP_USE_STATIC_LIB)
#          define _STLP_USE_STATIC_LIB
#        endif
#      else


#        define _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#      endif
#    endif
#  else



#    define _STLP_USE_STATIC_LIB
#  endif
#endif


#ifdef _STLP_WCE
#  undef _STLP_USING_CROSS_NATIVE_RUNTIME_LIB
#endif

#if !defined (_STLP_USE_DYNAMIC_LIB) && !defined (_STLP_USE_STATIC_LIB)
#  error Unknown STLport usage config (dll/lib?)
#endif

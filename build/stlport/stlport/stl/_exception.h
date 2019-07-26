


























#ifndef _STLP_INTERNAL_EXCEPTION
#define _STLP_INTERNAL_EXCEPTION

#if !defined (_STLP_NO_EXCEPTION_HEADER)

#  if defined ( _UNCAUGHT_EXCEPTION )
#    undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#  endif

#  if defined (_STLP_BROKEN_EXCEPTION_CLASS)
#    define exception     _STLP_NULLIFIED_BROKEN_EXCEPTION_CLASS
#    define bad_exception _STLP_NULLIFIED_BROKEN_BAD_EXCEPTION_CLASS
#    if defined (_STLP_NO_NEW_NEW_HEADER)
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(Exception.h)
#    else
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(Exception)
#    endif
#    undef exception
#    undef bad_exception
#  else
#    if defined (_STLP_NO_NEW_NEW_HEADER)
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <exception.h>
#      else
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(exception.h)
#      endif
#    else
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <exception>
#      else
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(exception)
#      endif
#    endif
#  endif

#  if defined (_STLP_HAS_SPECIFIC_PROLOG_EPILOG) && defined (_STLP_MSVC_LIB) && (_STLP_MSVC_LIB < 1300)



#    include <stl/config/_warnings_off.h>
#  endif

#  if defined (_STLP_USE_OWN_NAMESPACE)

_STLP_BEGIN_NAMESPACE
#    if !defined (_STLP_BROKEN_EXCEPTION_CLASS)
#      if !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_WIN64)
using _STLP_VENDOR_EXCEPT_STD::exception;
#      else
using ::exception;
#      endif
using _STLP_VENDOR_EXCEPT_STD::bad_exception;
#    endif

#    if !defined (_STLP_NO_USING_FOR_GLOBAL_FUNCTIONS)


#      if !defined (_STLP_VENDOR_UNEXPECTED_STD)
#        define _STLP_VENDOR_UNEXPECTED_STD _STLP_VENDOR_EXCEPT_STD
#      else




#        if !defined (_STLP_VENDOR_TERMINATE_STD)
#          define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_UNEXPECTED_STD
#        endif
#        if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#          define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_UNEXPECTED_STD
#        endif
#      endif
#      if !defined (_STLP_VENDOR_TERMINATE_STD)
#        define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#        define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_TERMINATE_STD)
#        define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#        define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_EXCEPT_STD
#      endif

#        if !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
#          if defined (__ICL) && (__ICL >= 900) && (_STLP_MSVC_LIB < 1300)

using std::unexpected;
#          else
using _STLP_VENDOR_UNEXPECTED_STD::unexpected;
#          endif
using _STLP_VENDOR_UNEXPECTED_STD::unexpected_handler;
using _STLP_VENDOR_UNEXPECTED_STD::set_unexpected;
#        endif
using _STLP_VENDOR_TERMINATE_STD::terminate;
using _STLP_VENDOR_TERMINATE_STD::terminate_handler;
using _STLP_VENDOR_TERMINATE_STD::set_terminate;

#      if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
using _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD::uncaught_exception;
#      endif
#    endif 
_STLP_END_NAMESPACE
#  endif 
#else 








#if 0











#endif

#endif 

#if defined (_STLP_NO_EXCEPTION_HEADER) || defined (_STLP_BROKEN_EXCEPTION_CLASS)
_STLP_BEGIN_NAMESPACE


class _STLP_CLASS_DECLSPEC exception {
public:
#  ifndef _STLP_USE_NO_IOSTREAMS
  exception() _STLP_NOTHROW;
  virtual ~exception() _STLP_NOTHROW;
  virtual const char* what() const _STLP_NOTHROW;
#  else
  exception() _STLP_NOTHROW {}
  virtual ~exception() _STLP_NOTHROW {}
  virtual const char* what() const _STLP_NOTHROW {return "class exception";}
#  endif
};


class _STLP_CLASS_DECLSPEC bad_exception : public exception {
public:
#  ifndef _STLP_USE_NO_IOSTREAMS
  bad_exception() _STLP_NOTHROW;
  ~bad_exception() _STLP_NOTHROW;
  const char* what() const _STLP_NOTHROW;
#  else
  bad_exception() _STLP_NOTHROW {}
  ~bad_exception() _STLP_NOTHROW {}
  const char* what() const _STLP_NOTHROW {return "class bad_exception";}
#  endif
};

#ifdef _STLP_USE_EXCEPTIONS

class __Named_exception;
#endif

_STLP_END_NAMESPACE
#endif

#endif 

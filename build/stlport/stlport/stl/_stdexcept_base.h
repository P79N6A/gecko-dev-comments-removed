

















#ifndef _STLP_INTERNAL_STDEXCEPT_BASE
#define _STLP_INTERNAL_STDEXCEPT_BASE

#if !defined (_STLP_USE_NATIVE_STDEXCEPT) || defined (_STLP_USE_OWN_NAMESPACE)

#  ifndef _STLP_INTERNAL_EXCEPTION
#    include <stl/_exception.h>
#  endif

#  if defined(_STLP_USE_EXCEPTIONS)

#    ifndef _STLP_INTERNAL_CSTRING
#      include <stl/_cstring.h>
#    endif

#    ifndef _STLP_STRING_FWD_H
#      include <stl/_string_fwd.h>
#    endif

#    ifndef _STLP_USE_NO_IOSTREAMS
#      define _STLP_OWN_STDEXCEPT 1
#    endif

_STLP_BEGIN_NAMESPACE








#    if defined (_STLP_WCE_NET) || \
        defined (_STLP_USE_DYNAMIC_LIB) && defined (_STLP_USING_CROSS_NATIVE_RUNTIME_LIB)
#      define _STLP_DO_WARNING_POP
#      pragma warning (push)
#      pragma warning (disable: 4275) // Non dll interface class 'exception' used as base
                                      
#    endif

#    if !defined (_STLP_NO_EXCEPTION_HEADER)
#      if !defined (_STLP_EXCEPTION_BASE) && !defined (_STLP_BROKEN_EXCEPTION_CLASS) && \
           defined (_STLP_USE_NAMESPACES) &&  defined (_STLP_USE_OWN_NAMESPACE)
using _STLP_VENDOR_EXCEPT_STD::exception;
#      endif
#    endif
#    define _STLP_EXCEPTION_BASE exception

class _STLP_CLASS_DECLSPEC __Named_exception : public _STLP_EXCEPTION_BASE {
public:
  __Named_exception(const string& __str);
  __Named_exception(const __Named_exception&);
  __Named_exception& operator = (const __Named_exception&);

  const char* what() const _STLP_NOTHROW_INHERENTLY;
  ~__Named_exception() _STLP_NOTHROW_INHERENTLY;

private:
  enum { _S_bufsize = 256 };
  char _M_static_name[_S_bufsize];
  char *_M_name;
};

#    if defined (_STLP_USE_NO_IOSTREAMS) && !defined (__BUILDING_STLPORT)
       
#      include <stl/_stdexcept_base.c>
#    endif

#    if defined (_STLP_DO_WARNING_POP)
#      pragma warning (pop)
#      undef _STLP_DO_WARNING_POP
#    endif

_STLP_END_NAMESPACE

#  endif
#endif

#endif

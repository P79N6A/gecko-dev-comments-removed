
















#ifndef _STLP_STREAMBUF_H
#define _STLP_STREAMBUF_H

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x2067
#  include <stl/_prolog.h>
#endif

#include <stl/_ioserr.h>

#include <streambuf>

#include <ios.h>

#ifndef _STLP_HAS_NO_NAMESPACES
#  ifdef _STLP_BROKEN_USING_DIRECTIVE
_STLP_USING_NAMESPACE(stlport)
#  else
using _STLP_STD::basic_streambuf;
using _STLP_STD::streambuf;
#    ifndef _STLP_NO_WCHAR_T
using _STLP_STD::wstreambuf;
#    endif
#  endif
#endif 

#if (_STLP_OUTERMOST_HEADER_ID == 0x2067)
#  include <stl/_epilog.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#endif

#endif 





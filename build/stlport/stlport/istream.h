














#ifndef _STLP_ISTREAM_H
#define _STLP_ISTREAM_H

#ifndef _STLP_OUTERMOST_HEADER_ID
#  define _STLP_OUTERMOST_HEADER_ID 0x2037
#  include <stl/_prolog.h>
#endif

#include <stl/_ioserr.h>

#include <istream>

#ifndef _STLP_HAS_NO_NAMESPACES
#  ifdef _STLP_BROKEN_USING_DIRECTIVE
_STLP_USING_NAMESPACE(stlport)
#  else
using _STLP_STD::basic_istream;
using _STLP_STD::basic_iostream;
using _STLP_STD::istream;
using _STLP_STD::iostream;
using _STLP_STD::ios;
#    ifndef _STLP_NO_WCHAR_T
using _STLP_STD::wistream;
using _STLP_STD::wiostream;
#    endif
using _STLP_STD::ws;
#  endif
#endif

#if (_STLP_OUTERMOST_HEADER_ID == 0x2037)
#  include <stl/_epilog.h>
#  undef _STLP_OUTERMOST_HEADER_ID
#endif

#endif 
